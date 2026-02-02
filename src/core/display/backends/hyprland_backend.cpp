#include "hyprland_backend.h"
#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <fcntl.h>
#include <unistd.h>

std::string HyprlandBackend::exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool HyprlandBackend::is_available() {
    const char* sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    return sig != nullptr;
}

static DisplayMode parse_mode_str(const std::string& s) {
    DisplayMode m = {0,0,0};
    size_t x = s.find('x');
    size_t at = s.find('@');
    if (x != std::string::npos && at != std::string::npos) {
        m.width = std::stoi(s.substr(0, x));
        m.height = std::stoi(s.substr(x + 1, at - x - 1));
        double hz = std::stod(s.substr(at + 1));
        m.refresh_rate_mHz = (int)(hz * 1000);
    }
    return m;
}

static std::vector<DisplayMode> get_drm_modes(const std::string& conn_name) {
    std::vector<DisplayMode> modes;
    
    for (int i = 0; i < 2; ++i) {
        std::string path = "/dev/dri/card" + std::to_string(i);
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) continue;
        
        drmModeRes* res = drmModeGetResources(fd);
        if (!res) {
            close(fd);
            continue;
        }
        
        for (int j = 0; j < res->count_connectors; ++j) {
            drmModeConnector* conn = drmModeGetConnector(fd, res->connectors[j]);
            if (!conn) continue;
            
            const char* type_str = "Unknown";
            switch(conn->connector_type) {
                case DRM_MODE_CONNECTOR_HDMIA: type_str = "HDMI-A"; break;
                case DRM_MODE_CONNECTOR_eDP: type_str = "eDP"; break;
                case DRM_MODE_CONNECTOR_DisplayPort: type_str = "DP"; break;
                case DRM_MODE_CONNECTOR_DVII: type_str = "DVI-I"; break;
                case DRM_MODE_CONNECTOR_DVID: type_str = "DVI-D"; break;
                case DRM_MODE_CONNECTOR_VGA: type_str = "VGA"; break;
                case DRM_MODE_CONNECTOR_VIRTUAL: type_str = "Virtual"; break;
            }
            
            std::string detected_id = std::string(type_str) + "-" + std::to_string(conn->connector_type_id);
            
            if (detected_id == conn_name || (conn_name == "eDP-1" && conn->connector_type == DRM_MODE_CONNECTOR_eDP)) {
                for (int m = 0; m < conn->count_modes; ++m) {
                    const auto& dmode = conn->modes[m];
                    DisplayMode mode;
                    mode.width = dmode.hdisplay;
                    mode.height = dmode.vdisplay;
                    mode.refresh_rate_mHz = dmode.vrefresh * 1000; 
                    
                    if (dmode.htotal > 0 && dmode.vtotal > 0) {
                        double real_hz = (dmode.clock * 1000.0) / (dmode.htotal * dmode.vtotal);
                        mode.refresh_rate_mHz = (int)(real_hz * 1000);
                    }
                    
                    bool found = false;
                    for(const auto& existing : modes) if(existing.width == mode.width && existing.height == mode.height && std::abs(existing.refresh_rate_mHz - mode.refresh_rate_mHz) < 500) found = true;
                    
                    if(!found) modes.push_back(mode);
                }
            }
            
            drmModeFreeConnector(conn);
        }
        
        drmModeFreeResources(res);
        close(fd);
        if (!modes.empty()) return modes;
    }
    return modes;
}

std::vector<DisplayInfo> HyprlandBackend::get_displays() {
    std::vector<DisplayInfo> displays;
    std::string out = exec("hyprctl monitors");
    std::stringstream ss(out);
    std::string line;
    
    DisplayInfo current;
    bool in_monitor = false;
    
    while(std::getline(ss, line)) {
        if (line.find("Monitor") == 0) {
            if (in_monitor) {
                 current.available_modes = get_drm_modes(current.id);
                 if (current.available_modes.empty()) {
                     current.available_modes = {{1920, 1080, 60000}}; 
                 }
                 displays.push_back(current);
            }
            in_monitor = true;
            current = DisplayInfo();
            // Monitor eDP-1 (ID 0):
            size_t space = line.find(' ');
            size_t paren = line.find('(');
            if (space != std::string::npos && paren != std::string::npos) {
                current.id = line.substr(space + 1, paren - space - 2);
            }
            current.enabled = true;
        } else if (in_monitor) {
            if (line.find("\tscale: ") != std::string::npos) {
                current.scale = std::stod(line.substr(8));
            }
            if (line.find('@') != std::string::npos && line.find("at") != std::string::npos) {
                std::stringstream ls(line);
                std::string mode_str;
                ls >> mode_str;
                current.current_mode = parse_mode_str(mode_str);
            }
        }
    }
    if (in_monitor) {
        current.available_modes = get_drm_modes(current.id);
        if (current.available_modes.empty()) {
             current.available_modes = {{1920, 1080, 60000}}; 
        }
        displays.push_back(current);
    }
    
    return displays;
}

bool HyprlandBackend::set_mode(const std::string& display_id, const DisplayMode& mode) {
    double scale = 1.0;
    auto displays = get_displays();
    for(const auto& d : displays) {
        if(d.id == display_id) {
            scale = d.scale;
            break;
        }
    }
    
    std::string cmd = "hyprctl keyword monitor \"" + display_id + "," + 
                      std::to_string(mode.width) + "x" + std::to_string(mode.height) + "@" + std::to_string(mode.refresh_rate_mHz/1000.0) + 
                      ",auto," + std::to_string(scale) + "\"";
    exec(cmd.c_str());
    return true;
}

bool HyprlandBackend::set_scale(const std::string& display_id, double scale) {
    DisplayMode mode = {1920, 1080, 60000};
    auto displays = get_displays();
    for(const auto& d : displays) {
        if(d.id == display_id) {
            mode = d.current_mode;
            break;
        }
    }
    
    std::string cmd = "hyprctl keyword monitor \"" + display_id + "," + 
                      std::to_string(mode.width) + "x" + std::to_string(mode.height) + "@" + std::to_string(mode.refresh_rate_mHz/1000.0) + 
                      ",auto," + std::to_string(scale) + "\"";
    exec(cmd.c_str());
    return true;
}

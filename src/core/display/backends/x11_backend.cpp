#include "x11_backend.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <cstdlib>
#include <array>
#include <memory>
#include <iostream>
#include <cmath>

static std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool X11Backend::is_available() {
    const char* session = std::getenv("XDG_SESSION_TYPE");
    if (session && std::string(session) == "x11") return true;
    
    Display* d = XOpenDisplay(NULL);
    if (d) {
        XCloseDisplay(d);
        return true;
    }
    return false;
}

std::vector<DisplayInfo> X11Backend::get_displays() {
    std::vector<DisplayInfo> infos;
    Display* dpy = XOpenDisplay(NULL);
    if (!dpy) return infos;

    Window root = DefaultRootWindow(dpy);
    XRRScreenResources* res = XRRGetScreenResources(dpy, root);
    if (!res) {
        XCloseDisplay(dpy);
        return infos;
    }
    for (int i = 0; i < res->noutput; ++i) {
        XRROutputInfo* out_info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
        if (!out_info) continue;

        if (out_info->connection == RR_Connected) {
            DisplayInfo info;
            info.id = out_info->name;
            info.enabled = (out_info->crtc != 0);
            info.scale = 1.0;

            if (out_info->crtc != 0) {
                XRRCrtcInfo* crtc = XRRGetCrtcInfo(dpy, res, out_info->crtc);
                if (crtc) {
                    for (int j = 0; j < res->nmode; ++j) {
                        if (res->modes[j].id == crtc->mode) {
                            info.current_mode.width = res->modes[j].width;
                            info.current_mode.height = res->modes[j].height;
                            
                            double rate = 0;
                            if (res->modes[j].hTotal && res->modes[j].vTotal)
                                rate = ((double)res->modes[j].dotClock) / (res->modes[j].hTotal * res->modes[j].vTotal);
                            info.current_mode.refresh_rate_mHz = (int)(rate * 1000);
                            break;
                        }
                    }
                    XRRFreeCrtcInfo(crtc);
                }
            }

            for (int k = 0; k < out_info->nmode; ++k) {
                RRMode m_id = out_info->modes[k];
                for (int m = 0; m < res->nmode; ++m) {
                    if (res->modes[m].id == m_id) {
                        DisplayMode dm;
                        dm.width = res->modes[m].width;
                        dm.height = res->modes[m].height;
                         double rate = 0;
                        if (res->modes[m].hTotal && res->modes[m].vTotal)
                            rate = ((double)res->modes[m].dotClock) / (res->modes[m].hTotal * res->modes[m].vTotal);
                        dm.refresh_rate_mHz = (int)(rate * 1000);
                        
                        info.available_modes.push_back(dm);
                    }
                }
            }
            
            infos.push_back(info);
        }
        XRRFreeOutputInfo(out_info);
    }

    XRRFreeScreenResources(res);
    XCloseDisplay(dpy);
    return infos;
}

bool X11Backend::set_mode(const std::string& display_id, const DisplayMode& mode) {
    std::string cmd = "xrandr --output " + display_id + " --mode " + 
                      std::to_string(mode.width) + "x" + std::to_string(mode.height) + 
                      " --rate " + std::to_string(mode.refresh_rate_mHz / 1000.0);
    exec(cmd.c_str());
    return true;
}

bool X11Backend::set_scale(const std::string& display_id, double scale) {
    std::string cmd = "xrandr --output " + display_id + " --scale " + 
                      std::to_string(scale) + "x" + std::to_string(scale);
    exec(cmd.c_str());
    return true;
}

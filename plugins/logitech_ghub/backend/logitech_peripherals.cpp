// experimental code : need  to RE

#include "logitech_peripherals.h"
#include "hidpp_driver.h"
#include <iostream>

namespace Logitech {

    void init() {
        HidppDriver::init();
    }

    void shutdown() {
        HidppDriver::shutdown();
    }

    std::vector<Device> get_devices() {
        std::vector<Device> out;
        auto devs = HidppDriver::enumerate();
        std::vector<std::string> seen_paths;
        
        for (const auto& d : devs) {
            if (d.interface_number == 1 || d.usage_page == 0xFF00) { 
                bool seen = false;
                for(const auto& p : seen_paths) { if (p == d.path) { seen = true; break; } }
                if (seen) continue;
                
                seen_paths.push_back(d.path);

                Device dev;
                dev.path = d.path;
                
                std::string product(d.product.begin(), d.product.end());
                dev.name = product.empty() ? "Logitech Device" : product;
                
                dev.current_dpi = 1600;
                dev.max_dpi = 8000; 
                dev.dpi_levels = {400, 800, 1600, 3200};
                
                out.push_back(dev);
            }
        }
        return out;
    }

    bool set_dpi(const std::string& path, int dpi) {
        auto devs = HidppDriver::enumerate();
        unsigned short vid = 0, pid = 0;
        
        for(const auto& d : devs) {
            if (d.path == path) {
                vid = d.vendor_id;
                pid = d.product_id;
                break;
            }
        }
        
        if (vid == 0) return false;

        HidppDriver driver;
        if (!driver.open_path(path.c_str())) return false;

        std::cout << "[Logitech] Setting DPI to " << dpi << " via HID++ (Short Report)" << std::endl;

        unsigned char feat_high = 0x22;
        unsigned char feat_low = 0x01;
        
        std::vector<unsigned char> cmd_get_feat(8, 0x00); 
        cmd_get_feat[0] = 0x10; 
        cmd_get_feat[1] = 0xFF; 
        cmd_get_feat[2] = 0x00; 
        cmd_get_feat[3] = 0x00; 
        cmd_get_feat[4] = feat_high;
        cmd_get_feat[5] = feat_low;
        
        auto resp = driver.send_recv(cmd_get_feat);
        if (resp.empty() || resp[0] != 0x10 || resp[4] == 0) {
            std::cerr << "[Logitech] Failed to find DPI feature (0x2201). Resp Size: " << resp.size() << std::endl;
            return false;
        }
        
        unsigned char dpi_feature_index = resp[4];
        std::cout << "[Logitech] Found DPI Feature Index: " << (int)dpi_feature_index << std::endl;

        unsigned char sensor_index = 0; 
        unsigned char dpi_h = (dpi >> 8) & 0xFF;
        unsigned char dpi_l = (dpi & 0xFF);
        
        std::vector<unsigned char> cmd_set_dpi(8, 0x00);
        cmd_set_dpi[0] = 0x10;
        cmd_set_dpi[1] = 0xFF;
        cmd_set_dpi[2] = dpi_feature_index;
        cmd_set_dpi[3] = 0x10; 
        cmd_set_dpi[4] = sensor_index;
        cmd_set_dpi[5] = dpi_h;
        cmd_set_dpi[6] = dpi_l;
        
        auto res = driver.send_recv(cmd_set_dpi);
        if (res.empty()) {
             std::cerr << "[Logitech] Set DPI command failed." << std::endl;
             return false;
        }
        
        return true;
    }
}

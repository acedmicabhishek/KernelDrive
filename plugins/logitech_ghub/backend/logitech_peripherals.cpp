#include "hidpp20_device.h"
#include "logitech_peripherals.h"

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
                
                Hidpp20Device hdev(d.path, dev.name);
                if (hdev.connect()) {
                    int dpi = hdev.get_dpi();
                    if (dpi > 0) dev.current_dpi = dpi;
                    else dev.current_dpi = 1600; 
                    
                    int rate = hdev.get_polling_rate();
                    if (rate > 0) dev.current_rate_ms = rate; 
                    else dev.current_rate_ms = 1; 
                    
                    int max_dpi = hdev.get_max_dpi();
                    if (max_dpi > 0) dev.max_dpi = max_dpi;
                    else dev.max_dpi = 25600;
                    
                } else {
                     dev.current_dpi = 1600;
                     dev.current_rate_ms = 1;
                     dev.max_dpi = 25600;
                }

                dev.dpi_levels = {400, 800, 1600, 3200};
                
                out.push_back(dev);
            }
        }
        return out;
    }

    bool set_dpi(const std::string& path, int dpi) {
        Hidpp20Device dev(path, "");
        if (!dev.connect()) return false;
        return dev.set_dpi(dpi);
    }

    bool set_polling_rate(const std::string& path, int rate_ms) {
        Hidpp20Device dev(path, "");
        if (!dev.connect()) return false;
        return dev.set_polling_rate(rate_ms);
    }
}

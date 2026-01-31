#pragma once
#include <string>
#include <vector>

namespace Logitech {
    
    struct Device {
        std::string path;
        std::string name;
        int current_dpi = 0;
        int max_dpi = 25600;
        std::vector<int> dpi_levels;
        int current_rate_ms;
    };

    void init();
    void shutdown();

    std::vector<Device> get_devices();
    bool set_dpi(const std::string& path, int dpi);
    bool set_polling_rate(const std::string& path, int rate_ms);
    bool set_rgb(const std::string& path, int r, int g, int b);
    bool set_led(const std::string& path, int mode, int r, int g, int b, int period);
}

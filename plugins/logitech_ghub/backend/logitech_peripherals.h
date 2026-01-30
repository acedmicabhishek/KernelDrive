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
    };

    void init();
    void shutdown();

    std::vector<Device> get_devices();
    bool set_dpi(const std::string& path, int dpi);
    bool set_polling_rate(const std::string& path, int rate_ms);
}

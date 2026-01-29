#include "fans.h"
#include "../../../src/core/sysfs_writer.h"
#include <filesystem>
#include <string>

namespace AsusFans {
    static std::string find_asus_hwmon_path() {
        static std::string cached_path;
        if (!cached_path.empty() && std::filesystem::exists(cached_path)) return cached_path;

        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/hwmon")) {
            auto name_path = entry.path() / "name";
            auto name_opt = SysfsWriter::read(name_path.string());
            if (name_opt && name_opt->find("asus") != std::string::npos) {
                cached_path = entry.path().string();
                return cached_path;
            }
        }
        return "";
    }

    bool is_supported() {
        return !find_asus_hwmon_path().empty();
    }

    std::pair<int, int> get_fan_speeds() {
        std::string base = find_asus_hwmon_path();
        if (base.empty()) return {0, 0};

        int cpu = 0, gpu = 0;
        
        if (auto val = SysfsWriter::read(base + "/fan1_input")) {
            try { cpu = std::stoi(*val); } catch (...) {}
        }
        
        if (auto val = SysfsWriter::read(base + "/fan2_input")) {
            try { gpu = std::stoi(*val); } catch (...) {}
        }

        return {cpu, gpu};
    }
}

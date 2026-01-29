#include "fans.h"
#include "../../../src/core/sysfs_writer.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <map>

namespace AsusFans {
    namespace fs = std::filesystem;

    struct SensorPaths {
        std::string cpu_fan;
        std::string gpu_fan;
        std::string cpu_temp;
        std::string gpu_temp;
        bool valid = false;
    };

    static SensorPaths g_sensors;

    static int read_int(const std::string& path) {
        if (path.empty()) return 0;
        auto val = SysfsWriter::read(path);
        if (val) {
            try { return std::stoi(*val); } catch (...) {}
        }
        return 0;
    }

    static void scan_sensors() {
        if (g_sensors.valid) return;

        g_sensors.cpu_fan = "";
        g_sensors.gpu_fan = "";
        g_sensors.cpu_temp = "";
        g_sensors.gpu_temp = "";

        if (fs::exists("/sys/class/hwmon")) {
            for (const auto& entry : fs::directory_iterator("/sys/class/hwmon")) {
                std::string path = entry.path().string();
                std::string name_path = path + "/name";
                
                auto name_opt = SysfsWriter::read(name_path);
                if (name_opt) {
                    std::string name = *name_opt;
                    if (name == "asus") {
                        if (fs::exists(path + "/fan1_input")) g_sensors.cpu_fan = path + "/fan1_input";
                        if (fs::exists(path + "/fan2_input")) g_sensors.gpu_fan = path + "/fan2_input";
                    }
                    if (name == "k10temp" || name == "coretemp") {
                         if (fs::exists(path + "/temp1_input")) g_sensors.cpu_temp = path + "/temp1_input";
                    }
                    if (name == "amdgpu") {
                        if (fs::exists(path + "/temp1_input")) {
                             g_sensors.gpu_temp = path + "/temp1_input";
                        }
                    }
                    if (name == "nvidia") {
                         if (fs::exists(path + "/temp1_input")) g_sensors.gpu_temp = path + "/temp1_input";
                    }
                }
            }
        }

        g_sensors.valid = true;
        
        std::cout << "[AsusFans] Scanned Sensors:" << std::endl;
        std::cout << "  CPU Fan: " << g_sensors.cpu_fan << std::endl;
        std::cout << "  GPU Fan: " << g_sensors.gpu_fan << std::endl;
        std::cout << "  CPU Temp: " << g_sensors.cpu_temp << std::endl;
        std::cout << "  GPU Temp: " << g_sensors.gpu_temp << std::endl;
    }

    FanMetrics get_metrics() {
        scan_sensors();
        FanMetrics m;
        m.cpu_rpm = read_int(g_sensors.cpu_fan);
        m.gpu_rpm = read_int(g_sensors.gpu_fan);
        
        m.cpu_temp = read_int(g_sensors.cpu_temp) / 1000;
        m.gpu_temp = read_int(g_sensors.gpu_temp) / 1000;
        
        return m;
    }

    bool is_supported() {
        scan_sensors();
        return !g_sensors.cpu_fan.empty();
    }
}

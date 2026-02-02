#include "power_manager.h"
#include "../config_manager.h"
#include "../sysfs_writer.h"
#include "backends/linux_power.h" 
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <fstream>

namespace fs = std::filesystem;

PowerManager::PowerManager() {
    backend = std::make_shared<LinuxPowerBackend>();
    apply_stored_settings();
}

PowerManager& PowerManager::get() {
    static PowerManager instance;
    return instance;
}

std::shared_ptr<IPowerBackend> PowerManager::get_backend() {
    return backend;
}

void PowerManager::apply_stored_settings() {
    auto& cfg = ConfigManager::get();
    
    
    std::string profile = cfg.get_string("Power", "profile", "");
    if (!profile.empty() && backend) {
        backend->set_profile(profile);
    }
    
    
    std::string gov = cfg.get_string("Power", "governor", "");
    if (!gov.empty()) {

        std::string base_path = "/sys/devices/system/cpu/";
        if (fs::exists(base_path)) {
            for (const auto& entry : fs::directory_iterator(base_path)) {
                std::string filename = entry.path().filename().string();
                if (filename.rfind("cpu", 0) == 0 && std::isdigit(filename.back())) { 
                     std::string path = entry.path().string() + "/cpufreq/scaling_governor";
                     if (fs::exists(path)) {
                         SysfsWriter::write(path, gov);
                     }
                }
            }
        }
    }
    
    
    int tdp = cfg.get_int("Power", "tdp_limit", 0);
    if (tdp > 0) {
        set_tdp_limit(tdp);
    }
}

void PowerManager::set_profile(const std::string& profile) {
    if(backend) backend->set_profile(profile);
    ConfigManager::get().set_string("Power", "profile", profile);
}

std::string PowerManager::get_active_profile() {
    
    if(backend) return backend->get_profile_info().active_profile;
    return ConfigManager::get().get_string("Power", "profile", "");
}

void PowerManager::set_cpu_governor(const std::string& gov) {
    ConfigManager::get().set_string("Power", "governor", gov);
    
    std::string base_path = "/sys/devices/system/cpu/";
    if (!fs::exists(base_path)) return;

    for (const auto& entry : fs::directory_iterator(base_path)) {
        std::string filename = entry.path().filename().string();
        
        if (filename.rfind("cpu", 0) == 0 && filename.length() > 3 && std::isdigit(filename[3])) { 
             std::string path = entry.path().string() + "/cpufreq/scaling_governor";
             if (fs::exists(path)) {
                 SysfsWriter::write(path, gov);
             }
        }
    }
}

std::string PowerManager::get_cpu_governor() {
    
    std::string path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";
    if (std::filesystem::exists(path)) {
        std::ifstream f(path);
        std::string s;
        if (f.is_open()) {
            f >> s;
            return s;
        }
    }
    return ConfigManager::get().get_string("Power", "governor", "powersave");
}

std::vector<std::string> PowerManager::get_available_governors() {
    if (backend) return backend->get_cpu_governors();
    return {};
}

bool PowerManager::is_rapl_supported() {
    if(!backend) return false;
    return backend->get_power_limit_uw() != -1;
}

void PowerManager::set_tdp_limit(int watts) {
    if(!backend) return;
    
    long uw;
    if (watts <= 0) {
        
        uw = backend->get_max_power_limit_uw();
    } else {
        uw = (long)watts * 1000000;
    }
    
    backend->set_power_limit_uw(uw);
    ConfigManager::get().set_int("Power", "tdp_limit", watts);
}

int PowerManager::get_tdp_limit() {
    
    
    if(!backend) return 0;
    long uw = backend->get_power_limit_uw();
    if (uw <= 0) return 0;
    return (int)(uw / 1000000);
}

#include "linux_power.h"
#include "../../sysfs_writer.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace fs = std::filesystem;

bool LinuxPowerBackend::is_available() {
    return true; 
}

std::vector<BatteryInfo> LinuxPowerBackend::get_batteries() {
        std::vector<BatteryInfo> batteries;
        std::string base = "/sys/class/power_supply/";
        if (!fs::exists(base)) return batteries;
        
        for (const auto& entry : fs::directory_iterator(base)) {
            std::string name = entry.path().filename().string();
            
            
            std::string type = read_file(entry.path() / "type");
            if (remove_newline(type) != "Battery") continue;
            
            BatteryInfo info;
            info.present = true;
            
            
            std::string cap = read_file(entry.path() / "capacity");
            info.percentage = safe_stoi(cap);
            
            
            std::string st = remove_newline(read_file(entry.path() / "status"));
            if (st == "Charging") info.state = BatteryState::Charging;
            else if (st == "Discharging") info.state = BatteryState::Discharging;
            else if (st == "Full") info.state = BatteryState::Full;
            else if (st == "Not charging") info.state = BatteryState::NotCharging;
            else info.state = BatteryState::Unknown;
            

            long energy_now = safe_stol(read_file(entry.path() / "energy_now"));
            long energy_full = safe_stol(read_file(entry.path() / "energy_full"));
            long power_now = safe_stol(read_file(entry.path() / "power_now"));
            
            
            if (energy_now == 0) {
                 long charge_now = safe_stol(read_file(entry.path() / "charge_now"));
                 long charge_full = safe_stol(read_file(entry.path() / "charge_full"));
                 long voltage = safe_stol(read_file(entry.path() / "voltage_now"));
                 
                 energy_now = (long)((double)charge_now * voltage / 1000000.0);
                 energy_full = (long)((double)charge_full * voltage / 1000000.0);
            }
             if (power_now == 0) {
                 long current = safe_stol(read_file(entry.path() / "current_now"));
                 long voltage = safe_stol(read_file(entry.path() / "voltage_now"));
                 power_now = (long)((double)current * voltage / 1000000.0);
            }

            info.energy_now_wh = energy_now / 1000000.0;
            info.energy_full_wh = energy_full / 1000000.0;
            info.energy_rate_w = power_now / 1000000.0;
            
            
            if (info.energy_rate_w > 0.1) {
                double hours = 0;
                if (info.state == BatteryState::Discharging) {
                    hours = info.energy_now_wh / info.energy_rate_w;
                } else if (info.state == BatteryState::Charging) {
                     hours = (info.energy_full_wh - info.energy_now_wh) / info.energy_rate_w;
                }
                
                if (hours > 0) {
                    int h = (int)hours;
                    int m = (int)((hours - h) * 60);
                    info.time_remaining_str = std::to_string(h) + "h " + std::to_string(m) + "m";
                }
            }
            
            batteries.push_back(info);
        }
        return batteries;
}

PowerProfileInfo LinuxPowerBackend::get_profile_info() {
    PowerProfileInfo info;
    std::string path = "/sys/firmware/acpi/platform_profile";
    if (fs::exists(path)) {
        info.active_profile = remove_newline(read_file(path));
        
        std::string choices = read_file("/sys/firmware/acpi/platform_profile_choices");
        std::stringstream ss(choices);
        std::string item;
        while (ss >> item) {
            info.available_profiles.push_back(item);
        }
    }
    return info;
}

void LinuxPowerBackend::set_profile(const std::string& profile) {
    std::string path = "/sys/firmware/acpi/platform_profile";
    if (fs::exists(path)) {
        SysfsWriter::write(path, profile);
    }
}

std::vector<std::string> LinuxPowerBackend::get_cpu_governors() {
    std::vector<std::string> govs;
    std::string path = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors";
    if (fs::exists(path)) {
        std::string content = read_file(path);
        std::stringstream ss(content);
        std::string item;
        while (ss >> item) {
            govs.push_back(item);
        }
    }
    return govs;
}


static std::string find_powercap_package_path() {
    
    std::string path = "/sys/class/powercap/intel-rapl/intel-rapl:0/constraint_0_power_limit_uw";
    if (fs::exists(path)) return "/sys/class/powercap/intel-rapl/intel-rapl:0";
    
    
    if (fs::exists("/sys/class/powercap/intel-rapl:0/constraint_0_power_limit_uw")) 
        return "/sys/class/powercap/intel-rapl:0";
        
    
    std::string base = "/sys/class/powercap";
    if (fs::exists(base)) {
        for (const auto& entry : fs::directory_iterator(base)) {
            
            std::string name_path = entry.path().string() + "/name";
            if (fs::exists(name_path)) {
                std::ifstream f(name_path);
                std::string name;
                f >> name;
                
                if (name.find("package") != std::string::npos || name.find("domain") != std::string::npos) {
                     if (fs::exists(entry.path().string() + "/constraint_0_power_limit_uw")) {
                         return entry.path().string();
                     }
                }
            }
        }
    }
    return "";
}

long LinuxPowerBackend::get_power_limit_uw() {
    
    std::string base = find_powercap_package_path();
    if (!base.empty()) {
        std::string p = base + "/constraint_0_power_limit_uw";
        return safe_stol(read_file(p));
    }
    
    
    if (fs::exists("/usr/bin/ryzenadj") || fs::exists("/usr/local/bin/ryzenadj")) {
        
        
        
        return 15000000; 
    }
    
    return -1;
}

long LinuxPowerBackend::get_max_power_limit_uw() {
    
    std::string base = find_powercap_package_path();
    if (!base.empty()) {
        
        std::string p = base + "/constraint_0_max_power_uw";
        if (fs::exists(p)) return safe_stol(read_file(p));
        
        
        p = base + "/max_power_uw";
        if (fs::exists(p)) return safe_stol(read_file(p));
    }

    
    if (fs::exists("/usr/bin/ryzenadj") || fs::exists("/usr/local/bin/ryzenadj")) {
        return 80000000; 
    }

    return 65000000; 
}

void LinuxPowerBackend::set_power_limit_uw(long uw) {
    
    std::string base = find_powercap_package_path();
    if (!base.empty()) {
        std::string p = base + "/constraint_0_power_limit_uw";
        SysfsWriter::write(p, std::to_string(uw));
            
        std::string en = base + "/enabled";
        if (fs::exists(en)) SysfsWriter::write(en, "1");
        return;
    }
    
    
    if (fs::exists("/usr/bin/ryzenadj") || fs::exists("/usr/local/bin/ryzenadj")) {
        long mw = uw / 1000;
        std::string cmd = "pkexec ryzenadj --stapm-limit=" + std::to_string(mw) + 
                          " --fast-limit=" + std::to_string(mw) + 
                          " --slow-limit=" + std::to_string(mw) + " >/dev/null 2>&1 &";
        system(cmd.c_str());
    }
}
    

std::string LinuxPowerBackend::read_file(const fs::path& p) {
    if (!fs::exists(p)) return "";
    std::ifstream f(p);
    std::stringstream buffer;
    buffer << f.rdbuf();
    return buffer.str();
}

std::string LinuxPowerBackend::remove_newline(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
    return s;
}

int LinuxPowerBackend::safe_stoi(const std::string& s) {
    try { return std::stoi(s); } catch(...) { return 0; }
}

long LinuxPowerBackend::safe_stol(const std::string& s) {
    try { return std::stol(s); } catch(...) { return 0; }
}

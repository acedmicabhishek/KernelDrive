#pragma once
#include "../power_backend.h"
#include <filesystem>

class LinuxPowerBackend : public IPowerBackend {
public:
    bool is_available() override;
    std::vector<BatteryInfo> get_batteries() override;
    PowerProfileInfo get_profile_info() override;
    void set_profile(const std::string& profile) override;
    std::vector<std::string> get_cpu_governors() override;
    long get_power_limit_uw() override;
    long get_max_power_limit_uw() override;
    void set_power_limit_uw(long uw) override;
    
private:
    std::string read_file(const std::filesystem::path& p);
    std::string remove_newline(std::string s);
    int safe_stoi(const std::string& s);
    long safe_stol(const std::string& s);
};

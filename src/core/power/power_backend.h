#pragma once
#include <string>
#include <vector>
#include <optional>

enum class BatteryState {
    Unknown,
    Charging,
    Discharging,
    Full,
    NotCharging
};

struct BatteryInfo {
    bool present;
    int percentage; 
    BatteryState state;
    double energy_rate_w; 
    double energy_now_wh;
    double energy_full_wh;
    
    
    std::string time_remaining_str; 
};

struct PowerProfileInfo {
    std::string active_profile; 
    std::vector<std::string> available_profiles;
};

class IPowerBackend {
public:
    virtual ~IPowerBackend() = default;
    
    virtual bool is_available() = 0;
    
    
    virtual std::vector<BatteryInfo> get_batteries() = 0;
    
    
    virtual PowerProfileInfo get_profile_info() = 0;
    virtual void set_profile(const std::string& profile) = 0;
    
    
    virtual std::vector<std::string> get_cpu_governors() = 0;
    
    
    virtual long get_power_limit_uw() = 0; 
    virtual long get_max_power_limit_uw() = 0; 
    virtual void set_power_limit_uw(long uw) = 0;
};

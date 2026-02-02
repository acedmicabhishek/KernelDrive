#pragma once
#include "power_backend.h"
#include <memory>

class PowerManager {
public:
    static PowerManager& get();
    
    std::shared_ptr<IPowerBackend> get_backend();
    
    
    void set_profile(const std::string& profile);
    std::string get_active_profile(); 
    
    void set_cpu_governor(const std::string& gov);
    std::string get_cpu_governor();
    std::vector<std::string> get_available_governors();
    
    
    bool is_rapl_supported();
    void set_tdp_limit(int watts); 
    int get_tdp_limit();
    
private:
    PowerManager();
    void apply_stored_settings();
    std::shared_ptr<IPowerBackend> backend;
};

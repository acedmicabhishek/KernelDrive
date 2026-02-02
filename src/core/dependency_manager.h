#pragma once
#include <string>
#include <vector>
#include <map>

struct Dependency {
    std::string name;
    std::string description;
    std::string check_command;  
    std::string install_arch;   
    std::string install_debian; 
    std::string install_fedora; 
    bool required;              
};

class DependencyManager {
public:
    static DependencyManager& get();
    
    
    std::vector<Dependency> get_missing_dependencies();
    
    
    std::vector<Dependency> get_all_dependencies();
    
    
    bool is_available(const std::string& tool_name);
    
    
    std::string get_install_command(const Dependency& dep);
    
    
    std::string get_distro_id();
    
private:
    DependencyManager();
    void init_dependencies();
    
    std::vector<Dependency> dependencies;
    std::string distro_id;
};

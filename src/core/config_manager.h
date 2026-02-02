#pragma once
#include <string>
#include <glib.h>

class ConfigManager {
public:
    static ConfigManager& get();
    
    ConfigManager();
    ~ConfigManager();
    
    
    void set_string(const std::string& group, const std::string& key, const std::string& val);
    std::string get_string(const std::string& group, const std::string& key, const std::string& default_val = "");
    
    void set_bool(const std::string& group, const std::string& key, bool val);
    bool get_bool(const std::string& group, const std::string& key, bool default_val = false);
    
    void set_int(const std::string& group, const std::string& key, int val);
    int get_int(const std::string& group, const std::string& key, int default_val = 0);
    
    void set_double(const std::string& group, const std::string& key, double val);
    double get_double(const std::string& group, const std::string& key, double default_val = 0.0);
    
    void save();
    
private:
    void ensure_dir();
    std::string get_config_path();
    
    GKeyFile* key_file;
    std::string config_path;
};

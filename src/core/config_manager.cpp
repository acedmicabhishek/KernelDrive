#include "config_manager.h"
#include <glib/gstdio.h>
#include <sys/stat.h>
#include <iostream>
#include <filesystem>

ConfigManager& ConfigManager::get() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    key_file = g_key_file_new();
    config_path = get_config_path();
    
    
    GError* error = nullptr;
    if (!g_key_file_load_from_file(key_file, config_path.c_str(), G_KEY_FILE_NONE, &error)) {
        
        if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            std::cerr << "ConfigManager: Failed to load config: " << error->message << std::endl;
        }
        g_error_free(error);
    } else {
        std::cout << "ConfigManager: Loaded from " << config_path << std::endl;
    }
}

ConfigManager::~ConfigManager() {
    if (key_file) g_key_file_free(key_file);
}

std::string ConfigManager::get_config_path() {
    const char* config_home = g_get_user_config_dir();
    std::string path = std::string(config_home) + "/kerneldrive";
    return path + "/settings.ini";
}

void ConfigManager::ensure_dir() {
    const char* config_home = g_get_user_config_dir();
    std::string dir = std::string(config_home) + "/kerneldrive";
    std::filesystem::create_directories(dir);
}

void ConfigManager::save() {
    ensure_dir();
    GError* error = nullptr;
    if (!g_key_file_save_to_file(key_file, config_path.c_str(), &error)) {
        std::cerr << "ConfigManager: Failed to save: " << error->message << std::endl;
        g_error_free(error);
    }
}


void ConfigManager::set_string(const std::string& group, const std::string& key, const std::string& val) {
    g_key_file_set_string(key_file, group.c_str(), key.c_str(), val.c_str());
    save(); 
}

std::string ConfigManager::get_string(const std::string& group, const std::string& key, const std::string& default_val) {
    GError* error = nullptr;
    gchar* val = g_key_file_get_string(key_file, group.c_str(), key.c_str(), &error);
    if (error) {
        g_error_free(error);
        return default_val;
    }
    std::string ret = val;
    g_free(val);
    return ret;
}

void ConfigManager::set_bool(const std::string& group, const std::string& key, bool val) {
    g_key_file_set_boolean(key_file, group.c_str(), key.c_str(), val);
    save();
}

bool ConfigManager::get_bool(const std::string& group, const std::string& key, bool default_val) {
    GError* error = nullptr;
    gboolean val = g_key_file_get_boolean(key_file, group.c_str(), key.c_str(), &error);
    if (error) {
        g_error_free(error);
        return default_val;
    }
    return val;
}

void ConfigManager::set_int(const std::string& group, const std::string& key, int val) {
    g_key_file_set_integer(key_file, group.c_str(), key.c_str(), val);
    save();
}

int ConfigManager::get_int(const std::string& group, const std::string& key, int default_val) {
    GError* error = nullptr;
    int val = g_key_file_get_integer(key_file, group.c_str(), key.c_str(), &error);
    if (error) {
        g_error_free(error);
        return default_val;
    }
    return val;
}

void ConfigManager::set_double(const std::string& group, const std::string& key, double val) {
    g_key_file_set_double(key_file, group.c_str(), key.c_str(), val);
    save();
}

double ConfigManager::get_double(const std::string& group, const std::string& key, double default_val) {
    GError* error = nullptr;
    double val = g_key_file_get_double(key_file, group.c_str(), key.c_str(), &error);
    if (error) {
        g_error_free(error);
        return default_val;
    }
    return val;
}

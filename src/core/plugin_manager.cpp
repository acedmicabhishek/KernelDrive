#include "plugin_manager.h"
#include <dlfcn.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

std::string expand_user(std::string path) {
    if (path.empty()) return path;
    if (path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            path.replace(0, 1, home);
        }
    }
    return path;
}

static PluginManager::UninstallCallback g_uninstall_cb = nullptr;

void PluginManager::set_uninstall_callback(UninstallCallback cb) {
    g_uninstall_cb = cb;
}

PluginManager& PluginManager::get() {
    static PluginManager instance;
    return instance;
}

void PluginManager::load_default_locations() {
    load_from_directory(expand_user("~/.local/share/kerneldrive/plugins")); 
    load_from_directory("build"); 
    load_from_directory("/usr/lib/kerneldrive/plugins");
}

const std::vector<std::unique_ptr<KdPlugin>>& PluginManager::get_plugins() const {
    return plugins;
}

static PluginManager::PluginLoadedCallback g_loaded_cb = nullptr;

void PluginManager::set_plugin_loaded_callback(PluginLoadedCallback cb) {
    g_loaded_cb = cb;
}

void PluginManager::load_plugin(const std::string& path) {
    if (!fs::exists(path) || fs::path(path).extension() != ".so") return;
    
    // Check for duplicates
    std::string abs_path = fs::absolute(path).string();
    for (const auto& existing : plugin_paths) {
        if (fs::equivalent(existing, abs_path)) return; 
    }

    try {
        void* handle = dlopen(abs_path.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
            return;
        }
        dlerror();

        CreatePluginFunc create_plugin = (CreatePluginFunc)dlsym(handle, "create_plugin");
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            dlclose(handle);
            return;
        }

        KdPlugin* plugin = create_plugin();
        if (plugin && plugin->init()) {
            for (const auto& p : plugins) {
                if (p->get_slug() == plugin->get_slug()) {
                     std::cout << "Ignoring duplicate plugin slug: " << plugin->get_slug() << std::endl;
                     delete plugin;
                     dlclose(handle);
                     return;
                }
            }

            plugins.emplace_back(plugin);
            library_handles.push_back(handle);
            plugin_paths.push_back(abs_path);
            std::cout << "Loaded plugin: " << plugin->get_name() << std::endl;
            
            if (g_loaded_cb) g_loaded_cb(plugin);
        } else {
            if (plugin) delete plugin;
            dlclose(handle);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading plugin " << path << ": " << e.what() << std::endl;
    }
}

void PluginManager::load_from_directory(const std::string& path) {
    if (!fs::exists(path)) return;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".so") {
                load_plugin(entry.path().string());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory " << path << ": " << e.what() << std::endl;
    }
}

bool PluginManager::uninstall_plugin(KdPlugin* plugin) {
    for (size_t i = 0; i < plugins.size(); ++i) {
        if (plugins[i].get() == plugin) {
            std::cout << "Uninstalling plugin: " << plugin->get_name() << std::endl;
            
            if (g_uninstall_cb) g_uninstall_cb(plugin);

            try {
                fs::remove(plugin_paths[i]);
            } catch (const fs::filesystem_error& e) {
                std::cerr << "Failed to delete plugin file: " << e.what() << std::endl;
                return false;
            }
            
            void* handle = library_handles[i];
            
            plugins.erase(plugins.begin() + i);
            plugin_paths.erase(plugin_paths.begin() + i);
            library_handles.erase(library_handles.begin() + i);

            dlclose(handle);
            
            return true;
        }
    }
    return false;
}

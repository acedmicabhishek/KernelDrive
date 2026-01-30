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
    load_from_directory("build"); 
    load_from_directory("/usr/lib/kerneldrive/plugins");
    load_from_directory(expand_user("~/.local/share/kerneldrive/plugins")); 
}

const std::vector<std::unique_ptr<KdPlugin>>& PluginManager::get_plugins() const {
    return plugins;
}

void PluginManager::load_from_directory(const std::string& path) {
    if (!fs::exists(path)) return;

    for (const auto& entry : fs::directory_iterator(path)) {
        if (entry.path().extension() == ".so") {
            std::string filepath = entry.path().string();
            
            void* handle = dlopen(filepath.c_str(), RTLD_LAZY);
            if (!handle) {
                std::cerr << "Failed to load plugin: " << dlerror() << std::endl;
                continue;
            }
            dlerror();

            CreatePluginFunc create_plugin = (CreatePluginFunc)dlsym(handle, "create_plugin");
            const char* dlsym_error = dlerror();
            if (dlsym_error) {
                std::cerr << "Cannot load symbol 'create_plugin': " << dlsym_error << std::endl;
                dlclose(handle);
                continue;
            }

            KdPlugin* plugin = create_plugin();
            if (plugin && plugin->init()) {
                plugins.emplace_back(plugin);
                library_handles.push_back(handle);
                plugin_paths.push_back(filepath);
                std::cout << "Loaded plugin: " << plugin->get_name() << std::endl;
            } else {
                if (plugin) delete plugin;
                dlclose(handle);
            }
        }
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

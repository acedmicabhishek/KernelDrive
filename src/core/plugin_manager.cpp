#include "plugin_manager.h"
#include <dlfcn.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

PluginManager& PluginManager::get() {
    static PluginManager instance;
    return instance;
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
                std::cout << "Loaded plugin: " << plugin->get_name() << std::endl;
            } else {
                if (plugin) delete plugin;
                dlclose(handle);
            }
        }
    }
}

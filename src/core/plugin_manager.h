#pragma once

#include "plugin_interface.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

class PluginManager {
public:
    using UninstallCallback = std::function<void(KdPlugin*)>;
    void set_uninstall_callback(UninstallCallback cb);

    using PluginLoadedCallback = std::function<void(KdPlugin*)>;
    void set_plugin_loaded_callback(PluginLoadedCallback cb);

    void load_plugin(const std::string& path);
    void load_from_directory(const std::string& path);
    void load_default_locations();

    const std::vector<std::unique_ptr<KdPlugin>>& get_plugins() const;

    static PluginManager& get();
    
    bool uninstall_plugin(KdPlugin* plugin);

private:
   std::vector<std::unique_ptr<KdPlugin>> plugins;
   std::vector<void*> library_handles;
   std::vector<std::string> plugin_paths;
};

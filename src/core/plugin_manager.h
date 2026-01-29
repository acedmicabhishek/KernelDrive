#pragma once

#include "plugin_interface.h"
#include <vector>
#include <memory>
#include <string>

class PluginManager {
public:
    void load_from_directory(const std::string& path);

    const std::vector<std::unique_ptr<KdPlugin>>& get_plugins() const;

    static PluginManager& get();

private:
   std::vector<std::unique_ptr<KdPlugin>> plugins;
   std::vector<void*> library_handles;
};

#pragma once
#include "../../src/core/plugin_interface.h"
#include <gtk/gtk.h>

class SystemInfoPlugin : public KdPlugin {
public:
    std::string get_name() const override { return "System Info"; }
    std::string get_slug() const override { return "system_info"; }
    
    bool init() override { return true; }
    
    GtkWidget* create_config_widget() override;
};

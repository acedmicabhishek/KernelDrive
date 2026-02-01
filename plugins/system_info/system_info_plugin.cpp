#include "system_info_plugin.h"
#include "ui/info_page.h"

GtkWidget* SystemInfoPlugin::create_config_widget() {
    return InfoPage::create();
}

extern "C" KdPlugin* create_plugin() {
    return new SystemInfoPlugin();
}

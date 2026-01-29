#include "src/core/plugin_interface.h"
#include <gtk/gtk.h>
#include <adwaita.h>

class SystemInfoPlugin : public KdPlugin {
public:
    std::string get_name() const override {
        return "System Info";
    }

    std::string get_slug() const override {
        return "system-info";
    }

    bool init() override {
        return true;
    }

    GtkWidget* create_config_widget() override {
        GtkWidget* page = adw_preferences_page_new();
        GtkWidget* group = adw_preferences_group_new();
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "System Information");
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

        GtkWidget* row_kernel = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row_kernel), "Kernel Version");
        
        // Get kernel version
        FILE* fp = popen("uname -r", "r");
        char buffer[128];
        if (fp) {
            if (fgets(buffer, sizeof(buffer), fp)) {
                buffer[strcspn(buffer, "\n")] = 0;
                adw_action_row_set_subtitle(ADW_ACTION_ROW(row_kernel), buffer);
            }
            pclose(fp);
        } else {
             adw_action_row_set_subtitle(ADW_ACTION_ROW(row_kernel), "Unknown");
        }
        
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), row_kernel);
        return page;
    }
};

extern "C" KdPlugin* create_plugin() {
    return new SystemInfoPlugin();
}

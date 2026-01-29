#include "../../src/core/plugin_interface.h"
#include "backend/logitech_peripherals.h"
#include <gtk/gtk.h>
#include <adwaita.h>
#include <string>

class LogitechPlugin : public KdPlugin {
public:
    std::string get_name() const override {
        return "Logitech G-Hub";
    }

    std::string get_slug() const override {
        return "logitech-ghub";
    }

    bool init() override {
        return LogitechPeripherals::is_supported();
    }

    GtkWidget* create_config_widget() override {
        GtkWidget* page = adw_preferences_page_new();
        GtkWidget* group = adw_preferences_group_new();
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Mouse Settings");
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

        GtkWidget* dpi_row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(dpi_row), "DPI Sensitivity");
        
        GtkWidget* dpi_spin = gtk_spin_button_new_with_range(100, 25600, 50);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(dpi_spin), LogitechPeripherals::get_dpi());
        
        g_signal_connect(dpi_spin, "value-changed", G_CALLBACK(+[](GtkSpinButton* btn, gpointer) {
            int val = gtk_spin_button_get_value_as_int(btn);
            LogitechPeripherals::set_dpi(val);
        }), NULL);
        
        adw_action_row_add_suffix(ADW_ACTION_ROW(dpi_row), dpi_spin);
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), dpi_row);

        GtkWidget* rate_row = adw_combo_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(rate_row), "Polling Rate");
        
        const char* rates[] = {"125 Hz", "250 Hz", "500 Hz", "1000 Hz", NULL};
        GtkStringList* rate_list = gtk_string_list_new(rates);
        adw_combo_row_set_model(ADW_COMBO_ROW(rate_row), G_LIST_MODEL(rate_list));

        int current = LogitechPeripherals::get_polling_rate();
        int idx = 3;
        if (current == 125) idx = 0;
        else if (current == 250) idx = 1;
        else if (current == 500) idx = 2;
        
        adw_combo_row_set_selected(ADW_COMBO_ROW(rate_row), idx);

        g_signal_connect(rate_row, "notify::selected", G_CALLBACK(+[](AdwComboRow* row, GParamSpec*, gpointer) {
            guint selected = adw_combo_row_get_selected(row);
            int rate = 1000;
            if (selected == 0) rate = 125;
            else if (selected == 1) rate = 250;
            else if (selected == 2) rate = 500;
            
            LogitechPeripherals::set_polling_rate(rate);
        }), NULL);

        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), rate_row);

        return page;
    }
};

extern "C" KdPlugin* create_plugin() {
    return new LogitechPlugin();
}

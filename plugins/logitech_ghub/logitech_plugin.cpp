#include "../../src/core/plugin_interface.h"
#include "backend/logitech_peripherals.h"
#include <adwaita.h>
#include <string>

class LogitechPlugin : public KdPlugin {
public:
    std::string get_name() const override { return "Logitech G-Hub"; }
    std::string get_slug() const override { return "logitech_ghub"; }

    bool init() override {
        Logitech::init();
        return true; 
    }

    GtkWidget* create_config_widget() override {
        AdwStatusPage* page = ADW_STATUS_PAGE(adw_status_page_new());
        adw_status_page_set_title(page, "Logitech G-Hub");
        adw_status_page_set_description(page, "Manage your Logitech peripherals.");
        adw_status_page_set_icon_name(page, "input-mouse-symbolic");

        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
        gtk_widget_set_valign(box, GTK_ALIGN_START);
        gtk_widget_set_margin_top(box, 32);
        gtk_widget_set_margin_bottom(box, 32);
        gtk_widget_set_margin_start(box, 24);
        gtk_widget_set_margin_end(box, 24);

        adw_status_page_set_child(page, box);

        GtkWidget* scan_btn = gtk_button_new_with_label("Scan for Devices");
        gtk_widget_set_halign(scan_btn, GTK_ALIGN_CENTER);
        gtk_widget_add_css_class(scan_btn, "pill");
        gtk_box_append(GTK_BOX(box), scan_btn);

        GtkWidget* device_list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_box_append(GTK_BOX(box), device_list);

        g_signal_connect(scan_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
            GtkWidget* list = GTK_WIDGET(data);

            GtkWidget* child = gtk_widget_get_first_child(list);
            while (child) {
                GtkWidget* next = gtk_widget_get_next_sibling(child);
                gtk_box_remove(GTK_BOX(list), child);
                child = next;
            }

            auto devices = Logitech::get_devices();
            if (devices.empty()) {
                 GtkWidget* lbl = gtk_label_new("No Logitech devices found (check permissions?).");
                 gtk_widget_add_css_class(lbl, "dim-label");
                 gtk_box_append(GTK_BOX(list), lbl);
            }

            for (const auto& dev : devices) {
                GtkWidget* group = adw_preferences_group_new();
                std::string title = dev.name;
                if (dev.path.find("/dev/hidraw") != std::string::npos) {
                    title += " (" + dev.path + ")";
                }
                adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), title.c_str());
                
                GtkWidget* dpi_row = adw_action_row_new();
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(dpi_row), "DPI Sensitivity");
                
                GtkWidget* spin = gtk_spin_button_new_with_range(100, dev.max_dpi, 50);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), dev.current_dpi);
                gtk_widget_set_valign(spin, GTK_ALIGN_CENTER);
                
                struct CbData { std::string path; };
                CbData* cbd = new CbData{dev.path};
                g_object_set_data_full(G_OBJECT(spin), "cbd", cbd, [](gpointer d) { delete (CbData*)d; });

                g_signal_connect(spin, "value-changed", G_CALLBACK(+[](GtkSpinButton* btn, gpointer) {
                    CbData* d = (CbData*)g_object_get_data(G_OBJECT(btn), "cbd");
                    int val = gtk_spin_button_get_value_as_int(btn);
                    Logitech::set_dpi(d->path, val);
                }), nullptr);

                adw_action_row_add_suffix(ADW_ACTION_ROW(dpi_row), spin);
                adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), dpi_row);
                
                gtk_box_append(GTK_BOX(list), group);
            }
        }), device_list);

        return GTK_WIDGET(page);
    }
};

extern "C" KdPlugin* create_plugin() {
    return new LogitechPlugin();
}

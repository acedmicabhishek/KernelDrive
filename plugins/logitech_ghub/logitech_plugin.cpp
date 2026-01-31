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

        g_signal_connect(scan_btn, "clicked", (GCallback)(+[](GtkButton*, gpointer data) {
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

                g_signal_connect(spin, "value-changed", (GCallback)(+[](GtkSpinButton* btn, gpointer) {
                    CbData* d = (CbData*)g_object_get_data(G_OBJECT(btn), "cbd");
                    int val = gtk_spin_button_get_value_as_int(btn);
                    Logitech::set_dpi(d->path, val);
                }), nullptr);

                adw_action_row_add_suffix(ADW_ACTION_ROW(dpi_row), spin);
                adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), dpi_row);

                // Polling Rate
                GtkWidget* rate_row = adw_combo_row_new();
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(rate_row), "Polling Rate");
                
                const char* rates[] = {"1000 Hz", "500 Hz", "250 Hz", "125 Hz", nullptr};
                GtkStringList* rate_list = gtk_string_list_new(rates);
                adw_combo_row_set_model(ADW_COMBO_ROW(rate_row), G_LIST_MODEL(rate_list));
                // Default to 1000Hz (Index 0) for now
                int rate_idx = 0;
                if (dev.current_rate_ms == 2) rate_idx = 1;
                else if (dev.current_rate_ms == 4) rate_idx = 2;
                else if (dev.current_rate_ms == 8) rate_idx = 3;
                
                adw_combo_row_set_selected(ADW_COMBO_ROW(rate_row), rate_idx);

                CbData* cbd_rate = new CbData{dev.path};
                g_object_set_data_full(G_OBJECT(rate_row), "cbd", cbd_rate, [](gpointer d) { delete (CbData*)d; });

                g_signal_connect(rate_row, "notify::selected", (GCallback)(+[](AdwComboRow* row, GParamSpec*, gpointer) {
                    CbData* d = (CbData*)g_object_get_data(G_OBJECT(row), "cbd");
                    guint selected = adw_combo_row_get_selected(row);
                    
                    int ms = 1;
                    if (selected == 1) ms = 2;
                    else if (selected == 2) ms = 4;
                    else if (selected == 3) ms = 8;
                    
                    Logitech::set_polling_rate(d->path, ms);
                }), nullptr);

                adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), rate_row);
                
                // RGB Mode
                GtkWidget* mode_row = adw_combo_row_new();
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(mode_row), "Lighting Mode (Experimental)");
                const char* modes[] = {"Off", "Static", "Breathing", "Cycle", nullptr};
                // 0=Off, 1=Static, 2=Breathing, 3=Cycle : experimenetal , idk dude we might need to update this sector
                adw_combo_row_set_model(ADW_COMBO_ROW(mode_row), G_LIST_MODEL(gtk_string_list_new(modes)));
                adw_combo_row_set_selected(ADW_COMBO_ROW(mode_row), 1);
                adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), mode_row);

                GtkWidget* rgb_row = adw_action_row_new();
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(rgb_row), "Color (Experimental)");
                
                GtkWidget* color_btn = gtk_color_dialog_button_new(gtk_color_dialog_new());
                gtk_widget_set_valign(color_btn, GTK_ALIGN_CENTER);
                GdkRGBA color = {0.0, 1.0, 1.0, 1.0};
                gtk_color_dialog_button_set_rgba(GTK_COLOR_DIALOG_BUTTON(color_btn), &color);
                adw_action_row_add_suffix(ADW_ACTION_ROW(rgb_row), color_btn);
                adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), rgb_row);

                GtkWidget* speed_row = adw_action_row_new();
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(speed_row), "Cycle Speed (ms) (Experimental)");
                GtkWidget* speed_spin = gtk_spin_button_new_with_range(100, 20000, 100);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(speed_spin), 5000); // 5s default
                gtk_widget_set_valign(speed_spin, GTK_ALIGN_CENTER);
                adw_action_row_add_suffix(ADW_ACTION_ROW(speed_row), speed_spin);
                adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), speed_row);

                struct RGBCbData {
                    std::string path;
                    GtkWidget* mode_w;
                    GtkWidget* color_w;
                    GtkWidget* speed_w;
                };
                RGBCbData* rgb_data = new RGBCbData{dev.path, mode_row, color_btn, speed_spin};
                auto get_rgb_and_update = [](RGBCbData* d) {
                    int mode_idx = adw_combo_row_get_selected(ADW_COMBO_ROW(d->mode_w));
                    const GdkRGBA* c = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(d->color_w));
                    int r = (int)(c->red * 255);
                    int g = (int)(c->green * 255);
                    int b = (int)(c->blue * 255);
                    int period = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->speed_w));
                    Logitech::set_led(d->path, mode_idx, r, g, b, period);
                };

                g_object_set_data_full(G_OBJECT(group), "rgb_data", rgb_data, [](gpointer d){ delete (RGBCbData*)d; });
                
                g_signal_connect(mode_row, "notify::selected", (GCallback)(+[](GtkWidget*, GParamSpec*, gpointer data) {
                     RGBCbData* d = (RGBCbData*)data;
                     int mode_idx = adw_combo_row_get_selected(ADW_COMBO_ROW(d->mode_w));
                     const GdkRGBA* c = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(d->color_w));
                     int r = (int)(c->red * 255);
                     int g = (int)(c->green * 255);
                     int b = (int)(c->blue * 255);
                     int period = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->speed_w));
                     Logitech::set_led(d->path, mode_idx, r, g, b, period);
                }), rgb_data);

                g_signal_connect(color_btn, "notify::rgba", (GCallback)(+[](GtkWidget*, GParamSpec*, gpointer data) {
                     RGBCbData* d = (RGBCbData*)data;
                     int mode_idx = adw_combo_row_get_selected(ADW_COMBO_ROW(d->mode_w));
                     const GdkRGBA* c = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(d->color_w));
                     int r = (int)(c->red * 255);
                     int g = (int)(c->green * 255);
                     int b = (int)(c->blue * 255);
                     int period = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->speed_w));
                     Logitech::set_led(d->path, mode_idx, r, g, b, period);
                }), rgb_data);
                
                g_signal_connect(speed_spin, "value-changed", (GCallback)(+[](GtkWidget*, gpointer data) {
                     RGBCbData* d = (RGBCbData*)data;
                     int mode_idx = adw_combo_row_get_selected(ADW_COMBO_ROW(d->mode_w));
                     const GdkRGBA* c = gtk_color_dialog_button_get_rgba(GTK_COLOR_DIALOG_BUTTON(d->color_w));
                     int r = (int)(c->red * 255);
                     int g = (int)(c->green * 255);
                     int b = (int)(c->blue * 255);
                     int period = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(d->speed_w));
                     Logitech::set_led(d->path, mode_idx, r, g, b, period);
                }), rgb_data);
                
                gtk_box_append(GTK_BOX(list), group);
            }
        }), device_list);

        return GTK_WIDGET(page);
    }
};

extern "C" KdPlugin* create_plugin() {
    return new LogitechPlugin();
}

#include "../../src/core/plugin_interface.h"
#include "backend/modes.h"
#include "backend/fans.h"
#include "backend/battery.h"
#include <gtk/gtk.h>
#include <adwaita.h>
#include <iostream>

class AsusArmouryPlugin : public KdPlugin {
public:
    std::string get_name() const override {
        return "Asus Armoury Control";
    }

    std::string get_slug() const override {
        return "asus-armoury-control";
    }

    bool init() override {
        return AsusModes::is_supported() || AsusFans::is_supported() || AsusBattery::is_supported();
    }

    GtkWidget* create_config_widget() override {
        GtkWidget* page = adw_preferences_page_new();
        GtkWidget* group = adw_preferences_group_new();
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Performance Modes");
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));

        GtkWidget* status_row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(status_row), "Current Mode");
        adw_action_row_set_subtitle(ADW_ACTION_ROW(status_row), "Reading...");
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), status_row);

        auto update_status_text = [](GtkWidget* row, AsusMode mode) {
            switch (mode) {
                case AsusMode::Silent:   adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Silent"); break;
                case AsusMode::Balanced: adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Balanced"); break;
                case AsusMode::Turbo:    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Turbo"); break;
                default:                 adw_action_row_set_subtitle(ADW_ACTION_ROW(row), "Unknown"); break;
            }
        };

        GtkWidget* fan_group = adw_preferences_group_new();
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(fan_group), "Fan Speeds");
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(fan_group));

        GtkWidget* cpu_row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cpu_row), "CPU Fan");
        adw_action_row_set_subtitle(ADW_ACTION_ROW(cpu_row), "0 RPM");
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(fan_group), cpu_row);

        GtkWidget* gpu_row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gpu_row), "GPU Fan");
        adw_action_row_set_subtitle(ADW_ACTION_ROW(gpu_row), "0 RPM");
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(fan_group), gpu_row);

        struct FanData { GtkWidget* cpu; GtkWidget* gpu; };
        FanData* fdata = new FanData{cpu_row, gpu_row};

        g_timeout_add(2000, +[](gpointer user_data) -> gboolean {
            FanData* d = static_cast<FanData*>(user_data);
            if (!GTK_IS_WIDGET(d->cpu)) return G_SOURCE_REMOVE;

            auto [cpu_rpm, gpu_rpm] = AsusFans::get_fan_speeds();
            
            char buf[32];
            snprintf(buf, sizeof(buf), "%d RPM", cpu_rpm);
            adw_action_row_set_subtitle(ADW_ACTION_ROW(d->cpu), buf);

            snprintf(buf, sizeof(buf), "%d RPM", gpu_rpm);
            adw_action_row_set_subtitle(ADW_ACTION_ROW(d->gpu), buf);

            return G_SOURCE_CONTINUE;
        }, fdata);


        
        // Battery
        GtkWidget* bat_group = adw_preferences_group_new();
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(bat_group), "Battery Health");
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(bat_group));

        GtkWidget* limit_row = adw_spin_row_new_with_range(40, 100, 1);
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(limit_row), "Charge Limit %");
        adw_action_row_set_subtitle(ADW_ACTION_ROW(limit_row), "Stop charging at this percentage to extend lifespan.");
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(bat_group), limit_row);

        int current_limit = AsusBattery::get_charge_limit();
        if (current_limit > 0) {
             adw_spin_row_set_value(ADW_SPIN_ROW(limit_row), current_limit);
        }

        g_signal_connect(limit_row, "notify::value", G_CALLBACK(+[](GObject* row, GParamSpec*, gpointer) {
             int val = (int)adw_spin_row_get_value(ADW_SPIN_ROW(row));
             std::cout << "[AsusPlugin] Setting charge limit to: " << val << "%" << std::endl;
             if (AsusBattery::set_charge_limit(val)) {
                 std::cout << "[AsusPlugin] Limit applied." << std::endl;
             } else {
                 std::cerr << "[AsusPlugin] Failed to set limit." << std::endl;
             }
        }), NULL);
        update_status_text(status_row, AsusModes::get_mode());

        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(box, 20);
        gtk_widget_set_margin_bottom(box, 20);

        auto create_mode_btn = [=](const char* label, const char* icon, AsusMode mode, const char* css_class) {
            GtkWidget* btn = gtk_button_new();
            GtkWidget* btn_content = adw_button_content_new();
            adw_button_content_set_label(ADW_BUTTON_CONTENT(btn_content), label);
            adw_button_content_set_icon_name(ADW_BUTTON_CONTENT(btn_content), icon);
            gtk_button_set_child(GTK_BUTTON(btn), btn_content);
            gtk_widget_add_css_class(btn, "suggested-action"); 
            gtk_widget_add_css_class(btn, css_class);
            
            struct CallbackData {
                AsusMode mode;
                GtkWidget* row;
            };
            CallbackData* data = new CallbackData{mode, status_row};

            g_signal_connect_data(btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer user_data) {
                CallbackData* d = static_cast<CallbackData*>(user_data);
                
                std::cout << "[AsusPlugin] Requesting mode switch to: " << (int)d->mode << std::endl;
                
                if (AsusModes::set_mode(d->mode)) {
                    std::cout << "[AsusPlugin] Mode applied successfully." << std::endl;
                     switch (d->mode) {
                        case AsusMode::Silent:   adw_action_row_set_subtitle(ADW_ACTION_ROW(d->row), "Silent"); break;
                        case AsusMode::Balanced: adw_action_row_set_subtitle(ADW_ACTION_ROW(d->row), "Balanced"); break;
                        case AsusMode::Turbo:    adw_action_row_set_subtitle(ADW_ACTION_ROW(d->row), "Turbo"); break;
                        default: break;
                    }
                } else {
                    std::cerr << "[AsusPlugin] Failed to set mode (Check permissions/Polkit)" << std::endl;
                }
            }), data, [](gpointer data, GClosure*) { delete static_cast<CallbackData*>(data); }, (GConnectFlags)0);
            
            return btn;
        };

        gtk_box_append(GTK_BOX(box), create_mode_btn("Silent", "weather-clear-night-symbolic", AsusMode::Silent, "silent-mode"));
        gtk_box_append(GTK_BOX(box), create_mode_btn("Balanced", "weather-clear-symbolic", AsusMode::Balanced, "balanced-mode"));
        gtk_box_append(GTK_BOX(box), create_mode_btn("Turbo", "weather-storm-symbolic", AsusMode::Turbo, "turbo-mode"));

        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), box);
        return page;
    }
};

extern "C" KdPlugin* create_plugin() {
    return new AsusArmouryPlugin();
}

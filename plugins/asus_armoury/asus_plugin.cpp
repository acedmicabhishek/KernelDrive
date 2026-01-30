#include "../../src/core/plugin_interface.h"
#include "backend/modes.h"
#include "backend/core.h"
#include "backend/battery.h"
#include "backend/battery.h"
#include "backend/display.h"
#include "backend/display.h"
#include "backend/stress.h"
#include "backend/monitor.h"
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
        return AsusModes::is_supported() || AsusCore::is_supported() || AsusBattery::is_supported();
    }

    GtkWidget* create_config_widget() override {
        AsusMonitor::init();
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
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(fan_group), "Core");
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(fan_group), "Core");
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(fan_group));

        // Advanced Toggle
        GtkWidget* adv_row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(adv_row), "Advanced Stats");
        GtkWidget* adv_switch = gtk_switch_new();
        gtk_widget_set_valign(adv_switch, GTK_ALIGN_CENTER);
        adw_action_row_add_suffix(ADW_ACTION_ROW(adv_row), adv_switch);
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(fan_group), adv_row);

        std::string cpu_name = "CPU • " + AsusCore::get_cpu_name();
        std::string gpu_name = "GPU • " + AsusCore::get_gpu_name();

        GtkWidget* cpu_row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(cpu_row), cpu_name.c_str());
        adw_action_row_set_subtitle(ADW_ACTION_ROW(cpu_row), "0 RPM");
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(fan_group), cpu_row);

        GtkWidget* gpu_row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(gpu_row), gpu_name.c_str());
        if (gpu_name == "GPU • dGPU Unavailable") {
             adw_action_row_set_subtitle(ADW_ACTION_ROW(gpu_row), "Not detected");
             gtk_widget_set_sensitive(gpu_row, FALSE);
        } else {
             adw_action_row_set_subtitle(ADW_ACTION_ROW(gpu_row), "0 RPM");
        }
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(fan_group), gpu_row);

        // Stress Test Buttons
        GtkWidget* stress_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_halign(stress_box, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(stress_box, 10);
        
        GtkWidget* cpu_stress_btn = gtk_button_new_with_label("Stress CPU");
        GtkWidget* gpu_stress_btn = gtk_button_new_with_label("Stress GPU");
        gtk_widget_add_css_class(cpu_stress_btn, "destructive-action");
        gtk_widget_add_css_class(gpu_stress_btn, "destructive-action");

        struct StressData { GtkWidget* c_btn; GtkWidget* g_btn; GtkWidget* page; };
        StressData* sdata = new StressData{cpu_stress_btn, gpu_stress_btn, page};

        auto show_install_error = [](GtkWidget* parent, const char* app) {
             GtkWidget* dlg = adw_message_dialog_new(GTK_WINDOW(gtk_widget_get_root(parent)), "Missing Component", NULL);
             std::string msg = std::string("Please install '") + app + "' to use this feature.";
             adw_message_dialog_set_body(ADW_MESSAGE_DIALOG(dlg), msg.c_str());
             adw_message_dialog_add_response(ADW_MESSAGE_DIALOG(dlg), "ok", "OK");
             gtk_window_present(GTK_WINDOW(dlg));
        };
        struct CallbackCtx { StressData* s; decltype(show_install_error) err_fn; };
        CallbackCtx* ctx = new CallbackCtx{sdata, show_install_error};

        g_signal_connect(cpu_stress_btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer user_data) {
            CallbackCtx* c = (CallbackCtx*)user_data;
            if (AsusStress::is_cpu_stress_running()) {
                AsusStress::stop_cpu_stress();
                gtk_button_set_label(btn, "Stress CPU");
            } else {
                if (AsusStress::has_cpu_burn()) {
                   if (AsusStress::start_cpu_stress()) {
                       gtk_button_set_label(btn, "Stop CPU");
                   }
                } else {
                   c->err_fn(GTK_WIDGET(btn), "stress-ng");
                }
            }
        }), ctx);

        g_signal_connect(gpu_stress_btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer user_data) {
            CallbackCtx* c = (CallbackCtx*)user_data;
            if (AsusStress::is_gpu_stress_running()) {
                AsusStress::stop_gpu_stress();
                gtk_button_set_label(btn, "Stress GPU");
            } else {
                if (AsusStress::has_gpu_burn()) {
                    if (AsusStress::start_gpu_stress()) {
                        gtk_button_set_label(btn, "Stop GPU");
                    }
                } else {
                   c->err_fn(GTK_WIDGET(btn), "gpu_burn");
                }
            }
        }), ctx);

        g_timeout_add(1000, +[](gpointer user_data) -> gboolean {
            StressData* s = (StressData*)user_data;
            if (!GTK_IS_WIDGET(s->c_btn)) return G_SOURCE_REMOVE; 

            if (!AsusStress::is_cpu_stress_running()) gtk_button_set_label(GTK_BUTTON(s->c_btn), "Stress CPU");
            if (!AsusStress::is_gpu_stress_running()) gtk_button_set_label(GTK_BUTTON(s->g_btn), "Stress GPU");
            
            return G_SOURCE_CONTINUE;
        }, sdata);

        gtk_box_append(GTK_BOX(stress_box), cpu_stress_btn);
        gtk_box_append(GTK_BOX(stress_box), gpu_stress_btn);
        
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(fan_group), stress_box);

        struct FanData { GtkWidget* cpu; GtkWidget* gpu; GtkWidget* adv; };
        FanData* fdata = new FanData{cpu_row, gpu_row, adv_switch};

        g_timeout_add(2000, +[](gpointer user_data) -> gboolean {
            FanData* d = static_cast<FanData*>(user_data);
            if (!GTK_IS_WIDGET(d->cpu)) return G_SOURCE_REMOVE;

            AsusCore::FanMetrics m = AsusCore::get_metrics();
            
            char buf[64];
            
            if (m.cpu_temp > 0) snprintf(buf, sizeof(buf), "%d RPM  •  %d°C", m.cpu_rpm, m.cpu_temp);
            else snprintf(buf, sizeof(buf), "%d RPM", m.cpu_rpm);
            
            std::string cpu_sub = buf;
            if (gtk_switch_get_active(GTK_SWITCH(d->adv))) {
                auto cm = AsusMonitor::get_cpu_metrics();
                if (cm.freq_mhz > 0) cpu_sub += "  •  " + AsusMonitor::format_freq(cm.freq_mhz);
                if (cm.ram_mt_s > 0) cpu_sub += "  •  " + AsusMonitor::format_speed(cm.ram_mt_s);
            }
            adw_action_row_set_subtitle(ADW_ACTION_ROW(d->cpu), cpu_sub.c_str());

            if (m.gpu_temp > 0) snprintf(buf, sizeof(buf), "%d RPM  •  %d°C", m.gpu_rpm, m.gpu_temp);
            else snprintf(buf, sizeof(buf), "%d RPM", m.gpu_rpm);
            
            std::string gpu_sub = buf;
            if (gtk_switch_get_active(GTK_SWITCH(d->adv))) {
                 auto gm = AsusMonitor::get_gpu_metrics();
                 if (gm.core_clock_mhz > 0) gpu_sub += "  •  " + std::to_string(gm.core_clock_mhz) + " MHz";
                 if (gm.vram_used_mb > 0) gpu_sub += "  •  " + std::to_string(gm.vram_used_mb) + " MB";
            }
            adw_action_row_set_subtitle(ADW_ACTION_ROW(d->gpu), gpu_sub.c_str());

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

        // Apply on change
        g_signal_connect(limit_row, "notify::value", G_CALLBACK(+[](GObject* row, GParamSpec*, gpointer) {
             int val = (int)adw_spin_row_get_value(ADW_SPIN_ROW(row));
             std::cout << "[AsusPlugin] Setting charge limit to: " << val << "%" << std::endl;
             if (AsusBattery::set_charge_limit(val)) {
                 std::cout << "[AsusPlugin] Limit applied." << std::endl;
             } else {
                 std::cerr << "[AsusPlugin] Failed to set limit." << std::endl;
             }
        }), NULL);

        // Display
        GtkWidget* disp_group = adw_preferences_group_new();
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(disp_group), "Display");
        adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(disp_group));

        // 1. Refresh Rate 
        GtkWidget* rate_row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(rate_row), "Refresh Rate");
        adw_action_row_set_subtitle(ADW_ACTION_ROW(rate_row), "Control screen smoothness.");
        
        GtkWidget* rate_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        
        std::vector<int> rates = AsusDisplay::get_available_refresh_rates();
        for (int hz : rates) {
             GtkWidget* btn = gtk_button_new_with_label((std::to_string(hz) + "Hz").c_str());
             g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data) {
                 int h = GPOINTER_TO_INT(data);
                 AsusDisplay::set_refresh_rate(h);
             }), GINT_TO_POINTER(hz));
             gtk_box_append(GTK_BOX(rate_box), btn);
        }
        adw_action_row_add_suffix(ADW_ACTION_ROW(rate_row), rate_box);
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(disp_group), rate_row);

        // 2. Panel Overdrive
        GtkWidget* od_row = adw_switch_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(od_row), "Panel Overdrive");
        
        if (AsusDisplay::is_overdrive_supported()) {
            adw_action_row_set_subtitle(ADW_ACTION_ROW(od_row), "Reduces ghosting but may cause overshoot.");
            adw_switch_row_set_active(ADW_SWITCH_ROW(od_row), AsusDisplay::get_panel_overdrive());
            g_signal_connect(od_row, "notify::active", G_CALLBACK(+[](GObject* row, GParamSpec*, gpointer) {
                bool active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
                AsusDisplay::set_panel_overdrive(active);
            }), NULL);
        } else {
            adw_action_row_set_subtitle(ADW_ACTION_ROW(od_row), "Not supported by your device.");
            gtk_widget_set_sensitive(od_row, FALSE);
        }
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(disp_group), od_row);

        // 3. Panel Power Saver (MiniLED)
        GtkWidget* miniled_row = adw_switch_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(miniled_row), "Panel Power Saver");
        
        if (AsusDisplay::is_miniled_supported()) {
            adw_action_row_set_subtitle(ADW_ACTION_ROW(miniled_row), "Optimizes MiniLED zones for power saving.");
            adw_switch_row_set_active(ADW_SWITCH_ROW(miniled_row), AsusDisplay::get_miniled_mode());
            g_signal_connect(miniled_row, "notify::active", G_CALLBACK(+[](GObject* row, GParamSpec*, gpointer) {
                bool active = adw_switch_row_get_active(ADW_SWITCH_ROW(row));
                AsusDisplay::set_miniled_mode(active);
            }), NULL);
        } else {
            adw_action_row_set_subtitle(ADW_ACTION_ROW(miniled_row), "Not supported by your device.");
            gtk_widget_set_sensitive(miniled_row, FALSE);
        }
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(disp_group), miniled_row);

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

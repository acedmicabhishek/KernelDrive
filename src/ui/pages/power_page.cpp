#include "power_page.h"
#include "../../core/sysfs_writer.h"
#include "../../core/power/power_manager.h"
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <thread>
#include <gtk/gtk.h>
#include <adwaita.h>

struct _KdPowerPage {
    AdwBin parent_instance;
    AdwComboRow* governor_row;
    guint update_source_id;
};

G_DEFINE_FINAL_TYPE(KdPowerPage, kd_power_page, ADW_TYPE_BIN)



// this will be our universal gov chnager for profiling because it is just dependent on wriritng file.
static void apply_governor_to_all_cpus(const char* governor) {
    namespace fs = std::filesystem;
    std::string base_path = "/sys/devices/system/cpu/";
    if (!fs::exists(base_path)) return;

    for (const auto& entry : fs::directory_iterator(base_path)) {
        std::string filename = entry.path().filename().string();
        if (filename.rfind("cpu", 0) == 0 && std::isdigit(filename[3])) {
             std::string path = entry.path().string() + "/cpufreq/scaling_governor";
             if (fs::exists(path)) {
                 SysfsWriter::write(path, governor);
             }
        }
    }
}

static void on_governor_selected(AdwComboRow* row, [[maybe_unused]] GParamSpec* pspec, [[maybe_unused]] gpointer user_data) {
    guint selected_idx = adw_combo_row_get_selected(row);
    GListModel* model = adw_combo_row_get_model(row);
    if (!model) return;

    const char* governor = gtk_string_list_get_string(GTK_STRING_LIST(model), selected_idx);
    if (governor) {
        apply_governor_to_all_cpus(governor);
    }
}

static void kd_power_page_dispose(GObject* object) {
    KdPowerPage* self = KD_POWER_PAGE(object);
    
    if (self->update_source_id > 0) {
        g_source_remove(self->update_source_id);
        self->update_source_id = 0;
    }
    
    G_OBJECT_CLASS(kd_power_page_parent_class)->dispose(object);
}

static void kd_power_page_class_init(KdPowerPageClass* klass) {
    G_OBJECT_CLASS(klass)->dispose = kd_power_page_dispose;
}

static void kd_power_page_init(KdPowerPage* self) {
    self->update_source_id = 0;
    
    GtkWidget* status_page = adw_status_page_new();
    adw_status_page_set_title(ADW_STATUS_PAGE(status_page), "Power Management");
    adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "Control CPU frequency scaling and power profiles.");
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "system-shutdown-symbolic");

    adw_bin_set_child(ADW_BIN(self), status_page);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    
    GtkWidget* group = adw_preferences_group_new();
    gtk_box_append(GTK_BOX(box), group);

    
    GtkWidget* bat_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(bat_group), "Battery Status");
    gtk_box_append(GTK_BOX(box), bat_group);
    
    GtkWidget* bat_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(bat_row), "Looking for battery...");
    GtkWidget* bat_icon = gtk_image_new_from_icon_name("battery-missing-symbolic");
    adw_action_row_add_prefix(ADW_ACTION_ROW(bat_row), bat_icon);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(bat_group), bat_row);
    
    auto update_bat = +[](gpointer data) -> gboolean {
        KdPowerPage* p = (KdPowerPage*)data;
 
        GtkWidget* row = (GtkWidget*)g_object_get_data(G_OBJECT(p), "bat_row");
        GtkWidget* icon = (GtkWidget*)g_object_get_data(G_OBJECT(p), "bat_icon");
        if (!row || !icon) return G_SOURCE_CONTINUE;

        auto bats = PowerManager::get().get_backend()->get_batteries();
        if (bats.empty()) {
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No Battery Found");
            gtk_image_set_from_icon_name(GTK_IMAGE(icon), "battery-missing-symbolic");
            return G_SOURCE_CONTINUE; 
        }

        const auto& b = bats[0];
        std::string title = std::to_string(b.percentage) + "%";
        if (b.state == BatteryState::Charging) title += " (Charging)";
        else if (b.state == BatteryState::Discharging) title += " (Discharging)";
        else if (b.state == BatteryState::Full) title += " (Full)";
        else if (b.state == BatteryState::NotCharging) title += " (Not Charging)";
        
        if (!b.time_remaining_str.empty()) title += " • " + b.time_remaining_str + " remaining";
        
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title.c_str());
        adw_action_row_set_subtitle(ADW_ACTION_ROW(row), (std::to_string(b.energy_rate_w) + " W").c_str());

        std::string icon_name = "battery-level-";
        int level = (b.percentage / 10) * 10;
        if (level == 0) level = 0; 
        if (level == 100) level = 100;
        
        icon_name += std::to_string(level);
        if (b.state == BatteryState::Charging) icon_name += "-charging-symbolic";
        else icon_name += "-symbolic"; 
        
        gtk_image_set_from_icon_name(GTK_IMAGE(icon), icon_name.c_str());
        
        return G_SOURCE_CONTINUE;
    };
    
    g_object_set_data(G_OBJECT(self), "bat_row", bat_row);
    g_object_set_data(G_OBJECT(self), "bat_icon", bat_icon);
    
    self->update_source_id = g_timeout_add(2000, update_bat, self);
    update_bat(self); 
    
    
    PowerProfileInfo profile = PowerManager::get().get_backend()->get_profile_info();
    if (!profile.available_profiles.empty()) {
        GtkWidget* prof_group = adw_preferences_group_new();
        gtk_box_append(GTK_BOX(box), prof_group);
        
        AdwComboRow* prof_row = ADW_COMBO_ROW(adw_combo_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(prof_row), "Power Profile");
        adw_action_row_set_subtitle(ADW_ACTION_ROW(prof_row), "Platform ACPI Profile");
        
        
        const char** items = new const char*[profile.available_profiles.size() + 1];
        int active_idx = 0;
        for(size_t i=0; i<profile.available_profiles.size(); ++i) {
            items[i] = profile.available_profiles[i].c_str(); 
            
            if(profile.available_profiles[i] == profile.active_profile) active_idx = i;
        }
        items[profile.available_profiles.size()] = NULL;
        
        GtkStringList* list = gtk_string_list_new(items);
        adw_combo_row_set_model(prof_row, G_LIST_MODEL(list));
        adw_combo_row_set_selected(prof_row, active_idx);
        
        g_signal_connect(prof_row, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
             guint idx = adw_combo_row_get_selected(ADW_COMBO_ROW(obj));
             GListModel* model = adw_combo_row_get_model(ADW_COMBO_ROW(obj));
             const char* s = gtk_string_list_get_string(GTK_STRING_LIST(model), idx);
             PowerManager::get().get_backend()->set_profile(s);
        }), NULL);
        
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(prof_group), GTK_WIDGET(prof_row));
        delete[] items;
    }

    
    self->governor_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(self->governor_row), "CPU Governor");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(self->governor_row), "Controls how the CPU scales frequency.");

    std::vector<std::string> govs_vec = PowerManager::get().get_available_governors();
    if (!govs_vec.empty()) {
        const char** items = new const char*[govs_vec.size() + 1];
        int active_idx = 0;
        std::string current_gov = PowerManager::get().get_cpu_governor();
        
        for(size_t i=0; i<govs_vec.size(); ++i) {
            items[i] = govs_vec[i].c_str();
            if (current_gov == govs_vec[i]) active_idx = i;
        }
        items[govs_vec.size()] = NULL;
        
        GtkStringList* list = gtk_string_list_new(items);
        adw_combo_row_set_model(self->governor_row, G_LIST_MODEL(list));
        adw_combo_row_set_selected(self->governor_row, active_idx);
        delete[] items;
    } else {
        adw_action_row_set_subtitle(ADW_ACTION_ROW(self->governor_row), "No governors found or access denied.");
    }
    
    g_signal_connect(self->governor_row, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
         guint idx = adw_combo_row_get_selected(ADW_COMBO_ROW(obj));
         GListModel* model = adw_combo_row_get_model(ADW_COMBO_ROW(obj));
         if (!model) return;
         const char* s = gtk_string_list_get_string(GTK_STRING_LIST(model), idx);
         if (s) PowerManager::get().set_cpu_governor(s);
    }), NULL);

    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(self->governor_row));

    
    GtkWidget* tdp_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(tdp_group), "Power Limits");
    gtk_box_append(GTK_BOX(box), tdp_group);

    
    GtkWidget* tdp_row = adw_spin_row_new_with_range(5.0, 80.0, 1.0); 
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(tdp_row), "Package TDP Limit");
    
    if (PowerManager::get().is_rapl_supported()) {
        adw_action_row_set_subtitle(ADW_ACTION_ROW(tdp_row), "Limit generic package power (Watts). Set high for unlimited.");
        
        
        int current_tdp = PowerManager::get().get_tdp_limit();
        adw_spin_row_set_value(ADW_SPIN_ROW(tdp_row), (double)current_tdp);
        
        g_signal_connect(tdp_row, "notify::value", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
            double v = adw_spin_row_get_value(ADW_SPIN_ROW(obj));
            PowerManager::get().set_tdp_limit((int)v);
        }), NULL);
    } else {
        gtk_widget_set_sensitive(tdp_row, FALSE);
        adw_action_row_set_subtitle(ADW_ACTION_ROW(tdp_row), "Function not supported on your system (Missing RAPL/RyzenAdj).");
        
        
        
        std::ifstream os_release("/etc/os-release");
        std::string line, distro_id;
        while (std::getline(os_release, line)) {
            if (line.find("ID=") == 0) {
                distro_id = line.substr(3);
                
                if (!distro_id.empty() && distro_id.front() == '"') distro_id = distro_id.substr(1, distro_id.size()-2);
            }
        }
        
        std::string install_cmd = "";
        std::string install_label = "Install Support";
        
        if (distro_id == "arch" || distro_id == "manjaro" || distro_id == "endeavouros") {
             
             install_label = "Build & Install (Source)";
             install_cmd = "pkexec sh -c 'pacman -S --noconfirm git cmake base-devel pciutils && "
                           "rm -rf /tmp/RyzenAdj && "
                           "git clone https:
                           "cd /tmp/RyzenAdj && mkdir build && cd build && "
                           "cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install'";
        } else if (distro_id == "ubuntu" || distro_id == "debian" || distro_id == "pop" || distro_id == "linuxmint") {
             
             
             
             
             
             
             install_cmd = "pkexec apt install -y ryzenadj || "
                           "pkexec sh -c 'apt install -y git cmake build-essential libpci-dev && "
                           "rm -rf /tmp/RyzenAdj && "
                           "git clone https:
                           "cd /tmp/RyzenAdj && mkdir build && cd build && "
                           "cmake -DCMAKE_BUILD_TYPE=Release .. && make && make install'";
        } else if (distro_id == "fedora") {
             install_cmd = "pkexec dnf install -y ryzenadj"; 
        }
        
        if (!install_cmd.empty()) {
            GtkWidget* install_btn = gtk_button_new_with_label(install_label.c_str());
            gtk_widget_set_valign(install_btn, GTK_ALIGN_CENTER);
            gtk_widget_add_css_class(install_btn, "suggested-action");
            
            
            
            
            
            GtkWidget* install_row = adw_action_row_new();
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(install_row), "Missing Dependencies");
            adw_action_row_set_subtitle(ADW_ACTION_ROW(install_row), ("Install 'ryzenadj' for " + distro_id).c_str());
            adw_action_row_add_suffix(ADW_ACTION_ROW(install_row), install_btn);
            adw_preferences_group_add(ADW_PREFERENCES_GROUP(tdp_group), install_row);
            
            
            struct InstallData { std::string cmd; GtkWidget* btn; };
            InstallData* idata = new InstallData{install_cmd, install_btn};
            g_object_set_data_full(G_OBJECT(install_btn), "install_data", idata, [](gpointer d){ delete (InstallData*)d; });
            
            g_signal_connect(install_btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
                InstallData* d = (InstallData*)g_object_get_data(G_OBJECT(btn), "install_data");
                gtk_button_set_label(btn, "Installing...");
                gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);
                
                
                
                
                
                std::thread([cmd = d->cmd, btn_ptr = GTK_WIDGET(btn)]() {
                    if (system(cmd.c_str()) != 0) { }
                    
                    
                    g_idle_add(+[](gpointer data) -> gboolean {
                        GtkWidget* b = (GtkWidget*)data;
                        if (GTK_IS_WIDGET(b)) {
                            gtk_button_set_label(GTK_BUTTON(b), "Restart App");
                            gtk_widget_set_sensitive(b, TRUE);
                        }
                        return G_SOURCE_REMOVE;
                    }, btn_ptr);
                }).detach();
            }), NULL);
        }
    }
    
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(tdp_group), tdp_row);

    adw_status_page_set_child(ADW_STATUS_PAGE(status_page), box);
}

GtkWidget* kd_power_page_new() {
    return GTK_WIDGET(g_object_new(KD_TYPE_POWER_PAGE, NULL));
}

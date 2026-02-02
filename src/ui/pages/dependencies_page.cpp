#include "dependencies_page.h"
#include "../../core/dependency_manager.h"
#include <adwaita.h>
#include <thread>

struct _KdDependenciesPage {
    AdwBin parent_instance;
};

G_DEFINE_FINAL_TYPE(KdDependenciesPage, kd_dependencies_page, ADW_TYPE_BIN)

struct UIData { GtkWidget* btn; GtkWidget* row; std::string name; };

static void kd_dependencies_page_class_init(KdDependenciesPageClass* klass) {
}

static void kd_dependencies_page_init(KdDependenciesPage* self) {
    AdwStatusPage* status_page = ADW_STATUS_PAGE(adw_status_page_new());
    adw_status_page_set_title(status_page, "System Dependencies");
    adw_status_page_set_description(status_page, "Manage external tools required for advanced features.");
    adw_status_page_set_icon_name(status_page, "system-software-install-symbolic");
    
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    
    auto& dep_mgr = DependencyManager::get();
    auto all_deps = dep_mgr.get_all_dependencies();
    
    if (!all_deps.empty()) {
        GtkWidget* group = adw_preferences_group_new();
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "External Tools");
        adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(group), 
            "These tools enable plugin features. Missing tools can be built from source.");
        gtk_box_append(GTK_BOX(box), group);
        
        for (const auto& dep : all_deps) {
            GtkWidget* row = adw_action_row_new();
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), dep.name.c_str());
            adw_action_row_set_subtitle(ADW_ACTION_ROW(row), dep.description.c_str());
            
            bool available = dep_mgr.is_available(dep.name);
            
            if (available) {
                
                GtkWidget* check = gtk_image_new_from_icon_name("emblem-ok-symbolic");
                gtk_widget_add_css_class(check, "success");
                adw_action_row_add_suffix(ADW_ACTION_ROW(row), check);
                
                GtkWidget* label = gtk_label_new("Installed");
                gtk_widget_add_css_class(label, "success");
                gtk_widget_add_css_class(label, "dim-label");
                adw_action_row_add_suffix(ADW_ACTION_ROW(row), label);
            } else {
                
                std::string install_cmd = dep_mgr.get_install_command(dep);
                
                if (!install_cmd.empty()) {
                    GtkWidget* install_btn = gtk_button_new_with_label("Build & Install");
                    gtk_widget_set_valign(install_btn, GTK_ALIGN_CENTER);
                    gtk_widget_add_css_class(install_btn, "suggested-action");
                    
                    
                    struct InstallData { std::string cmd; std::string name; GtkWidget* btn; GtkWidget* row; };
                    InstallData* idata = new InstallData{install_cmd, dep.name, install_btn, row};
                    g_object_set_data_full(G_OBJECT(install_btn), "install_data", idata, 
                                          [](gpointer d){ delete (InstallData*)d; });
                    
                    g_signal_connect(install_btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
                        InstallData* d = (InstallData*)g_object_get_data(G_OBJECT(btn), "install_data");
                        
                        gtk_button_set_label(btn, "Installing...");
                        gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);
                        
                        
                        std::thread([cmd = d->cmd, btn_ptr = GTK_WIDGET(btn), row_ptr = d->row, name = d->name]() {
                            if (system(cmd.c_str()) != 0) { }
                            
                            
                            g_idle_add(+[](gpointer data) -> gboolean {
                                UIData* ud = (UIData*)data;
                                
                                
                                bool success = DependencyManager::get().is_available(ud->name);
                                
                                if (success) {
                                    
                                    GtkWidget* parent = gtk_widget_get_parent(ud->btn);
                                    if (parent) {
                                        gtk_box_remove(GTK_BOX(parent), ud->btn);
                                    }
                                    
                                    GtkWidget* check = gtk_image_new_from_icon_name("emblem-ok-symbolic");
                                    gtk_widget_add_css_class(check, "success");
                                    adw_action_row_add_suffix(ADW_ACTION_ROW(ud->row), check);
                                    
                                    GtkWidget* label = gtk_label_new("Installed");
                                    gtk_widget_add_css_class(label, "success");
                                    gtk_widget_add_css_class(label, "dim-label");
                                    adw_action_row_add_suffix(ADW_ACTION_ROW(ud->row), label);
                                } else {
                                    gtk_button_set_label(GTK_BUTTON(ud->btn), "Retry");
                                    gtk_widget_set_sensitive(ud->btn, TRUE);
                                }
                                
                                delete ud;
                                return G_SOURCE_REMOVE;
                            }, new UIData{btn_ptr, row_ptr, name});
                        }).detach();
                    }), NULL);
                    
                    adw_action_row_add_suffix(ADW_ACTION_ROW(row), install_btn);
                } else {
                    GtkWidget* label = gtk_label_new("Not Available");
                    gtk_widget_add_css_class(label, "dim-label");
                    adw_action_row_add_suffix(ADW_ACTION_ROW(row), label);
                }
            }
            
            adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), row);
        }
    }
    
    adw_status_page_set_child(status_page, box);
    adw_bin_set_child(ADW_BIN(self), GTK_WIDGET(status_page));
}

GtkWidget* kd_dependencies_page_new() {
    return GTK_WIDGET(g_object_new(KD_TYPE_DEPENDENCIES_PAGE, NULL));
}

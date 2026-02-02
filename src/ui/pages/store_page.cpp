#include "store_page.h"
#include "../../core/plugin_manager.h"
#include "../../core/plugin_installer.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <regex>

struct StorePlugin {
    std::string slug;
    std::string name;
    std::string description;
    std::string repo_url;
    std::string author;
};

// Manual JSON parsing (Robust and simple for this use case)
static std::string extract_value(const std::string& block, const std::string& key) {
    std::string key_pattern = "\"" + key + "\":";
    size_t pos = block.find(key_pattern);
    if (pos == std::string::npos) return "";
    
    pos += key_pattern.length();
    // Skip whitespace
    while (pos < block.length() && (block[pos] == ' ' || block[pos] == '\n' || block[pos] == '\t')) pos++;
    
    if (pos >= block.length() || block[pos] != '"') return "";
    pos++; // Skip opening quote
    
    size_t end = block.find('"', pos);
    if (end == std::string::npos) return "";
    
    return block.substr(pos, end - pos);
}

static std::vector<StorePlugin> parse_registry(const std::string& path) {
    std::vector<StorePlugin> plugins;
    std::ifstream f(path);
    if (!f) return plugins;

    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    
    size_t pos = 0;
    while ((pos = content.find('{', pos)) != std::string::npos) {
        size_t end = content.find('}', pos);
        if (end == std::string::npos) break;
        
        std::string block = content.substr(pos, end - pos + 1);
        
        std::string slug = extract_value(block, "slug");
        if (!slug.empty()) {
            plugins.push_back({
                slug,
                extract_value(block, "name"),
                extract_value(block, "description"),
                extract_value(block, "repo_url"),
                extract_value(block, "author")
            });
        }
        pos = end + 1;
    }
    return plugins;
}

GtkWidget* kd_store_page_new(void) {
    GtkWidget* page = adw_preferences_page_new();
    
    GtkWidget* installed_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(installed_group), "Installed Plugins");
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(installed_group), "Manage your active system control modules.");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(installed_group));

    const auto& plugins = PluginManager::get().get_plugins();
    
    // installed plugins list
    if (plugins.empty()) {
        GtkWidget* row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), "No plugins loaded");
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(installed_group), row);
    } else {
        for (const auto& plugin : plugins) {
            GtkWidget* row = adw_action_row_new();
            adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), plugin->get_name().c_str());
            
            GtkWidget* uninstall_btn = gtk_button_new_with_label("Uninstall");
            gtk_widget_add_css_class(uninstall_btn, "destructive-action");
            
            g_object_set_data(G_OBJECT(uninstall_btn), "plugin-ptr", plugin.get());
            g_object_set_data(G_OBJECT(uninstall_btn), "row-ptr", row);

            g_signal_connect(uninstall_btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
                KdPlugin* plug = (KdPlugin*)g_object_get_data(G_OBJECT(btn), "plugin-ptr");
                GtkWidget* row_widget = (GtkWidget*)g_object_get_data(G_OBJECT(btn), "row-ptr");
                
                if (plug) {
                    AdwDialog* dialog = adw_alert_dialog_new("Uninstall Plugin?", "This will delete the plugin file. This action cannot be undone.");
                    adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "cancel", "Cancel");
                    adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "uninstall", "Uninstall");
                    adw_alert_dialog_set_response_appearance(ADW_ALERT_DIALOG(dialog), "uninstall", ADW_RESPONSE_DESTRUCTIVE); 
                    
                    g_object_set_data(G_OBJECT(dialog), "plugin-ptr", plug);
                    g_object_set_data(G_OBJECT(dialog), "row-ptr", row_widget);

                    g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog* self, const char* response, gpointer) {
                        if (g_strcmp0(response, "uninstall") == 0) {
                            KdPlugin* p = (KdPlugin*)g_object_get_data(G_OBJECT(self), "plugin-ptr");
                            GtkWidget* r = (GtkWidget*)g_object_get_data(G_OBJECT(self), "row-ptr");
                            
                            if (PluginManager::get().uninstall_plugin(p)) {
                                GtkWidget* group = gtk_widget_get_ancestor(r, ADW_TYPE_PREFERENCES_GROUP);
                                if (group) {
                                    adw_preferences_group_remove(ADW_PREFERENCES_GROUP(group), r);
                                }
                            }
                        }
                    }), NULL);
                    
                    adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(gtk_widget_get_root(GTK_WIDGET(btn))));
                }
            }), NULL);

            adw_action_row_add_suffix(ADW_ACTION_ROW(row), uninstall_btn);
            adw_preferences_group_add(ADW_PREFERENCES_GROUP(installed_group), row);
        }
    }

    GtkWidget* browse_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(browse_group), "Web Store (GitHub Registry)");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(browse_group));

    auto store_plugins = parse_registry("data/registry.json");
    if (store_plugins.empty()) store_plugins = parse_registry("/usr/share/kerneldrive/data/registry.json"); 

    if (store_plugins.empty()) {
        GtkWidget* warn = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(warn), "Unable to load registry");
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(browse_group), warn);
    }

    for (const auto& sp : store_plugins) {
        GtkWidget* row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), sp.name.c_str());
        std::string sub = sp.description + "\nby " + sp.author;
        adw_action_row_set_subtitle(ADW_ACTION_ROW(row), sub.c_str());

        bool installed = false;
        for (const auto& p : plugins) if(p->get_slug() == sp.slug) installed = true;

        if (installed) {
            GtkWidget* lbl = gtk_label_new("Installed");
            gtk_widget_add_css_class(lbl, "dim-label");
            adw_action_row_add_suffix(ADW_ACTION_ROW(row), lbl);
        } else {
            GtkWidget* install_btn = gtk_button_new_with_label("Install");
            gtk_widget_add_css_class(install_btn, "suggested-action");
            
            struct InstallData { std::string url; std::string slug; GtkWidget* btn; };
            InstallData* idata = new InstallData{sp.repo_url, sp.slug, install_btn};
            g_object_set_data_full(G_OBJECT(install_btn), "idata", idata, [](gpointer d){ delete (InstallData*)d; });

            g_signal_connect(install_btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer) {
                InstallData* d = (InstallData*)g_object_get_data(G_OBJECT(btn), "idata");
                
                gtk_widget_set_sensitive(GTK_WIDGET(btn), false);
                gtk_button_set_label(GTK_BUTTON(btn), "Installing...");

                PluginInstaller::get().install_plugin(d->url, d->slug, [b = GTK_WIDGET(btn)](InstallStatus s, std::string msg) {
                    
                    struct UpdateData { GtkWidget* btn; InstallStatus status; std::string message; };
                    UpdateData* ud = new UpdateData{b, s, msg};

                    g_idle_add(+[](gpointer data) -> gboolean {
                        UpdateData* u = (UpdateData*)data;
                        if (!GTK_IS_WIDGET(u->btn)) { delete u; return G_SOURCE_REMOVE; }

                        if (u->status == InstallStatus::Success) {
                             gtk_button_set_label(GTK_BUTTON(u->btn), "Installed");
                             adw_toast_overlay_add_toast(
                                 ADW_TOAST_OVERLAY(gtk_widget_get_ancestor(u->btn, ADW_TYPE_TOAST_OVERLAY)),
                                 adw_toast_new(u->message.c_str())
                             );
                        } else if (u->status == InstallStatus::Failed) {
                             gtk_widget_set_sensitive(u->btn, true);
                             gtk_button_set_label(GTK_BUTTON(u->btn), "Retry");
                             
                             AdwDialog* d = adw_alert_dialog_new("Installation Failed", u->message.c_str());
                             adw_alert_dialog_add_response(ADW_ALERT_DIALOG(d), "ok", "OK");
                             adw_dialog_present(ADW_DIALOG(d), GTK_WIDGET(gtk_widget_get_root(u->btn)));
                        } else {
                             gtk_button_set_label(GTK_BUTTON(u->btn), u->message.c_str());
                        }
                        
                        delete u;
                        return G_SOURCE_REMOVE;
                    }, ud);
                });
            }), NULL);
            
            adw_action_row_add_suffix(ADW_ACTION_ROW(row), install_btn);
        }
        
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(browse_group), row);
    }

    return page;
}

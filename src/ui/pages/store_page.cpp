#include "store_page.h"
#include "../../core/plugin_manager.h"
#include <string>

GtkWidget* kd_store_page_new(void) {
    GtkWidget* page = adw_preferences_page_new();
    
    GtkWidget* installed_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(installed_group), "Installed Plugins");
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(installed_group), "Manage your active system control modules.");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(installed_group));

    const auto& plugins = PluginManager::get().get_plugins();
    
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

            g_signal_connect(uninstall_btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer user_data) {
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
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(browse_group), "Get More");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(browse_group));

    GtkWidget* browse_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(browse_row), "Browse KernelDrive Repo");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(browse_row), "Find drivers for other hardware.");
    
    GtkWidget* open_btn = gtk_button_new_from_icon_name("system-software-install-symbolic");
    adw_action_row_add_suffix(ADW_ACTION_ROW(browse_row), open_btn);
    
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(browse_group), browse_row);

    return page;
}

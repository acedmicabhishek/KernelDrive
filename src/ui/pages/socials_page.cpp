#include "socials_page.h"
#include "../../config/socials.h"
#include <gtk/gtk.h>

GtkWidget* SocialsPage::create() {
    GtkWidget* page = adw_preferences_page_new();
    
    GtkWidget* group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), "Connect");
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(group), "Reach out via these platforms.");
    
    for (const auto& link : MY_SOCIALS) {
        GtkWidget* row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), link.name.c_str());
        adw_action_row_set_subtitle(ADW_ACTION_ROW(row), link.subtitle.c_str());
        
        adw_action_row_add_prefix(ADW_ACTION_ROW(row), gtk_image_new_from_icon_name(link.icon_name.c_str()));
        
        GtkWidget* btn = gtk_button_new_from_icon_name("external-link-symbolic");
        gtk_widget_add_css_class(btn, "flat");
        gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);

        g_object_set_data(G_OBJECT(btn), "url", (gpointer)link.url.c_str());
        
        g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkWidget* w, gpointer) {
            const char* url = (const char*)g_object_get_data(G_OBJECT(w), "url");
            if (!url) return;

            const char* sudo_user = std::getenv("SUDO_USER");
            if (sudo_user) {
                std::string cmd = "runuser -u " + std::string(sudo_user) + " -- xdg-open " + std::string(url) + " > /dev/null 2>&1 &";
                system(cmd.c_str());
            } else {
                gtk_show_uri(GTK_WINDOW(gtk_widget_get_root(w)), url, GDK_CURRENT_TIME);
            }
        }), nullptr);
        
        adw_action_row_add_suffix(ADW_ACTION_ROW(row), btn);
        adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), btn);
        
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), row);
    }
    
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(group));
    
    // About Section
    GtkWidget* about_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(about_group), "About");
    
    GtkWidget* about_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(about_row), "KernelDrive");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(about_row), "Version 0.1.0");
    adw_action_row_add_prefix(ADW_ACTION_ROW(about_row), gtk_image_new_from_icon_name("application-x-executable"));
    
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(about_group), about_row);
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(about_group));
    
    return page;
}

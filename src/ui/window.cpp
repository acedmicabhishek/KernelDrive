#include "window.h"
#include "../core/plugin_manager.h"
#include "pages/power_page.h"
#include "pages/input_page.h"
#include "pages/display_page.h"
#include "pages/dependencies_page.h"
#include "pages/socials_page.h"
#include "pages/store_page.h"
#include <unistd.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <sys/wait.h>

struct _KdMainWindow {
    AdwApplicationWindow parent_instance;
    AdwNavigationSplitView* split_view;
    AdwPreferencesGroup* nav_group;
};


extern "C" void kd_show_toast(const char* msg);

G_DEFINE_FINAL_TYPE(KdMainWindow, kd_main_window, ADW_TYPE_APPLICATION_WINDOW)

static void kd_main_window_dispose(GObject* object) {
    G_OBJECT_CLASS(kd_main_window_parent_class)->dispose(object);
}

static void kd_main_window_class_init(KdMainWindowClass* klass) {
    G_OBJECT_CLASS(klass)->dispose = kd_main_window_dispose;
}

static void on_sidebar_row_activated(AdwActionRow* row, KdMainWindow* self) {
    const char* page_id = (const char*)g_object_get_data(G_OBJECT(row), "page-id");
    KdPlugin* plugin = (KdPlugin*)g_object_get_data(G_OBJECT(row), "plugin-ptr");

    const char* title = adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row));
    g_print("Sidebar row activated: %s (ID: %s)\n", title, page_id ? page_id : "null");

    GtkWidget* content_widget = NULL;

    if (plugin) {
        content_widget = plugin->create_config_widget();
    } else if (g_strcmp0(page_id, "power") == 0) {
        content_widget = kd_power_page_new();
    } else if (g_strcmp0(page_id, "input") == 0) {
        content_widget = kd_input_page_new();
    } else if (g_strcmp0(page_id, "display") == 0) {
        content_widget = kd_display_page_new();
    } else if (g_strcmp0(page_id, "dependencies") == 0) {
        content_widget = kd_dependencies_page_new();
    } else if (g_strcmp0(page_id, "store") == 0) {
        content_widget = kd_store_page_new();
    } else if (g_strcmp0(page_id, "socials") == 0) {
        content_widget = SocialsPage::create();
    } else {
        content_widget = adw_status_page_new();
        adw_status_page_set_title(ADW_STATUS_PAGE(content_widget), title);
        adw_status_page_set_description(ADW_STATUS_PAGE(content_widget), "Implementation ongoing");
        adw_status_page_set_icon_name(ADW_STATUS_PAGE(content_widget), "applications-system-symbolic");
    }

    if (content_widget) {
        if (!ADW_IS_NAVIGATION_PAGE(content_widget)) {
            GtkWidget* wrapper = GTK_WIDGET(adw_navigation_page_new(content_widget, title));
            content_widget = wrapper;
        }
        adw_navigation_split_view_set_content(self->split_view, ADW_NAVIGATION_PAGE(content_widget));
    }
}

static void kd_main_window_init(KdMainWindow* self) {
    gtk_window_set_default_size(GTK_WINDOW(self), 1100, 700);
    gtk_window_set_title(GTK_WINDOW(self), "KernelDrive");

    // Enforce Root
    if (geteuid() != 0) {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
        gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
        
        GtkWidget* icon = gtk_image_new_from_icon_name("dialog-password-symbolic");
        gtk_widget_set_size_request(icon, 96, 96);
        gtk_widget_add_css_class(icon, "dim-label");
        
        GtkWidget* title = gtk_label_new("Administrator Privileges Required");
        gtk_widget_add_css_class(title, "title-1");
        
        GtkWidget* desc = gtk_label_new("KernelDrive needs to access hardware controls.\nPlease enter your password to continue.");
        gtk_widget_add_css_class(desc, "body");
        gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
        gtk_label_set_justify(GTK_LABEL(desc), GTK_JUSTIFY_CENTER);
        
        GtkWidget* entry = gtk_password_entry_new();
        gtk_widget_set_size_request(entry, 300, -1);
        gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(entry), TRUE);
        
        GtkWidget* btn = gtk_button_new_with_label("Authenticate");
        gtk_widget_add_css_class(btn, "suggested-action");
        gtk_widget_set_size_request(btn, 150, 44);
        
        gtk_box_append(GTK_BOX(box), icon);
        gtk_box_append(GTK_BOX(box), title);
        gtk_box_append(GTK_BOX(box), desc);
        gtk_box_append(GTK_BOX(box), entry);
        gtk_box_append(GTK_BOX(box), btn);
        
        adw_application_window_set_content(ADW_APPLICATION_WINDOW(self), box);
        
        auto try_auth = +[](GtkWidget*, gpointer data) {
             GtkWidget* e = (GtkWidget*)data;
             const char* pass = gtk_editable_get_text(GTK_EDITABLE(e));
             if (!pass || strlen(pass) == 0) return;
             
             // 1. Allow X11
             if (getenv("DISPLAY")) system("xhost +SI:localuser:root > /dev/null 2>&1");
             
             int pipefd[2];
             if (pipe(pipefd) == -1) return;
             
             pid_t pid = fork();
             if (pid == -1) return;
             
             if (pid == 0) {
                 close(pipefd[1]); 
                 dup2(pipefd[0], STDIN_FILENO);
                 close(pipefd[0]);
                 
                 char self_path[PATH_MAX];
                 ssize_t count = readlink("/proc/self/exe", self_path, PATH_MAX);
                 if (count != -1) self_path[count] = '\0';
                 else exit(1);
                 
                 execlp("sudo", "sudo", "-S", "-E", self_path, NULL);
                 exit(1);
             } else {
                 close(pipefd[0]);
                 write(pipefd[1], pass, strlen(pass));
                 write(pipefd[1], "\n", 1);
                 close(pipefd[1]);
                 
                 struct CheckData { pid_t p; };
                 CheckData* d = new CheckData{pid};
                 
                 g_timeout_add(500, +[](gpointer data) -> gboolean {
                     CheckData* cd = (CheckData*)data;
                     int status;
                     pid_t res = waitpid(cd->p, &status, WNOHANG);
                     
                     if (res == 0) {
                         exit(0);
                     } else {
                         kd_show_toast("Authentication Failed. Incorrect Password?");
                     }
                     delete cd;
                     return G_SOURCE_REMOVE;
                 }, d);
             }
        };
        
        g_signal_connect(btn, "clicked", G_CALLBACK(try_auth), entry);
        g_signal_connect(entry, "activate", G_CALLBACK(try_auth), entry);
        
        return;
    }

    self->split_view = ADW_NAVIGATION_SPLIT_VIEW(adw_navigation_split_view_new());
    
    GtkWidget* toast_overlay = adw_toast_overlay_new();
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(toast_overlay), GTK_WIDGET(self->split_view));
    
    g_object_set_data(G_OBJECT(self), "toast-overlay", toast_overlay);
    
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(self), toast_overlay);

    // stupid sidebar
    GtkWidget* sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    AdwNavigationPage* sidebar_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(sidebar_box, "Sidebar"));
    adw_navigation_split_view_set_sidebar(self->split_view, sidebar_page);

    GtkWidget* sidebar_header = adw_header_bar_new();
    gtk_box_append(GTK_BOX(sidebar_box), sidebar_header);

    GtkWidget* sidebar_content = adw_preferences_page_new();
    gtk_box_append(GTK_BOX(sidebar_box), sidebar_content);
    gtk_widget_set_vexpand(sidebar_content, TRUE);

    // Core Group
    GtkWidget* core_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(core_group), "Core");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(sidebar_content), ADW_PREFERENCES_GROUP(core_group));

    // Plugins Group
    GtkWidget* plugins_group_widget = adw_preferences_group_new();
    self->nav_group = ADW_PREFERENCES_GROUP(plugins_group_widget);
    adw_preferences_group_set_title(self->nav_group, "Plugins");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(sidebar_content), self->nav_group);
    
    PluginManager::get().set_uninstall_callback([self](KdPlugin* p) {
        GtkWidget* target_row = nullptr;
        
        auto find_row = [&](auto&& self_lambda, GtkWidget* widget) -> void {
            if (!widget) return;
            
            KdPlugin* ptr = (KdPlugin*)g_object_get_data(G_OBJECT(widget), "plugin-ptr");
            if (ptr == p) {
                target_row = widget;
                return;
            }
            
            for (GtkWidget* c = gtk_widget_get_first_child(widget); c != nullptr; c = gtk_widget_get_next_sibling(c)) {
                self_lambda(self_lambda, c);
                if (target_row) return;
            }
        };
        
        find_row(find_row, GTK_WIDGET(self->nav_group));
        
        if (target_row) {
            adw_preferences_group_remove(self->nav_group, target_row);
        }
    });

    auto add_row = [self](AdwPreferencesGroup* group, const char* title, const char* icon_name, const char* id, KdPlugin* plugin = nullptr) {
        GtkWidget* row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
        if (icon_name) {
            GtkWidget* icon = gtk_image_new_from_icon_name(icon_name);
            adw_action_row_add_prefix(ADW_ACTION_ROW(row), icon);
        }
        
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
        
        if (id) g_object_set_data(G_OBJECT(row), "page-id", (gpointer)id);
        if (plugin) g_object_set_data(G_OBJECT(row), "plugin-ptr", plugin);

        g_signal_connect(row, "activated", G_CALLBACK(on_sidebar_row_activated), self);
        adw_preferences_group_add(group, GTK_WIDGET(row));
    };

    add_row(ADW_PREFERENCES_GROUP(core_group), "Power", "system-shutdown-symbolic", "power");
    add_row(ADW_PREFERENCES_GROUP(core_group), "Input", "input-keyboard-symbolic", "input");
    add_row(ADW_PREFERENCES_GROUP(core_group), "Display", "video-display-symbolic", "display");
    add_row(ADW_PREFERENCES_GROUP(core_group), "Dependencies", "system-software-install-symbolic", "dependencies");
    add_row(ADW_PREFERENCES_GROUP(core_group), "Plugins Store", "software-store-symbolic", "store");

    PluginManager::get().set_plugin_loaded_callback([self, add_row](KdPlugin* p) {
         add_row(self->nav_group, p->get_name().c_str(), "application-x-addon-symbolic", "plugin", p);
    });


    PluginManager::get().load_default_locations();

    // Footer
    GtkWidget* footer_group = adw_preferences_group_new();
    gtk_widget_set_margin_bottom(footer_group, 12);
    gtk_widget_set_margin_start(footer_group, 12);
    gtk_widget_set_margin_end(footer_group, 12);
    gtk_box_append(GTK_BOX(sidebar_box), footer_group);

    auto add_footer_row = [&](const char* title, const char* icon_name, const char* id) {
        GtkWidget* row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
        if (icon_name) {
            GtkWidget* icon = gtk_image_new_from_icon_name(icon_name);
            adw_action_row_add_prefix(ADW_ACTION_ROW(row), icon);
        }
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), TRUE);
        g_object_set_data(G_OBJECT(row), "page-id", (gpointer)id);
        g_signal_connect(row, "activated", G_CALLBACK(on_sidebar_row_activated), self);
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(footer_group), GTK_WIDGET(row));
    };

    add_footer_row("AboutMe", "network-workgroup-symbolic", "socials");
    
    // Main initial screen 
    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    AdwNavigationPage* content_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(content_box, "Content"));
    adw_navigation_split_view_set_content(self->split_view, content_page);

    GtkWidget* content_header = adw_header_bar_new();
    gtk_box_append(GTK_BOX(content_box), content_header);

    GtkWidget* status_page = adw_status_page_new();
    adw_status_page_set_title(ADW_STATUS_PAGE(status_page), "KernelDrive");
    adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "Select a category to strat.");
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "utilities-terminal-symbolic");
    gtk_box_append(GTK_BOX(content_box), status_page);
    gtk_widget_set_vexpand(status_page, TRUE);
}

KdMainWindow* kd_main_window_new(GtkApplication* app) {
    return (KdMainWindow*)g_object_new(KD_TYPE_MAIN_WINDOW, "application", app, NULL);
}

extern "C" void kd_show_toast(const char* msg) {
    GList* windows = gtk_application_get_windows(GTK_APPLICATION(g_application_get_default()));
    if (windows) {
        GtkWidget* win = GTK_WIDGET(windows->data);
        GtkWidget* overlay = (GtkWidget*)g_object_get_data(G_OBJECT(win), "toast-overlay");
        if (overlay) {
            adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(overlay), adw_toast_new(msg));
        }
    }
}

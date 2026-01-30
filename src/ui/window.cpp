#include "window.h"
#include "../core/plugin_manager.h"
#include "pages/power_page.h"
#include "pages/input_page.h"
#include "pages/display_page.h"
#include "pages/store_page.h"

struct _KdMainWindow {
    AdwApplicationWindow parent_instance;
    AdwNavigationSplitView* split_view;
    AdwPreferencesGroup* nav_group;
};

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
    } else if (g_strcmp0(page_id, "store") == 0) {
        content_widget = kd_store_page_new();
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

    self->split_view = ADW_NAVIGATION_SPLIT_VIEW(adw_navigation_split_view_new());
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(self), GTK_WIDGET(self->split_view));

    // stupid sidebar
    GtkWidget* sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    AdwNavigationPage* sidebar_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(sidebar_box, "Sidebar"));
    adw_navigation_split_view_set_sidebar(self->split_view, sidebar_page);

    GtkWidget* sidebar_header = adw_header_bar_new();
    gtk_box_append(GTK_BOX(sidebar_box), sidebar_header);

    GtkWidget* sidebar_content = adw_preferences_page_new();
    gtk_box_append(GTK_BOX(sidebar_box), sidebar_content);
    gtk_widget_set_vexpand(sidebar_content, TRUE);

    GtkWidget* nav_group_widget = adw_preferences_group_new();
    self->nav_group = ADW_PREFERENCES_GROUP(nav_group_widget);
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(sidebar_content), self->nav_group);
    
    PluginManager::get().set_uninstall_callback([self](KdPlugin* p) {
        GtkWidget* child = gtk_widget_get_first_child(GTK_WIDGET(self->nav_group));
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

    auto add_row = [&](const char* title, const char* icon_name, const char* id, KdPlugin* plugin = nullptr) {
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
        adw_preferences_group_add(self->nav_group, GTK_WIDGET(row));
    };

    // Built-in Features
    add_row("Power", "system-shutdown-symbolic", "power");
    add_row("Input", "input-keyboard-symbolic", "input");
    add_row("Display", "video-display-symbolic", "display");
    add_row("Plugins Store", "software-store-symbolic", "store");

    // Load Plugins
    PluginManager::get().load_default_locations(); 

    for (const auto& plugin : PluginManager::get().get_plugins()) {
         add_row(plugin->get_name().c_str(), "application-x-addon-symbolic", "plugin", plugin.get());
    }

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

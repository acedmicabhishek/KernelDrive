#include "window.h"

struct _KdMainWindow {
    AdwApplicationWindow parent_instance;
    AdwNavigationSplitView* split_view;
};

G_DEFINE_FINAL_TYPE(KdMainWindow, kd_main_window, ADW_TYPE_APPLICATION_WINDOW)

static void kd_main_window_dispose(GObject* object) {
    G_OBJECT_CLASS(kd_main_window_parent_class)->dispose(object);
}

static void kd_main_window_class_init(KdMainWindowClass* klass) {
    G_OBJECT_CLASS(klass)->dispose = kd_main_window_dispose;
}

static void kd_main_window_init(KdMainWindow* self) {

    gtk_window_set_default_size(GTK_WINDOW(self), 1100, 700);
    gtk_window_set_title(GTK_WINDOW(self), "KernelDrive");

    self->split_view = ADW_NAVIGATION_SPLIT_VIEW(adw_navigation_split_view_new());
    gtk_window_set_child(GTK_WINDOW(self), GTK_WIDGET(self->split_view));

    AdwNavigationPage* sidebar_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(NULL, "Sidebar"));
    adw_navigation_split_view_set_sidebar(self->split_view, sidebar_page);

    GtkWidget* sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    adw_navigation_page_set_child(sidebar_page, sidebar_box);

    GtkWidget* sidebar_header = adw_header_bar_new();
    gtk_box_append(GTK_BOX(sidebar_box), sidebar_header);

    GtkWidget* sidebar_content = adw_preferences_page_new();
    gtk_box_append(GTK_BOX(sidebar_box), sidebar_content);
    gtk_widget_set_vexpand(sidebar_content, TRUE);

    GtkWidget* nav_group = adw_preferences_group_new();
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(sidebar_content), ADW_PREFERENCES_GROUP(nav_group));

    const char* items[] = {"Power", "Input", "Display", "Plugins", NULL};
    const char* icons[] = {"system-shutdown-symbolic", "input-keyboard-symbolic", "video-display-symbolic", "application-x-addon-symbolic", NULL};

    for (int i = 0; items[i] != NULL; i++) {
        GtkWidget* row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), items[i]);
        GtkWidget* icon = gtk_image_new_from_icon_name(icons[i]);
        adw_action_row_add_prefix(ADW_ACTION_ROW(row), icon);
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(nav_group), GTK_WIDGET(row));
    }

    AdwNavigationPage* content_page = ADW_NAVIGATION_PAGE(adw_navigation_page_new(NULL, "Content"));
    adw_navigation_split_view_set_content(self->split_view, content_page);

    GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    adw_navigation_page_set_child(content_page, content_box);

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

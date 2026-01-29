#include "display_page.h"

struct _KdDisplayPage {
    AdwBin parent_instance;
};

G_DEFINE_FINAL_TYPE(KdDisplayPage, kd_display_page, ADW_TYPE_BIN)

static void kd_display_page_dispose(GObject* object) {
    G_OBJECT_CLASS(kd_display_page_parent_class)->dispose(object);
}

static void kd_display_page_class_init(KdDisplayPageClass* klass) {
    G_OBJECT_CLASS(klass)->dispose = kd_display_page_dispose;
}

static void kd_display_page_init(KdDisplayPage* self) {
    GtkWidget* status_page = adw_status_page_new();
    adw_status_page_set_title(ADW_STATUS_PAGE(status_page), "Display Settings");
    adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "Manage screen resolution and scaling.");
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "video-display-symbolic");
    adw_bin_set_child(ADW_BIN(self), status_page);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    
    GtkWidget* group = adw_preferences_group_new();
    gtk_box_append(GTK_BOX(box), group);
    
    // Resolution
    GtkWidget* res_row = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(res_row), "Resolution");
    const char* res_opts[] = {"1920x1080", "2560x1440", "3840x2160", NULL};
    adw_combo_row_set_model(ADW_COMBO_ROW(res_row), G_LIST_MODEL(gtk_string_list_new(res_opts)));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), res_row);

    // Scale
    GtkWidget* scale_row = adw_spin_row_new_with_range(1.0, 3.0, 0.25);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scale_row), "Scale Factor");
    adw_spin_row_set_value(ADW_SPIN_ROW(scale_row), 1.0);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), scale_row);
    
    adw_status_page_set_child(ADW_STATUS_PAGE(status_page), box);
}

GtkWidget* kd_display_page_new() {
    return GTK_WIDGET(g_object_new(KD_TYPE_DISPLAY_PAGE, NULL));
}

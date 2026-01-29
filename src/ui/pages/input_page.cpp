#include "input_page.h"

struct _KdInputPage {
    AdwBin parent_instance;
};

G_DEFINE_FINAL_TYPE(KdInputPage, kd_input_page, ADW_TYPE_BIN)

static void kd_input_page_dispose(GObject* object) {
    G_OBJECT_CLASS(kd_input_page_parent_class)->dispose(object);
}

static void kd_input_page_class_init(KdInputPageClass* klass) {
    G_OBJECT_CLASS(klass)->dispose = kd_input_page_dispose;
}

static void kd_input_page_init(KdInputPage* self) {
    GtkWidget* status_page = adw_status_page_new();
    adw_status_page_set_title(ADW_STATUS_PAGE(status_page), "Input Devices");
    adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "Configure mouse, keyboard, and touchpad settings.");
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "input-keyboard-symbolic");
    adw_bin_set_child(ADW_BIN(self), status_page);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    
    // Mouse
    GtkWidget* mouse_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(mouse_group), "Mouse & Touchpad");
    gtk_box_append(GTK_BOX(box), mouse_group);

    // Accel
    GtkWidget* accel_row = adw_spin_row_new_with_range(-1.0, 1.0, 0.1);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(accel_row), "Pointer Speed");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(accel_row), "Adjust cursor acceleration.");
    adw_spin_row_set_value(ADW_SPIN_ROW(accel_row), 0.0);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(mouse_group), accel_row);

    // Keyboard
    GtkWidget* kb_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(kb_group), "Keyboard");
    gtk_box_append(GTK_BOX(box), kb_group);

    GtkWidget* repeat_row = adw_spin_row_new_with_range(1, 100, 1);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(repeat_row), "Repeat Rate");
    adw_spin_row_set_value(ADW_SPIN_ROW(repeat_row), 25.0);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(kb_group), repeat_row);
    
    adw_status_page_set_child(ADW_STATUS_PAGE(status_page), box);
}

GtkWidget* kd_input_page_new() {
    return GTK_WIDGET(g_object_new(KD_TYPE_INPUT_PAGE, NULL));
}

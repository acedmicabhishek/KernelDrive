#include "input_page.h"
#include "../../core/input/input_manager.h"

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
    
    // Mouse & Touchpad
    GtkWidget* mouse_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(mouse_group), "Mouse & Touchpad");
    gtk_box_append(GTK_BOX(box), mouse_group);

    // Natural Scrolling
    GtkWidget* natural_row = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(natural_row), "Natural Scrolling");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(natural_row), "Content moves in the direction of your fingers.");
    adw_switch_row_set_active(ADW_SWITCH_ROW(natural_row), InputManager::get().get_touchpad_natural_scroll());
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(mouse_group), natural_row);
    
    // Tap to Click
    GtkWidget* tap_row = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(tap_row), "Tap to Click");
    adw_switch_row_set_active(ADW_SWITCH_ROW(tap_row), InputManager::get().get_touchpad_tap_to_click());
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(mouse_group), tap_row);

    // Pointer Speed
    GtkWidget* accel_row = adw_spin_row_new_with_range(-1.0, 1.0, 0.1);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(accel_row), "Pointer Speed");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(accel_row), "Adjust cursor acceleration.");
    adw_spin_row_set_value(ADW_SPIN_ROW(accel_row), InputManager::get().get_pointer_speed());
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(mouse_group), accel_row);

    // Keyboard
    GtkWidget* kb_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(kb_group), "Keyboard");
    gtk_box_append(GTK_BOX(box), kb_group);

    GtkWidget* repeat_row = adw_spin_row_new_with_range(1, 100, 1);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(repeat_row), "Repeat Rate");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(repeat_row), "Characters per second.");
    adw_spin_row_set_value(ADW_SPIN_ROW(repeat_row), (double)InputManager::get().get_keyboard_rate());
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(kb_group), repeat_row);
    
    GtkWidget* delay_row = adw_spin_row_new_with_range(100, 1000, 50);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(delay_row), "Repeat Delay");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(delay_row), "Delay before repeating starts (ms).");
    adw_spin_row_set_value(ADW_SPIN_ROW(delay_row), (double)InputManager::get().get_keyboard_delay());
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(kb_group), delay_row);
    
    adw_status_page_set_child(ADW_STATUS_PAGE(status_page), box);
    
    // Natural Scroll
    g_signal_connect(natural_row, "notify::active", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
        bool val = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
        InputManager::get().set_touchpad_natural_scroll(val);
    }), NULL);
    
    // Tap to Click
    g_signal_connect(tap_row, "notify::active", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
        bool val = adw_switch_row_get_active(ADW_SWITCH_ROW(obj));
        InputManager::get().set_touchpad_tap_to_click(val);
    }), NULL);
    
    // Speed
    g_signal_connect(accel_row, "notify::value", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
        double val = adw_spin_row_get_value(ADW_SPIN_ROW(obj));
        InputManager::get().set_pointer_speed(val);
    }), NULL);
    
    // Keyboard Rate & Delay
    struct KbData { GtkWidget* rate; GtkWidget* delay; };
    KbData* kb_data = new KbData{repeat_row, delay_row};
    g_object_set_data_full(G_OBJECT(kb_group), "kb_data", kb_data, [](gpointer d){ delete (KbData*)d; });
    
    auto update_kb = +[](GObject* obj, GParamSpec*, gpointer user_data) {
        GtkWidget* group = (GtkWidget*)user_data;
        KbData* d = (KbData*)g_object_get_data(G_OBJECT(group), "kb_data");
        
        int rate = (int)adw_spin_row_get_value(ADW_SPIN_ROW(d->rate));
        int delay = (int)adw_spin_row_get_value(ADW_SPIN_ROW(d->delay));
        
        InputManager::get().set_keyboard_repeat_info(rate, delay);
    };
    
    g_signal_connect(repeat_row, "notify::value", G_CALLBACK(update_kb), kb_group);
    g_signal_connect(delay_row, "notify::value", G_CALLBACK(update_kb), kb_group);
}

GtkWidget* kd_input_page_new() {
    return GTK_WIDGET(g_object_new(KD_TYPE_INPUT_PAGE, NULL));
}

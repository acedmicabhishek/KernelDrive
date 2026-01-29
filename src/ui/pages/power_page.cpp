#include "power_page.h"
#include "../../core/sysfs_writer.h"
#include <vector>
#include <string>
#include <filesystem>

struct _KdPowerPage {
    AdwBin parent_instance;
    AdwComboRow* governor_row;
};

G_DEFINE_FINAL_TYPE(KdPowerPage, kd_power_page, ADW_TYPE_BIN)

static void apply_governor_to_all_cpus(const char* governor) {
    namespace fs = std::filesystem;
    std::string base_path = "/sys/devices/system/cpu/";
    if (!fs::exists(base_path)) return;

    for (const auto& entry : fs::directory_iterator(base_path)) {
        std::string filename = entry.path().filename().string();
        if (filename.rfind("cpu", 0) == 0 && std::isdigit(filename[3])) {
             std::string path = entry.path().string() + "/cpufreq/scaling_governor";
             if (fs::exists(path)) {
                 SysfsWriter::write(path, governor);
             }
        }
    }
}

static void on_governor_selected(AdwComboRow* row, [[maybe_unused]] GParamSpec* pspec, [[maybe_unused]] gpointer user_data) {
    guint selected_idx = adw_combo_row_get_selected(row);
    GListModel* model = adw_combo_row_get_model(row);
    if (!model) return;

    const char* governor = gtk_string_list_get_string(GTK_STRING_LIST(model), selected_idx);
    if (governor) {
        apply_governor_to_all_cpus(governor);
    }
}

static void kd_power_page_dispose(GObject* object) {
    G_OBJECT_CLASS(kd_power_page_parent_class)->dispose(object);
}

static void kd_power_page_class_init(KdPowerPageClass* klass) {
    G_OBJECT_CLASS(klass)->dispose = kd_power_page_dispose;
}

static void kd_power_page_init(KdPowerPage* self) {
    GtkWidget* status_page = adw_status_page_new();
    adw_status_page_set_title(ADW_STATUS_PAGE(status_page), "Power Management");
    adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "Control CPU frequency scaling governors.");
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "system-shutdown-symbolic");

    adw_bin_set_child(ADW_BIN(self), status_page);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    
    GtkWidget* group = adw_preferences_group_new();
    gtk_box_append(GTK_BOX(box), group);

    self->governor_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(self->governor_row), "CPU Governor");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(self->governor_row), "Controls how the CPU scales frequency.");

    const char* govs[] = {"schedutil", "powersave", "performance", "ondemand", "conservative", NULL};
    GtkStringList* list = gtk_string_list_new(govs);
    adw_combo_row_set_model(self->governor_row, G_LIST_MODEL(list));
    
    g_signal_connect(self->governor_row, "notify::selected", G_CALLBACK(on_governor_selected), NULL);

    adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), GTK_WIDGET(self->governor_row));

    adw_status_page_set_child(ADW_STATUS_PAGE(status_page), box);
}

GtkWidget* kd_power_page_new() {
    return GTK_WIDGET(g_object_new(KD_TYPE_POWER_PAGE, NULL));
}

#include "display_page.h"
#include "../../core/display/display_manager.h"
#include <string>


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

static void populate_displays(GtkWidget* box) {
    auto backend = DisplayManager::get().get_backend();
    auto displays = backend->get_displays();
    
    if (displays.empty()) {
        GtkWidget* label = gtk_label_new("No displays detected or backend not supported.");
        gtk_widget_set_margin_top(label, 32);
        gtk_box_append(GTK_BOX(box), label);
        return;
    }

    for (const auto& display : displays) {
        GtkWidget* group = adw_preferences_group_new();
        std::string title = display.model.empty() ? display.id : (display.make + " " + display.model);
        adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(group), title.c_str());
        adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(group), display.id.c_str());
        gtk_box_append(GTK_BOX(box), group);

        
        GtkWidget* scale_row = adw_spin_row_new_with_range(1.0, 3.0, 0.25);
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(scale_row), "Scale");
        adw_spin_row_set_value(ADW_SPIN_ROW(scale_row), display.scale);
        
        
        
        std::string* id_ptr = new std::string(display.id); 
        g_object_set_data_full(G_OBJECT(scale_row), "display_id", id_ptr, (GDestroyNotify)operator delete);
        
        g_signal_connect(scale_row, "notify::value", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer) {
            AdwSpinRow* row = ADW_SPIN_ROW(obj);
            double val = adw_spin_row_get_value(row);
            std::string* id = (std::string*)g_object_get_data(obj, "display_id");
            if(id) DisplayManager::get().get_backend()->set_scale(*id, val);
        }), NULL);

        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), scale_row);
        
        
        GtkWidget* res_row = adw_combo_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(res_row), "Resolution");
        
        
        GtkWidget* rate_row = adw_combo_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(rate_row), "Refresh Rate");

        
        std::vector<std::string> unique_res;
        std::vector<DisplayMode> all_modes = display.available_modes;
        if (all_modes.empty()) all_modes.push_back(display.current_mode);

        for (const auto& m : all_modes) {
            std::string s = std::to_string(m.width) + "x" + std::to_string(m.height);
            bool exists = false;
            for(const auto& r : unique_res) if(r == s) exists = true;
            if(!exists) unique_res.push_back(s);
        }

        GtkStringList* res_list = gtk_string_list_new(NULL);
        int active_res_idx = 0;
        for (size_t i = 0; i < unique_res.size(); ++i) {
            gtk_string_list_append(res_list, unique_res[i].c_str());
            std::string cur_res_s = std::to_string(display.current_mode.width) + "x" + std::to_string(display.current_mode.height);
            if (unique_res[i] == cur_res_s) active_res_idx = i;
        }
        adw_combo_row_set_model(ADW_COMBO_ROW(res_row), G_LIST_MODEL(res_list));
        adw_combo_row_set_selected(ADW_COMBO_ROW(res_row), active_res_idx);

        
        
        auto populate_rates = [](AdwComboRow* r_row, const std::string& res_str, const std::vector<DisplayMode>& modes, int current_rate = -1) {
            GtkStringList* list = gtk_string_list_new(NULL);
            int valid_idx = 0;
            int count = 0;
            for (const auto& m : modes) {
                std::string s = std::to_string(m.width) + "x" + std::to_string(m.height);
                if (s == res_str) {
                    std::string rate_s = std::to_string(m.refresh_rate_mHz / 1000.0) + " Hz";
                    gtk_string_list_append(list, rate_s.c_str());
                    if (current_rate > 0 && std::abs(m.refresh_rate_mHz - current_rate) < 100) valid_idx = count;
                    count++;
                }
            }
            if (count == 0) gtk_string_list_append(list, "N/A");
            adw_combo_row_set_model(r_row, G_LIST_MODEL(list));
            adw_combo_row_set_selected(r_row, valid_idx);
        };

        
        std::string initial_res = std::to_string(display.current_mode.width) + "x" + std::to_string(display.current_mode.height);
        populate_rates(ADW_COMBO_ROW(rate_row), initial_res, all_modes, display.current_mode.refresh_rate_mHz);
        
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), res_row);
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(group), rate_row);

        
        
        struct CbData {
             std::string id;
             std::vector<DisplayMode> modes;
             GtkWidget *res_row;
             GtkWidget *rate_row;
        };
        CbData* data = new CbData{display.id, all_modes, res_row, rate_row};

        
        g_object_set_data_full(G_OBJECT(group), "cb_data", data, [](gpointer d){ delete (CbData*)d; });
        
        
        g_signal_connect(res_row, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer user_data){
            GtkWidget* group = (GtkWidget*)user_data;
            CbData* d = (CbData*)g_object_get_data(G_OBJECT(group), "cb_data");
            
            AdwComboRow* r_row = ADW_COMBO_ROW(d->res_row);
            guint idx = adw_combo_row_get_selected(r_row);
            
            GtkStringList* sl = GTK_STRING_LIST(adw_combo_row_get_model(r_row));
            const char* res_str = gtk_string_list_get_string(sl, idx);
            
            if (res_str) {
                
                
                 GtkStringList* list = gtk_string_list_new(NULL);
                 for (const auto& m : d->modes) {
                    std::string s = std::to_string(m.width) + "x" + std::to_string(m.height);
                    if (s == res_str) {
                        std::string rate_s = std::to_string(m.refresh_rate_mHz / 1000.0) + " Hz";
                        gtk_string_list_append(list, rate_s.c_str());
                    }
                }
                if (g_list_model_get_n_items(G_LIST_MODEL(list)) == 0) gtk_string_list_append(list, "N/A");
                adw_combo_row_set_model(ADW_COMBO_ROW(d->rate_row), G_LIST_MODEL(list));
                adw_combo_row_set_selected(ADW_COMBO_ROW(d->rate_row), 0);
                
                
                
                
                for (const auto& m : d->modes) {
                    std::string s = std::to_string(m.width) + "x" + std::to_string(m.height);
                    if (s == res_str) {
                        DisplayManager::get().get_backend()->set_mode(d->id, m);
                        break;
                    }
                }
            }
        }), group);

        
        g_signal_connect(rate_row, "notify::selected", G_CALLBACK(+[](GObject* obj, GParamSpec*, gpointer user_data){
             GtkWidget* group = (GtkWidget*)user_data;
             CbData* d = (CbData*)g_object_get_data(G_OBJECT(group), "cb_data");
             
             AdwComboRow* res_r = ADW_COMBO_ROW(d->res_row);
             AdwComboRow* rate_r = ADW_COMBO_ROW(d->rate_row);
             
             
             guint r_idx = adw_combo_row_get_selected(res_r);
             GtkStringList* r_sl = GTK_STRING_LIST(adw_combo_row_get_model(res_r));
             const char* res_str = gtk_string_list_get_string(r_sl, r_idx);
             
             
             guint rate_idx = adw_combo_row_get_selected(rate_r);
             
             
             int count = 0;
             for (const auto& m : d->modes) {
                std::string s = std::to_string(m.width) + "x" + std::to_string(m.height);
                if (s == res_str) {
                    if (count == (int)rate_idx) {
                        DisplayManager::get().get_backend()->set_mode(d->id, m);
                        break;
                    }
                    count++;
                }
            }
        }), group);
    }
}

static void kd_display_page_init(KdDisplayPage* self) {
    GtkWidget* status_page = adw_status_page_new();
    adw_status_page_set_title(ADW_STATUS_PAGE(status_page), "Display Settings");
    adw_status_page_set_description(ADW_STATUS_PAGE(status_page), "Manage screen resolution and scaling.");
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(status_page), "video-display-symbolic");
    adw_bin_set_child(ADW_BIN(self), status_page);

    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    
    
    GtkWidget* refresh_btn = gtk_button_new_with_label("Refresh Detect");
    gtk_widget_set_halign(refresh_btn, GTK_ALIGN_END);
    g_signal_connect(refresh_btn, "clicked", G_CALLBACK(+[](GtkButton*, gpointer data){
         
         
    }), box);
    

    populate_displays(box);
    
    adw_status_page_set_child(ADW_STATUS_PAGE(status_page), box);
}

GtkWidget* kd_display_page_new() {
    return GTK_WIDGET(g_object_new(KD_TYPE_DISPLAY_PAGE, NULL));
}

#include "info_page.h"
#include "../backend/sys_stats.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>

static GtkWidget* make_label(const std::string& text, const char* css_class = nullptr) {
    GtkWidget* l = gtk_label_new(text.c_str());
    if (css_class) gtk_widget_add_css_class(l, css_class);
    gtk_widget_set_halign(l, GTK_ALIGN_START);
    return l;
}

struct InfoWidgets {
    GtkWidget *cpu_bar, *cpu_label;
    GtkWidget *ram_bar, *ram_label;
    GtkWidget *swap_bar, *swap_label;
    GtkWidget *disk_bar, *disk_label;
    
    std::vector<std::pair<GtkWidget*, GtkWidget*>> gpu_widgets;

    GtkWidget *net_label;
    GtkWidget *uptime_label;
};

static std::string fmt_bytes(long bytes) {
    if (bytes > 1024 * 1024) return std::to_string(bytes / 1024 / 1024) + " MB/s";
    if (bytes > 1024) return std::to_string(bytes / 1024) + " KB/s";
    return std::to_string(bytes) + " B/s";
}

static gboolean update_stats(gpointer data) {
    InfoWidgets* w = (InfoWidgets*)data;
    
    auto cpu = SysStats::get_cpu_stats();
    gtk_level_bar_set_value(GTK_LEVEL_BAR(w->cpu_bar), cpu.usage_percent / 100.0);
    
    std::stringstream cpu_ss;
    cpu_ss << std::fixed << std::setprecision(1) << cpu.usage_percent << "%";
    if (cpu.frequency_mhz > 0) cpu_ss << " • " << (int)cpu.frequency_mhz << " MHz";
    if (cpu.temp_celsius > 0) cpu_ss << " • " << (int)cpu.temp_celsius << "°C";
    gtk_label_set_text(GTK_LABEL(w->cpu_label), cpu_ss.str().c_str());

    auto gpus = SysStats::get_gpu_stats();
    size_t count = std::min(gpus.size(), w->gpu_widgets.size());
    for (size_t i = 0; i < count; ++i) {
        gtk_level_bar_set_value(GTK_LEVEL_BAR(w->gpu_widgets[i].first), gpus[i].usage_percent / 100.0);
        
        std::stringstream gpu_ss;
        gpu_ss << std::fixed << std::setprecision(1) << gpus[i].usage_percent << "%";
        gtk_label_set_text(GTK_LABEL(w->gpu_widgets[i].second), gpu_ss.str().c_str());
    }

    auto ram = SysStats::get_ram_stats();
    double ram_usage = (double)ram.used_mb / ram.total_mb;
    gtk_level_bar_set_value(GTK_LEVEL_BAR(w->ram_bar), ram_usage);
    
    std::stringstream ram_ss;
    ram_ss << ram.used_mb << " MB / " << ram.total_mb << " MB";
    gtk_label_set_text(GTK_LABEL(w->ram_label), ram_ss.str().c_str());

    if (ram.swap_total_mb > 0) {
        double swap_usage = (double)ram.swap_used_mb / ram.swap_total_mb;
        gtk_level_bar_set_value(GTK_LEVEL_BAR(w->swap_bar), swap_usage);
        std::stringstream swap_ss;
        swap_ss << ram.swap_used_mb << " MB / " << ram.swap_total_mb << " MB";
        gtk_label_set_text(GTK_LABEL(w->swap_label), swap_ss.str().c_str());
    }

    auto disk = SysStats::get_disk_stats();
    if (disk.total_gb > 0) {
        gtk_level_bar_set_value(GTK_LEVEL_BAR(w->disk_bar), disk.usage_percent / 100.0);
        std::stringstream disk_ss;
        disk_ss << disk.used_gb << " GB / " << disk.total_gb << " GB (" << (int)disk.usage_percent << "%)";
        gtk_label_set_text(GTK_LABEL(w->disk_label), disk_ss.str().c_str());
    }

    auto net = SysStats::get_net_stats();
    std::stringstream net_ss;
    net_ss << "↓ " << fmt_bytes(net.rx_bytes_sec) << "   ↑ " << fmt_bytes(net.tx_bytes_sec);
    gtk_label_set_text(GTK_LABEL(w->net_label), net_ss.str().c_str());

    auto os = SysStats::get_os_info();
    gtk_label_set_text(GTK_LABEL(w->uptime_label), ("Uptime: " + os.uptime).c_str());

    return G_SOURCE_CONTINUE;
}

GtkWidget* InfoPage::create() {
    GtkWidget* page = adw_preferences_page_new();
    
    GtkWidget* header_group = adw_preferences_group_new();
    auto os = SysStats::get_os_info();
    
    GtkWidget* banner_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 24);
    gtk_widget_set_margin_top(banner_box, 24);
    gtk_widget_set_margin_bottom(banner_box, 24);
    gtk_widget_set_halign(banner_box, GTK_ALIGN_CENTER);

    GtkWidget* text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_valign(text_box, GTK_ALIGN_CENTER);

    GtkWidget* host = gtk_label_new(os.hostname.c_str());
    gtk_widget_add_css_class(host, "title-1");
    gtk_widget_set_halign(host, GTK_ALIGN_CENTER); 
    
    GtkWidget* distro = gtk_label_new(os.os_name.c_str());
    gtk_widget_add_css_class(distro, "title-2");
    gtk_widget_set_halign(distro, GTK_ALIGN_CENTER);
    
    GtkWidget* kernel = gtk_label_new(("Kernel: " + os.kernel_version).c_str());
    gtk_widget_add_css_class(kernel, "caption");
    gtk_widget_set_halign(kernel, GTK_ALIGN_CENTER);
    
    GtkWidget* uptime = gtk_label_new("Calculating...");
    gtk_widget_add_css_class(uptime, "caption");
    gtk_widget_set_halign(uptime, GTK_ALIGN_CENTER);

    gtk_box_append(GTK_BOX(text_box), host);
    gtk_box_append(GTK_BOX(text_box), distro);
    gtk_box_append(GTK_BOX(text_box), kernel);
    gtk_box_append(GTK_BOX(text_box), uptime);

    gtk_box_append(GTK_BOX(banner_box), text_box);

    adw_preferences_group_add(ADW_PREFERENCES_GROUP(header_group), banner_box);
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(header_group));


    GtkWidget* dash_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(dash_group), "Resources");
    
    auto make_row = [&](const char* title, const char* icon_name, GtkWidget** bar_out, GtkWidget** label_out) {
        GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_widget_set_margin_top(row_box, 12);
        gtk_widget_set_margin_bottom(row_box, 12);
        gtk_widget_set_margin_start(row_box, 12);
        gtk_widget_set_margin_end(row_box, 12);
        
        GtkWidget* header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        GtkWidget* img = gtk_image_new_from_icon_name(icon_name);
        GtkWidget* tit = gtk_label_new(title);
        gtk_widget_add_css_class(tit, "heading");
        GtkWidget* val = gtk_label_new("Loading...");
        gtk_widget_set_hexpand(val, TRUE);
        gtk_widget_set_halign(val, GTK_ALIGN_END);
        
        gtk_box_append(GTK_BOX(header), img);
        gtk_box_append(GTK_BOX(header), tit);
        gtk_box_append(GTK_BOX(header), val);
        
        GtkWidget* bar = gtk_level_bar_new();
        gtk_level_bar_set_mode(GTK_LEVEL_BAR(bar), GTK_LEVEL_BAR_MODE_CONTINUOUS);
        
        gtk_box_append(GTK_BOX(row_box), header);
        gtk_box_append(GTK_BOX(row_box), bar);
        
        GtkWidget* card = adw_bin_new();
        gtk_widget_add_css_class(card, "card");
        adw_bin_set_child(ADW_BIN(card), row_box);
        
        *bar_out = bar;
        *label_out = val;
        
        return card;
    };

    InfoWidgets* w = new InfoWidgets();
    w->uptime_label = uptime;
    
    GtkWidget* grid_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    
    std::string cpu_name = SysStats::get_cpu_model();
    gtk_box_append(GTK_BOX(grid_box), make_row(cpu_name.c_str(), "cpu-symbolic", &w->cpu_bar, &w->cpu_label));
    
    auto initial_gpus = SysStats::get_gpu_stats();
    for (const auto& gpu : initial_gpus) {
        GtkWidget *bar, *label;
        gtk_box_append(GTK_BOX(grid_box), make_row(gpu.name.c_str(), "video-display-symbolic", &bar, &label));
        w->gpu_widgets.push_back({bar, label});
    }

    gtk_box_append(GTK_BOX(grid_box), make_row("Memory", "drive-harddisk-solidstate-symbolic", &w->ram_bar, &w->ram_label));
    gtk_box_append(GTK_BOX(grid_box), make_row("Swap", "emblem-synchronizing-symbolic", &w->swap_bar, &w->swap_label));
    gtk_box_append(GTK_BOX(grid_box), make_row("Root Disk", "drive-harddisk-symbolic", &w->disk_bar, &w->disk_label));

    GtkWidget* net_card = adw_bin_new();
    gtk_widget_add_css_class(net_card, "card");
    
    GtkWidget* net_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_margin_top(net_box, 16);
    gtk_widget_set_margin_bottom(net_box, 16);
    gtk_widget_set_margin_start(net_box, 12);
    gtk_widget_set_margin_end(net_box, 12);

    GtkWidget* net_icon = gtk_image_new_from_icon_name("network-transmit-receive-symbolic");
    GtkWidget* net_title = gtk_label_new("Network");
    gtk_widget_add_css_class(net_title, "heading");
    
    w->net_label = gtk_label_new("Loading...");
    gtk_widget_set_hexpand(w->net_label, TRUE);
    gtk_widget_set_halign(w->net_label, GTK_ALIGN_END);

    gtk_box_append(GTK_BOX(net_box), net_icon);
    gtk_box_append(GTK_BOX(net_box), net_title);
    gtk_box_append(GTK_BOX(net_box), w->net_label);
    
    adw_bin_set_child(ADW_BIN(net_card), net_box);
    gtk_box_append(GTK_BOX(grid_box), net_card);

    adw_preferences_group_add(ADW_PREFERENCES_GROUP(dash_group), grid_box);
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(dash_group));

    g_timeout_add(1000, update_stats, w);
    
    g_object_set_data_full(G_OBJECT(page), "widgets", w, [](gpointer d) {
        delete (InfoWidgets*)d;
    });

    update_stats(w);

    return page;
}

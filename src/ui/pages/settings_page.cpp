#include "settings_page.h"
#include "../../core/config_manager.h"
#include "../../core/service_manager.h"
#include "../../config/socials.h"
#include <unistd.h>
#include <cstdlib>
#include <string>

struct _KdSettingsPage {
    AdwBin parent_instance;
};

G_DEFINE_FINAL_TYPE(KdSettingsPage, kd_settings_page, ADW_TYPE_BIN)

static void kd_settings_page_class_init(KdSettingsPageClass*) {}

// ---- appearance helpers -----------------------------------------------------

// A user's ~/.config/gtk-4.0/gtk.css (e.g. Catppuccin) may statically redefine
// window_bg_color / view_bg_color / sidebar_bg_color to dark values. GTK loads
// that for every app, so our light mode looks broken. We install our own
// provider ABOVE the user stylesheet and redefine the Adwaita named colors to
// match the active scheme, so the whole window stays consistent.
static void apply_color_overrides(bool dark) {
    static GtkCssProvider* provider = nullptr;
    if (!provider) {
        provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_display(
            gdk_display_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER + 1);
    }

    const char* css = dark ?
        "@define-color window_bg_color #242424;"
        "@define-color window_fg_color #ffffff;"
        "@define-color view_bg_color #1e1e1e;"
        "@define-color view_fg_color #ffffff;"
        "@define-color headerbar_bg_color #2e2e2e;"
        "@define-color headerbar_fg_color #ffffff;"
        "@define-color sidebar_bg_color #2e2e2e;"
        "@define-color sidebar_fg_color #ffffff;"
        "@define-color sidebar_backdrop_color #282828;"
        "@define-color card_bg_color rgba(255,255,255,0.08);"
        "@define-color popover_bg_color #383838;"
        :
        "@define-color window_bg_color #fafafb;"
        "@define-color window_fg_color rgba(0,0,6,0.8);"
        "@define-color view_bg_color #ffffff;"
        "@define-color view_fg_color rgba(0,0,6,0.8);"
        "@define-color headerbar_bg_color #ffffff;"
        "@define-color headerbar_fg_color rgba(0,0,6,0.8);"
        "@define-color sidebar_bg_color #ebebed;"
        "@define-color sidebar_fg_color rgba(0,0,6,0.8);"
        "@define-color sidebar_backdrop_color #f2f2f4;"
        "@define-color card_bg_color #ffffff;"
        "@define-color popover_bg_color #ffffff;";

    gtk_css_provider_load_from_string(provider, css);
}

static void apply_theme(const std::string& theme) {
    AdwStyleManager* mgr = adw_style_manager_get_default();
    if (theme == "light") {
        adw_style_manager_set_color_scheme(mgr, ADW_COLOR_SCHEME_FORCE_LIGHT);
    } else if (theme == "dark") {
        adw_style_manager_set_color_scheme(mgr, ADW_COLOR_SCHEME_FORCE_DARK);
    } else {
        adw_style_manager_set_color_scheme(mgr, ADW_COLOR_SCHEME_DEFAULT);
    }
    apply_color_overrides(adw_style_manager_get_dark(mgr));
}

static void apply_font(const std::string& font) {
    if (font.empty()) return;
    g_object_set(gtk_settings_get_default(), "gtk-font-name", font.c_str(), NULL);
}

void kd_settings_apply_saved() {
    auto& cfg = ConfigManager::get();

    // Default to dark to preserve the previous hardcoded behaviour.
    apply_theme(cfg.get_string("Appearance", "theme", "dark"));

    // Keep the colour overrides in sync if the effective scheme changes (e.g.
    // "System" mode while the OS toggles light/dark).
    static gulong handler = 0;
    if (handler == 0) {
        AdwStyleManager* mgr = adw_style_manager_get_default();
        handler = g_signal_connect(mgr, "notify::dark", G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer) {
            apply_color_overrides(adw_style_manager_get_dark(ADW_STYLE_MANAGER(o)));
        }), nullptr);
    }

    // When running as root the user's gsettings/dconf is unreadable, so pin a
    // theme that ships all the icons we use.
    if (geteuid() == 0) {
        g_object_set(gtk_settings_get_default(), "gtk-icon-theme-name", "Adwaita", NULL);
    }

    std::string font = cfg.get_string("Appearance", "font", "");
    if (!font.empty()) {
        apply_font(font);
    } else if (geteuid() == 0) {
        // Root fallback: the user's configured font is unreachable.
        apply_font("Noto Sans 11");
    }
}

static void open_url(GtkWidget* w, const std::string& url) {
    const char* sudo_user = std::getenv("SUDO_USER");
    if (sudo_user) {
        std::string cmd = "runuser -u " + std::string(sudo_user) +
                          " -- xdg-open " + url + " > /dev/null 2>&1 &";
        std::system(cmd.c_str());
    } else {
        gtk_show_uri(GTK_WINDOW(gtk_widget_get_root(w)), url.c_str(), GDK_CURRENT_TIME);
    }
}

// ---- page construction ------------------------------------------------------

static void kd_settings_page_init(KdSettingsPage* self) {
    auto& cfg = ConfigManager::get();

    GtkWidget* page = adw_preferences_page_new();
    adw_bin_set_child(ADW_BIN(self), page);

    // --- Appearance ---
    GtkWidget* appearance = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(appearance), "Appearance");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(appearance));

    // Theme combo: System / Light / Dark
    AdwComboRow* theme_row = ADW_COMBO_ROW(adw_combo_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(theme_row), "Theme");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(theme_row), "Light, dark, or follow the system");

    const char* theme_items[] = {"System", "Light", "Dark", NULL};
    GtkStringList* theme_list = gtk_string_list_new(theme_items);
    adw_combo_row_set_model(theme_row, G_LIST_MODEL(theme_list));

    std::string saved_theme = cfg.get_string("Appearance", "theme", "dark");
    guint theme_idx = 0; // System
    if (saved_theme == "light") theme_idx = 1;
    else if (saved_theme == "dark") theme_idx = 2;
    adw_combo_row_set_selected(theme_row, theme_idx);

    g_signal_connect(theme_row, "notify::selected", G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer) {
        guint idx = adw_combo_row_get_selected(ADW_COMBO_ROW(o));
        const char* val = (idx == 1) ? "light" : (idx == 2) ? "dark" : "system";
        apply_theme(val);
        ConfigManager::get().set_string("Appearance", "theme", val);
    }), nullptr);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(appearance), GTK_WIDGET(theme_row));

    // Font picker
    GtkWidget* font_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(font_row), "Font");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(font_row), "Application interface font");

    GtkFontDialog* font_dialog = gtk_font_dialog_new();
    GtkWidget* font_btn = gtk_font_dialog_button_new(font_dialog); // takes ownership
    gtk_widget_set_valign(font_btn, GTK_ALIGN_CENTER);

    // Initialise from the saved font, or the current effective font.
    std::string saved_font = cfg.get_string("Appearance", "font", "");
    if (saved_font.empty()) {
        gchar* cur = nullptr;
        g_object_get(gtk_settings_get_default(), "gtk-font-name", &cur, NULL);
        if (cur) { saved_font = cur; g_free(cur); }
    }
    if (!saved_font.empty()) {
        PangoFontDescription* desc = pango_font_description_from_string(saved_font.c_str());
        gtk_font_dialog_button_set_font_desc(GTK_FONT_DIALOG_BUTTON(font_btn), desc);
        pango_font_description_free(desc);
    }

    g_signal_connect(font_btn, "notify::font-desc", G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer) {
        const PangoFontDescription* desc = gtk_font_dialog_button_get_font_desc(GTK_FONT_DIALOG_BUTTON(o));
        if (!desc) return;
        gchar* str = pango_font_description_to_string(desc);
        apply_font(str);
        ConfigManager::get().set_string("Appearance", "font", str);
        g_free(str);
    }), nullptr);

    adw_action_row_add_suffix(ADW_ACTION_ROW(font_row), font_btn);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(appearance), font_row);

    // --- System ---
    GtkWidget* system_group = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(system_group), "System");
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(system_group),
        "Re-apply saved hardware settings automatically");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(system_group));

    GtkWidget* service_row = adw_switch_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(service_row), "Run as Service");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(service_row),
        "Apply CPU governor & power profile on every boot (systemd)");
    adw_switch_row_set_active(ADW_SWITCH_ROW(service_row), ServiceManager::get().apply_is_enabled());

    g_signal_connect(service_row, "notify::active", G_CALLBACK(+[](GObject* o, GParamSpec*, gpointer) {
        bool want = adw_switch_row_get_active(ADW_SWITCH_ROW(o));
        auto& svc = ServiceManager::get();
        bool ok = want ? svc.apply_enable() : svc.apply_disable();

        // If the operation failed, snap the switch back to the real state. The
        // re-set only emits notify::active when it actually differs, and the
        // follow-up enable/disable is then a no-op, so this terminates.
        bool actual = svc.apply_is_enabled();
        if (!ok && actual != want) {
            adw_switch_row_set_active(ADW_SWITCH_ROW(o), actual);
        }
        ConfigManager::get().set_bool("System", "run_as_service", actual);
    }), nullptr);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(system_group), service_row);

    // --- About ---
    GtkWidget* about = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(about), "About");
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(page), ADW_PREFERENCES_GROUP(about));

    GtkWidget* app_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(app_row), "KernelDrive");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(app_row), "Modular Linux system control application");
    adw_action_row_add_prefix(ADW_ACTION_ROW(app_row),
        gtk_image_new_from_icon_name("applications-system-symbolic"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(about), app_row);

    GtkWidget* ver_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(ver_row), "Version");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(ver_row), KD_VERSION);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(about), ver_row);

    GtkWidget* author_row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(author_row), "Author");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(author_row), "acedmicabhishek");
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(about), author_row);

    for (const auto& link : MY_SOCIALS) {
        GtkWidget* row = adw_action_row_new();
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), link.name.c_str());
        adw_action_row_set_subtitle(ADW_ACTION_ROW(row), link.subtitle.c_str());
        adw_action_row_add_prefix(ADW_ACTION_ROW(row),
            gtk_image_new_from_icon_name(link.icon_name.c_str()));

        GtkWidget* btn = gtk_button_new_from_icon_name("external-link-symbolic");
        gtk_widget_add_css_class(btn, "flat");
        gtk_widget_set_valign(btn, GTK_ALIGN_CENTER);
        g_object_set_data_full(G_OBJECT(btn), "url",
                               g_strdup(link.url.c_str()), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkWidget* w, gpointer) {
            const char* url = (const char*)g_object_get_data(G_OBJECT(w), "url");
            if (url) open_url(w, url);
        }), nullptr);

        adw_action_row_add_suffix(ADW_ACTION_ROW(row), btn);
        adw_action_row_set_activatable_widget(ADW_ACTION_ROW(row), btn);
        adw_preferences_group_add(ADW_PREFERENCES_GROUP(about), row);
    }
}

GtkWidget* kd_settings_page_new() {
    return GTK_WIDGET(g_object_new(KD_TYPE_SETTINGS_PAGE, NULL));
}

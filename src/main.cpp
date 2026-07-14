#include <adwaita.h>
#include "ui/window.h"
#include "ui/pages/settings_page.h"
#include "core/power/power_manager.h"
#include <cstring>

static void activate(GtkApplication* app, [[maybe_unused]] gpointer user_data) {
    // Applies saved theme + font, plus the root icon/font fallbacks.
    kd_settings_apply_saved();

    KdMainWindow* window = kd_main_window_new(app);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char* argv[]) {
    // Headless mode: re-apply saved hardware settings and exit. Used by the
    // systemd boot service so changes stay active without the GUI.
    if (argc > 1 && std::strcmp(argv[1], "--apply") == 0) {
        // Constructing the manager applies the stored governor + profile.
        PowerManager::get();
        return 0;
    }

    g_autoptr(AdwApplication) app = adw_application_new("com.acedmicabhishek.kerneldrive", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

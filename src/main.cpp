#include <adwaita.h>
#include "ui/window.h"
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

static void activate(GtkApplication* app, [[maybe_unused]] gpointer user_data) {
    AdwStyleManager* mgr = adw_style_manager_get_default();
    adw_style_manager_set_color_scheme(mgr, ADW_COLOR_SCHEME_PREFER_DARK);

    KdMainWindow* window = kd_main_window_new(app);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char* argv[]) {
    g_autoptr(AdwApplication) app = adw_application_new("com.acedmicabhishek.kerneldrive", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

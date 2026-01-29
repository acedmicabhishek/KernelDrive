#include <adwaita.h>

static void activate(GtkApplication* app, [[maybe_unused]] gpointer user_data) {
    GtkWidget* window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "KernelDrive");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget* label = gtk_label_new("KernelDrive");
    gtk_window_set_child(GTK_WINDOW(window), label);

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char* argv[]) {
    g_autoptr(AdwApplication) app = adw_application_new("com.acedmicabhishek.kerneldrive", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

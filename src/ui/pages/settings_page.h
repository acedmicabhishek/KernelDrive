#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define KD_TYPE_SETTINGS_PAGE (kd_settings_page_get_type())

G_DECLARE_FINAL_TYPE(KdSettingsPage, kd_settings_page, KD, SETTINGS_PAGE, AdwBin)

GtkWidget* kd_settings_page_new();

// Apply the persisted appearance settings (theme, font, and the root icon/font
// fallbacks). Call once at startup before the main window is shown.
void kd_settings_apply_saved();

G_END_DECLS

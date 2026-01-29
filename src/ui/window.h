#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define KD_TYPE_MAIN_WINDOW (kd_main_window_get_type())

G_DECLARE_FINAL_TYPE(KdMainWindow, kd_main_window, KD, MAIN_WINDOW, AdwApplicationWindow)

KdMainWindow* kd_main_window_new(GtkApplication* app);

G_END_DECLS

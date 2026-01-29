#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define KD_TYPE_DISPLAY_PAGE (kd_display_page_get_type())

G_DECLARE_FINAL_TYPE(KdDisplayPage, kd_display_page, KD, DISPLAY_PAGE, AdwBin)

GtkWidget* kd_display_page_new();

G_END_DECLS

#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define KD_TYPE_POWER_PAGE (kd_power_page_get_type())

G_DECLARE_FINAL_TYPE(KdPowerPage, kd_power_page, KD, POWER_PAGE, AdwBin)

GtkWidget* kd_power_page_new();

G_END_DECLS

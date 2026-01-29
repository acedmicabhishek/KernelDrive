#pragma once

#include <adwaita.h>

G_BEGIN_DECLS

#define KD_TYPE_INPUT_PAGE (kd_input_page_get_type())

G_DECLARE_FINAL_TYPE(KdInputPage, kd_input_page, KD, INPUT_PAGE, AdwBin)

GtkWidget* kd_input_page_new();

G_END_DECLS

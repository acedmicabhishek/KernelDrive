#pragma once
#include <adwaita.h>

G_BEGIN_DECLS

#define KD_TYPE_DEPENDENCIES_PAGE (kd_dependencies_page_get_type())
G_DECLARE_FINAL_TYPE(KdDependenciesPage, kd_dependencies_page, KD, DEPENDENCIES_PAGE, AdwBin)

GtkWidget* kd_dependencies_page_new();

G_END_DECLS

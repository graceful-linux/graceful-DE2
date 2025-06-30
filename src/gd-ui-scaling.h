//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_UI_SCALING_H
#define graceful_DE2_GD_UI_SCALING_H
#include <glib-object.h>
#include "backends/gd-backend.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_UI_SCALING (gd_ui_scaling_get_type ())
G_DECLARE_FINAL_TYPE (GDUiScaling, gd_ui_scaling, GD, UI_SCALING, GObject)

GDUiScaling* gd_ui_scaling_new (GDBackend *backend);

C_END_EXTERN_C

#endif // graceful_DE2_GD_UI_SCALING_H

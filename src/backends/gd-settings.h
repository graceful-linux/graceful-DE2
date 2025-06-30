//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_SETTINGS_H
#define graceful_DE2_GD_SETTINGS_H
#include <glib-object.h>

#include "macros/macros.h"


C_BEGIN_EXTERN_C

typedef struct _GDSettings GDSettings;

int gd_settings_get_ui_scaling_factor (GDSettings *settings);

C_END_EXTERN_C

#endif // graceful_DE2_GD_SETTINGS_H

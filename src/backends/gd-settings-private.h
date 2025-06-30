//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_SETTINGS_PRIVATE_H
#define graceful_DE2_GD_SETTINGS_PRIVATE_H
#include "gd-backend.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_SETTINGS (gd_settings_get_type ())
G_DECLARE_FINAL_TYPE (GDSettings, gd_settings, GD, SETTINGS, GObject)

GDSettings* gd_settings_new                       (GDBackend* backend);
void        gd_settings_post_init                 (GDSettings* settings);
bool        gd_settings_get_global_scaling_factor (GDSettings* settings, gint* globalScalingFactor);

C_END_EXTERN_C

#endif // graceful_DE2_GD_SETTINGS_PRIVATE_H

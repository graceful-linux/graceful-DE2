//
// Created by dingjing on 25-7-3.
//

#ifndef graceful_DE2_GD_ROOT_BACKGROUND_H
#define graceful_DE2_GD_ROOT_BACKGROUND_H
#include "macros/macros.h"

#include <glib-object.h>

C_BEGIN_EXTERN_C

#define GD_TYPE_ROOT_BACKGROUND (gd_root_background_get_type ())
G_DECLARE_FINAL_TYPE (GDRootBackground, gd_root_background, GD, ROOT_BACKGROUND, GObject)

GDRootBackground *gd_root_background_new (void);

C_END_EXTERN_C

#endif // graceful_DE2_GD_ROOT_BACKGROUND_H

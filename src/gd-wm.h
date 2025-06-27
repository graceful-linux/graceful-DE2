//
// Created by dingjing on 25-6-27.
//

#ifndef graceful_DE2_GD_WM_H
#define graceful_DE2_GD_WM_H
#include "macros/macros.h"
#include <glib-object.h>

#define GD_TYPE_WM              (gd_wm_get_type())
G_DECLARE_FINAL_TYPE(GDWm, gd_wm, GD, WM, GObject)

C_BEGIN_EXTERN_C

GDWm*       gd_wm_new   (void);

C_END_EXTERN_C

#endif // graceful_DE2_GD_WM_H

//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_BACKGROUND_H
#define graceful_DE2_GD_BACKGROUND_H
#include "macros/macros.h"

#include <gtk/gtk.h>

C_BEGIN_EXTERN_C

#define GD_TYPE_BACKGROUND (gd_background_get_type ())
G_DECLARE_FINAL_TYPE (GDBackground, gd_background, GD, BACKGROUND, GObject)

GDBackground *gd_background_new               (GtkWidget    *window);

GdkRGBA      *gd_background_get_average_color (GDBackground *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_BACKGROUND_H

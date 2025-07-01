//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_DUMMY_ICON_H
#define graceful_DE2_GD_DUMMY_ICON_H
#include "macros/macros.h"

#include "gd-icon.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_DUMMY_ICON (gd_dummy_icon_get_type ())
G_DECLARE_FINAL_TYPE (GDDummyIcon, gd_dummy_icon, GD, DUMMY_ICON, GDIcon)

GtkWidget *gd_dummy_icon_new        (GDIconView  *icon_view);

int        gd_dummy_icon_get_width  (GDDummyIcon *self);

int        gd_dummy_icon_get_height (GDDummyIcon *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_DUMMY_ICON_H

//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_HOME_ICON_H
#define graceful_DE2_GD_HOME_ICON_H
#include "macros/macros.h"
#include "gd-icon.h"

C_BEGIN_EXTERN_C


#define GD_TYPE_HOME_ICON (gd_home_icon_get_type ())
G_DECLARE_FINAL_TYPE (GDHomeIcon, gd_home_icon, GD, HOME_ICON, GDIcon)

GtkWidget *gd_home_icon_new (GDIconView  *icon_view, GError** error);

C_END_EXTERN_C

#endif // graceful_DE2_GD_HOME_ICON_H

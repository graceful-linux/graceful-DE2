//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_TRASH_ICON_H
#define graceful_DE2_GD_TRASH_ICON_H
#include "gd-icon.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_TRASH_ICON (gd_trash_icon_get_type ())
G_DECLARE_FINAL_TYPE (GDTrashIcon, gd_trash_icon, GD, TRASH_ICON, GDIcon)

GtkWidget *gd_trash_icon_new      (GDIconView* iconView, GError** error);

bool   gd_trash_icon_is_empty (GDTrashIcon  *self);


C_END_EXTERN_C

#endif // graceful_DE2_GD_TRASH_ICON_H

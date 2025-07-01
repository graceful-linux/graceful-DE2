//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_DESKTOP_ENUM_TYPES_H
#define graceful_DE2_GD_DESKTOP_ENUM_TYPES_H
#include "macros/macros.h"

#include <glib-object.h>

C_BEGIN_EXTERN_C

GType gd_icon_size_get_type (void) G_GNUC_CONST;
#define GD_TYPE_ICON_SIZE (gd_icon_size_get_type())
GType gd_placement_get_type (void) G_GNUC_CONST;
#define GD_TYPE_PLACEMENT (gd_placement_get_type())
GType gd_sort_by_get_type (void) G_GNUC_CONST;
#define GD_TYPE_SORT_BY (gd_sort_by_get_type())

C_END_EXTERN_C

#endif // graceful_DE2_GD_DESKTOP_ENUM_TYPES_H

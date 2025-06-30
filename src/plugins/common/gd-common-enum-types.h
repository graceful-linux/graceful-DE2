//
// Created by dingjing on 25-6-30.
//

#ifndef graceful_DE2_GD_COMMON_ENUM_TYPES_H
#define graceful_DE2_GD_COMMON_ENUM_TYPES_H
#include "macros/macros.h"
#include <glib-object.h>

C_BEGIN_EXTERN_C

GType gd_keybinding_type_get_type (void) G_GNUC_CONST;
#define GD_TYPE_KEYBINDING_TYPE (gd_keybinding_type_get_type())

C_BEGIN_EXTERN_C

#endif // graceful_DE2_GD_COMMON_ENUM_TYPES_H

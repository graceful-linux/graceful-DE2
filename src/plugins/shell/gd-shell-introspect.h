//
// Created by dingjing on 25-7-3.
//

#ifndef graceful_DE2_GD_SHELL_INTROSPECT_H
#define graceful_DE2_GD_SHELL_INTROSPECT_H
#include "macros/macros.h"

#include <glib-object.h>


C_BEGIN_EXTERN_C


#define GD_TYPE_SHELL_INTROSPECT (gd_shell_introspect_get_type ())
G_DECLARE_FINAL_TYPE (GDShellIntrospect, gd_shell_introspect, GD, SHELL_INTROSPECT, GObject)

GDShellIntrospect *gd_shell_introspect_new (void);

C_END_EXTERN_C

#endif // graceful_DE2_GD_SHELL_INTROSPECT_H

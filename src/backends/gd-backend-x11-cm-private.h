//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_BACKEND_X11_CM_PRIVATE_H
#define graceful_DE2_GD_BACKEND_X11_CM_PRIVATE_H
#include "gd-backend-x11-private.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_BACKEND_X11_CM  (gd_backend_x11_cm_get_type ())
G_DECLARE_FINAL_TYPE (GDBackendX11Cm, gd_backend_x11_cm, GD, BACKEND_X11_CM, GDBackendX11)

C_END_EXTERN_C

#endif // graceful_DE2_GD_BACKEND_X11_CM_PRIVATE_H

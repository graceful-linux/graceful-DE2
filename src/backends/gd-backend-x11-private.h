//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_BACKEND_X11_PRIVATE_H
#define graceful_DE2_GD_BACKEND_X11_PRIVATE_H
#include <X11/Xlib.h>

#include "macros/macros.h"
#include "gd-backend-private.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_BACKEND_X11 (gd_backend_x11_get_type ())
G_DECLARE_DERIVABLE_TYPE (GDBackendX11, gd_backend_x11, GD, BACKEND_X11, GDBackend)

struct _GDBackendX11Class
{
    GDBackendClass  parentClass;
    bool            (*handle_host_xevent) (GDBackendX11* x11, XEvent* event);
};

Display* gd_backend_x11_get_xdisplay (GDBackendX11 *x11);

C_END_EXTERN_C

#endif // graceful_DE2_GD_BACKEND_X11_PRIVATE_H

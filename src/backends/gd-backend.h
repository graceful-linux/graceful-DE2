//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_BACKEND_H
#define graceful_DE2_GD_BACKEND_H
#include "macros/macros.h"

#include "gd-settings.h"
#include "gd-monitor-manager.h"


C_BEGIN_EXTERN_C

#define GD_TYPE_BACKEND (gd_backend_get_type ())
G_DECLARE_DERIVABLE_TYPE (GDBackend, gd_backend, GD, BACKEND, GObject)

typedef enum
{
    GD_BACKEND_TYPE_X11_CM
} GDBackendType;

GDBackend*          gd_backend_new                  (GDBackendType type);
GDMonitorManager*   gd_backend_get_monitor_manager  (GDBackend* backend);
GDSettings*         gd_backend_get_settings         (GDBackend* backend);


C_END_EXTERN_C

#endif // graceful_DE2_GD_BACKEND_H

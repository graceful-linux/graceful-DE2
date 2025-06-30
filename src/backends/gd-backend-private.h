//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_BACKEND_PRIVATE_H
#define graceful_DE2_GD_BACKEND_PRIVATE_H
#include "gd-backend.h"

#include <glib.h>

#include "macros/macros.h"
#include "gd-gpu-private.h"
#include "gd-orientation-manager-private.h"


C_BEGIN_EXTERN_C

struct _GDBackendClass
{
    GObjectClass        parentClass;
    void                (*post_init)                        (GDBackend* backend);
    GDMonitorManager*   (*create_monitor_manager)           (GDBackend* backend, GError** error);
    bool                (*is_lid_closed)                    (GDBackend* self);
};

GDOrientationManager*   gd_backend_get_orientation_manager  (GDBackend* backend);
void                    gd_backend_monitors_changed         (GDBackend* backend);
bool                    gd_backend_is_lid_closed            (GDBackend* self);
void                    gd_backend_add_gpu                  (GDBackend* self, GDGpu* gpu);
GList*                  gd_backend_get_gpus                 (GDBackend* self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_BACKEND_PRIVATE_H

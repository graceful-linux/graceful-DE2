//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_GPU_PRIVATE_H
#define graceful_DE2_GD_GPU_PRIVATE_H
#include <glib.h>

#include "gd-backend.h"
#include "macros/macros.h"
#include "gd-monitor-manager-private.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_GPU (gd_gpu_get_type ())
G_DECLARE_DERIVABLE_TYPE (GDGpu, gd_gpu, GD, GPU, GObject)

struct _GDGpuClass
{
    GObjectClass parent_class;
    bool (*read_current) (GDGpu* gpu, GError **error);
};

bool        gd_gpu_read_current            (GDGpu* self, GError **error);
bool        gd_gpu_has_hotplug_mode_update (GDGpu* self);
GDBackend*  gd_gpu_get_backend             (GDGpu* self);
GList*      gd_gpu_get_outputs             (GDGpu* self);
GList*      gd_gpu_get_crtcs               (GDGpu* self);
GList*      gd_gpu_get_modes               (GDGpu* self);
void        gd_gpu_take_outputs            (GDGpu* self, GList* outputs);
void        gd_gpu_take_crtcs              (GDGpu* self, GList* crtcs);
void        gd_gpu_take_modes              (GDGpu* self, GList* modes);

C_END_EXTERN_C

#endif // graceful_DE2_GD_GPU_PRIVATE_H

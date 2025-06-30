//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_GPU_XRANDR_PRIVATE_H
#define graceful_DE2_GD_GPU_XRANDR_PRIVATE_H
#include <glib-object.h>
#include <X11/extensions/Xrandr.h>

#include "macros/macros.h"
#include "gd-gpu-private.h"
#include "gd-backend-x11-private.h"


C_BEGIN_EXTERN_C

#define GD_TYPE_GPU_XRANDR (gd_gpu_xrandr_get_type ())
G_DECLARE_FINAL_TYPE (GDGpuXrandr, gd_gpu_xrandr, GD, GPU_XRANDR, GDGpu)

GDGpuXrandr*        gd_gpu_xrandr_new                 (GDBackendX11* backendX11);
XRRScreenResources* gd_gpu_xrandr_get_resources       (GDGpuXrandr* self);
void                gd_gpu_xrandr_get_max_screen_size (GDGpuXrandr* self, int* maxWidth, int* maxHeight);

C_END_EXTERN_C

#endif // graceful_DE2_GD_GPU_XRANDR_PRIVATE_H

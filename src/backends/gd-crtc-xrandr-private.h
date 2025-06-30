//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_CRTC_XRANDR_PRIVATE_H
#define graceful_DE2_GD_CRTC_XRANDR_PRIVATE_H
#include <xcb/randr.h>
#include <X11/extensions/Xrandr.h>

#include "macros/macros.h"
#include "gd-crtc-private.h"
#include "gd-gpu-xrandr-private.h"


C_BEGIN_EXTERN_C


#define GD_TYPE_CRTC_XRANDR (gd_crtc_xrandr_get_type ())
G_DECLARE_FINAL_TYPE (GDCrtcXrandr, gd_crtc_xrandr, GD, CRTC_XRANDR, GDCrtc)

GDCrtcXrandr *gd_crtc_xrandr_new                   (GDGpuXrandr          *gpu_xrandr,
                                                    XRRCrtcInfo          *xrandr_crtc,
                                                    RRCrtc                crtc_id,
                                                    XRRScreenResources   *resources);

bool         gd_crtc_xrandr_set_config            (GDCrtcXrandr         *self,
                                                    xcb_randr_crtc_t      xrandr_crtc,
                                                    xcb_timestamp_t       timestamp,
                                                    int                   x,
                                                    int                   y,
                                                    xcb_randr_mode_t      mode,
                                                    xcb_randr_rotation_t  rotation,
                                                    xcb_randr_output_t   *outputs,
                                                    int                   n_outputs,
                                                    xcb_timestamp_t      *out_timestamp);

bool          gd_crtc_xrandr_is_assignment_changed (GDCrtcXrandr         *self,
                                                    GDCrtcAssignment     *crtc_assignment);

GDCrtcMode   *gd_crtc_xrandr_get_current_mode      (GDCrtcXrandr         *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_CRTC_XRANDR_PRIVATE_H

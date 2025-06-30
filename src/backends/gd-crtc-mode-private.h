//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_CRTC_MODE_PRIVATE_H
#define graceful_DE2_GD_CRTC_MODE_PRIVATE_H
#include <glib.h>
#include <stdint.h>

#include "macros/macros.h"
#include "gd-crtc-mode-info-private.h"

C_BEGIN_EXTERN_C

#define GD_TYPE_CRTC_MODE (gd_crtc_mode_get_type ())
G_DECLARE_DERIVABLE_TYPE (GDCrtcMode, gd_crtc_mode, GD, CRTC_MODE, GObject)

struct _GDCrtcModeClass
{
    GObjectClass parentClass;
};

uint64_t                gd_crtc_mode_get_id   (GDCrtcMode *self);
const char*             gd_crtc_mode_get_name (GDCrtcMode *self);
const GDCrtcModeInfo*   gd_crtc_mode_get_info (GDCrtcMode *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_CRTC_MODE_PRIVATE_H

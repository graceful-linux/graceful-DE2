//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_CRTC_MODE_INFO_PRIVATE_H
#define graceful_DE2_GD_CRTC_MODE_INFO_PRIVATE_H
#include <glib-object.h>

#include "macros/macros.h"

C_BEGIN_EXTERN_C

typedef enum
{
    GD_CRTC_MODE_FLAG_NONE = 0,

    GD_CRTC_MODE_FLAG_PHSYNC = (1 << 0),
    GD_CRTC_MODE_FLAG_NHSYNC = (1 << 1),
    GD_CRTC_MODE_FLAG_PVSYNC = (1 << 2),
    GD_CRTC_MODE_FLAG_NVSYNC = (1 << 3),
    GD_CRTC_MODE_FLAG_INTERLACE = (1 << 4),
    GD_CRTC_MODE_FLAG_DBLSCAN = (1 << 5),
    GD_CRTC_MODE_FLAG_CSYNC = (1 << 6),
    GD_CRTC_MODE_FLAG_PCSYNC = (1 << 7),
    GD_CRTC_MODE_FLAG_NCSYNC = (1 << 8),
    GD_CRTC_MODE_FLAG_HSKEW = (1 << 9),
    GD_CRTC_MODE_FLAG_BCAST = (1 << 10),
    GD_CRTC_MODE_FLAG_PIXMUX = (1 << 11),
    GD_CRTC_MODE_FLAG_DBLCLK = (1 << 12),
    GD_CRTC_MODE_FLAG_CLKDIV2 = (1 << 13),

    GD_CRTC_MODE_FLAG_MASK = 0x3fff
  } GDCrtcModeFlag;

typedef struct
{
    grefcount      ref_count;

    int            width;
    int            height;
    float          refresh_rate;
    GDCrtcModeFlag flags;
} GDCrtcModeInfo;

#define GD_TYPE_CRTC_MODE_INFO (gd_crtc_mode_info_get_type ())

GType           gd_crtc_mode_info_get_type (void);
GDCrtcModeInfo* gd_crtc_mode_info_new      (void);
GDCrtcModeInfo* gd_crtc_mode_info_ref      (GDCrtcModeInfo *self);
void            gd_crtc_mode_info_unref    (GDCrtcModeInfo *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_CRTC_MODE_INFO_PRIVATE_H

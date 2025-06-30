//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_CRTC_PRIVATE_H
#define graceful_DE2_GD_CRTC_PRIVATE_H
#include <glib.h>
#include <stdint.h>

#include "gd-rectangle.h"
#include "macros/macros.h"
#include "gd-gpu-private.h"
#include "gd-monitor-manager-types-private.h"


C_BEGIN_EXTERN_C

typedef struct
{
    GDRectangle         layout;
    GDMonitorTransform  transform;

    GDCrtcMode         *mode;
} GDCrtcConfig;

typedef struct
{
    GDCrtc             *crtc;
    GDCrtcMode         *mode;
    GDRectangle         layout;
    GDMonitorTransform  transform;
    GPtrArray          *outputs;
} GDCrtcAssignment;

#define GD_TYPE_CRTC (gd_crtc_get_type ())
G_DECLARE_DERIVABLE_TYPE (GDCrtc, gd_crtc, GD, CRTC, GObject)

struct _GDCrtcClass
{
    GObjectClass parentClass;
};

uint64_t            gd_crtc_get_id             (GDCrtc             *self);
GDGpu              *gd_crtc_get_gpu            (GDCrtc             *self);
GDMonitorTransform  gd_crtc_get_all_transforms (GDCrtc             *self);
const GList        *gd_crtc_get_outputs        (GDCrtc             *self);
void                gd_crtc_assign_output      (GDCrtc             *self, GDOutput* output);
void                gd_crtc_unassign_output    (GDCrtc             *self, GDOutput* output);
void                gd_crtc_set_config         (GDCrtc             *self, GDRectangle* layout, GDCrtcMode* mode, GDMonitorTransform transform);
void                gd_crtc_unset_config       (GDCrtc             *self);
const GDCrtcConfig *gd_crtc_get_config         (GDCrtc             *self);

C_END_EXTERN_C

#endif // graceful_DE2_GD_CRTC_PRIVATE_H

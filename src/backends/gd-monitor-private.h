//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_PRIVATE_H
#define graceful_DE2_GD_MONITOR_PRIVATE_H
#include "macros/macros.h"

#include <glib-object.h>

#include "gd-backend.h"
#include "gd-rectangle.h"
#include "gd-monitor-transform.h"
#include "gd-crtc-mode-info-private.h"
#include "gd-monitor-manager-enums-private.h"
#include "gd-monitor-manager-types-private.h"


C_BEGIN_EXTERN_C

#define HANDLED_CRTC_MODE_FLAGS (GD_CRTC_MODE_FLAG_INTERLACE)


typedef struct _GDMonitorMode   GDMonitorMode;


typedef enum
{
    GD_MONITOR_SCALES_CONSTRAINT_NONE = 0,
    GD_MONITOR_SCALES_CONSTRAINT_NO_FRAC = (1 << 0)
} GDMonitorScalesConstraint;

typedef struct
{
    gint                        width;
    gint                        height;
    gfloat                      refreshRate;
    GDCrtcModeFlag              flags;
} GDMonitorModeSpec;

typedef struct
{
    GDOutput*                   output;
    GDCrtcMode*                 crtcMode;
} GDMonitorCrtcMode;

struct _GDMonitorMode
{
    GDMonitor*                  monitor;
    gchar*                      id;
    GDMonitorModeSpec           spec;
    GDMonitorCrtcMode*          crtcModes;
};

typedef bool (*GDMonitorModeFunc) (GDMonitor* monitor, GDMonitorMode* mode, GDMonitorCrtcMode* monitorCrtcMode, void* uData, GError** error);

#define GD_TYPE_MONITOR (gd_monitor_get_type ())
G_DECLARE_DERIVABLE_TYPE (GDMonitor, gd_monitor, GD, MONITOR, GObject)

struct _GDMonitorClass
{
    GObjectClass parent_class;

    GDOutput*   (*get_main_output)        (GDMonitor* monitor);
    void        (*derive_layout)          (GDMonitor* monitor, GDRectangle* layout);
    void        (*calculate_crtc_pos)     (GDMonitor* monitor, GDMonitorMode* monitorMode, GDOutput* output, GDMonitorTransform crtcTransform, gint* outX, gint* outY);
    bool        (*get_suggested_position) (GDMonitor* monitor, gint* width, gint* height);
};

GDBackend*          gd_monitor_get_backend                (GDMonitor* self);
void                gd_monitor_make_display_name          (GDMonitor* monitor);
const char*         gd_monitor_get_display_name           (GDMonitor* monitor);
bool                gd_monitor_is_mode_assigned           (GDMonitor* monitor, GDMonitorMode* mode);
void                gd_monitor_append_output              (GDMonitor* monitor, GDOutput* output);
void                gd_monitor_set_winsys_id              (GDMonitor* monitor, glong winsysId);
void                gd_monitor_set_preferred_mode         (GDMonitor* monitor, GDMonitorMode* mode);
GDMonitorModeSpec   gd_monitor_create_spec                (GDMonitor* monitor, int width, int height, GDCrtcMode* crtcMode);
const GDOutputInfo* gd_monitor_get_main_output_info       (GDMonitor* self);
void                gd_monitor_generate_spec              (GDMonitor* monitor);
bool                gd_monitor_add_mode                   (GDMonitor* monitor, GDMonitorMode* monitorMode, bool replace);
void                gd_monitor_mode_free                  (GDMonitorMode* monitorMode);
gchar*              gd_monitor_mode_spec_generate_id      (GDMonitorModeSpec* spec);
GDMonitorSpec*      gd_monitor_get_spec                   (GDMonitor* monitor);
bool                gd_monitor_is_active                  (GDMonitor* monitor);
GDOutput*           gd_monitor_get_main_output            (GDMonitor* monitor);
bool                gd_monitor_is_primary                 (GDMonitor* monitor);
bool                gd_monitor_supports_underscanning     (GDMonitor* monitor);
bool                gd_monitor_is_underscanning           (GDMonitor* monitor);
bool                gd_monitor_get_max_bpc                (GDMonitor* self, unsigned int *maxBpc);
bool                gd_monitor_is_laptop_panel            (GDMonitor* monitor);
bool                gd_monitor_is_same_as                 (GDMonitor* monitor, GDMonitor* otherMonitor);
GList*              gd_monitor_get_outputs                (GDMonitor* monitor);
void                gd_monitor_get_current_resolution     (GDMonitor* monitor, int* width, int* height);
void                gd_monitor_derive_layout              (GDMonitor* monitor, GDRectangle* layout);
void                gd_monitor_get_physical_dimensions    (GDMonitor* monitor, gint* widthMM, gint* heightMM);
const gchar*        gd_monitor_get_connector              (GDMonitor* monitor);
const gchar*        gd_monitor_get_vendor                 (GDMonitor* monitor);
const gchar*        gd_monitor_get_product                (GDMonitor* monitor);
const gchar*        gd_monitor_get_serial                 (GDMonitor* monitor);
GDConnectorType     gd_monitor_get_connector_type         (GDMonitor* monitor);
GDMonitorTransform  gd_monitor_logical_to_crtc_transform  (GDMonitor* monitor, GDMonitorTransform transform);
GDMonitorTransform  gd_monitor_crtc_to_logical_transform  (GDMonitor* monitor, GDMonitorTransform transform);
bool                gd_monitor_get_suggested_position     (GDMonitor* monitor, gint* x, gint* y);
GDLogicalMonitor*   gd_monitor_get_logical_monitor        (GDMonitor* monitor);
GDMonitorMode*      gd_monitor_get_mode_from_id           (GDMonitor* monitor, const gchar* monitorModeId);
bool                gd_monitor_mode_spec_has_similar_size (GDMonitorModeSpec* monitorModeSpec, GDMonitorModeSpec* otherMonitorModeSpec);
GDMonitorMode*      gd_monitor_get_mode_from_spec         (GDMonitor* monitor, GDMonitorModeSpec* monitorModeSpec);
GDMonitorMode*      gd_monitor_get_preferred_mode         (GDMonitor* monitor);
GDMonitorMode*      gd_monitor_get_current_mode           (GDMonitor* monitor);
void                gd_monitor_derive_current_mode        (GDMonitor* monitor);
void                gd_monitor_set_current_mode           (GDMonitor* monitor, GDMonitorMode* mode);
GList*              gd_monitor_get_modes                  (GDMonitor* monitor);
void                gd_monitor_calculate_crtc_pos         (GDMonitor* monitor, GDMonitorMode* monitorMode, GDOutput* output, GDMonitorTransform crtcTransform, gint* outX, gint* outY);
gfloat              gd_monitor_calculate_mode_scale       (GDMonitor* monitor, GDMonitorMode* monitorMode, GDMonitorScalesConstraint constraints);
gfloat*             gd_monitor_calculate_supported_scales (GDMonitor* monitor, GDMonitorMode* monitorMode, GDMonitorScalesConstraint constraints, gint* nSupportedScales);
float  gd_get_closest_monitor_scale_factor_for_resolution (float width, float height, float scale, float threshold);
const gchar*        gd_monitor_mode_get_id                (GDMonitorMode* monitorMode);
GDMonitorModeSpec*  gd_monitor_mode_get_spec              (GDMonitorMode* monitorMode);
void                gd_monitor_mode_get_resolution        (GDMonitorMode* monitorMode, gint* width, gint* height);
gfloat              gd_monitor_mode_get_refresh_rate      (GDMonitorMode* monitorMode);
GDCrtcModeFlag      gd_monitor_mode_get_flags             (GDMonitorMode* monitorMode);
bool                gd_monitor_mode_foreach_crtc          (GDMonitor* monitor, GDMonitorMode* mode, GDMonitorModeFunc func, void* uData, GError** error);
bool                gd_monitor_mode_foreach_output        (GDMonitor* monitor, GDMonitorMode* mode, GDMonitorModeFunc func, void* uData, GError** error);
bool                gd_monitor_mode_should_be_advertised  (GDMonitorMode* monitorMode);
bool                gd_verify_monitor_mode_spec           (GDMonitorModeSpec* modeSpec, GError** error);
bool                gd_monitor_has_aspect_as_size         (GDMonitor* monitor);
void                gd_monitor_set_logical_monitor        (GDMonitor* monitor, GDLogicalMonitor* logicalMonitor);
bool                gd_monitor_get_backlight_info         (GDMonitor* self, int* backlightMin, int* backlightMax);
void                gd_monitor_set_backlight              (GDMonitor* self, int value);
bool                gd_monitor_get_backlight              (GDMonitor* self, int* value);

C_END_EXTERN_C

#endif // graceful_DE2_GD_MONITOR_PRIVATE_H

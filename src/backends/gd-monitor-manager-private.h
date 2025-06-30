//
// Created by dingjing on 25-6-28.
//

#ifndef graceful_DE2_GD_MONITOR_MANAGER_PRIVATE_H
#define graceful_DE2_GD_MONITOR_MANAGER_PRIVATE_H
#include "macros/macros.h"

#include "gd-monitor-private.h"
#include "gd-monitor-transform.h"
#include "gd-display-config-shared.h"
#include "gd-monitor-manager-enums-private.h"
#include "gd-monitor-config-manager-private.h"


C_BEGIN_EXTERN_C

#define GD_TYPE_MONITOR_MANAGER         (gd_monitor_manager_get_type ())
#define GD_MONITOR_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GD_TYPE_MONITOR_MANAGER, GDMonitorManager))
#define GD_MONITOR_MANAGER_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c),    GD_TYPE_MONITOR_MANAGER, GDMonitorManagerClass))
#define GD_IS_MONITOR_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GD_TYPE_MONITOR_MANAGER))
#define GD_IS_MONITOR_MANAGER_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c),    GD_TYPE_MONITOR_MANAGER))
#define GD_MONITOR_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o),  GD_TYPE_MONITOR_MANAGER, GDMonitorManagerClass))

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDMonitorManager, g_object_unref)

struct _GDMonitorManager
{
  GObject                      parent;

  GDDBusDisplayConfig         *display_config;

  bool                     in_init;

  guint                        serial;

  GDLogicalMonitorLayoutMode   layout_mode;

  gint                         screen_width;
  gint                         screen_height;

  GList                       *monitors;

  GList                       *logical_monitors;
  GDLogicalMonitor            *primary_logical_monitor;

  bool                     panel_orientation_managed;

  GDMonitorConfigManager      *config_manager;

  // GnomePnpIds                 *pnp_ids;

  GDMonitorSwitchConfigType    current_switch_config;
};

typedef struct
{
  GObjectClass parent_class;

  GBytes                     * (* read_edid)                    (GDMonitorManager            *manager,
                                                                 GDOutput                    *output);

  void                         (* read_current_state)           (GDMonitorManager            *manager);

  void                         (* ensure_initial_config)        (GDMonitorManager            *manager);

  bool                     (* apply_monitors_config)        (GDMonitorManager            *manager,
                                                                 GDMonitorsConfig            *config,
                                                                 GDMonitorsConfigMethod       method,
                                                                 GError                     **error);

  void                         (* set_power_save_mode)          (GDMonitorManager            *manager,
                                                                 GDPowerSave                  mode);

  void                         (* get_crtc_gamma)               (GDMonitorManager            *manager,
                                                                 GDCrtc                      *crtc,
                                                                 gsize                       *size,
                                                                 gushort                    **red,
                                                                 gushort                    **green,
                                                                 gushort                    **blue);

  void                         (* set_crtc_gamma)               (GDMonitorManager            *manager,
                                                                 GDCrtc                      *crtc,
                                                                 gsize                        size,
                                                                 gushort                     *red,
                                                                 gushort                     *green,
                                                                 gushort                     *blue);

  void                         (* tiled_monitor_added)          (GDMonitorManager            *manager,
                                                                 GDMonitor                   *monitor);

  void                         (* tiled_monitor_removed)        (GDMonitorManager            *manager,
                                                                 GDMonitor                   *monitor);

  bool                     (* is_transform_handled)         (GDMonitorManager            *manager,
                                                                 GDCrtc                      *crtc,
                                                                 GDMonitorTransform           transform);

  gfloat                       (* calculate_monitor_mode_scale) (GDMonitorManager            *manager,
                                                                 GDLogicalMonitorLayoutMode   layout_mode,
                                                                 GDMonitor                   *monitor,
                                                                 GDMonitorMode               *monitor_mode);

  gfloat                     * (* calculate_supported_scales)   (GDMonitorManager            *manager,
                                                                 GDLogicalMonitorLayoutMode   layout_mode,
                                                                 GDMonitor                   *monitor,
                                                                 GDMonitorMode               *monitor_mode,
                                                                 gint                         *n_supported_scales);

  GDMonitorManagerCapability   (* get_capabilities)             (GDMonitorManager            *manager);

  bool                     (* get_max_screen_size)          (GDMonitorManager            *manager,
                                                                 gint                        *max_width,
                                                                 gint                        *max_height);

  GDLogicalMonitorLayoutMode   (* get_default_layout_mode)      (GDMonitorManager            *manager);

  void                         (* set_output_ctm)               (GDOutput                    *output,
                                                                 const GDOutputCtm           *ctm);
} GDMonitorManagerClass;

GType                       gd_monitor_manager_get_type                     (void);

GDBackend                  *gd_monitor_manager_get_backend                  (GDMonitorManager            *manager);

void                        gd_monitor_manager_setup                        (GDMonitorManager            *manager);

void                        gd_monitor_manager_rebuild_derived              (GDMonitorManager            *manager,
                                                                             GDMonitorsConfig            *config);

GList                      *gd_monitor_manager_get_logical_monitors         (GDMonitorManager            *manager);

GDMonitor                  *gd_monitor_manager_get_primary_monitor          (GDMonitorManager            *manager);

GDMonitor                  *gd_monitor_manager_get_laptop_panel             (GDMonitorManager            *manager);

GDMonitor                  *gd_monitor_manager_get_monitor_from_spec        (GDMonitorManager            *manager,
                                                                             GDMonitorSpec               *monitor_spec);

GList                      *gd_monitor_manager_get_monitors                 (GDMonitorManager            *manager);

GDLogicalMonitor           *gd_monitor_manager_get_primary_logical_monitor  (GDMonitorManager            *manager);

bool                    gd_monitor_manager_has_hotplug_mode_update      (GDMonitorManager            *manager);
void                        gd_monitor_manager_read_current_state           (GDMonitorManager            *manager);

void                        gd_monitor_manager_reconfigure                  (GDMonitorManager            *self);

bool                    gd_monitor_manager_get_monitor_matrix           (GDMonitorManager            *manager,
                                                                             GDMonitor                   *monitor,
                                                                             GDLogicalMonitor            *logical_monitor,
                                                                             gfloat                       matrix[6]);

void                        gd_monitor_manager_tiled_monitor_added          (GDMonitorManager            *manager,
                                                                             GDMonitor                   *monitor);

void                        gd_monitor_manager_tiled_monitor_removed        (GDMonitorManager            *manager,
                                                                             GDMonitor                   *monitor);

bool                    gd_monitor_manager_is_transform_handled         (GDMonitorManager            *manager,
                                                                             GDCrtc                      *crtc,
                                                                             GDMonitorTransform           transform);

GDMonitorsConfig           *gd_monitor_manager_ensure_configured            (GDMonitorManager            *manager);

void                        gd_monitor_manager_update_logical_state_derived (GDMonitorManager            *manager,
                                                                             GDMonitorsConfig            *config);

gfloat                      gd_monitor_manager_calculate_monitor_mode_scale (GDMonitorManager            *manager,
                                                                             GDLogicalMonitorLayoutMode   layout_mode,
                                                                             GDMonitor                   *monitor,
                                                                             GDMonitorMode               *monitor_mode);

gfloat                     *gd_monitor_manager_calculate_supported_scales   (GDMonitorManager            *manager,
                                                                             GDLogicalMonitorLayoutMode   layout_mode,
                                                                             GDMonitor                   *monitor,
                                                                             GDMonitorMode               *monitor_mode,
                                                                             gint                        *n_supported_scales);

bool                    gd_monitor_manager_is_scale_supported           (GDMonitorManager            *manager,
                                                                             GDLogicalMonitorLayoutMode   layout_mode,
                                                                             GDMonitor                   *monitor,
                                                                             GDMonitorMode               *monitor_mode,
                                                                             gfloat                       scale);

GDMonitorManagerCapability  gd_monitor_manager_get_capabilities             (GDMonitorManager            *manager);

bool                    gd_monitor_manager_get_max_screen_size          (GDMonitorManager            *manager,
                                                                             gint                        *max_width,
                                                                             gint                        *max_height);

GDLogicalMonitorLayoutMode  gd_monitor_manager_get_default_layout_mode      (GDMonitorManager            *manager);

GDMonitorConfigManager     *gd_monitor_manager_get_config_manager           (GDMonitorManager            *manager);

char                       *gd_monitor_manager_get_vendor_name              (GDMonitorManager            *manager,
                                                                             const char                  *vendor);

GDPowerSave                 gd_monitor_manager_get_power_save_mode          (GDMonitorManager            *manager);

void                        gd_monitor_manager_power_save_mode_changed      (GDMonitorManager            *manager,
                                                                             GDPowerSave                  mode);

bool                    gd_monitor_manager_is_monitor_visible           (GDMonitorManager            *manager,
                                                                             GDMonitor                   *monitor);


C_END_EXTERN_C


#endif // graceful_DE2_GD_MONITOR_MANAGER_PRIVATE_H

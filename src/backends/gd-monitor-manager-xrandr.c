//
// Created by dingjing on 25-6-28.
//
#include "gd-monitor-manager-xrandr-private.h"

#include <math.h>
#include <string.h>
#include <xcb/randr.h>
#include <X11/Xlibint.h>
#include <X11/Xlib-xcb.h>
#include <X11/extensions/dpms.h>

#include "gd-monitor-private.h"
#include "gd-crtc-mode-private.h"
#include "gd-gpu-xrandr-private.h"
#include "gd-backend-x11-private.h"
#include "gd-crtc-xrandr-private.h"
#include "gd-monitor-tiled-private.h"
#include "gd-output-xrandr-private.h"
#include "gd-monitor-config-manager-private.h"


#define DPI_FALLBACK 96.0

struct _GDMonitorManagerXrandr
{
    GDMonitorManager    parent;

    Display*            xdisplay;
    Window              xroot;

    gint                rr_event_base;
    gint                rr_error_base;

    bool                has_randr15;
    GHashTable*         tiled_monitor_atoms;

    Time                last_xrandr_set_timestamp;
};

typedef struct
{
    Atom xrandr_name;
} GDMonitorData;

G_DEFINE_TYPE(GDMonitorManagerXrandr, gd_monitor_manager_xrandr, GD_TYPE_MONITOR_MANAGER)static bool xrandr_set_crtc_config(GDMonitorManagerXrandr* xrandr, GDCrtc* crtc, bool save_timestamp, xcb_randr_crtc_t xrandr_crtc,
                                                                                                                                xcb_timestamp_t timestamp, gint x, gint y, xcb_randr_mode_t mode, xcb_randr_rotation_t rotation,
                                                                                                                                xcb_randr_output_t* outputs, gint n_outputs)
{
    xcb_timestamp_t new_timestamp;

    if (!gd_crtc_xrandr_set_config(GD_CRTC_XRANDR(crtc), xrandr_crtc, timestamp, x, y, mode, rotation, outputs, n_outputs, &new_timestamp)) return FALSE;

    if (save_timestamp) xrandr->last_xrandr_set_timestamp = new_timestamp;

    return TRUE;
}

static bool
is_crtc_assignment_changed(GDCrtc* crtc, GDCrtcAssignment** crtc_assignments, unsigned int n_crtc_assignments)
{
    guint i;

    for (i = 0; i < n_crtc_assignments; i++) {
        GDCrtcAssignment* crtc_assignment;

        crtc_assignment = crtc_assignments[i];

        if (crtc_assignment->crtc != crtc) continue;

        return gd_crtc_xrandr_is_assignment_changed(GD_CRTC_XRANDR(crtc), crtc_assignment);
    }

    return !!gd_crtc_xrandr_get_current_mode(GD_CRTC_XRANDR(crtc));
}

static bool
is_output_assignment_changed(GDOutput* output, GDCrtcAssignment** crtc_assignments, guint n_crtc_assignments, GDOutputAssignment** output_assignments, guint n_output_assignments)
{
    bool output_is_found;
    GDCrtc* assigned_crtc;
    guint i;

    output_is_found = FALSE;

    for (i = 0; i < n_output_assignments; i++) {
        GDOutputAssignment* output_assignment;
        unsigned int max_bpc;

        output_assignment = output_assignments[i];

        if (output_assignment->output != output) continue;

        if (gd_output_is_primary(output) != output_assignment->is_primary) return TRUE;

        if (gd_output_is_presentation(output) != output_assignment->is_presentation) return TRUE;

        if (gd_output_is_underscanning(output) != output_assignment->is_underscanning) return TRUE;

        if (gd_output_get_max_bpc(output, &max_bpc)) {
            if (!output_assignment->has_max_bpc || max_bpc != output_assignment->max_bpc) return TRUE;
        }
        else if (output_assignment->has_max_bpc) {
            return TRUE;
        }

        output_is_found = TRUE;
    }

    assigned_crtc = gd_output_get_assigned_crtc(output);

    if (!output_is_found) return assigned_crtc != NULL;

    for (i = 0; i < n_crtc_assignments; i++) {
        GDCrtcAssignment* crtc_assignment;
        guint j;

        crtc_assignment = crtc_assignments[i];

        for (j = 0; j < crtc_assignment->outputs->len; j++) {
            GDOutput* crtc_assignment_output;

            crtc_assignment_output = ((GDOutput**)crtc_assignment->outputs->pdata)[j];

            if (crtc_assignment_output == output && crtc_assignment->crtc == assigned_crtc) return FALSE;
        }
    }

    return TRUE;
}

static GDGpu*
get_gpu(GDMonitorManagerXrandr* self)
{
    GDMonitorManager* manager;
    GDBackend* backend;

    manager = GD_MONITOR_MANAGER(self);
    backend = gd_monitor_manager_get_backend(manager);

    return GD_GPU(gd_backend_get_gpus(backend)->data);
}

static bool
is_assignments_changed(GDMonitorManager* manager, GDCrtcAssignment** crtc_assignments, guint n_crtc_assignments, GDOutputAssignment** output_assignments, guint n_output_assignments)
{
    GDMonitorManagerXrandr* manager_xrandr;
    GDGpu* gpu;
    GList* l;

    manager_xrandr = GD_MONITOR_MANAGER_XRANDR(manager);
    gpu = get_gpu(manager_xrandr);

    for (l = gd_gpu_get_crtcs(gpu); l; l = l->next) {
        GDCrtc* crtc = l->data;

        if (is_crtc_assignment_changed(crtc, crtc_assignments, n_crtc_assignments)) return TRUE;
    }

    for (l = gd_gpu_get_outputs(gpu); l; l = l->next) {
        GDOutput* output = l->data;

        if (is_output_assignment_changed(output, crtc_assignments, n_crtc_assignments, output_assignments, n_output_assignments)) return TRUE;
    }

    return FALSE;
}

static xcb_randr_rotation_t
gd_monitor_transform_to_xrandr(GDMonitorTransform transform)
{
    xcb_randr_rotation_t rotation;

    rotation = XCB_RANDR_ROTATION_ROTATE_0;

    switch (transform) {
    case GD_MONITOR_TRANSFORM_NORMAL:
        rotation = XCB_RANDR_ROTATION_ROTATE_0;
        break;

    case GD_MONITOR_TRANSFORM_90:
        rotation = XCB_RANDR_ROTATION_ROTATE_90;
        break;

    case GD_MONITOR_TRANSFORM_180:
        rotation = XCB_RANDR_ROTATION_ROTATE_180;
        break;

    case GD_MONITOR_TRANSFORM_270:
        rotation = XCB_RANDR_ROTATION_ROTATE_270;
        break;

    case GD_MONITOR_TRANSFORM_FLIPPED:
        rotation = XCB_RANDR_ROTATION_REFLECT_X | XCB_RANDR_ROTATION_ROTATE_0;
        break;

    case GD_MONITOR_TRANSFORM_FLIPPED_90:
        rotation = XCB_RANDR_ROTATION_REFLECT_X | XCB_RANDR_ROTATION_ROTATE_90;
        break;

    case GD_MONITOR_TRANSFORM_FLIPPED_180:
        rotation = XCB_RANDR_ROTATION_REFLECT_X | XCB_RANDR_ROTATION_ROTATE_180;
        break;

    case GD_MONITOR_TRANSFORM_FLIPPED_270:
        rotation = XCB_RANDR_ROTATION_REFLECT_X | XCB_RANDR_ROTATION_ROTATE_270;
        break;

    default:
        g_assert_not_reached();
        break;
    }

    return rotation;
}

static void
apply_crtc_assignments(GDMonitorManager* manager, bool save_timestamp, GDCrtcAssignment** crtcs, guint n_crtcs, GDOutputAssignment** outputs, guint n_outputs)
{
    GDMonitorManagerXrandr* xrandr;
    GDGpu* gpu;
    GList* to_configure_outputs;
    GList* to_disable_crtcs;
    gint width, height, width_mm, height_mm;
    guint i;
    GList* l;

    xrandr = GD_MONITOR_MANAGER_XRANDR(manager);
    gpu = get_gpu(xrandr);

    to_configure_outputs = g_list_copy(gd_gpu_get_outputs(gpu));
    to_disable_crtcs = g_list_copy(gd_gpu_get_crtcs(gpu));

    XGrabServer(xrandr->xdisplay);

    /* First compute the new size of the screen (framebuffer) */
    width = 0;
    height = 0;
    for (i = 0; i < n_crtcs; i++) {
        GDCrtcAssignment* crtc_assignment = crtcs[i];
        GDCrtc* crtc = crtc_assignment->crtc;

        if (crtc_assignment->mode == NULL) continue;

        to_disable_crtcs = g_list_remove(to_disable_crtcs, crtc);

        width = MAX(width, crtc_assignment->layout.x + crtc_assignment->layout.width);
        height = MAX(height, crtc_assignment->layout.y + crtc_assignment->layout.height);
    }

    /* Second disable all newly disabled CRTCs, or CRTCs that in the previous
     * configuration would be outside the new framebuffer (otherwise X complains
     * loudly when resizing)
     * CRTC will be enabled again after resizing the FB
     */
    for (i = 0; i < n_crtcs; i++) {
        GDCrtcAssignment* crtc_assignment = crtcs[i];
        GDCrtc* crtc = crtc_assignment->crtc;
        const GDCrtcConfig* crtc_config;
        int x2, y2;

        crtc_config = gd_crtc_get_config(crtc);
        if (crtc_config == NULL) continue;

        x2 = crtc_config->layout.x + crtc_config->layout.width;
        y2 = crtc_config->layout.y + crtc_config->layout.height;

        if (crtc_assignment->mode == NULL || x2 > width || y2 > height) {
            xrandr_set_crtc_config(xrandr, crtc, save_timestamp, (xcb_randr_crtc_t)gd_crtc_get_id(crtc), XCB_CURRENT_TIME, 0, 0, XCB_NONE, XCB_RANDR_ROTATION_ROTATE_0, NULL, 0);

            gd_crtc_unset_config(crtc);
        }
    }

    for (l = to_disable_crtcs; l; l = l->next) {
        GDCrtc* crtc = l->data;

        if (!gd_crtc_get_config(crtc)) continue;

        xrandr_set_crtc_config(xrandr, crtc, save_timestamp, (xcb_randr_crtc_t)gd_crtc_get_id(crtc), XCB_CURRENT_TIME, 0, 0, XCB_NONE, XCB_RANDR_ROTATION_ROTATE_0, NULL, 0);

        gd_crtc_unset_config(crtc);
    }

    if (n_crtcs == 0) goto out;

    g_assert(width > 0 && height > 0);
    /* The 'physical size' of an X screen is meaningless if that screen
     * can consist of many monitors. So just pick a size that make the
     * dpi 96.
     *
     * Firefox and Evince apparently believe what X tells them.
     */
    width_mm = (width / DPI_FALLBACK) * 25.4 + 0.5;
    height_mm = (height / DPI_FALLBACK) * 25.4 + 0.5;
    XRRSetScreenSize(xrandr->xdisplay, xrandr->xroot, width, height, width_mm, height_mm);

    for (i = 0; i < n_crtcs; i++) {
        GDCrtcAssignment* crtc_assignment = crtcs[i];
        GDCrtc* crtc = crtc_assignment->crtc;

        if (crtc_assignment->mode != NULL) {
            GDCrtcMode* crtc_mode;
            xcb_randr_output_t* output_ids;
            guint j, n_output_ids;
            xcb_randr_rotation_t rotation;
            xcb_randr_mode_t mode;

            crtc_mode = crtc_assignment->mode;

            n_output_ids = crtc_assignment->outputs->len;
            output_ids = g_new0(xcb_randr_output_t, n_output_ids);

            for (j = 0; j < n_output_ids; j++) {
                GDOutput* output;
                GDOutputAssignment* output_assignment;

                output = ((GDOutput**)crtc_assignment->outputs->pdata)[j];

                to_configure_outputs = g_list_remove(to_configure_outputs, output);

                output_assignment = gd_find_output_assignment(outputs, n_outputs, output);
                gd_output_assign_crtc(output, crtc, output_assignment);

                output_ids[j] = gd_output_get_id(output);
            }

            rotation = gd_monitor_transform_to_xrandr(crtc_assignment->transform);
            mode = gd_crtc_mode_get_id(crtc_mode);

            if (!xrandr_set_crtc_config(xrandr, crtc, save_timestamp, (xcb_randr_crtc_t)gd_crtc_get_id(crtc), XCB_CURRENT_TIME, crtc_assignment->layout.x, crtc_assignment->layout.y, mode, rotation, output_ids, n_output_ids)) {
                const GDCrtcModeInfo* crtc_mode_info;

                crtc_mode_info = gd_crtc_mode_get_info(crtc_mode);

                g_warning("Configuring CRTC %d with mode %d (%d x %d @ %f) at position %d, %d and transform %u failed\n", (unsigned)gd_crtc_get_id(crtc), (unsigned)mode, crtc_mode_info->width, crtc_mode_info->height,
                          (double)crtc_mode_info->refresh_rate, crtc_assignment->layout.x, crtc_assignment->layout.y, crtc_assignment->transform);

                g_free(output_ids);
                continue;
            }

            gd_crtc_set_config(crtc, &crtc_assignment->layout, crtc_mode, crtc_assignment->transform);

            g_free(output_ids);
        }
    }

    for (i = 0; i < n_outputs; i++) {
        GDOutputAssignment* output_assignment = outputs[i];
        GDOutput* output = output_assignment->output;

        gd_output_xrandr_apply_mode(GD_OUTPUT_XRANDR(output));
    }

    for (l = to_configure_outputs; l; l = l->next) {
        GDOutput* output = l->data;

        gd_output_unassign_crtc(output);
    }

out:
    XUngrabServer(xrandr->xdisplay);
    XFlush(xrandr->xdisplay);

    g_clear_pointer(&to_configure_outputs, g_list_free);
    g_clear_pointer(&to_disable_crtcs, g_list_free);
}

static GQuark
gd_monitor_data_quark(void)
{
    static GQuark quark;

    if (G_UNLIKELY(quark == 0)) quark = g_quark_from_static_string("gf-monitor-data-quark");

    return quark;
}

static GDMonitorData*
data_from_monitor(GDMonitor* monitor)
{
    GDMonitorData* data;
    GQuark quark;

    quark = gd_monitor_data_quark();
    data = g_object_get_qdata(G_OBJECT(monitor), quark);

    if (data) return data;

    data = g_new0(GDMonitorData, 1);
    g_object_set_qdata_full(G_OBJECT(monitor), quark, data, g_free);

    return data;
}

static void
increase_monitor_count(GDMonitorManagerXrandr* xrandr, Atom name_atom)
{
    GHashTable* atoms;
    gpointer key;
    gint count;

    atoms = xrandr->tiled_monitor_atoms;
    key = GSIZE_TO_POINTER(name_atom);

    count = GPOINTER_TO_INT(g_hash_table_lookup(atoms, key));
    count++;

    g_hash_table_insert(atoms, key, GINT_TO_POINTER(count));
}

static gint decrease_monitor_count(GDMonitorManagerXrandr* xrandr, Atom nameAtom)
{
    GHashTable* atoms = xrandr->tiled_monitor_atoms;
    gpointer key = GSIZE_TO_POINTER(nameAtom);

    gint count = GPOINTER_TO_SIZE(g_hash_table_lookup(atoms, key));
    count--;

    g_hash_table_insert(atoms, key, GINT_TO_POINTER(count));

    return count;
}

static void init_monitors(GDMonitorManagerXrandr* xrandr)
{
    gint n = 0, i = 0;

    C_RETURN_IF_FAIL(xrandr->has_randr15);

    XRRMonitorInfo* m = XRRGetMonitors(xrandr->xdisplay, xrandr->xroot, FALSE, &n);

    if (n == -1) return;

    for (i = 0; i < n; i++) {
        if (m[i].noutput > 1) {
            XRRDeleteMonitor(xrandr->xdisplay, xrandr->xroot, m[i].name);
        }
    }
    XRRFreeMonitors(m);
}

static void gd_monitor_manager_xrandr_constructed(GObject* object)
{
    gint rrEventBase;
    gint rrErrorBase;

    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(object);
    GDBackend* backend = gd_monitor_manager_get_backend(GD_MONITOR_MANAGER(xrandr));

    xrandr->xdisplay = gd_backend_x11_get_xdisplay(GD_BACKEND_X11(backend));
    xrandr->xroot = DefaultRootWindow(xrandr->xdisplay);

    if (XRRQueryExtension(xrandr->xdisplay, &rrEventBase, &rrErrorBase)) {
        gint majorVersion;
        gint minorVersion;

        xrandr->rr_event_base = rrEventBase;
        xrandr->rr_error_base = rrErrorBase;

        XRRSelectInput(xrandr->xdisplay, xrandr->xroot, RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask | RROutputPropertyNotifyMask);
        XRRQueryVersion(xrandr->xdisplay, &majorVersion, &minorVersion);

        xrandr->has_randr15 = FALSE;
        if (majorVersion > 1 || (majorVersion == 1 && minorVersion >= 5)) {
            xrandr->has_randr15 = TRUE;
            xrandr->tiled_monitor_atoms = g_hash_table_new(NULL, NULL);
        }
        init_monitors(xrandr);
    }

    G_OBJECT_CLASS(gd_monitor_manager_xrandr_parent_class)->constructed(object);
}

static void gd_monitor_manager_xrandr_dispose(GObject* object)
{
    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(object);

    g_clear_pointer(&xrandr->tiled_monitor_atoms, g_hash_table_destroy);

    G_OBJECT_CLASS(gd_monitor_manager_xrandr_parent_class)->dispose(object);
}

static GBytes* gd_monitor_manager_xrandr_read_edid(GDMonitorManager* manager, GDOutput* output)
{
    return gd_output_xrandr_read_edid(GD_OUTPUT_XRANDR(output));
}

static void gd_monitor_manager_xrandr_read_current_state(GDMonitorManager* manager)
{
    CARD16 dpmsState;
    BOOL dpmsEnabled;
    GDPowerSave power_save_mode;

    GDMonitorManagerXrandr* self = GD_MONITOR_MANAGER_XRANDR(manager);

    if (DPMSCapable(self->xdisplay) && DPMSInfo(self->xdisplay, &dpmsState, &dpmsEnabled) && dpmsEnabled) {
        switch (dpmsState) {
        case DPMSModeOn:
            power_save_mode = GD_POWER_SAVE_ON;
            break;

        case DPMSModeStandby:
            power_save_mode = GD_POWER_SAVE_STANDBY;
            break;

        case DPMSModeSuspend:
            power_save_mode = GD_POWER_SAVE_SUSPEND;
            break;

        case DPMSModeOff:
            power_save_mode = GD_POWER_SAVE_OFF;
            break;

        default:
            power_save_mode = GD_POWER_SAVE_UNSUPPORTED;
            break;
        }
    }
    else {
        power_save_mode = GD_POWER_SAVE_UNSUPPORTED;
    }

    gd_monitor_manager_power_save_mode_changed(manager, power_save_mode);

    GDMonitorManagerClass* pc = GD_MONITOR_MANAGER_CLASS(gd_monitor_manager_xrandr_parent_class);
    pc->read_current_state(manager);
}

static void gd_monitor_manager_xrandr_ensure_initial_config(GDMonitorManager* manager)
{
    gd_monitor_manager_ensure_configured(manager);

    gd_monitor_manager_read_current_state(manager);

    GDMonitorConfigManager* cm = gd_monitor_manager_get_config_manager(manager);
    GDMonitorsConfig* config = gd_monitor_config_manager_get_current(cm);

    gd_monitor_manager_update_logical_state_derived(manager, config);
}

static bool gd_monitor_manager_xrandr_apply_monitors_config(GDMonitorManager* manager, GDMonitorsConfig* config, GDMonitorsConfigMethod method, GError** error)
{
    GPtrArray* crtcAssignments;
    GPtrArray* outputAssignments;

    if (!config) {
        if (!manager->in_init) {
            apply_crtc_assignments(manager, TRUE, NULL, 0, NULL, 0);
        }
        gd_monitor_manager_rebuild_derived(manager, NULL);
        return true;
    }

    C_RETURN_VAL_IF_FAIL(gd_monitor_config_manager_assign(manager, config, &crtcAssignments, &outputAssignments, error), false);

    if (method != GD_MONITORS_CONFIG_METHOD_VERIFY) {
        if (is_assignments_changed(manager, (GDCrtcAssignment**)crtcAssignments->pdata, crtcAssignments->len, (GDOutputAssignment**)outputAssignments->pdata, outputAssignments->len)) {
            apply_crtc_assignments(manager, TRUE, (GDCrtcAssignment**)crtcAssignments->pdata, crtcAssignments->len, (GDOutputAssignment**)outputAssignments->pdata, outputAssignments->len);
        }
        else {
            gd_monitor_manager_rebuild_derived(manager, config);
        }
    }

    g_ptr_array_free(crtcAssignments, TRUE);
    g_ptr_array_free(outputAssignments, TRUE);

    return TRUE;
}

static void gd_monitor_manager_xrandr_set_power_save_mode(GDMonitorManager* manager, GDPowerSave mode)
{
    CARD16 state;

    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(manager);

    switch (mode) {
    case GD_POWER_SAVE_ON:
        state = DPMSModeOn;
        break;

    case GD_POWER_SAVE_STANDBY:
        state = DPMSModeStandby;
        break;

    case GD_POWER_SAVE_SUSPEND:
        state = DPMSModeSuspend;
        break;

    case GD_POWER_SAVE_OFF:
        state = DPMSModeOff;
        break;

    case GD_POWER_SAVE_UNSUPPORTED: default:
        return;
    }

    DPMSForceLevel(xrandr->xdisplay, state);
    DPMSSetTimeouts(xrandr->xdisplay, 0, 0, 0);
}

static void gd_monitor_manager_xrandr_get_crtc_gamma(GDMonitorManager* manager, GDCrtc* crtc, gsize* size, gushort** red, gushort** green, gushort** blue)
{
    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(manager);
    XRRCrtcGamma* gamma = XRRGetCrtcGamma(xrandr->xdisplay, (XID)gd_crtc_get_id(crtc));

    if (size) *size = gamma->size;

    if (red) *red = g_memdup2(gamma->red, sizeof (gushort) * gamma->size);

    if (green) *green = g_memdup2(gamma->green, sizeof (gushort) * gamma->size);

    if (blue) *blue = g_memdup2(gamma->blue, sizeof (gushort) * gamma->size);

    XRRFreeGamma(gamma);
}

static void gd_monitor_manager_xrandr_set_crtc_gamma(GDMonitorManager* manager, GDCrtc* crtc, gsize size, gushort* red, gushort* green, gushort* blue)
{
    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(manager);

    XRRCrtcGamma* gamma = XRRAllocGamma(size);
    memcpy(gamma->red, red, sizeof (gushort) * size);
    memcpy(gamma->green, green, sizeof (gushort) * size);
    memcpy(gamma->blue, blue, sizeof (gushort) * size);

    XRRSetCrtcGamma(xrandr->xdisplay, (XID)gd_crtc_get_id(crtc), gamma);
    XRRFreeGamma(gamma);
}

static void gd_monitor_manager_xrandr_tiled_monitor_added(GDMonitorManager* manager, GDMonitor* monitor)
{
    gchar* name;
    GList* l;
    gint i;

    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(manager);
    C_RETURN_IF_FAIL(xrandr->has_randr15);

    GDMonitorTiled* monitorTiled = GD_MONITOR_TILED(monitor);
    const gchar* product = gd_monitor_get_product(monitor);
    const uint32_t tileGroupId = gd_monitor_tiled_get_tile_group_id(monitorTiled);

    if (product) name = g_strdup_printf("%s-%d", product, tileGroupId);
    else name = g_strdup_printf("Tiled-%d", tileGroupId);

    const Atom nameAtom = XInternAtom(xrandr->xdisplay, name, False);
    g_free(name);

    GDMonitorData* data = data_from_monitor(monitor);
    data->xrandr_name = nameAtom;

    increase_monitor_count(xrandr, nameAtom);

    GList* outputs = gd_monitor_get_outputs(monitor);
    XRRMonitorInfo* monitorInfo = XRRAllocateMonitor(xrandr->xdisplay, g_list_length(outputs));
    monitorInfo->name = nameAtom;
    monitorInfo->primary = gd_monitor_is_primary(monitor);
    monitorInfo->automatic = True;

    for (l = outputs, i = 0; l; l = l->next, i++) {
        GDOutput* output = l->data;
        monitorInfo->outputs[i] = gd_output_get_id(output);
    }

    XRRSetMonitor(xrandr->xdisplay, xrandr->xroot, monitorInfo);
    XRRFreeMonitors(monitorInfo);
}

static void gd_monitor_manager_xrandr_tiled_monitor_removed(GDMonitorManager* manager, GDMonitor* monitor)
{
    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(manager);

    C_RETURN_IF_FAIL(xrandr->has_randr15);

    const GDMonitorData* data = data_from_monitor(monitor);
    const gint mc = decrease_monitor_count(xrandr, data->xrandr_name);
    if (mc == 0) {
        XRRDeleteMonitor(xrandr->xdisplay, xrandr->xroot, data->xrandr_name);
    }
}

static bool gd_monitor_manager_xrandr_is_transform_handled(GDMonitorManager* manager, GDCrtc* crtc, GDMonitorTransform transform)
{
    g_warn_if_fail((gd_crtc_get_all_transforms(crtc) & transform) == transform);

    return TRUE;
}

static gfloat gd_monitor_manager_xrandr_calculate_monitor_mode_scale(GDMonitorManager* manager, GDLogicalMonitorLayoutMode layoutMode, GDMonitor* monitor, GDMonitorMode* monitorMode)
{
    GDMonitorScalesConstraint constraints = GD_MONITOR_SCALES_CONSTRAINT_NO_FRAC;

    return gd_monitor_calculate_mode_scale(monitor, monitorMode, constraints);
}

static gfloat* gd_monitor_manager_xrandr_calculate_supported_scales(GDMonitorManager* manager, GDLogicalMonitorLayoutMode layoutMode, GDMonitor* monitor, GDMonitorMode* monitorMode, gint* nSupportedScales)
{
    GDMonitorScalesConstraint constraints = GD_MONITOR_SCALES_CONSTRAINT_NO_FRAC;

    return gd_monitor_calculate_supported_scales(monitor, monitorMode, constraints, nSupportedScales);
}

static GDMonitorManagerCapability gd_monitor_manager_xrandr_get_capabilities(GDMonitorManager* manager)
{
    return GD_MONITOR_MANAGER_CAPABILITY_GLOBAL_SCALE_REQUIRED;
}

static bool gd_monitor_manager_xrandr_get_max_screen_size(GDMonitorManager* manager, gint* max_width, gint* max_height)
{
    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(manager);
    GDGpu* gpu = get_gpu(xrandr);

    gd_gpu_xrandr_get_max_screen_size(GD_GPU_XRANDR(gpu), max_width, max_height);

    return true;
}

static GDLogicalMonitorLayoutMode gd_monitor_manager_xrandr_get_default_layout_mode(GDMonitorManager* manager)
{
    return GD_LOGICAL_MONITOR_LAYOUT_MODE_PHYSICAL;
}

static void gd_monitor_manager_xrandr_set_output_ctm(GDOutput* output, const GDOutputCtm* ctm)
{
    gd_output_xrandr_set_ctm(GD_OUTPUT_XRANDR(output), ctm);
}

static void gd_monitor_manager_xrandr_class_init(GDMonitorManagerXrandrClass* xrandr_class)
{

    GObjectClass* oc = G_OBJECT_CLASS(xrandr_class);
    GDMonitorManagerClass* mc = GD_MONITOR_MANAGER_CLASS(xrandr_class);

    oc->constructed = gd_monitor_manager_xrandr_constructed;
    oc->dispose = gd_monitor_manager_xrandr_dispose;

    mc->read_edid = gd_monitor_manager_xrandr_read_edid;
    mc->read_current_state = gd_monitor_manager_xrandr_read_current_state;
    mc->ensure_initial_config = gd_monitor_manager_xrandr_ensure_initial_config;
    mc->apply_monitors_config = gd_monitor_manager_xrandr_apply_monitors_config;
    mc->set_power_save_mode = gd_monitor_manager_xrandr_set_power_save_mode;
    mc->get_crtc_gamma = gd_monitor_manager_xrandr_get_crtc_gamma;
    mc->set_crtc_gamma = gd_monitor_manager_xrandr_set_crtc_gamma;
    mc->tiled_monitor_added = gd_monitor_manager_xrandr_tiled_monitor_added;
    mc->tiled_monitor_removed = gd_monitor_manager_xrandr_tiled_monitor_removed;
    mc->is_transform_handled = gd_monitor_manager_xrandr_is_transform_handled;
    mc->calculate_monitor_mode_scale = gd_monitor_manager_xrandr_calculate_monitor_mode_scale;
    mc->calculate_supported_scales = gd_monitor_manager_xrandr_calculate_supported_scales;
    mc->get_capabilities = gd_monitor_manager_xrandr_get_capabilities;
    mc->get_max_screen_size = gd_monitor_manager_xrandr_get_max_screen_size;
    mc->get_default_layout_mode = gd_monitor_manager_xrandr_get_default_layout_mode;
    mc->set_output_ctm = gd_monitor_manager_xrandr_set_output_ctm;
}

static void gd_monitor_manager_xrandr_init(GDMonitorManagerXrandr* xrandr)
{
}

Display* gd_monitor_manager_xrandr_get_xdisplay(GDMonitorManagerXrandr* xrandr)
{
    return xrandr->xdisplay;
}

bool gd_monitor_manager_xrandr_has_randr15(GDMonitorManagerXrandr* xrandr)
{
    return xrandr->has_randr15;
}

bool gd_monitor_manager_xrandr_handle_xevent(GDMonitorManagerXrandr* xrandr, XEvent* event)
{
    GDMonitorManager* manager = GD_MONITOR_MANAGER(xrandr);
    C_RETURN_VAL_IF_OK((event->type - xrandr->rr_event_base) != RRScreenChangeNotify, false);

    XRRUpdateConfiguration(event);
    gd_monitor_manager_read_current_state(manager);

    GDGpu* gpu = get_gpu(xrandr);
    GDGpuXrandr* gx = GD_GPU_XRANDR(gpu);
    XRRScreenResources* resources = gd_gpu_xrandr_get_resources(gx);

    C_RETURN_VAL_IF_FAIL(resources, true);

    if (resources->timestamp < resources->configTimestamp) {
        gd_monitor_manager_reconfigure(manager);
    }
    else {
        GDMonitorsConfig* config = NULL;
        if (resources->timestamp == xrandr->last_xrandr_set_timestamp) {
            GDMonitorConfigManager* cm = gd_monitor_manager_get_config_manager(manager);
            config = gd_monitor_config_manager_get_current(cm);
        }
        gd_monitor_manager_rebuild_derived(manager, config);
    }

    return true;
}

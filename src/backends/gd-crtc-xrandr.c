//
// Created by dingjing on 25-6-28.
//
#include "gd-crtc-xrandr-private.h"

#include <X11/Xlib-xcb.h>

#include "gd-output-private.h"
#include "gd-backend-private.h"
#include "gd-monitor-manager-xrandr-private.h"


#define ALL_ROTATIONS (RR_Rotate_0 | RR_Rotate_90 | RR_Rotate_180 | RR_Rotate_270)

struct _GDCrtcXrandr
{
    GDCrtc parent;

    GDRectangle rect;
    GDMonitorTransform transform;

    GDCrtcMode* current_mode;
};

G_DEFINE_TYPE(GDCrtcXrandr, gd_crtc_xrandr, GD_TYPE_CRTC)

static GDMonitorTransform gd_monitor_transform_from_xrandr(Rotation rotation)
{
    static const GDMonitorTransform y_reflected_map[4] = {
        GD_MONITOR_TRANSFORM_FLIPPED_180,
        GD_MONITOR_TRANSFORM_FLIPPED_90,
        GD_MONITOR_TRANSFORM_FLIPPED,
        GD_MONITOR_TRANSFORM_FLIPPED_270};
    GDMonitorTransform ret;

    switch (rotation & 0x7F) {
    default: case RR_Rotate_0:
        ret = GD_MONITOR_TRANSFORM_NORMAL;
        break;

    case RR_Rotate_90:
        ret = GD_MONITOR_TRANSFORM_90;
        break;

    case RR_Rotate_180:
        ret = GD_MONITOR_TRANSFORM_180;
        break;

    case RR_Rotate_270:
        ret = GD_MONITOR_TRANSFORM_270;
        break;
    }

    if (rotation & RR_Reflect_X) return ret + 4;
    else if (rotation & RR_Reflect_Y) return y_reflected_map[ret];
    else return ret;
}

static GDMonitorTransform gd_monitor_transform_from_xrandr_all(Rotation rotation)
{
    GDMonitorTransform ret;

    /* Handle the common cases first (none or all) */
    if (rotation == 0 || rotation == RR_Rotate_0) return (1 << GD_MONITOR_TRANSFORM_NORMAL);

    /* All rotations and one reflection -> all of them by composition */
    if ((rotation & ALL_ROTATIONS) && ((rotation & RR_Reflect_X) || (rotation & RR_Reflect_Y))) return GD_MONITOR_ALL_TRANSFORMS;

    ret = 1 << GD_MONITOR_TRANSFORM_NORMAL;
    if (rotation & RR_Rotate_90) ret |= 1 << GD_MONITOR_TRANSFORM_90;
    if (rotation & RR_Rotate_180) ret |= 1 << GD_MONITOR_TRANSFORM_180;
    if (rotation & RR_Rotate_270) ret |= 1 << GD_MONITOR_TRANSFORM_270;
    if (rotation & (RR_Rotate_0 | RR_Reflect_X)) ret |= 1 << GD_MONITOR_TRANSFORM_FLIPPED;
    if (rotation & (RR_Rotate_90 | RR_Reflect_X)) ret |= 1 << GD_MONITOR_TRANSFORM_FLIPPED_90;
    if (rotation & (RR_Rotate_180 | RR_Reflect_X)) ret |= 1 << GD_MONITOR_TRANSFORM_FLIPPED_180;
    if (rotation & (RR_Rotate_270 | RR_Reflect_X)) ret |= 1 << GD_MONITOR_TRANSFORM_FLIPPED_270;

    return ret;
}

static void gd_crtc_xrandr_class_init(GDCrtcXrandrClass* selfClass)
{
}

static void gd_crtc_xrandr_init(GDCrtcXrandr* self)
{
}

GDCrtcXrandr* gd_crtc_xrandr_new(GDGpuXrandr* gpu_xrandr, XRRCrtcInfo* xrandr_crtc, RRCrtc crtc_id, XRRScreenResources* resources)

{
    GDGpu* gpu;
    GDBackend* backend;
    GDMonitorManager* monitor_manager;
    GDMonitorManagerXrandr* monitor_manager_xrandr;
    Display* xdisplay;
    GDMonitorTransform all_transforms;
    GDCrtcXrandr* crtc_xrandr;
    XRRPanning* panning;
    unsigned int i;
    GList* modes;

    gpu = GD_GPU(gpu_xrandr);

    backend = gd_gpu_get_backend(gpu);
    monitor_manager = gd_backend_get_monitor_manager(backend);
    monitor_manager_xrandr = GD_MONITOR_MANAGER_XRANDR(monitor_manager);
    xdisplay = gd_monitor_manager_xrandr_get_xdisplay(monitor_manager_xrandr);

    all_transforms = gd_monitor_transform_from_xrandr_all(xrandr_crtc->rotations);

    crtc_xrandr = g_object_new(GD_TYPE_CRTC_XRANDR, "id", (uint64_t)crtc_id, "gpu", gpu, "all-transforms", all_transforms, NULL);

    crtc_xrandr->transform = gd_monitor_transform_from_xrandr(xrandr_crtc->rotation);

    panning = XRRGetPanning(xdisplay, resources, crtc_id);
    if (panning && panning->width > 0 && panning->height > 0) {
        crtc_xrandr->rect = (GDRectangle) {
            .
            x = panning->left,
            .
            y = panning->top,
            .
            width = panning->width,
            .
            height = panning->height
        };
    }
    else {
        crtc_xrandr->rect = (GDRectangle) {
            .
            x = xrandr_crtc->x,
            .
            y = xrandr_crtc->y,
            .
            width = xrandr_crtc->width,
            .
            height = xrandr_crtc->height
        };
    }
    XRRFreePanning(panning);

    modes = gd_gpu_get_modes(gpu);
    for (i = 0; i < (guint)resources->nmode; i++) {
        if (resources->modes[i].id == xrandr_crtc->mode) {
            crtc_xrandr->current_mode = g_list_nth_data(modes, i);
            break;
        }
    }

    if (crtc_xrandr->current_mode) {
        gd_crtc_set_config(GD_CRTC(crtc_xrandr), &crtc_xrandr->rect, crtc_xrandr->current_mode, crtc_xrandr->transform);
    }

    return crtc_xrandr;
}

bool
gd_crtc_xrandr_set_config(GDCrtcXrandr* self, xcb_randr_crtc_t xrandr_crtc, xcb_timestamp_t timestamp, int x, int y, xcb_randr_mode_t mode, xcb_randr_rotation_t rotation, xcb_randr_output_t* outputs, int n_outputs,
                          xcb_timestamp_t* out_timestamp)
{
    GDGpu* gpu;
    GDGpuXrandr* gpu_xrandr;
    GDBackend* backend;
    GDMonitorManager* monitor_manager;
    GDMonitorManagerXrandr* monitor_manager_xrandr;
    Display* xdisplay;
    XRRScreenResources* resources;
    xcb_connection_t* xcb_conn;
    xcb_timestamp_t config_timestamp;
    xcb_randr_set_crtc_config_cookie_t cookie;
    xcb_randr_set_crtc_config_reply_t* reply;
    xcb_generic_error_t* xcb_error;

    gpu = gd_crtc_get_gpu(GD_CRTC(self));
    gpu_xrandr = GD_GPU_XRANDR(gpu);

    backend = gd_gpu_get_backend(gpu);

    monitor_manager = gd_backend_get_monitor_manager(backend);
    monitor_manager_xrandr = GD_MONITOR_MANAGER_XRANDR(monitor_manager);

    xdisplay = gd_monitor_manager_xrandr_get_xdisplay(monitor_manager_xrandr);
    resources = gd_gpu_xrandr_get_resources(gpu_xrandr);
    xcb_conn = XGetXCBConnection(xdisplay);

    config_timestamp = resources->configTimestamp;
    cookie = xcb_randr_set_crtc_config(xcb_conn, xrandr_crtc, timestamp, config_timestamp, x, y, mode, rotation, n_outputs, outputs);

    xcb_error = NULL;
    reply = xcb_randr_set_crtc_config_reply(xcb_conn, cookie, &xcb_error);
    if (xcb_error || !reply) {
        g_free(xcb_error);
        g_free(reply);

        return FALSE;
    }

    *out_timestamp = reply->timestamp;
    g_free(reply);

    return TRUE;
}

bool gd_crtc_xrandr_is_assignment_changed(GDCrtcXrandr* self, GDCrtcAssignment* crtc_assignment)
{
    unsigned int i;

    if (self->current_mode != crtc_assignment->mode) return TRUE;

    if (self->rect.x != crtc_assignment->layout.x) return TRUE;

    if (self->rect.y != crtc_assignment->layout.y) return TRUE;

    if (self->transform != crtc_assignment->transform) return TRUE;

    for (i = 0; i < crtc_assignment->outputs->len; i++) {
        GDOutput* output;
        GDCrtc* assigned_crtc;

        output = ((GDOutput**)crtc_assignment->outputs->pdata)[i];
        assigned_crtc = gd_output_get_assigned_crtc(output);

        if (assigned_crtc != GD_CRTC(self)) return TRUE;
    }

    return FALSE;
}

GDCrtcMode* gd_crtc_xrandr_get_current_mode(GDCrtcXrandr* self)
{
    return self->current_mode;
}

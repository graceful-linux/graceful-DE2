//
// Created by dingjing on 25-6-28.
//
#include "gd-gpu-xrandr-private.h"

#include <gio/gio.h>
#include <X11/Xlibint.h>
#include <X11/extensions/dpms.h>

#include "gd-output-private.h"
#include "gd-crtc-mode-private.h"
#include "gd-crtc-xrandr-private.h"
#include "gd-output-xrandr-private.h"
#include "gd-monitor-manager-xrandr-private.h"


struct _GDGpuXrandr
{
    GDGpu parent;

    XRRScreenResources* resources;

    int max_screen_width;
    int max_screen_height;
};

G_DEFINE_TYPE(GDGpuXrandr, gd_gpu_xrandr, GD_TYPE_GPU)

static gint compare_outputs(const void* one, const void* two)
{
    GDOutput* o_one;
    GDOutput* o_two;
    const GDOutputInfo* output_info_one;
    const GDOutputInfo* output_info_two;

    o_one = (GDOutput*)one;
    o_two = (GDOutput*)two;

    output_info_one = gd_output_get_info(o_one);
    output_info_two = gd_output_get_info(o_two);

    return strcmp(output_info_one->name, output_info_two->name);
}

static char* get_xmode_name(XRRModeInfo* xmode)
{
    int width = xmode->width;
    int height = xmode->height;

    return g_strdup_printf("%dx%d", width, height);
}

static float calculate_refresh_rate(XRRModeInfo* xmode)
{
    float h_total;
    float v_total;

    h_total = (float)xmode->hTotal;
    v_total = (float)xmode->vTotal;

    if (h_total == 0.0f || v_total == 0.0f) return 0.0;

    if (xmode->modeFlags & RR_DoubleScan) v_total *= 2.0f;

    if (xmode->modeFlags & RR_Interlace) v_total /= 2.0f;

    return xmode->dotClock / (h_total * v_total);
}

static void gd_gpu_xrandr_finalize(GObject* object)
{
    GDGpuXrandr* gpu_xrandr;

    gpu_xrandr = GD_GPU_XRANDR(object);

    g_clear_pointer(&gpu_xrandr->resources, XRRFreeScreenResources);

    G_OBJECT_CLASS(gd_gpu_xrandr_parent_class)->finalize(object);
}

static bool gd_gpu_xrandr_read_current(GDGpu* gpu, GError** error)
{
    GDGpuXrandr* gpu_xrandr;
    GDBackend* backend;
    GDMonitorManager* monitor_manager;
    GDMonitorManagerXrandr* monitor_manager_xrandr;
    Display* xdisplay;
    XRRScreenResources* resources;
    gint min_width;
    gint min_height;
    Screen* screen;
    GList* outputs;
    GList* modes;
    GList* crtcs;
    guint i, j;
    GList* l;
    RROutput primary_output;

    gpu_xrandr = GD_GPU_XRANDR(gpu);

    backend = gd_gpu_get_backend(gpu);
    monitor_manager = gd_backend_get_monitor_manager(backend);
    monitor_manager_xrandr = GD_MONITOR_MANAGER_XRANDR(monitor_manager);
    xdisplay = gd_monitor_manager_xrandr_get_xdisplay(monitor_manager_xrandr);

    g_clear_pointer(&gpu_xrandr->resources, XRRFreeScreenResources);

    XRRGetScreenSizeRange(xdisplay, DefaultRootWindow(xdisplay), &min_width, &min_height, &gpu_xrandr->max_screen_width, &gpu_xrandr->max_screen_height);

    screen = ScreenOfDisplay(xdisplay, DefaultScreen (xdisplay));
    /* This is updated because we called RRUpdateConfiguration below */
    monitor_manager->screen_width = WidthOfScreen(screen);
    monitor_manager->screen_height = HeightOfScreen(screen);

    resources = XRRGetScreenResourcesCurrent(xdisplay, DefaultRootWindow(xdisplay));

    if (!resources) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Failed to retrieve Xrandr screen resources");
        return FALSE;
    }

    gpu_xrandr->resources = resources;

    outputs = NULL;
    modes = NULL;
    crtcs = NULL;

    for (i = 0; i < (guint)resources->nmode; i++) {
        XRRModeInfo* xmode;
        GDCrtcModeInfo* crtc_mode_info;
        char* crtc_mode_name;
        GDCrtcMode* mode;

        xmode = &resources->modes[i];

        crtc_mode_info = gd_crtc_mode_info_new();
        crtc_mode_name = get_xmode_name(xmode);

        crtc_mode_info->width = xmode->width;
        crtc_mode_info->height = xmode->height;
        crtc_mode_info->refresh_rate = calculate_refresh_rate(xmode);
        crtc_mode_info->flags = xmode->modeFlags;

        mode = g_object_new(GD_TYPE_CRTC_MODE, "id", (uint64_t)xmode->id, "name", crtc_mode_name, "info", crtc_mode_info, NULL);

        modes = g_list_append(modes, mode);

        gd_crtc_mode_info_unref(crtc_mode_info);
        g_free(crtc_mode_name);
    }

    gd_gpu_take_modes(gpu, modes);

    for (i = 0; i < (guint)resources->ncrtc; i++) {
        XRRCrtcInfo* xrandr_crtc;
        RRCrtc crtc_id;
        GDCrtcXrandr* crtc_xrandr;

        crtc_id = resources->crtcs[i];
        xrandr_crtc = XRRGetCrtcInfo(xdisplay, resources, crtc_id);
        crtc_xrandr = gd_crtc_xrandr_new(gpu_xrandr, xrandr_crtc, crtc_id, resources);

        crtcs = g_list_append(crtcs, crtc_xrandr);
        XRRFreeCrtcInfo(xrandr_crtc);
    }

    gd_gpu_take_crtcs(gpu, crtcs);

    primary_output = XRRGetOutputPrimary(xdisplay, DefaultRootWindow(xdisplay));

    for (i = 0; i < (guint)resources->noutput; i++) {
        RROutput output_id;
        XRROutputInfo* xrandr_output;

        output_id = resources->outputs[i];
        xrandr_output = XRRGetOutputInfo(xdisplay, resources, output_id);

        if (!xrandr_output) continue;

        if (xrandr_output->connection != RR_Disconnected) {
            GDOutputXrandr* output_xrandr;

            output_xrandr = gd_output_xrandr_new(gpu_xrandr, xrandr_output, output_id, primary_output);

            if (output_xrandr) outputs = g_list_prepend(outputs, output_xrandr);
        }

        XRRFreeOutputInfo(xrandr_output);
    }

    /* Sort the outputs for easier handling in GfMonitorConfig */
    outputs = g_list_sort(outputs, compare_outputs);

    gd_gpu_take_outputs(gpu, outputs);

    /* Now fix the clones */
    for (l = outputs; l; l = l->next) {
        GDOutput* output;
        const GDOutputInfo* output_info;
        GList* k;

        output = l->data;
        output_info = gd_output_get_info(output);

        for (j = 0; j < output_info->n_possible_clones; j++) {
            RROutput clone = GPOINTER_TO_INT(output_info->possible_clones[j]);
            for (k = outputs; k; k = k->next) {
                GDOutput* possible_clone = k->data;
                if (clone == (XID)gd_output_get_id(possible_clone)) {
                    output_info->possible_clones[j] = possible_clone;
                    break;
                }
            }
        }
    }

    return TRUE;
}

static void gd_gpu_xrandr_class_init(GDGpuXrandrClass* gpu_xrandr_class)
{
    GObjectClass* object_class;
    GDGpuClass* gpu_class;

    object_class = G_OBJECT_CLASS(gpu_xrandr_class);
    gpu_class = GD_GPU_CLASS(gpu_xrandr_class);

    object_class->finalize = gd_gpu_xrandr_finalize;
    gpu_class->read_current = gd_gpu_xrandr_read_current;
}

static void gd_gpu_xrandr_init(GDGpuXrandr* gpu_xrandr)
{
}

GDGpuXrandr* gd_gpu_xrandr_new(GDBackendX11* backend_x11)
{
    return g_object_new(GD_TYPE_GPU_XRANDR, "backend", backend_x11, NULL);
}

XRRScreenResources* gd_gpu_xrandr_get_resources(GDGpuXrandr* gpu_xrandr)
{
    return gpu_xrandr->resources;
}

void gd_gpu_xrandr_get_max_screen_size(GDGpuXrandr* gpu_xrandr, int* max_width, int* max_height)
{
    *max_width = gpu_xrandr->max_screen_width;
    *max_height = gpu_xrandr->max_screen_height;
}

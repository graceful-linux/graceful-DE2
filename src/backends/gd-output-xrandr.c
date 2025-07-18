//
// Created by dingjing on 25-6-28.
//
#include "gd-output-xrandr-private.h"

#include <math.h>
#include <X11/Xatom.h>
#include <xcb/randr.h>
#include <X11/Xlib-xcb.h>
#include <X11/extensions/Xrandr.h>

#include "gd-crtc-private.h"
#include "gd-crtc-mode-private.h"
#include "gd-monitor-manager-xrandr-private.h"


struct _GDOutputXrandr
{
    GDOutput parent;

    gboolean ctm_initialized;
    GDOutputCtm ctm;
};

G_DEFINE_TYPE(GDOutputXrandr, gd_output_xrandr, GD_TYPE_OUTPUT)

static gboolean ctm_is_equal(const GDOutputCtm* ctm1, const GDOutputCtm* ctm2)
{
    int i;

    for (i = 0; i < 9; i++) {
        if (ctm1->matrix[i] != ctm2->matrix[i]) return FALSE;
    }

    return TRUE;
}

static Display*
xdisplay_from_gpu(GDGpu* gpu)
{
    GDBackend* backend;
    GDMonitorManager* monitor_manager;
    GDMonitorManagerXrandr* monitor_manager_xrandr;

    backend = gd_gpu_get_backend(gpu);
    monitor_manager = gd_backend_get_monitor_manager(backend);
    monitor_manager_xrandr = GD_MONITOR_MANAGER_XRANDR(monitor_manager);

    return gd_monitor_manager_xrandr_get_xdisplay(monitor_manager_xrandr);
}

static Display*
xdisplay_from_output(GDOutput* output)
{
    return xdisplay_from_gpu(gd_output_get_gpu(output));
}

static void
backlight_changed_cb(GDOutput* output)
{
    Display* xdisplay;
    Atom atom;
    int value;

    xdisplay = xdisplay_from_output(output);
    atom = XInternAtom(xdisplay, "Backlight", False);
    value = gd_output_get_backlight(output);

    xcb_randr_change_output_property(XGetXCBConnection(xdisplay), (XID)gd_output_get_id(output), atom, XCB_ATOM_INTEGER, 32, XCB_PROP_MODE_REPLACE, 1, &value);
}

static void
output_set_presentation_xrandr(GDOutput* output, gboolean presentation)
{
    Display* xdisplay;
    Atom atom;
    gint value;

    xdisplay = xdisplay_from_output(output);
    atom = XInternAtom(xdisplay, "_GNOME_FLASHBACK_PRESENTATION_OUTPUT", False);
    value = presentation;

    xcb_randr_change_output_property(XGetXCBConnection(xdisplay), (XID)gd_output_get_id(output), atom, XCB_ATOM_CARDINAL, 32, XCB_PROP_MODE_REPLACE, 1, &value);
}

static void
output_set_underscanning_xrandr(GDOutput* output, gboolean underscanning)
{
    Display* xdisplay;
    Atom prop, valueatom;
    const gchar* value;

    xdisplay = xdisplay_from_output(output);
    prop = XInternAtom(xdisplay, "underscan", False);

    value = underscanning ? "on" : "off";
    valueatom = XInternAtom(xdisplay, value, False);

    xcb_randr_change_output_property(XGetXCBConnection(xdisplay), (XID)gd_output_get_id(output), prop, XCB_ATOM_ATOM, 32, XCB_PROP_MODE_REPLACE, 1, &valueatom);

    /* Configure the border at the same time. Currently, we use a
     * 5% of the width/height of the mode. In the future, we should
     * make the border configurable.
     */
    if (underscanning) {
        GDCrtc* crtc;
        const GDCrtcConfig* crtc_config;
        const GDCrtcModeInfo* crtc_mode_info;
        uint32_t border_value;

        crtc = gd_output_get_assigned_crtc(output);
        crtc_config = gd_crtc_get_config(crtc);
        crtc_mode_info = gd_crtc_mode_get_info(crtc_config->mode);

        prop = XInternAtom(xdisplay, "underscan hborder", False);
        border_value = crtc_mode_info->width * 0.05;

        xcb_randr_change_output_property(XGetXCBConnection(xdisplay), (XID)gd_output_get_id(output), prop, XCB_ATOM_INTEGER, 32, XCB_PROP_MODE_REPLACE, 1, &border_value);

        prop = XInternAtom(xdisplay, "underscan vborder", False);
        border_value = crtc_mode_info->height * 0.05;

        xcb_randr_change_output_property(XGetXCBConnection(xdisplay), (XID)gd_output_get_id(output), prop, XCB_ATOM_INTEGER, 32, XCB_PROP_MODE_REPLACE, 1, &border_value);
    }
}

static void
output_set_max_bpc_xrandr(GDOutput* output, unsigned int max_bpc)
{
    Display* xdisplay;
    Atom prop;
    uint32_t value;

    xdisplay = xdisplay_from_output(output);
    prop = XInternAtom(xdisplay, "max bpc", False);
    value = max_bpc;

    xcb_randr_change_output_property(XGetXCBConnection(xdisplay), (XID)gd_output_get_id(output), prop, XCB_ATOM_INTEGER, 32, XCB_PROP_MODE_REPLACE, 1, &value);
}

static guint8*
get_edid_property(Display* xdisplay, RROutput output, Atom atom, gsize* len)
{
    guchar* prop;
    gint actual_format;
    gulong nitems, bytes_after;
    Atom actual_type;
    guint8* result;

    XRRGetOutputProperty(xdisplay, output, atom, 0, 100, False, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &prop);

    if (actual_type == XA_INTEGER && actual_format == 8) {
        result = g_memdup2(prop, nitems);
        if (len) *len = nitems;
    }
    else {
        result = NULL;
    }

    if (prop) XFree(prop);

    return result;
}

static GBytes*
read_xrandr_edid(Display* xdisplay, RROutput output_id)
{
    Atom edid_atom;
    guint8* result;
    gsize len;

    edid_atom = XInternAtom(xdisplay, "EDID", FALSE);
    result = get_edid_property(xdisplay, output_id, edid_atom, &len);

    if (!result) {
        edid_atom = XInternAtom(xdisplay, "EDID_DATA", FALSE);
        result = get_edid_property(xdisplay, output_id, edid_atom, &len);
    }

    if (result) {
        if (len > 0 && len % 128 == 0) return g_bytes_new_take(result, len);
        else g_free(result);
    }

    return NULL;
}

static gboolean
output_get_property_exists(Display* xdisplay, RROutput output_id, const gchar* propname)
{
    gboolean exists;
    Atom atom, actual_type;
    gint actual_format;
    gulong nitems, bytes_after;
    guchar* buffer;

    atom = XInternAtom(xdisplay, propname, False);
    XRRGetOutputProperty(xdisplay, (XID)output_id, atom, 0, G_MAXLONG, False, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    exists = (actual_type != None);

    if (buffer) XFree(buffer);

    return exists;
}

static gboolean
output_get_hotplug_mode_update(Display* xdisplay, RROutput output_id)
{
    return output_get_property_exists(xdisplay, output_id, "hotplug_mode_update");
}

static gboolean
output_get_integer_property(Display* xdisplay, RROutput output_id, const gchar* propname, gint* value)
{
    gboolean exists;
    Atom atom, actual_type;
    gint actual_format;
    gulong nitems, bytes_after;
    guchar* buffer;

    atom = XInternAtom(xdisplay, propname, False);
    XRRGetOutputProperty(xdisplay, (XID)output_id, atom, 0, G_MAXLONG, False, False, XA_INTEGER, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    exists = (actual_type == XA_INTEGER && actual_format == 32 && nitems == 1);

    if (exists && value != NULL) *value = ((gint*)buffer)[0];

    if (buffer) XFree(buffer);

    return exists;
}

static gint
output_get_suggested_x(Display* xdisplay, RROutput output_id)
{
    gint val;

    if (output_get_integer_property(xdisplay, output_id, "suggested X", &val)) return val;

    return -1;
}

static gint
output_get_suggested_y(Display* xdisplay, RROutput output_id)
{
    gint val;

    if (output_get_integer_property(xdisplay, output_id, "suggested Y", &val)) return val;

    return -1;
}

static GDConnectorType
connector_type_from_atom(Display* xdisplay, Atom atom)
{
    if (atom == XInternAtom(xdisplay, "HDMI", True)) return GD_CONNECTOR_TYPE_HDMIA;
    if (atom == XInternAtom(xdisplay, "VGA", True)) return GD_CONNECTOR_TYPE_VGA;
    /* Doesn't have a DRM equivalent, but means an internal panel.
     * We could pick either LVDS or eDP here. */
    if (atom == XInternAtom(xdisplay, "Panel", True)) return GD_CONNECTOR_TYPE_LVDS;
    if (atom == XInternAtom(xdisplay, "DVI", True) || atom == XInternAtom(xdisplay, "DVI-I", True)) return GD_CONNECTOR_TYPE_DVII;
    if (atom == XInternAtom(xdisplay, "DVI-A", True)) return GD_CONNECTOR_TYPE_DVIA;
    if (atom == XInternAtom(xdisplay, "DVI-D", True)) return GD_CONNECTOR_TYPE_DVID;
    if (atom == XInternAtom(xdisplay, "DisplayPort", True)) return GD_CONNECTOR_TYPE_DisplayPort;
    if (atom == XInternAtom(xdisplay, "TV", True)) return GD_CONNECTOR_TYPE_TV;
    if (atom == XInternAtom(xdisplay, "TV-Composite", True)) return GD_CONNECTOR_TYPE_Composite;
    if (atom == XInternAtom(xdisplay, "TV-SVideo", True)) return GD_CONNECTOR_TYPE_SVIDEO;
    /* Another set of mismatches. */
    if (atom == XInternAtom(xdisplay, "TV-SCART", True)) return GD_CONNECTOR_TYPE_TV;
    if (atom == XInternAtom(xdisplay, "TV-C4", True)) return GD_CONNECTOR_TYPE_TV;

    return GD_CONNECTOR_TYPE_Unknown;
}

static GDConnectorType
output_get_connector_type_from_prop(Display* xdisplay, RROutput output_id)
{
    Atom atom, actual_type, connector_type_atom;
    gint actual_format;
    gulong nitems, bytes_after;
    guchar* buffer;
    GDConnectorType ret;

    atom = XInternAtom(xdisplay, "ConnectorType", False);
    XRRGetOutputProperty(xdisplay, (XID)output_id, atom, 0, G_MAXLONG, False, False, XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    if (actual_type != XA_ATOM || actual_format != 32 || nitems < 1) {
        if (buffer) XFree(buffer);

        return GD_CONNECTOR_TYPE_Unknown;
    }

    connector_type_atom = ((Atom*)buffer)[0];
    ret = connector_type_from_atom(xdisplay, connector_type_atom);
    XFree(buffer);

    return ret;
}

static GDConnectorType
output_info_get_connector_type_from_name(const GDOutputInfo* output_info)
{
    const gchar* name;

    name = output_info->name;

    /* drmmode_display.c, which was copy/pasted across all the FOSS
     * xf86-video-* drivers, seems to name its outputs based on the
     * connector type, so look for that....
     *
     * SNA has its own naming scheme, because what else did you expect
     * from SNA, but it's not too different, so we can thankfully use
     * that with minor changes.
     *
     * http://cgit.freedesktop.org/xorg/xserver/tree/hw/xfree86/drivers/modesetting/drmmode_display.c#n953
     * http://cgit.freedesktop.org/xorg/driver/xf86-video-intel/tree/src/sna/sna_display.c#n3486
     */

    if (g_str_has_prefix(name, "DVI")) return GD_CONNECTOR_TYPE_DVII;
    if (g_str_has_prefix(name, "LVDS")) return GD_CONNECTOR_TYPE_LVDS;
    if (g_str_has_prefix(name, "HDMI")) return GD_CONNECTOR_TYPE_HDMIA;
    if (g_str_has_prefix(name, "VGA")) return GD_CONNECTOR_TYPE_VGA;
    /* SNA uses DP, not DisplayPort. Test for both. */
    if (g_str_has_prefix(name, "DP") || g_str_has_prefix(name, "DisplayPort")) return GD_CONNECTOR_TYPE_DisplayPort;
    if (g_str_has_prefix(name, "eDP")) return GD_CONNECTOR_TYPE_eDP;
    if (g_str_has_prefix(name, "Virtual")) return GD_CONNECTOR_TYPE_VIRTUAL;
    if (g_str_has_prefix(name, "Composite")) return GD_CONNECTOR_TYPE_Composite;
    if (g_str_has_prefix(name, "S-video")) return GD_CONNECTOR_TYPE_SVIDEO;
    if (g_str_has_prefix(name, "TV")) return GD_CONNECTOR_TYPE_TV;
    if (g_str_has_prefix(name, "CTV")) return GD_CONNECTOR_TYPE_Composite;
    if (g_str_has_prefix(name, "DSI")) return GD_CONNECTOR_TYPE_DSI;
    if (g_str_has_prefix(name, "DIN")) return GD_CONNECTOR_TYPE_9PinDIN;

    return GD_CONNECTOR_TYPE_Unknown;
}

static GDConnectorType
output_info_get_connector_type(GDOutputInfo* output_info, Display* xdisplay, RROutput output_id)
{
    GDConnectorType ret;

    /* The "ConnectorType" property is considered mandatory since RandR 1.3,
     * but none of the FOSS drivers support it, because we're a bunch of
     * professional software developers.
     *
     * Try poking it first, without any expectations that it will work.
     * If it's not there, we thankfully have other bonghits to try next.
     */
    ret = output_get_connector_type_from_prop(xdisplay, output_id);
    if (ret != GD_CONNECTOR_TYPE_Unknown) return ret;

    /* Fall back to heuristics based on the output name. */
    ret = output_info_get_connector_type_from_name(output_info);
    if (ret != GD_CONNECTOR_TYPE_Unknown) return ret;

    return GD_CONNECTOR_TYPE_Unknown;
}

static GDMonitorTransform
output_get_panel_orientation_transform(Display* xdisplay, RROutput output_id)
{
    unsigned long nitems;
    unsigned long bytes_after;
    Atom atom;
    Atom actual_type;
    int actual_format;
    unsigned char* buffer;
    char* str;
    GDMonitorTransform transform;

    buffer = NULL;
    str = NULL;

    atom = XInternAtom(xdisplay, "panel orientation", False);
    XRRGetOutputProperty(xdisplay, (XID)output_id, atom, 0, G_MAXLONG, False, False, XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    if (actual_type != XA_ATOM || actual_format != 32 || nitems < 1) {
        g_free(buffer);
        return GD_MONITOR_TRANSFORM_NORMAL;
    }

    str = XGetAtomName(xdisplay, *(Atom*)buffer);
    g_free(buffer);

    transform = GD_MONITOR_TRANSFORM_NORMAL;

    if (strcmp(str, "Upside Down") == 0) transform = GD_MONITOR_TRANSFORM_180;
    else if (strcmp(str, "Left Side Up") == 0) transform = GD_MONITOR_TRANSFORM_90;
    else if (strcmp(str, "Right Side Up") == 0) transform = GD_MONITOR_TRANSFORM_270;

    g_free(str);

    return transform;
}

static void
output_info_init_tile_info(GDOutputInfo* output_info, Display* xdisplay, RROutput output_id)
{
    Atom tile_atom;
    guchar* prop;
    gulong nitems, bytes_after;
    gint actual_format;
    Atom actual_type;

    tile_atom = XInternAtom(xdisplay, "TILE", FALSE);
    XRRGetOutputProperty(xdisplay, (XID)output_id, tile_atom, 0, 100, False, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after, &prop);

    if (actual_type == XA_INTEGER && actual_format == 32 && nitems == 8) {
        glong* values = (glong*)prop;

        output_info->tile_info.group_id = values[0];
        output_info->tile_info.flags = values[1];
        output_info->tile_info.max_h_tiles = values[2];
        output_info->tile_info.max_v_tiles = values[3];
        output_info->tile_info.loc_h_tile = values[4];
        output_info->tile_info.loc_v_tile = values[5];
        output_info->tile_info.tile_w = values[6];
        output_info->tile_info.tile_h = values[7];
    }

    if (prop) XFree(prop);
}

static void
output_info_init_modes(GDOutputInfo* output_info, GDGpu* gpu, XRROutputInfo* xrandr_output)
{
    guint j;
    guint n_actual_modes;

    output_info->modes = g_new0(GDCrtcMode *, xrandr_output->nmode);

    n_actual_modes = 0;
    for (j = 0; j < (guint)xrandr_output->nmode; j++) {
        GList* l;

        for (l = gd_gpu_get_modes(gpu); l; l = l->next) {
            GDCrtcMode* mode = l->data;

            if (xrandr_output->modes[j] == (XID)gd_crtc_mode_get_id(mode)) {
                output_info->modes[n_actual_modes] = mode;
                n_actual_modes += 1;
                break;
            }
        }
    }

    output_info->n_modes = n_actual_modes;
    if (n_actual_modes > 0) output_info->preferred_mode = output_info->modes[0];
}

static void
output_info_init_crtcs(GDOutputInfo* output_info, GDGpu* gpu, XRROutputInfo* xrandr_output)
{
    guint j;
    guint n_actual_crtcs;
    GList* l;

    output_info->possible_crtcs = g_new0(GDCrtc *, xrandr_output->ncrtc);

    n_actual_crtcs = 0;
    for (j = 0; j < (guint)xrandr_output->ncrtc; j++) {
        for (l = gd_gpu_get_crtcs(gpu); l; l = l->next) {
            GDCrtc* crtc = l->data;

            if ((XID)gd_crtc_get_id(crtc) == xrandr_output->crtcs[j]) {
                output_info->possible_crtcs[n_actual_crtcs] = crtc;
                n_actual_crtcs += 1;
                break;
            }
        }
    }
    output_info->n_possible_crtcs = n_actual_crtcs;
}

static GDCrtc*
find_assigned_crtc(GDGpu* gpu, XRROutputInfo* xrandr_output)
{
    GList* l;

    for (l = gd_gpu_get_crtcs(gpu); l; l = l->next) {
        GDCrtc* crtc = l->data;

        if ((XID)gd_crtc_get_id(crtc) == xrandr_output->crtc) return crtc;
    }

    return NULL;
}

static gboolean
output_get_boolean_property(GDOutput* output, const gchar* propname)
{
    Display* xdisplay;
    Atom atom, actual_type;
    gint actual_format;
    gulong nitems, bytes_after;
    guchar* buffer;
    gboolean value;

    xdisplay = xdisplay_from_output(output);
    atom = XInternAtom(xdisplay, propname, False);
    XRRGetOutputProperty(xdisplay, (XID)gd_output_get_id(output), atom, 0, G_MAXLONG, False, False, XA_CARDINAL, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    if (actual_type != XA_CARDINAL || actual_format != 32 || nitems < 1) {
        if (buffer) XFree(buffer);

        return FALSE;
    }

    value = ((gint*)buffer)[0];
    XFree(buffer);

    return value;
}

static gboolean
output_get_max_bpc_xrandr(GDOutput* output, unsigned int* max_bpc)
{
    Display* xdisplay;
    Atom atom;
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char* buffer;

    xdisplay = xdisplay_from_output(output);
    atom = XInternAtom(xdisplay, "max bpc", False);
    buffer = NULL;

    XRRGetOutputProperty(xdisplay, (XID)gd_output_get_id(output), atom, 0, G_MAXLONG, False, False, XCB_ATOM_INTEGER, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    if (actual_type != XCB_ATOM_INTEGER || actual_format != 32 || nitems < 1) {
        if (buffer != NULL) XFree(buffer);

        return FALSE;
    }

    if (max_bpc) *max_bpc = *((uint32_t*)buffer);

    XFree(buffer);

    return TRUE;
}

static gboolean
output_get_presentation_xrandr(GDOutput* output)
{
    return output_get_boolean_property(output, "_GNOME_FLASHBACK_PRESENTATION_OUTPUT");
}

static gboolean
output_get_underscanning_xrandr(GDOutput* output)
{
    Display* xdisplay;
    Atom atom, actual_type;
    gint actual_format;
    gulong nitems, bytes_after;
    guchar* buffer;
    gchar* str;
    gboolean value;

    xdisplay = xdisplay_from_output(output);
    atom = XInternAtom(xdisplay, "underscan", False);
    XRRGetOutputProperty(xdisplay, (XID)gd_output_get_id(output), atom, 0, G_MAXLONG, False, False, XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    if (actual_type != XA_ATOM || actual_format != 32 || nitems < 1) {
        if (buffer) XFree(buffer);

        return FALSE;
    }

    str = XGetAtomName(xdisplay, *(Atom*)buffer);
    XFree(buffer);

    value = !strcmp(str, "on");
    XFree(str);

    return value;
}

static gboolean
output_get_supports_underscanning_xrandr(Display* xdisplay, RROutput output_id)
{
    Atom atom, actual_type;
    gint actual_format, i;
    gulong nitems, bytes_after;
    unsigned char* buffer;
    XRRPropertyInfo* property_info;
    Atom* values;
    gboolean supports_underscanning = FALSE;

    atom = XInternAtom(xdisplay, "underscan", False);
    buffer = NULL;

    XRRGetOutputProperty(xdisplay, (XID)output_id, atom, 0, G_MAXLONG, False, False, XA_ATOM, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    if (actual_type != XA_ATOM || actual_format != 32 || nitems < 1) {
        if (buffer) XFree(buffer);

        return FALSE;
    }

    XFree(buffer);

    property_info = XRRQueryOutputProperty(xdisplay, (XID)output_id, atom);
    values = (Atom*)property_info->values;

    for (i = 0; i < property_info->num_values; i++) {
        /* The output supports underscanning if "on" is a valid value
         * for the underscan property.
         */
        gchar* name = XGetAtomName(xdisplay, values[i]);
        if (strcmp(name, "on") == 0) supports_underscanning = TRUE;

        XFree(name);
    }

    XFree(property_info);

    return supports_underscanning;
}

static gboolean
output_get_max_bpc_range_xrandr(Display* xdisplay, RROutput output_id, unsigned int* min, unsigned int* max)
{
    Atom atom;
    XRRPropertyInfo* property_info;
    long* values;

    if (!output_get_property_exists(xdisplay, output_id, "max bpc")) return FALSE;

    atom = XInternAtom(xdisplay, "max bpc", False);
    property_info = XRRQueryOutputProperty(xdisplay, (XID)output_id, atom);

    if (property_info == NULL) return FALSE;

    if (property_info->num_values != 2) {
        XFree(property_info);
        return FALSE;
    }

    values = (long*)property_info->values;

    if (min) *min = values[0];

    if (max) *max = values[1];

    XFree(property_info);

    return TRUE;
}

static gboolean
output_get_supports_color_transform_xrandr(Display* xdisplay, RROutput output_id)
{
    Atom atom, actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char* buffer;

    atom = XInternAtom(xdisplay, "CTM", False);
    buffer = NULL;

    XRRGetOutputProperty(xdisplay, (XID)output_id, atom, 0, G_MAXLONG, False, False, XA_INTEGER, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    XFree(buffer);

    /* X's CTM property is 9 64-bit integers represented as an array of 18
     * 32-bit integers.
     */
    return (actual_type == XA_INTEGER && actual_format == 32 && nitems == 18);
}

static gint
output_get_backlight_xrandr(GDOutput* output)
{
    Display* xdisplay;
    gint value = -1;
    Atom atom, actual_type;
    gint actual_format;
    gulong nitems, bytes_after;
    guchar* buffer;

    xdisplay = xdisplay_from_output(output);
    atom = XInternAtom(xdisplay, "Backlight", False);
    XRRGetOutputProperty(xdisplay, (XID)gd_output_get_id(output), atom, 0, G_MAXLONG, False, False, XA_INTEGER, &actual_type, &actual_format, &nitems, &bytes_after, &buffer);

    if (actual_type != XA_INTEGER || actual_format != 32 || nitems < 1) {
        if (buffer) XFree(buffer);

        return FALSE;
    }

    value = ((gint*)buffer)[0];
    XFree(buffer);

    return value;
}

static void
output_info_init_backlight_limits_xrandr(GDOutputInfo* output_info, Display* xdisplay, xcb_randr_output_t output_id)
{
    Atom atom;
    xcb_connection_t* xcb_conn;
    xcb_randr_query_output_property_cookie_t cookie;
    xcb_randr_query_output_property_reply_t* reply;
    int32_t* values;

    atom = XInternAtom(xdisplay, "Backlight", False);

    xcb_conn = XGetXCBConnection(xdisplay);
    cookie = xcb_randr_query_output_property(xcb_conn, output_id, (xcb_atom_t)atom);

    reply = xcb_randr_query_output_property_reply(xcb_conn, cookie, NULL);

    /* This can happen on systems without backlights. */
    if (reply == NULL) return;

    if (!reply->range || reply->length != 2) {
        g_warning("backlight %s was not range\n", output_info->name);
        g_free(reply);
        return;
    }

    values = xcb_randr_query_output_property_valid_values(reply);

    output_info->backlight_min = values[0];
    output_info->backlight_max = values[1];

    g_free(reply);
}

static void
gd_output_xrandr_class_init(GDOutputXrandrClass* self_class)
{
}

static void
gd_output_xrandr_init(GDOutputXrandr* self)
{
}

GDOutputXrandr*
gd_output_xrandr_new(GDGpuXrandr* gpu_xrandr, XRROutputInfo* xrandr_output, RROutput output_id, RROutput primary_output)
{
    GDGpu* gpu;
    GDBackend* backend;
    GDMonitorManager* monitor_manager;
    GDMonitorManagerXrandr* monitor_manager_xrandr;
    Display* xdisplay;
    GDOutputInfo* output_info;
    GDOutput* output;
    GBytes* edid;
    GDCrtc* assigned_crtc;
    unsigned int i;

    gpu = GD_GPU(gpu_xrandr);
    backend = gd_gpu_get_backend(gpu);
    monitor_manager = gd_backend_get_monitor_manager(backend);
    monitor_manager_xrandr = GD_MONITOR_MANAGER_XRANDR(monitor_manager);
    xdisplay = gd_monitor_manager_xrandr_get_xdisplay(monitor_manager_xrandr);

    output_info = gd_output_info_new();

    output_info->name = g_strdup(xrandr_output->name);

    edid = read_xrandr_edid(xdisplay, output_id);
    gd_output_info_parse_edid(output_info, edid);
    g_bytes_unref(edid);

    output_info->hotplug_mode_update = output_get_hotplug_mode_update(xdisplay, output_id);
    output_info->suggested_x = output_get_suggested_x(xdisplay, output_id);
    output_info->suggested_y = output_get_suggested_y(xdisplay, output_id);
    output_info->connector_type = output_info_get_connector_type(output_info, xdisplay, output_id);
    output_info->panel_orientation_transform = output_get_panel_orientation_transform(xdisplay, output_id);

    if (gd_monitor_transform_is_rotated(output_info->panel_orientation_transform)) {
        output_info->width_mm = xrandr_output->mm_height;
        output_info->height_mm = xrandr_output->mm_width;
    }
    else {
        output_info->width_mm = xrandr_output->mm_width;
        output_info->height_mm = xrandr_output->mm_height;
    }

    if (gd_monitor_manager_xrandr_has_randr15(monitor_manager_xrandr)) output_info_init_tile_info(output_info, xdisplay, output_id);

    output_info_init_modes(output_info, gpu, xrandr_output);
    output_info_init_crtcs(output_info, gpu, xrandr_output);

    output_info->n_possible_clones = xrandr_output->nclone;
    output_info->possible_clones = g_new0(GDOutput *, output_info->n_possible_clones);

    /* We can build the list of clones now, because we don't have
     * the list of outputs yet, so temporarily set the pointers to
     * the bare XIDs, and then we'll fix them in a second pass
     */
    for (i = 0; i < (unsigned int)xrandr_output->nclone; i++) {
        output_info->possible_clones[i] = GINT_TO_POINTER(xrandr_output->clones[i]);
    }

    output_info->supports_underscanning = output_get_supports_underscanning_xrandr(xdisplay, output_id);

    output_get_max_bpc_range_xrandr(xdisplay, output_id, &output_info->max_bpc_min, &output_info->max_bpc_max);

    output_info->supports_color_transform = output_get_supports_color_transform_xrandr(xdisplay, output_id);

    output_info_init_backlight_limits_xrandr(output_info, xdisplay, output_id);

    output = g_object_new(GD_TYPE_OUTPUT_XRANDR, "id", (uint64_t)output_id, "gpu", gpu_xrandr, "info", output_info, NULL);

    assigned_crtc = find_assigned_crtc(gpu, xrandr_output);
    if (assigned_crtc) {
        GDOutputAssignment output_assignment;
        gboolean has_max_bpc;

        output_assignment = (GDOutputAssignment)
        {
            .
            is_primary = (XID)gd_output_get_id(output) == primary_output,
            .
            is_presentation = output_get_presentation_xrandr(output),
            .
            is_underscanning = output_get_underscanning_xrandr(output),
        };

        has_max_bpc = output_get_max_bpc_xrandr(output, &output_assignment.max_bpc);
        output_assignment.has_max_bpc = has_max_bpc;

        gd_output_assign_crtc(output, assigned_crtc, &output_assignment);
    }
    else {
        gd_output_unassign_crtc(output);
    }

    if (!(output_info->backlight_min == 0 && output_info->backlight_max == 0)) {
        gd_output_set_backlight(output, output_get_backlight_xrandr(output));

        g_signal_connect(output, "backlight-changed", G_CALLBACK(backlight_changed_cb), NULL);
    }

    if (output_info->n_modes == 0 || output_info->n_possible_crtcs == 0) {
        gd_output_info_unref(output_info);
        g_object_unref(output);
        return NULL;
    }

    gd_output_info_unref(output_info);

    return GD_OUTPUT_XRANDR(output);
}

GBytes*
gd_output_xrandr_read_edid(GDOutputXrandr* self)
{
    GDOutput* output;
    Display* xdisplay;
    RROutput output_id;

    output = GD_OUTPUT(self);
    xdisplay = xdisplay_from_output(output);
    output_id = (RROutput)gd_output_get_id(output);

    return read_xrandr_edid(xdisplay, output_id);
}

void
gd_output_xrandr_apply_mode(GDOutputXrandr* self)
{
    GDOutput* output;
    Display* xdisplay;
    const GDOutputInfo* output_info;
    unsigned int max_bpc;

    output = GD_OUTPUT(self);
    xdisplay = xdisplay_from_output(output);
    output_info = gd_output_get_info(output);

    if (gd_output_is_primary(output)) {
        XRRSetOutputPrimary(xdisplay, DefaultRootWindow(xdisplay), (XID)gd_output_get_id(output));
    }

    output_set_presentation_xrandr(output, gd_output_is_presentation(output));

    if (output_info->supports_underscanning) {
        output_set_underscanning_xrandr(output, gd_output_is_underscanning(output));
    }

    if (gd_output_get_max_bpc(output, &max_bpc) && max_bpc >= output_info->max_bpc_min && max_bpc <= output_info->max_bpc_max) {
        output_set_max_bpc_xrandr(output, max_bpc);
    }
}

void
gd_output_xrandr_set_ctm(GDOutputXrandr* self, const GDOutputCtm* ctm)
{
    if (!self->ctm_initialized || !ctm_is_equal(ctm, &self->ctm)) {
        GDOutput* output;
        Display* xdisplay;
        Atom ctm_atom;

        output = GD_OUTPUT(self);

        xdisplay = xdisplay_from_output(output);
        ctm_atom = XInternAtom(xdisplay, "CTM", False);

        xcb_randr_change_output_property(XGetXCBConnection(xdisplay), (XID)gd_output_get_id(output), ctm_atom, XCB_ATOM_INTEGER, 32, XCB_PROP_MODE_REPLACE, 18, &ctm->matrix);

        self->ctm_initialized = TRUE;
        self->ctm = *ctm;
    }
}

//
// Created by dingjing on 25-6-27.
//

#include "gd-wm.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

struct _GDWm
{
    GObject         parent;

    Atom            wmCheckAtom;
    Atom            wmNameAtom;
    Atom            utf8StringAtom;
    Window          wmCheck;
};
G_DEFINE_TYPE (GDWm, gd_wm, G_TYPE_OBJECT)


static char * get_wm_name (GDWm *self)
{
    Atom actualType;
    int actualFormat;
    unsigned long nItems;
    unsigned long bytesAfter;
    unsigned char* prop = NULL;

    GdkDisplay* display = gdk_display_get_default ();
    Display* xdisplay = gdk_x11_display_get_xdisplay (display);

    gdk_x11_display_error_trap_push (display);

    const int result = XGetWindowProperty (xdisplay,
                               self->wmCheck,
                               self->wmNameAtom,
                               0,
                               G_MAXLONG,
                               False,
                               self->utf8StringAtom,
                               &actualType,
                               &actualFormat,
                               &nItems,
                               &bytesAfter,
                               &prop);

    gdk_x11_display_error_trap_pop_ignored (display);

    if (result != Success || actualType != self->utf8StringAtom || actualFormat != 8 || nItems == 0) {
        XFree (prop);
        return NULL;
    }

    char *wmName = g_strndup ((const char *) prop, nItems);
    XFree (prop);

    if (!g_utf8_validate (wmName, -1, NULL)) {
        g_free (wmName);
        return NULL;
    }

    return wmName;
}


static void update_gnome_wm_keybindings (GDWm *self)
{
    GdkDisplay *display = gdk_display_get_default ();
    Display *xdisplay = gdk_x11_display_get_xdisplay (display);

    char* wmName = get_wm_name (self);
    char* wmKeybindings = g_strdup_printf ("%s,Graceful DE2", wmName ? wmName : "Unknown");

    gdk_x11_display_error_trap_push (display);

    XChangeProperty (xdisplay,
                   self->wmCheck,
                   XInternAtom (xdisplay, "_GRACEFUL_WM_KEYBINDINGS", False),
                   self->utf8StringAtom,
                   8,
                   PropModeReplace,
                   (guchar *) wmKeybindings,
                   strlen (wmKeybindings));

    gdk_x11_display_error_trap_pop_ignored (display);

    g_free (wmKeybindings);
    g_free (wmName);
}

static Window get_net_supporting_wm_check (GDWm* self, GdkDisplay *display, Window window)
{
    Atom actualType;
    int actualFormat;
    unsigned long nItems;
    unsigned long bytesAfter;
    unsigned char* prop = NULL;

    Display *xdisplay = gdk_x11_display_get_xdisplay (display);
    gdk_x11_display_error_trap_push (display);

    const int result = XGetWindowProperty (xdisplay,
                               window,
                               self->wmCheckAtom,
                               0,
                               G_MAXLONG,
                               False,
                               XA_WINDOW,
                               &actualType,
                               &actualFormat,
                               &nItems,
                               &bytesAfter,
                               &prop);

    gdk_x11_display_error_trap_pop_ignored (display);

    if (result != Success || actualType != XA_WINDOW || nItems == 0) {
        XFree (prop);
        return None;
    }

    const Window wmCheck = *(Window*) prop;
    XFree (prop);

    return wmCheck;
}

static void update_wm_check (GDWm *self)
{
    GdkDisplay *display = gdk_display_get_default ();
    Display *xdisplay = gdk_x11_display_get_xdisplay (display);
    const Window wmCheck = get_net_supporting_wm_check (self, display, XDefaultRootWindow (xdisplay));

    if (wmCheck == None) {
        return;
    }

    if (wmCheck != get_net_supporting_wm_check (self, display, wmCheck)) {
        return;
    }
    self->wmCheck = wmCheck;

    gdk_x11_display_error_trap_push (display);
    XSelectInput (xdisplay, self->wmCheck, StructureNotifyMask);

    if (gdk_x11_display_error_trap_pop (display) != 0) {
        self->wmCheck = None;
    }

    update_gnome_wm_keybindings (self);
}

static GdkFilterReturn event_filter_cb (GdkXEvent *xevent, GdkEvent* event, void* data)
{
    XEvent* e = xevent;
    GDWm* self = GD_WM (data);

    if (self->wmCheck != None) {
        if (e->type == DestroyNotify && e->xdestroywindow.window == self->wmCheck) {
            update_wm_check (self);
        }
        else if (e->type == PropertyNotify && e->xproperty.window == self->wmCheck) {
            if (e->xproperty.atom == self->wmCheckAtom) {
                update_wm_check (self);
            }
            else if (e->xproperty.atom == self->wmNameAtom) {
                update_gnome_wm_keybindings (self);
            }
        }
    }
    else {
        GdkDisplay *display = gdk_display_get_default ();
        Display *xdisplay = gdk_x11_display_get_xdisplay (display);
        if (e->type == PropertyNotify && e->xproperty.window == XDefaultRootWindow (xdisplay) && e->xproperty.atom == self->wmCheckAtom) {
            update_wm_check (self);
        }
    }

    return GDK_FILTER_CONTINUE;
}

static void gd_wm_finalize (GObject *object)
{
    GDWm *self = GD_WM (object);

    gdk_window_remove_filter (NULL, event_filter_cb, self);

    G_OBJECT_CLASS (gd_wm_parent_class)->finalize (object);
}

static void gd_wm_class_init (GDWmClass *self_class)
{
    GObjectClass *objClass = G_OBJECT_CLASS (self_class);
    objClass->finalize = gd_wm_finalize;
}

static void gd_wm_init (GDWm *self)
{
    GdkDisplay *display;
    Display *xdisplay;
    Window xroot;
    XWindowAttributes attrs;

    display = gdk_display_get_default ();
    xdisplay = gdk_x11_display_get_xdisplay (display);
    xroot = DefaultRootWindow (xdisplay);

    self->wmCheckAtom = XInternAtom (xdisplay, "_NET_SUPPORTING_WM_CHECK", False);

    self->wmNameAtom = XInternAtom (xdisplay, "_NET_WM_NAME", False);
    self->utf8StringAtom = XInternAtom (xdisplay, "UTF8_STRING", False);

    gdk_window_add_filter (NULL, event_filter_cb, self);

    XGetWindowAttributes (xdisplay, xroot, &attrs);
    XSelectInput (xdisplay, xroot, PropertyChangeMask | attrs.your_event_mask);
    XSync (xdisplay, False);

    update_wm_check (self);
}

GDWm* gd_wm_new (void)
{
    return g_object_new (GD_TYPE_WM, NULL);
}




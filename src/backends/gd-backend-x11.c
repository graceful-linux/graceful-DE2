//
// Created by dingjing on 25-6-28.
//
#include <gio/gio.h>
#include <X11/Xlib.h>

#include "gd-backend-x11-private.h"

typedef struct
{
    GSource         source;
    GPollFD         eventPollFd;
    GDBackend*      backend;
} XEventSource;

typedef struct
{
    Display*        xdisplay;
    GSource*        source;
} GDBackendX11Private;

static void initable_iface_init(GInitableIface* initable_iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GDBackendX11, gd_backend_x11, GD_TYPE_BACKEND, G_ADD_PRIVATE (GDBackendX11) G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, initable_iface_init))

static void handle_host_xevent(GDBackend* backend, XEvent* event)
{
    GDBackendX11* x11;
    GDBackendX11Private* priv;

    x11 = GD_BACKEND_X11(backend);
    priv = gd_backend_x11_get_instance_private(x11);

    XGetEventData(priv->xdisplay, &event->xcookie);

    GD_BACKEND_X11_GET_CLASS(x11)->handle_host_xevent(x11, event);

    XFreeEventData(priv->xdisplay, &event->xcookie);
}

static bool x_event_source_prepare(GSource* source, int* timeout)
{
    XEventSource* x_source;
    GDBackendX11* x11;
    GDBackendX11Private* priv;

    x_source = (XEventSource*)source;
    x11 = GD_BACKEND_X11(x_source->backend);
    priv = gd_backend_x11_get_instance_private(x11);

    *timeout = -1;

    return XPending(priv->xdisplay);
}

static bool x_event_source_check(GSource* source)
{
    XEventSource* x_source;
    GDBackendX11* x11;
    GDBackendX11Private* priv;

    x_source = (XEventSource*)source;
    x11 = GD_BACKEND_X11(x_source->backend);
    priv = gd_backend_x11_get_instance_private(x11);

    return XPending(priv->xdisplay);
}

static bool x_event_source_dispatch(GSource* source, GSourceFunc callback, gpointer uData)
{
    XEventSource* x_source;
    GDBackendX11* x11;
    GDBackendX11Private* priv;

    x_source = (XEventSource*)source;
    x11 = GD_BACKEND_X11(x_source->backend);
    priv = gd_backend_x11_get_instance_private(x11);

    while (XPending(priv->xdisplay)) {
        XEvent event;
        XNextEvent(priv->xdisplay, &event);
        handle_host_xevent(x_source->backend, &event);
    }

    return TRUE;
}

GSource *g_source_new (GSourceFuncs* sourceFuncs, guint structSize);
static GSourceFuncs x_event_funcs = {
    (GSourceFuncsPrepareFunc) x_event_source_prepare,
    (GSourceFuncsCheckFunc) x_event_source_check,
    (GSourceFuncsDispatchFunc) x_event_source_dispatch,
};

static GSource* x_event_source_new(GDBackend* backend)
{
    GDBackendX11* x11;
    GDBackendX11Private* priv;
    GSource* source;
    XEventSource* x_source;

    x11 = GD_BACKEND_X11(backend);
    priv = gd_backend_x11_get_instance_private(x11);

    source = g_source_new(&x_event_funcs, sizeof(XEventSource));

    x_source = (XEventSource*)source;
    x_source->eventPollFd.fd = ConnectionNumber(priv->xdisplay);
    x_source->eventPollFd.events = G_IO_IN;
    x_source->backend = backend;

    g_source_add_poll(source, &x_source->eventPollFd);
    g_source_attach(source, NULL);

    return source;
}

static bool gd_backend_x11_initable_init(GInitable* initable, GCancellable* cancellable, GError** error)
{
    GDBackendX11* x11;
    GDBackendX11Private* priv;
    GInitableIface* parent_iface;
    const gchar* display;

    x11 = GD_BACKEND_X11(initable);
    priv = gd_backend_x11_get_instance_private(x11);
    parent_iface = g_type_interface_peek_parent(G_INITABLE_GET_IFACE(x11));

    display = g_getenv("DISPLAY");
    if (!display) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Unable to open display, DISPLAY not set");

        return FALSE;
    }

    priv->xdisplay = XOpenDisplay(display);
    if (!priv->xdisplay) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Unable to open display '%s'", display);

        return FALSE;
    }

    return parent_iface->init(initable, cancellable, error);
}

static void initable_iface_init(GInitableIface* initable_iface)
{
    initable_iface->init = (gboolean(*)(GInitable*, GCancellable*, GError**))gd_backend_x11_initable_init;
}

static void gd_backend_x11_finalize(GObject* object)
{
    GDBackendX11* x11;
    GDBackendX11Private* priv;

    x11 = GD_BACKEND_X11(object);
    priv = gd_backend_x11_get_instance_private(x11);

    if (priv->source != NULL) {
        g_source_unref(priv->source);
        priv->source = NULL;
    }

    if (priv->xdisplay != NULL) {
        XCloseDisplay(priv->xdisplay);
        priv->xdisplay = NULL;
    }

    G_OBJECT_CLASS(gd_backend_x11_parent_class)->finalize(object);
}

static void gd_backend_x11_post_init(GDBackend* backend)
{
    GDBackendX11* x11;
    GDBackendX11Private* priv;

    x11 = GD_BACKEND_X11(backend);
    priv = gd_backend_x11_get_instance_private(x11);

    priv->source = x_event_source_new(backend);

    GD_BACKEND_CLASS(gd_backend_x11_parent_class)->post_init(backend);
}

static void gd_backend_x11_class_init(GDBackendX11Class* x11_class)
{
    GObjectClass* object_class;
    GDBackendClass* backend_class;

    object_class = G_OBJECT_CLASS(x11_class);
    backend_class = GD_BACKEND_CLASS(x11_class);

    object_class->finalize = gd_backend_x11_finalize;
    backend_class->post_init = gd_backend_x11_post_init;
}

static void gd_backend_x11_init(GDBackendX11* x11)
{
}

Display* gd_backend_x11_get_xdisplay(GDBackendX11* x11)
{
    GDBackendX11Private* priv;

    priv = gd_backend_x11_get_instance_private(x11);

    return priv->xdisplay;
}

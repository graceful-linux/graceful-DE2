//
// Created by dingjing on 25-6-28.
//
#include "gd-gpu-xrandr-private.h"
#include "gd-backend-x11-cm-private.h"
#include "gd-monitor-manager-xrandr-private.h"


struct _GDBackendX11Cm
{
    GDBackendX11 parent;
};

G_DEFINE_TYPE(GDBackendX11Cm, gd_backend_x11_cm, GD_TYPE_BACKEND_X11)

static GDMonitorManager* gd_backend_x11_cm_create_monitor_manager(GDBackend* backend, GError** error)
{
    return g_object_new(GD_TYPE_MONITOR_MANAGER_XRANDR, "backend", backend, NULL);
}

static bool gd_backend_x11_cm_handle_host_xevent(GDBackendX11* x11, XEvent* event)
{
    GDBackend* backend = GD_BACKEND(x11);
    GDMonitorManager* mm = gd_backend_get_monitor_manager(backend);
    GDMonitorManagerXrandr* xrandr = GD_MONITOR_MANAGER_XRANDR(mm);

    return gd_monitor_manager_xrandr_handle_xevent(xrandr, event);
}

static void gd_backend_x11_cm_class_init(GDBackendX11CmClass* x11_cm_class)
{
    GDBackendClass* bc = GD_BACKEND_CLASS(x11_cm_class);
    GDBackendX11Class* bX11C = GD_BACKEND_X11_CLASS(x11_cm_class);

    bc->create_monitor_manager = gd_backend_x11_cm_create_monitor_manager;
    bX11C->handle_host_xevent = gd_backend_x11_cm_handle_host_xevent;
}

static void gd_backend_x11_cm_init(GDBackendX11Cm* self)
{
    GDGpuXrandr* gx = gd_gpu_xrandr_new(GD_BACKEND_X11(self));
    gd_backend_add_gpu(GD_BACKEND(self), GD_GPU(gx));
}

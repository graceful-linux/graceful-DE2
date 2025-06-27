//
// Created by dingjing on 25-6-27.
//

#include "gd-application.h"

#include "gd-wm.h"
#include "gd-session-manager-gen.dbus.h"


struct _GDApplication
{
    GObject                         parent;
    GDWm*                           wm;
    guint                           busName;
};
G_DEFINE_TYPE(GDApplication, gd_application, G_TYPE_OBJECT)


static void gd_application_dispose (GObject* obj);


static void gd_application_init (GDApplication* obj)
{
    obj->wm = gd_wm_new();
    obj->busName = g_bus_own_name(G_BUS_TYPE_SESSION, "org.graceful.DE2",
        G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
        NULL, NULL, NULL, NULL, NULL);
}

static void gd_application_class_init (GDApplicationClass* klass)
{
    GObjectClass* objClass = G_OBJECT_CLASS(klass);
    objClass->dispose = gd_application_dispose;
}


GDApplication * gd_application_new(void)
{
    return g_object_new(GD_TYPE_APPLICATION, NULL);
}

static void gd_application_dispose (GObject* obj)
{
    GDApplication* app = GD_APPLICATION(obj);

    C_FREE_FUNC(app->busName, g_bus_unown_name);
    C_FREE_FUNC_NULL(app->wm, g_object_unref);

    G_OBJECT_CLASS(gd_application_parent_class)->dispose(obj);
}

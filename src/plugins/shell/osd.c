//
// Created by dingjing on 25-7-3.
//
#include "osd.h"

#include "gd-osd-window.h"

struct _FlashbackOsd
{
    GObject parent;

    GDOsdWindow** windows;
    gint n_monitors;

    gulong monitors_changed_id;
};

G_DEFINE_TYPE(FlashbackOsd, flashback_osd, G_TYPE_OBJECT)

static void monitors_changed(GdkScreen* screen, gpointer user_data)
{
    FlashbackOsd * osd;
    GdkDisplay* display;
    gint n_monitors;
    gint i;

    osd = FLASHBACK_OSD(user_data);

    display = gdk_display_get_default();
    n_monitors = gdk_display_get_n_monitors(display);

    if (osd->windows != NULL) {
        for (i = 0; i < osd->n_monitors; i++) gtk_widget_destroy(GTK_WIDGET(osd->windows[i]));
        g_free(osd->windows);
    }

    osd->windows = g_new0(GDOsdWindow *, n_monitors);

    for (i = 0; i < n_monitors; i++) osd->windows[i] = gd_osd_window_new(i);

    osd->n_monitors = n_monitors;
}

static void
flashabck_osd_finalize(GObject* object)
{
    GdkScreen* screen;
    FlashbackOsd* osd;

    screen = gdk_screen_get_default();
    osd = FLASHBACK_OSD(object);

    g_signal_handler_disconnect(screen, osd->monitors_changed_id);

    if (osd->windows != NULL) {
        gint i;

        for (i = 0; i < osd->n_monitors; i++) gtk_widget_destroy(GTK_WIDGET(osd->windows[i]));
        g_free(osd->windows);
    }

    G_OBJECT_CLASS(flashback_osd_parent_class)->finalize(object);
}

static void
flashback_osd_class_init(FlashbackOsdClass* osd_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(osd_class);

    object_class->finalize = flashabck_osd_finalize;
}

static void
flashback_osd_init(FlashbackOsd* osd)
{
    GdkScreen* screen;

    screen = gdk_screen_get_default();

    osd->monitors_changed_id = g_signal_connect(screen, "monitors-changed", G_CALLBACK(monitors_changed), osd);

    monitors_changed(screen, osd);
}

FlashbackOsd*
flashback_osd_new(void)
{
    return g_object_new(FLASHBACK_TYPE_OSD, NULL);
}

void
flashback_osd_show(FlashbackOsd* osd, GDMonitorManager* monitor_manager, GVariant* params)
{
    GVariantDict dict;
    const gchar* icon_name;
    const gchar* label;
    GIcon* icon;
    gdouble level;
    gdouble max_level;
    const gchar* connector;
    gint monitor;
    gint i;

    g_variant_dict_init(&dict, params);

    if (!g_variant_dict_lookup(&dict, "icon", "&s", &icon_name)) icon_name = NULL;

    if (!g_variant_dict_lookup(&dict, "label", "&s", &label)) label = NULL;

    if (!g_variant_dict_lookup(&dict, "level", "d", &level)) level = -1;

    if (!g_variant_dict_lookup(&dict, "max_level", "d", &max_level)) max_level = 1.0;

    if (!g_variant_dict_lookup(&dict, "connector", "&s", &connector)) connector = NULL;

    monitor = -1;
    if (connector != NULL) monitor = gd_monitor_manager_get_monitor_for_connector(monitor_manager, connector);

    icon = NULL;
    if (icon_name) icon = g_icon_new_for_string(icon_name, NULL);

    if (monitor != -1) {
        for (i = 0; i < osd->n_monitors; i++) {
            if (i == monitor) {
                gd_osd_window_set_icon(osd->windows[i], icon);
                gd_osd_window_set_label(osd->windows[i], label);
                gd_osd_window_set_level(osd->windows[i], level);
                gd_osd_window_set_max_level(osd->windows[i], max_level);
                gd_osd_window_show(osd->windows[i]);
            }
            else {
                gd_osd_window_hide(osd->windows[i]);
            }
        }
    }
    else {
        for (i = 0; i < osd->n_monitors; i++) {
            gd_osd_window_set_icon(osd->windows[i], icon);
            gd_osd_window_set_label(osd->windows[i], label);
            gd_osd_window_set_level(osd->windows[i], level);
            gd_osd_window_set_max_level(osd->windows[i], max_level);
            gd_osd_window_show(osd->windows[i]);
        }
    }

    if (icon) g_object_unref(icon);
}

//
// Created by dingjing on 25-7-3.
//
#include "monitor-labeler.h"

#include <gio/gio.h>

#include "gd-label-window.h"
#include "backends/gd-monitor-manager.h"

struct _FlashbackMonitorLabeler
{
    GObject parent;

    guint watch_id;
    gchar* client;

    guint hide_id;

    GDLabelWindow** windows;
    gint n_windows;
};

typedef struct
{
    FlashbackMonitorLabeler* labeler;
    gchar* sender;
} CallbackData;

G_DEFINE_TYPE(FlashbackMonitorLabeler, flashback_monitor_labeler, G_TYPE_OBJECT)

static void
real_hide(FlashbackMonitorLabeler* labeler)
{
    gint i;

    if (labeler->windows != NULL) {
        for (i = 0; i < labeler->n_windows; i++) gd_label_window_hide(labeler->windows[i]);
        g_free(labeler->windows);
        labeler->windows = NULL;
    }
}

static void
name_vanished_handler(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
    FlashbackMonitorLabeler* labeler;

    labeler = FLASHBACK_MONITOR_LABELER(user_data);

    real_hide(labeler);
}

static bool
track_client(FlashbackMonitorLabeler* labeler, const gchar* client)
{
    if (labeler->client != NULL) return g_strcmp0(labeler->client, client) == 0;

    labeler->client = g_strdup(client);
    labeler->watch_id = g_bus_watch_name(G_BUS_TYPE_SESSION, client, G_BUS_NAME_WATCHER_FLAGS_NONE, NULL, (GBusNameVanishedCallback)name_vanished_handler, labeler, NULL);

    return TRUE;
}

static bool
untrack_client(FlashbackMonitorLabeler* labeler, const gchar* client)
{
    if (labeler->client == NULL || g_strcmp0(labeler->client, client) != 0) return FALSE;

    if (labeler->watch_id > 0) {
        g_bus_unwatch_name(labeler->watch_id);
        labeler->watch_id = 0;
    }

    g_free(labeler->client);
    labeler->client = NULL;

    return TRUE;
}

static void
free_callback_data(CallbackData* data)
{
    g_object_unref(data->labeler);
    g_free(data->sender);
    g_free(data);
}

static bool
hide_cb(gpointer user_data)
{
    CallbackData* data;

    data = (CallbackData*)user_data;

    data->labeler->hide_id = 0;

    if (!untrack_client(data->labeler, data->sender)) return FALSE;

    real_hide(data->labeler);

    return FALSE;
}

static void
flashback_monitor_labeler_finalize(GObject* object)
{
    FlashbackMonitorLabeler* labeler;

    labeler = FLASHBACK_MONITOR_LABELER(object);

    if (labeler->watch_id > 0) {
        g_bus_unwatch_name(labeler->watch_id);
        labeler->watch_id = 0;
    }

    if (labeler->hide_id > 0) {
        g_source_remove(labeler->hide_id);
        labeler->hide_id = 0;
    }

    real_hide(labeler);

    g_free(labeler->client);

    G_OBJECT_CLASS(flashback_monitor_labeler_parent_class)->finalize(object);
}

static void
flashback_monitor_labeler_class_init(FlashbackMonitorLabelerClass* labeler_class)
{
    GObjectClass* object_class;

    object_class = G_OBJECT_CLASS(labeler_class);

    object_class->finalize = flashback_monitor_labeler_finalize;
}

static void
flashback_monitor_labeler_init(FlashbackMonitorLabeler* labeler)
{
}

FlashbackMonitorLabeler*
flashback_monitor_labeler_new(void)
{
    return g_object_new(FLASHBACK_TYPE_MONITOR_LABELER, NULL);
}

void
flashback_monitor_labeler_show(FlashbackMonitorLabeler* labeler, GDMonitorManager* monitor_manager, const gchar* sender, GVariant* params)
{
    GVariantIter iter;
    const gchar* connector;
    gint number;
    GVariant* v;
    GHashTable* monitors;
    GList* keys;
    GList* key;
    gint i;

    if (labeler->hide_id > 0) {
        g_source_remove(labeler->hide_id);
        labeler->hide_id = 0;
    }

    if (!track_client(labeler, sender)) return;

    if (labeler->windows != NULL) return;

    /*if (labeler->windows != NULL)
      {
        for (i = 0; i < labeler->n_windows; i++)
          gtk_widget_destroy (GTK_WIDGET (labeler->windows[i]));
        g_free (labeler->windows);
        labeler->windows = NULL;
      }*/

    g_variant_iter_init(&iter, params);

    monitors = g_hash_table_new(g_direct_hash, g_direct_equal);

    while (g_variant_iter_next(&iter, "{&sv}", &connector, &v)) {
        gint monitor;

        g_variant_get(v, "i", &number);

        monitor = gd_monitor_manager_get_monitor_for_connector(monitor_manager, connector);

        if (monitor != -1) {
            GSList* list;
            bool insert;

            list = (GSList*)g_hash_table_lookup(monitors, GINT_TO_POINTER(monitor));
            insert = (list == NULL);

            list = g_slist_append(list, GINT_TO_POINTER(number));

            if (insert) g_hash_table_insert(monitors, GINT_TO_POINTER(monitor), list);
        }

        g_variant_unref(v);
    }

    keys = g_hash_table_get_keys(monitors);

    labeler->n_windows = g_hash_table_size(monitors);
    labeler->windows = g_new0(GDLabelWindow *, labeler->n_windows);
    i = 0;

    for (key = keys; key; key = key->next) {
        GSList* labels;
        GSList* label;
        GString* string;
        gchar* real_label;

        labels = (GSList*)g_hash_table_lookup(monitors, key->data);
        string = g_string_new("");

        for (label = labels; label; label = label->next) g_string_append_printf(string, "%d ", GPOINTER_TO_INT(label->data));
        g_string_set_size(string, string->len - 1);

        g_slist_free(labels);

        real_label = g_string_free(string, FALSE);
        labeler->windows[i] = gd_label_window_new(GPOINTER_TO_INT(key->data), real_label);
        g_free(real_label);

        gd_label_window_show(labeler->windows[i]);

        i++;
    }

    g_list_free(keys);
    g_hash_table_destroy(monitors);
}

void
flashback_monitor_labeler_hide(FlashbackMonitorLabeler* labeler, const gchar* sender)
{
    CallbackData* data;

    data = g_new0(CallbackData, 1);
    data->labeler = g_object_ref(labeler);
    data->sender = g_strdup(sender);

    labeler->hide_id = g_timeout_add_full(G_PRIORITY_DEFAULT, 100, (GSourceFunc)hide_cb, data, (GDestroyNotify)free_callback_data);
}

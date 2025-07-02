//
// Created by dingjing on 25-7-2.
//
#include "nd-notification.h"

#include <string.h>
#include <strings.h>
#include <gtk/gtk.h>

#define ND_NOTIFICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ND_TYPE_NOTIFICATION, NdNotificationClass))
#define ND_IS_NOTIFICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ND_TYPE_NOTIFICATION))
#define ND_NOTIFICATION_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), ND_TYPE_NOTIFICATION, NdNotificationClass))

enum
{
    CHANGED,
    CLOSED,
    ACTION_INVOKED,
    LAST_SIGNAL
};

struct _NdNotification
{
    GObject         parent;

    bool            isQueued;
    gint64          updateTime;
    char*           sender;
    guint32         id;
    char*           appName;
    GIcon*          icon;
    char*           summary;
    char*           body;
    char**          actions;
    bool            transient;
    bool            resident;
    bool            actionIcons;
    int             timeout;
};

static void nd_notification_finalize(GObject* object);

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(NdNotification, nd_notification, G_TYPE_OBJECT)

static guint32 notification_serial = 1;

static guint32
get_next_notification_serial(void)
{
    guint32 serial;

    serial = notification_serial++;

    if ((gint32)notification_serial < 0) {
        notification_serial = 1;
    }

    return serial;
}

static void
start_element_cb(GMarkupParseContext* context, const gchar* element_name, const gchar** attribute_names, const gchar** attribute_values, gpointer user_data, GError** error)
{
    GString* str;
    gint i;

    if (g_strcmp0(element_name, "a") == 0) return;

    str = user_data;

    g_string_append(str, "<");
    g_string_append(str, element_name);

    for (i = 0; attribute_names[i] != NULL; i++) {
        gchar* tmp;

        tmp = g_markup_escape_text(attribute_values[i], -1);
        g_string_append_printf(str, " %s=\"%s\"", attribute_names[i], tmp);
        g_free(tmp);
    }

    g_string_append(str, ">");
}

static void
end_element_cb(GMarkupParseContext* context, const gchar* element_name, gpointer user_data, GError** error)
{
    GString* str;

    if (g_strcmp0(element_name, "a") == 0) return;

    str = user_data;

    g_string_append_printf(str, "</%s>", element_name);
}

static void
text_cb(GMarkupParseContext* context, const gchar* text, gsize text_len, gpointer user_data, GError** error)
{
    GString* str;
    gchar* tmp;

    str = user_data;

    tmp = g_markup_escape_text(text, text_len);
    g_string_append(str, tmp);
    g_free(tmp);
}

static bool
parse_markup(const gchar* text, gchar** parsed_markup, GError** error)
{
    GString* str;
    GMarkupParseContext* context;

    str = g_string_new(NULL);
    context = g_markup_parse_context_new(&(GMarkupParser){start_element_cb, end_element_cb, text_cb}, 0, str, NULL);

    if (!g_markup_parse_context_parse(context, "<markup>", -1, error)) {
        g_markup_parse_context_free(context);
        g_string_free(str, TRUE);

        return FALSE;
    }

    if (!g_markup_parse_context_parse(context, text, -1, error)) {
        g_markup_parse_context_free(context);
        g_string_free(str, TRUE);

        return FALSE;
    }

    if (!g_markup_parse_context_parse(context, "</markup>", -1, error)) {
        g_markup_parse_context_free(context);
        g_string_free(str, TRUE);

        return FALSE;
    }

    if (!g_markup_parse_context_end_parse(context, error)) {
        g_markup_parse_context_free(context);
        g_string_free(str, TRUE);

        return FALSE;
    }

    *parsed_markup = g_string_free(str, FALSE);
    g_markup_parse_context_free(context);

    return TRUE;
}

static void
free_pixels(guchar* pixels, gpointer user_data)
{
    g_free(pixels);
}

static GIcon*
icon_from_data(GVariant* icon_data)
{
    bool has_alpha;
    int bits_per_sample;
    int width;
    int height;
    int rowstride;
    int n_channels;
    GVariant* data_variant;
    gsize expected_len;
    guchar* data;
    GdkPixbuf* pixbuf;

    g_variant_get(icon_data, "(iiibii@ay)", &width, &height, &rowstride, &has_alpha, &bits_per_sample, &n_channels, &data_variant);

    expected_len = (height - 1) * rowstride + width * ((n_channels * bits_per_sample + 7) / 8);

    if (expected_len != g_variant_get_size(data_variant)) {
        g_warning("Expected image data to be of length %" G_GSIZE_FORMAT " but got a " "length of %" G_GSIZE_FORMAT, expected_len, g_variant_get_size (data_variant));
        return NULL;
    }

    data = g_memdup2(g_variant_get_data(data_variant), g_variant_get_size(data_variant));

    pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, has_alpha, bits_per_sample, width, height, rowstride, free_pixels, NULL);

    return G_ICON(pixbuf);
}

static GIcon*
icon_from_path(const char* path)
{
    GFile* file;
    GIcon* icon;

    if (path == NULL || *path == '\0') return NULL;

    if (g_str_has_prefix(path, "file://")) file = g_file_new_for_uri(path);
    else if (*path == '/') file = g_file_new_for_path(path);
    else file = NULL;

    if (file != NULL) {
        icon = g_file_icon_new(file);
        g_object_unref(file);
    }
    else {
        icon = g_themed_icon_new(path);
    }

    return icon;
}

static void
update_icon(NdNotification* notification, const gchar* app_icon, GVariantDict* hints)
{
    GIcon* icon;
    GVariant* image_data;
    const char* image_path;

    if (g_variant_dict_lookup(hints, "image-data", "@(iiibiiay)", &image_data) || g_variant_dict_lookup(hints, "image_data", "@(iiibiiay)", &image_data)) {
        icon = icon_from_data(image_data);
    }
    else if (g_variant_dict_lookup(hints, "image-path", "&s", &image_path) || g_variant_dict_lookup(hints, "image_path", "&s", &image_path)) {
        icon = icon_from_path(image_path);
    }
    else if (*app_icon != '\0') {
        icon = icon_from_path(app_icon);
    }
    else if (g_variant_dict_lookup(hints, "icon_data", "v", &image_data)) {
        icon = icon_from_data(image_data);
    }
    else {
        icon = NULL;
    }

    g_clear_object(&notification->icon);
    notification->icon = icon;
}

static void
nd_notification_class_init(NdNotificationClass* class)
{
    GObjectClass* gobject_class;

    gobject_class = G_OBJECT_CLASS(class);

    gobject_class->finalize = nd_notification_finalize;

    signals[CHANGED] = g_signal_new("changed", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
    signals[CLOSED] = g_signal_new("closed", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__INT, G_TYPE_NONE, 1, G_TYPE_INT);
    signals[ACTION_INVOKED] = g_signal_new("action-invoked", G_TYPE_FROM_CLASS(class), G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
nd_notification_init(NdNotification* notification)
{
    notification->id = get_next_notification_serial();

    notification->appName = NULL;
    notification->icon = NULL;
    notification->summary = NULL;
    notification->body = NULL;
    notification->actions = NULL;
    notification->transient = FALSE;
    notification->resident = FALSE;
    notification->actionIcons = FALSE;
}

static void
nd_notification_finalize(GObject* object)
{
    NdNotification* notification;

    notification = ND_NOTIFICATION(object);

    g_free(notification->sender);
    g_free(notification->appName);
    g_clear_object(&notification->icon);
    g_free(notification->summary);
    g_free(notification->body);
    g_strfreev(notification->actions);

    if (G_OBJECT_CLASS(nd_notification_parent_class)->finalize) (*G_OBJECT_CLASS(nd_notification_parent_class)->finalize)(object);
}

bool
nd_notification_update(NdNotification* notification, const gchar* app_name, const gchar* app_icon, const gchar* summary, const gchar* body, const gchar* const * actions, GVariant* hints, gint timeout)
{
    GVariantDict dict;

    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), FALSE);

    g_free(notification->appName);
    notification->appName = g_strdup(app_name);

    g_free(notification->summary);
    notification->summary = g_strdup(summary);

    g_free(notification->body);
    notification->body = g_strdup(body);

    g_strfreev(notification->actions);
    notification->actions = g_strdupv((char**)actions);

    g_variant_dict_init(&dict, hints);

    update_icon(notification, app_icon, &dict);

    if (!g_variant_dict_lookup(&dict, "transient", "b", &notification->transient)) notification->transient = FALSE;

    if (!g_variant_dict_lookup(&dict, "resident", "b", &notification->resident)) notification->resident = FALSE;

    if (!g_variant_dict_lookup(&dict, "action-icons", "b", &notification->actionIcons)) notification->actionIcons = FALSE;

    notification->timeout = timeout;

    g_signal_emit(notification, signals[CHANGED], 0);

    notification->updateTime = g_get_real_time();

    return TRUE;
}

gint64
nd_notification_get_update_time(NdNotification* notification)
{
    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), 0);

    return notification->updateTime;
}

void
nd_notification_set_is_queued(NdNotification* notification, bool is_queued)
{
    g_return_if_fail(ND_IS_NOTIFICATION (notification));

    notification->isQueued = is_queued;
}

bool
nd_notification_get_is_queued(NdNotification* notification)
{
    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), FALSE);

    return notification->isQueued;
}

bool
nd_notification_get_is_transient(NdNotification* notification)
{
    return notification->transient;
}

bool
nd_notification_get_is_resident(NdNotification* notification)
{
    return notification->resident;
}

bool
nd_notification_get_action_icons(NdNotification* notification)
{
    return notification->actionIcons;
}

guint32
nd_notification_get_id(NdNotification* notification)
{
    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), -1);

    return notification->id;
}

char**
nd_notification_get_actions(NdNotification* notification)
{
    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), NULL);

    return notification->actions;
}

const char*
nd_notification_get_sender(NdNotification* notification)
{
    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), NULL);

    return notification->sender;
}

const char*
nd_notification_get_summary(NdNotification* notification)
{
    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), NULL);

    return notification->summary;
}

const char*
nd_notification_get_body(NdNotification* notification)
{
    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), NULL);

    return notification->body;
}

int
nd_notification_get_timeout(NdNotification* notification)
{
    g_return_val_if_fail(ND_IS_NOTIFICATION (notification), -1);

    return notification->timeout;
}

GIcon*
nd_notification_get_icon(NdNotification* notification)
{
    return notification->icon;
}

void
nd_notification_close(NdNotification* notification, NdNotificationClosedReason reason)
{
    g_return_if_fail(ND_IS_NOTIFICATION (notification));

    g_object_ref(notification);
    g_signal_emit(notification, signals[CLOSED], 0, reason);
    g_object_unref(notification);
}

void
nd_notification_action_invoked(NdNotification* notification, const char* action)
{
    g_return_if_fail(ND_IS_NOTIFICATION (notification));

    g_object_ref(notification);
    g_signal_emit(notification, signals[ACTION_INVOKED], 0, action);
    g_object_unref(notification);
}

NdNotification*
nd_notification_new(const char* sender)
{
    g_info("nd notification new\n");
    NdNotification* notification = (NdNotification*)g_object_new(ND_TYPE_NOTIFICATION, NULL);
    notification->sender = g_strdup(sender);

    return notification;
}

bool
validate_markup(const gchar* markup)
{
    GError* error = NULL;
    gchar* parsedMarkup = NULL;

    if (!parse_markup(markup, &parsedMarkup, &error)) {
        g_warning("%s", error->message);
        g_error_free(error);

        return FALSE;
    }

    if (!pango_parse_markup(parsedMarkup, -1, 0, NULL, NULL, NULL, &error)) {
        g_warning("%s", error->message);
        g_error_free(error);

        g_free(parsedMarkup);

        return FALSE;
    }

    g_free(parsedMarkup);

    return TRUE;
}

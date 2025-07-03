//
// Created by dingjing on 25-7-2.
//
#include "gd-bubble.h"

#include <string.h>
#include <strings.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "nd-notification.h"

#define EXPIRATION_TIME_DEFAULT         -1
#define EXPIRATION_TIME_NEVER_EXPIRES    0
#define TIMEOUT_SEC                      5
#define WIDTH                            400
#define IMAGE_SIZE                       48
#define BODY_X_OFFSET                   (IMAGE_SIZE + 8)

typedef struct
{
    NdNotification*         notification;

    GtkWidget*              icon;
    GtkWidget*              contentHbox;
    GtkWidget*              summaryLabel;
    GtkWidget*              closeButton;
    GtkWidget*              bodyLabel;
    GtkWidget*              actionsBox;
    bool                    urlClickedLock;
    guint                   timeoutId;
    gulong                  changedId;
    gulong                  closedId;
} GDBubblePrivate;

G_DEFINE_TYPE_WITH_PRIVATE(GDBubble, gd_bubble, GD_TYPE_POPUP_WINDOW)

static void close_button_clicked_cb(GtkButton* button, GDBubble* bubble)
{
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    nd_notification_close(priv->notification, ND_NOTIFICATION_CLOSED_USER);
    gtk_widget_destroy(GTK_WIDGET(bubble));
}

static bool activate_link_cb(GtkLabel* label, gchar* uri, GDBubble* bubble)
{
    GError* error;

    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    priv->urlClickedLock = TRUE;

    error = NULL;
    if (!gtk_show_uri_on_window(GTK_WINDOW(bubble), uri, gtk_get_current_event_time(), &error)) {
        g_warning("Could not show link: %s", error->message);
        g_error_free(error);
    }

    return TRUE;
}

static void button_release_event_cb(GtkButton* button, GdkEventButton* event, GDBubble* bubble)
{
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    const gchar* key = g_object_get_data(G_OBJECT(button), "_action_key");
    nd_notification_action_invoked(priv->notification, key);

    bool transient = nd_notification_get_is_transient(priv->notification);
    bool resident = nd_notification_get_is_resident(priv->notification);

    if (transient || !resident) gtk_widget_destroy(GTK_WIDGET(bubble));
}

static void add_notification_action(GDBubble* bubble, const gchar* text, const gchar* key)
{
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    GtkWidget* button = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(priv->actionsBox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);

    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_container_set_border_width(GTK_CONTAINER(button), 0);

    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_container_add(GTK_CONTAINER(button), hbox);
    gtk_widget_show(hbox);

    GdkPixbuf* pixBuf = NULL;
    if (nd_notification_get_action_icons(priv->notification))
        pixBuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), key, 20, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);

    if (pixBuf != NULL) {
        GtkWidget* image = gtk_image_new_from_pixbuf(pixBuf);
        gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
        gtk_widget_show(image);

        gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(image, GTK_ALIGN_CENTER);

        AtkObject* atkObj = gtk_widget_get_accessible(GTK_WIDGET(button));
        atk_object_set_name(atkObj, text);

        g_object_unref(pixBuf);
    }
    else {
        GtkWidget* label = gtk_label_new(NULL);
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
        gtk_widget_show(label);

        gtk_label_set_xalign(GTK_LABEL(label), 0.0);

        gchar* buf = g_strdup_printf("<small>%s</small>", text);
        gtk_label_set_markup(GTK_LABEL(label), buf);
        g_free(buf);
    }

    g_object_set_data_full(G_OBJECT(button), "_action_key", g_strdup(key), g_free);

    g_signal_connect(button, "button-release-event", G_CALLBACK (button_release_event_cb), bubble);

    gtk_widget_show(priv->actionsBox);
}

static bool timeout_bubble(gpointer userData)
{
    GDBubble* bubble = GD_BUBBLE(userData);
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);
    priv->timeoutId = 0;

    gtk_widget_destroy(GTK_WIDGET(bubble));

    return G_SOURCE_REMOVE;
}

static void add_timeout(GDBubble* bubble)
{
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    if (priv->timeoutId != 0) {
        g_source_remove(priv->timeoutId);
        priv->timeoutId = 0;
    }

    gint timeout = nd_notification_get_timeout(priv->notification);

    if (timeout == EXPIRATION_TIME_NEVER_EXPIRES) return;

    if (timeout == EXPIRATION_TIME_DEFAULT) timeout = TIMEOUT_SEC * 1000;

    priv->timeoutId = g_timeout_add(timeout, (void*) timeout_bubble, bubble);
}

static void destroy_widget(GtkWidget* widget, gpointer uData)
{
    gtk_widget_destroy(widget);
}

static void update_bubble(GDBubble* bubble)
{
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    gtk_widget_show_all(GTK_WIDGET(bubble));

    /* Summary label */
    const gchar* summary = nd_notification_get_summary(priv->notification);
    gchar* quoted = g_markup_escape_text(summary, -1);
    gchar* str = g_strdup_printf("<b><big>%s</big></b>", quoted);
    g_free(quoted);

    gtk_label_set_markup(GTK_LABEL(priv->summaryLabel), str);
    g_free(str);

    /* Body label */
    const gchar* body = nd_notification_get_body(priv->notification);
    if (validate_markup(body)) {
        gtk_label_set_markup(GTK_LABEL(priv->bodyLabel), body);
    }
    else {
        str = g_markup_escape_text(body, -1);
        gtk_label_set_text(GTK_LABEL(priv->bodyLabel), str);
        g_free(str);
    }

    bool haveBody = body && *body != '\0';
    gtk_widget_set_visible(priv->bodyLabel, haveBody);

    /* Actions */
    bool haveActions = FALSE;
    gtk_container_foreach(GTK_CONTAINER(priv->actionsBox), destroy_widget, NULL);

    gchar** actions = nd_notification_get_actions(priv->notification);

    gint i = 0;
    for (i = 0; actions[i] != NULL; i += 2) {
        gchar* l = actions[i + 1];
        if (l == NULL) {
            g_warning("Label not found for action - %s. The protocol specifies " "that a label must follow an action in the actions array", actions[i]);
        }
        else if (strcasecmp(actions[i], "default") != 0) {
            haveActions = TRUE;
            add_notification_action(bubble, l, actions[i]);
        }
    }

    gtk_widget_set_visible(priv->actionsBox, haveActions);

    /* Icon */
    GIcon* icon = nd_notification_get_icon(priv->notification);
    bool haveIcon = FALSE;

    if (icon != NULL) {
        gtk_image_set_from_gicon(GTK_IMAGE(priv->icon), icon, GTK_ICON_SIZE_DIALOG);
        gtk_image_set_pixel_size(GTK_IMAGE(priv->icon), IMAGE_SIZE);

        haveIcon = TRUE;
        gtk_widget_set_visible(priv->icon, haveIcon);
        gtk_widget_set_size_request(priv->icon, BODY_X_OFFSET, -1);
    }

    if (haveBody || haveActions || haveIcon) gtk_widget_show(priv->contentHbox);
    else gtk_widget_hide(priv->contentHbox);

    add_timeout(bubble);
}

static void notification_changed_cb(NdNotification* notification, GDBubble* bubble)
{
    g_info("[notify] notification change");
    update_bubble(bubble);
}

static void notification_closed_cb(NdNotification* notification, gint reason, GDBubble* bubble)
{
    g_info("[notify] notification closed");

    if (reason == ND_NOTIFICATION_CLOSED_API) {
        gtk_widget_destroy(GTK_WIDGET(bubble));
    }
}

static void gd_bubble_dispose(GObject* object)
{
    GDBubble* bubble = GD_BUBBLE(object);
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    if (priv->timeoutId != 0) {
        g_source_remove(priv->timeoutId);
        priv->timeoutId = 0;
    }

    if (priv->changedId != 0) {
        g_signal_handler_disconnect(priv->notification, priv->changedId);
        priv->changedId = 0;
    }

    if (priv->closedId != 0) {
        g_signal_handler_disconnect(priv->notification, priv->closedId);
        priv->closedId = 0;
    }

    G_OBJECT_CLASS(gd_bubble_parent_class)->dispose(object);
}

static void gd_bubble_finalize(GObject* object)
{
    GDBubble* bubble = GD_BUBBLE(object);
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    g_clear_object(&priv->notification);

    G_OBJECT_CLASS(gd_bubble_parent_class)->finalize(object);
}

static bool gd_bubble_button_release_event(GtkWidget* widget, GdkEventButton* event)
{
    GDBubble* bubble = GD_BUBBLE(widget);
    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    const bool retVal = GTK_WIDGET_CLASS(gd_bubble_parent_class)->button_release_event(widget, event);

    if (priv->urlClickedLock) {
        priv->urlClickedLock = FALSE;
        return retVal;
    }

    nd_notification_action_invoked(priv->notification, "default");
    gtk_widget_destroy(widget);

    return retVal;
}

static void gd_bubble_get_preferred_width(GtkWidget* widget, gint* min_width, gint* nat_width)
{
    GTK_WIDGET_CLASS(gd_bubble_parent_class)->get_preferred_width(widget, min_width, nat_width);

    *nat_width = WIDTH;
}

static bool gd_bubble_motion_notify_event(GtkWidget* widget, GdkEventMotion* event)
{
    GDBubble* bubble = GD_BUBBLE(widget);

    const bool retval = GTK_WIDGET_CLASS(gd_bubble_parent_class)->motion_notify_event(widget, event);

    add_timeout(bubble);

    return retval;
}

static void gd_bubble_realize(GtkWidget* widget)
{
    GDBubble* bubble = GD_BUBBLE(widget);

    GTK_WIDGET_CLASS(gd_bubble_parent_class)->realize(widget);

    add_timeout(bubble);
}

static void gd_bubble_class_init(GDBubbleClass* bubble_class)
{
    GObjectClass* object_class;
    GtkWidgetClass* widget_class;

    object_class = G_OBJECT_CLASS(bubble_class);
    widget_class = GTK_WIDGET_CLASS(bubble_class);

    object_class->dispose = gd_bubble_dispose;
    object_class->finalize = gd_bubble_finalize;

    widget_class->button_release_event = (void*) gd_bubble_button_release_event;
    widget_class->get_preferred_width = (void*) gd_bubble_get_preferred_width;
    widget_class->motion_notify_event = (void*) gd_bubble_motion_notify_event;
    widget_class->realize = gd_bubble_realize;
}

static void gd_bubble_init(GDBubble* bubble)
{
    GDBubblePrivate* priv;
    GtkWidget* widget;
    AtkObject* atkobj;
    gint events;
    GtkWidget* main_vbox;
    GtkWidget* main_hbox;
    GtkWidget* vbox;
    GtkWidget* image;

    priv = gd_bubble_get_instance_private(bubble);

    widget = GTK_WIDGET(bubble);
    atkobj = gtk_widget_get_accessible(widget);

    atk_object_set_role(atkobj, ATK_ROLE_ALERT);

    events = GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK;

    gtk_widget_add_events(widget, events);
    gtk_widget_set_name(widget, "gf-bubble");

    /* Main vbox */

    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(bubble), main_vbox);
    gtk_widget_show(main_vbox);

    gtk_container_set_border_width(GTK_CONTAINER(main_vbox), 12);

    /* Main hbox */

    main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(main_vbox), main_hbox, FALSE, FALSE, 0);
    gtk_widget_show(main_hbox);

    /* Icon */

    priv->icon = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(main_hbox), priv->icon, FALSE, FALSE, 0);
    gtk_widget_show(priv->icon);

    gtk_widget_set_margin_top(priv->icon, 5);
    gtk_widget_set_size_request(priv->icon, BODY_X_OFFSET, -1);
    gtk_widget_set_valign(priv->icon, GTK_ALIGN_START);

    /* Vbox */

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(main_hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show(vbox);

    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    /* Close button */

    priv->closeButton = gtk_button_new();
    gtk_box_pack_start(GTK_BOX(main_hbox), priv->closeButton, FALSE, FALSE, 0);
    gtk_widget_show(priv->closeButton);

    gtk_button_set_relief(GTK_BUTTON(priv->closeButton), GTK_RELIEF_NONE);
    gtk_widget_set_valign(priv->closeButton, GTK_ALIGN_START);

    g_signal_connect(priv->closeButton, "clicked", G_CALLBACK (close_button_clicked_cb), bubble);

    atkobj = gtk_widget_get_accessible(priv->closeButton);
    atk_object_set_description(atkobj, _("Closes the notification."));
    atk_object_set_name(atkobj, "");

    atk_action_set_description(ATK_ACTION(atkobj), 0, _("Closes the notification."));

    image = gtk_image_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
    gtk_container_add(GTK_CONTAINER(priv->closeButton), image);
    gtk_widget_show(image);

    /* Summary label */

    priv->summaryLabel = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(vbox), priv->summaryLabel, TRUE, TRUE, 0);
    gtk_widget_show(priv->summaryLabel);

    gtk_label_set_line_wrap(GTK_LABEL(priv->summaryLabel), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(priv->summaryLabel), PANGO_WRAP_WORD_CHAR);

    gtk_label_set_xalign(GTK_LABEL(priv->summaryLabel), 0.0);
    gtk_label_set_yalign(GTK_LABEL(priv->summaryLabel), 0.0);

    atkobj = gtk_widget_get_accessible(priv->summaryLabel);
    atk_object_set_description(atkobj, _("Notification summary text."));

    /* Content hbox */

    priv->contentHbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), priv->contentHbox, FALSE, FALSE, 0);
    gtk_widget_show(priv->contentHbox);

    /* Vbox */

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_box_pack_start(GTK_BOX(priv->contentHbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show(vbox);

    /* Body label */

    priv->bodyLabel = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(vbox), priv->bodyLabel, TRUE, TRUE, 0);
    gtk_widget_show(priv->bodyLabel);

    gtk_label_set_line_wrap(GTK_LABEL(priv->bodyLabel), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(priv->bodyLabel), PANGO_WRAP_WORD_CHAR);

    gtk_label_set_xalign(GTK_LABEL(priv->bodyLabel), 0.0);
    gtk_label_set_yalign(GTK_LABEL(priv->bodyLabel), 0.0);

    g_signal_connect(priv->bodyLabel, "activate-link", G_CALLBACK (activate_link_cb), bubble);

    atkobj = gtk_widget_get_accessible(priv->bodyLabel);
    atk_object_set_description(atkobj, _("Notification summary text."));

    /* Actions box */

    priv->actionsBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), priv->actionsBox, FALSE, TRUE, 0);
    gtk_widget_show(priv->actionsBox);

    gtk_widget_set_halign(priv->actionsBox, GTK_ALIGN_END);
}

GDBubble* gd_bubble_new_for_notification(NdNotification* notification)
{
    GDBubble* bubble = g_object_new(GD_TYPE_BUBBLE, "resizable", FALSE, "title", _("Notification"), "type", GTK_WINDOW_POPUP, NULL);

    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    priv->notification = g_object_ref(notification);
    priv->changedId = g_signal_connect(notification, "changed", G_CALLBACK (notification_changed_cb), bubble);
    priv->closedId = g_signal_connect(notification, "closed", G_CALLBACK (notification_closed_cb), bubble);

    update_bubble(bubble);

    return bubble;
}

NdNotification* gd_bubble_get_notification(GDBubble* bubble)
{
    g_return_val_if_fail(GD_IS_BUBBLE (bubble), NULL);

    GDBubblePrivate* priv = gd_bubble_get_instance_private(bubble);

    return priv->notification;
}

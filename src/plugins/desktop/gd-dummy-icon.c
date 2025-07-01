//
// Created by dingjing on 25-6-30.
//
#include "gd-dummy-icon.h"

struct _GDDummyIcon
{
    GDIcon parent;

    guint size_id;

    int width;
    int height;
};

enum { SIZE_CHANGED, LAST_SIGNAL };

static guint icon_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(GDDummyIcon, gd_dummy_icon, GD_TYPE_ICON)

static bool size_cb(gpointer user_data)
{
    GDDummyIcon * self;
    GtkRequisition icon_size;

    self = GD_DUMMY_ICON(user_data);

    gtk_widget_get_preferred_size(GTK_WIDGET(self), &icon_size, NULL);

    if (self->width != icon_size.width || self->height != icon_size.height) {
        self->width = icon_size.width;
        self->height = icon_size.height;

        g_signal_emit(self, icon_signals[SIZE_CHANGED], 0);
    }

    self->size_id = 0;

    return G_SOURCE_REMOVE;
}

static void
recalculate_size(GDDummyIcon* self)
{
    if (self->size_id != 0) return;

    self->size_id = g_idle_add((void*)size_cb, self);
    g_source_set_name_by_id(self->size_id, "[gnome-flashback] size_cb");
}

static void
icon_size_cb(GdkMonitor* monitor, GParamSpec* pspec, GDDummyIcon* self)
{
    recalculate_size(self);
}

static void
extra_text_width_cb(GdkMonitor* monitor, GParamSpec* pspec, GDDummyIcon* self)
{
    recalculate_size(self);
}

static void
gd_dummy_icon_dispose(GObject* object)
{
    GDDummyIcon* self;

    self = GD_DUMMY_ICON(object);

    if (self->size_id != 0) {
        g_source_remove(self->size_id);
        self->size_id = 0;
    }

    G_OBJECT_CLASS(gd_dummy_icon_parent_class)->dispose(object);
}

static void
gd_dummy_icon_style_updated(GtkWidget* widget)
{
    GTK_WIDGET_CLASS(gd_dummy_icon_parent_class)->style_updated(widget);
    recalculate_size(GD_DUMMY_ICON(widget));
}

static void
install_signals(void)
{
    icon_signals[SIZE_CHANGED] = g_signal_new("size-changed", GD_TYPE_DUMMY_ICON, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
gd_dummy_icon_class_init(GDDummyIconClass* self_class)
{
    GObjectClass* object_class;
    GtkWidgetClass* widget_class;

    object_class = G_OBJECT_CLASS(self_class);
    widget_class = GTK_WIDGET_CLASS(self_class);

    object_class->dispose = gd_dummy_icon_dispose;

    widget_class->style_updated = gd_dummy_icon_style_updated;

    install_signals();
}

static void
gd_dummy_icon_init(GDDummyIcon* self)
{
    g_signal_connect(self, "notify::icon-size", G_CALLBACK(icon_size_cb), self);

    g_signal_connect(self, "notify::extra-text-width", G_CALLBACK(extra_text_width_cb), self);
}

GtkWidget*
gd_dummy_icon_new(GDIconView* icon_view)
{
    GFile* file;
    GFileInfo* info;
    GIcon* icon;
    const char* name;
    GtkWidget* widget;

    file = g_file_new_for_commandline_arg("");
    info = g_file_info_new();

    icon = g_icon_new_for_string("text-x-generic", NULL);
    g_file_info_set_icon(info, icon);
    g_object_unref(icon);

    name = "Lorem Ipsum is simply dummy text of the printing and typesetting " "industry. Lorem Ipsum has been the industry's standard dummy text " "ever since the 1500s, when an unknown printer took a galley of "
        "type and scrambled it to make a type specimen book.";

    g_file_info_set_display_name(info, name);

    widget = g_object_new(GD_TYPE_DUMMY_ICON, "icon-view", icon_view, "file", file, "info", info, NULL);

    g_object_unref(file);
    g_object_unref(info);

    return widget;
}

int
gd_dummy_icon_get_width(GDDummyIcon* self)
{
    return self->width;
}

int
gd_dummy_icon_get_height(GDDummyIcon* self)
{
    return self->height;
}

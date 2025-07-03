//
// Created by dingjing on 25-7-3.
//
#include <math.h>
#include <gdk/gdk.h>
#include "gd-label-window.h"

#define HIDE_TIMEOUT 1500

struct _GDLabelWindow
{
    GDPopupWindow parent;

    GdkRectangle monitor;

    guint hide_timeout_id;

    GtkWidget* label;
};

G_DEFINE_TYPE(GDLabelWindow, gd_label_window, GD_TYPE_POPUP_WINDOW)

static void
fade_finished_cb(GDPopupWindow* window)
{
    gtk_widget_destroy(GTK_WIDGET(window));
}

static void
gd_label_window_realize(GtkWidget* widget)
{
    cairo_region_t* region;

    GTK_WIDGET_CLASS(gd_label_window_parent_class)->realize(widget);

    region = cairo_region_create();
    gtk_widget_input_shape_combine_region(widget, region);
    cairo_region_destroy(region);
}

static void
gd_label_window_class_init(GDLabelWindowClass* window_class)
{
    GtkWidgetClass* widget_class;

    widget_class = GTK_WIDGET_CLASS(window_class);

    widget_class->realize = gd_label_window_realize;
}

static void
gd_label_window_init(GDLabelWindow* window)
{
    GtkWidget* box;

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    gtk_container_add(GTK_CONTAINER(window), box);
    gtk_widget_show(box);

    window->label = gtk_label_new("");
    gtk_widget_set_halign(window->label, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(window->label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(box), window->label, TRUE, FALSE, 0);
    gtk_widget_show(window->label);

    g_signal_connect(window, "fade-finished", G_CALLBACK (fade_finished_cb), NULL);

    gtk_widget_set_name(GTK_WIDGET(window), "gf-label-window");
}

GDLabelWindow*
gd_label_window_new(gint monitor, const gchar* label)
{
    GDLabelWindow* window;
    GdkDisplay* display;
    GdkMonitor* gdk_monitor;
    gint width;
    gint height;
    gint size;

    display = gdk_display_get_default();
    window = g_object_new(GD_TYPE_LABEL_WINDOW, "type", GTK_WINDOW_POPUP, NULL);

    gdk_monitor = gdk_display_get_monitor(display, monitor);
    gdk_monitor_get_workarea(gdk_monitor, &window->monitor);

    width = window->monitor.width;
    height = window->monitor.height;
    size = 60 * MAX(1, MIN (width / 640.0, height / 480.0));

    gtk_window_resize(GTK_WINDOW(window), size, size);

    gtk_label_set_text(GTK_LABEL(window->label), label);

    return window;
}

void
gd_label_window_show(GDLabelWindow* window)
{
    gint width;
    gint height;
    GdkRectangle rect;
    GtkTextDirection dir;
    gint x;
    gint y;

    gtk_window_get_size(GTK_WINDOW(window), &width, &height);

    rect = window->monitor;
    dir = gtk_widget_get_direction(GTK_WIDGET(window));

    if (dir == GTK_TEXT_DIR_NONE) dir = gtk_widget_get_default_direction();

    if (dir == GTK_TEXT_DIR_RTL) x = rect.x + (rect.width - width - 20);
    else x = rect.x + 20;
    y = rect.y + 20;

    gtk_window_move(GTK_WINDOW(window), x, y);
    gtk_widget_show(GTK_WIDGET(window));

    gd_popup_window_fade_cancel(GD_POPUP_WINDOW(window));
}

void
gd_label_window_hide(GDLabelWindow* window)
{
    gd_popup_window_fade_start(GD_POPUP_WINDOW(window));
}

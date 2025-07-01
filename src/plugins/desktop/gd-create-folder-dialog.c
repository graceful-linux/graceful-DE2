//
// Created by dingjing on 25-6-30.
//
#include "gd-create-folder-dialog.h"

#include <glib/gi18n.h>

struct _GDCreateFolderDialog
{
    GtkDialog parent;

    GtkWidget* create_button;

    GtkWidget* name_entry;

    GtkWidget* error_revealer;
    GtkWidget* error_label;
};

enum { VALIDATE, LAST_SIGNAL };

static guint dialog_signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE(GDCreateFolderDialog, gd_create_folder_dialog, GTK_TYPE_DIALOG)

static void validate(GDCreateFolderDialog* self)
{
    const char* text;

    text = gtk_entry_get_text(GTK_ENTRY(self->name_entry));
    g_signal_emit(self, dialog_signals[VALIDATE], 0, text);
}

static void
cancel_clicked_cb(GtkWidget* widget, GDCreateFolderDialog* self)
{
    gtk_widget_destroy(GTK_WIDGET(self));
}

static void
create_clicked_cb(GtkWidget* widget, GDCreateFolderDialog* self)
{
    validate(self);

    if (!gtk_widget_get_sensitive(self->create_button)) return;

    gtk_dialog_response(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT);
}

static void
name_changed_cb(GtkEditable* editable, GDCreateFolderDialog* self)
{
    validate(self);
}

static void
name_activate_cb(GtkWidget* widget, GDCreateFolderDialog* self)
{
    validate(self);

    if (!gtk_widget_get_sensitive(self->create_button)) return;

    gtk_dialog_response(GTK_DIALOG(self), GTK_RESPONSE_ACCEPT);
}

static void
setup_header_bar(GDCreateFolderDialog* self)
{
    GtkWidget* header_bar;
    GtkWidget* cancel_button;
    GtkStyleContext* style;

    header_bar = gtk_dialog_get_header_bar(GTK_DIALOG(self));
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), FALSE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), _("New Folder"));

    cancel_button = gtk_button_new_with_label(_("Cancel"));
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), cancel_button);
    gtk_widget_show(cancel_button);

    g_signal_connect(cancel_button, "clicked", G_CALLBACK(cancel_clicked_cb), self);

    self->create_button = gtk_button_new_with_label(_("Create"));
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), self->create_button);
    gtk_widget_show(self->create_button);

    style = gtk_widget_get_style_context(self->create_button);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_DEFAULT);

    gtk_widget_set_sensitive(self->create_button, FALSE);

    g_signal_connect(self->create_button, "clicked", G_CALLBACK(create_clicked_cb), self);
}

static void
setup_content_area(GDCreateFolderDialog* self)
{
    GtkWidget* content;
    GtkWidget* label;

    content = gtk_dialog_get_content_area(GTK_DIALOG(self));

    g_object_set(content, "margin", 18, "margin-bottom", 12, "spacing", 6, NULL);

    label = gtk_label_new(_("Folder name"));
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_container_add(GTK_CONTAINER(content), label);
    gtk_widget_show(label);

    self->name_entry = gtk_entry_new();
    gtk_container_add(GTK_CONTAINER(content), self->name_entry);
    gtk_widget_show(self->name_entry);

    g_signal_connect(self->name_entry, "changed", G_CALLBACK(name_changed_cb), self);

    g_signal_connect(self->name_entry, "activate", G_CALLBACK(name_activate_cb), self);

    self->error_revealer = gtk_revealer_new();
    gtk_container_add(GTK_CONTAINER(content), self->error_revealer);
    gtk_widget_show(self->error_revealer);

    self->error_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(self->error_label), 0.0);
    gtk_container_add(GTK_CONTAINER(self->error_revealer), self->error_label);
    gtk_widget_show(self->error_label);
}

static void
install_signals(void)
{
    dialog_signals[VALIDATE] = g_signal_new("validate", GD_TYPE_CREATE_FOLDER_DIALOG, G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
gd_create_folder_dialog_class_init(GDCreateFolderDialogClass* self_class)
{
    install_signals();
}

static void
gd_create_folder_dialog_init(GDCreateFolderDialog* self)
{
    setup_header_bar(self);
    setup_content_area(self);
}

GtkWidget*
gd_create_folder_dialog_new(void)
{
    return g_object_new(GD_TYPE_CREATE_FOLDER_DIALOG, "use-header-bar", TRUE, "width-request", 450, "resizable", FALSE, NULL);
}

void
gd_create_folder_dialog_set_valid(GDCreateFolderDialog* self, bool valid, const char* message)
{
    GtkRevealer* revealer;

    revealer = GTK_REVEALER(self->error_revealer);

    gtk_label_set_text(GTK_LABEL(self->error_label), message);
    gtk_revealer_set_reveal_child(revealer, message != NULL);

    gtk_widget_set_sensitive(self->create_button, valid);
}

char*
gd_create_folder_dialog_get_folder_name(GDCreateFolderDialog* self)
{
    const char* text;
    char* folder_name;

    text = gtk_entry_get_text(GTK_ENTRY(self->name_entry));
    folder_name = g_strdup(text);

    return g_strstrip(folder_name);
}

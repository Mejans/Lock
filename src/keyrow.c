#include "keyrow.h"

#include <adwaita.h>
#include <glib/gi18n.h>
#include <locale.h>
#include "window.h"
#include "keydialog.h"
#include "config.h"

#include <gpgme.h>
#include "cryptography.h"
#include "threading.h"

/**
 * This structure handles data of a key row.
 */
struct _LockKeyRow {
    AdwActionRow parent;

    LockKeyDialog *dialog;

    const gchar *email;

    gboolean export_success;
    GtkButton *export_button;
    GFile *export_file;
};

G_DEFINE_TYPE(LockKeyRow, lock_key_row, ADW_TYPE_ACTION_ROW);

/* Export */
static void lock_key_row_export_file_present(GtkButton * self,
                                             LockKeyRow * row);
gboolean lock_key_row_export_on_completed(LockKeyRow * row);

/**
 * This function initializes a LockKeyRow.
 *
 * @param row Row to be initialized
 */
static void lock_key_row_init(LockKeyRow *row)
{
    gtk_widget_init_template(GTK_WIDGET(row));

    g_signal_connect(row->export_button, "clicked",
                     G_CALLBACK(lock_key_row_export_file_present), row);
}

/**
 * This function initializes a LockKeyRow class.
 *
 * @param class Row class to be initialized
 */
static void lock_key_row_class_init(LockKeyRowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class),
                                                UI_RESOURCE("keyrow.ui"));

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), LockKeyRow,
                                         export_button);
}

/**
 * This function creates a new LockKeyRow.
 *
 * @param dialog Dialog in which the row is presented
 * @param email Email of the key
 * @param title UID of the key
 * @param subtitle Fingerprint of the key
 * @param expiry_date Date of the expiry of the key
 * @param expiry_time Time of the day of the expiry of the key
 *
 * @return LockKeyRow
 */
LockKeyRow *lock_key_row_new(LockKeyDialog *dialog, const gchar *email,
                             const gchar *title, const gchar *subtitle,
                             const gchar *expiry_date, const gchar *expiry_time)
{
    gchar *tooltip_text;
    if (expiry_date == NULL || expiry_time == NULL) {
        tooltip_text = _("Key does not expire");
    } else {
        tooltip_text = g_strdup_printf(C_
                                       ("First formatter: YYYY-mm-dd; Second formatter: HH:MM",
                                        "Expires %s at %s"), expiry_date,
                                       expiry_time);
    }

    LockKeyRow *row =
        g_object_new(LOCK_TYPE_KEY_ROW, "title", title, "subtitle", subtitle,
                     "tooltip-text", tooltip_text, NULL);

    /* TODO: implement g_object_class_install_property() */
    row->dialog = dialog;
    row->email = email;

    return row;
}

/**** Export ****/

/**
 * This function opens the export file of a LockKeyRow.
 *
 * @param object https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param result https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 * @param user_data https://docs.gtk.org/gio/callback.AsyncReadyCallback.html
 */
static void lock_key_row_export_file_save(GObject *source_object,
                                          GAsyncResult *res, gpointer data)
{
    GtkFileDialog *file = GTK_FILE_DIALOG(source_object);
    LockKeyRow *row = LOCK_KEY_ROW(data);

    row->export_file = gtk_file_dialog_save_finish(file, res, NULL);
    if (row->export_file == NULL) {
        lock_key_row_export_on_completed(row);

        /* Cleanup */
        g_object_unref(file);
        file = NULL;

        row = NULL;

        return;
    }

    thread_export_key(row);
}

/**
 * This function opens a save file dialog for a LockKeyRow.
 *
 * @param self https://docs.gtk.org/gtk4/signal.Button.clicked.html
 * @param row https://docs.gtk.org/gtk4/signal.Button.clicked.html
 */
static void lock_key_row_export_file_present(GtkButton *self, LockKeyRow *row)
{
    GtkFileDialog *file = gtk_file_dialog_new();
    LockWindow *window = lock_key_dialog_get_window(row->dialog);
    GCancellable *cancel = g_cancellable_new();

    gtk_file_dialog_save(file, GTK_WINDOW(window),
                         cancel, lock_key_row_export_file_save, row);
}

/**
 * This function imports a key in a LockKeyDialog.
 *
 * @param row https://docs.gtk.org/glib/callback.ThreadFunc.html
 */
void lock_key_row_export(LockKeyRow *row)
{
    char *path = g_file_get_path(row->export_file);

    row->export_success = key_export(row->email, path);

    /* Cleanup */
    g_free(path);
    path = NULL;

    /* UI */
    g_idle_add((GSourceFunc) lock_key_row_export_on_completed, row);

    g_thread_exit(0);
}

/**
 * This function handles UI updates for key exports and is supposed to be called via g_idle_add().
 *
 * @param row https://docs.gtk.org/glib/callback.SourceFunc.html
 *
 * @return https://docs.gtk.org/glib/func.idle_add.html
 */
gboolean lock_key_row_export_on_completed(LockKeyRow *row)
{
    AdwToast *toast;

    if (row->export_file == NULL) {
        toast = adw_toast_new(_("Could not open file"));
    } else if (!row->export_success) {
        toast = adw_toast_new(_("Export failed"));
    } else {
        toast = adw_toast_new(_("Key exported"));
    }

    adw_toast_set_timeout(toast, 2);
    lock_key_dialog_add_toast(row->dialog, toast);

    /* Only execute once */
    return false;               // https://docs.gtk.org/glib/func.idle_add.html
}

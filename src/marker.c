/*
 * marker.c
 *
 * Copyright (C) 2017 - 2018 Fabio Colacio
 *
 * Marker is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * Marker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with Marker; see the file LICENSE.md. If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <gtk/gtk.h>
#include <stdlib.h>

#include <locale.h>
#include <glib/gi18n.h>

#include "marker-prefs.h"
#include "marker-window.h"
#include "marker-exporter.h"

#include "marker.h"

GtkApplication* app;

GtkApplication*
marker_get_app()
{
  return app;
}

static gboolean editor_mode_arg = FALSE;
static gboolean preview_mode_arg = FALSE;
static gboolean dual_pane_mode_arg = FALSE;
static gboolean dual_window_mode_arg = FALSE;
static gboolean batch_mode_arg = FALSE;
static gboolean landscape_arg = FALSE;
static gboolean watch_arg = FALSE;
static gboolean merge_arg = FALSE;
static gboolean no_border_arg = FALSE;
static gchar *outfile_arg = NULL;

static const GOptionEntry CLI_OPTIONS[] =
{
  { "editor", 'e', 0, G_OPTION_ARG_NONE, &editor_mode_arg, "Open in editor-only mode", NULL },
  { "preview", 'p', 0, G_OPTION_ARG_NONE, &preview_mode_arg, "Open in preview-only mode", NULL },
  { "dual-pane", 'd', 0, G_OPTION_ARG_NONE, &dual_pane_mode_arg, "Open in dual-pane mode", NULL },
  { "dual-window", 'w', 0, G_OPTION_ARG_NONE, &dual_window_mode_arg, "Open in dual-window mode", NULL },
  { "output", 'o', 0, G_OPTION_ARG_STRING, &outfile_arg, "Export markdown to the given output file", NULL },
  { "batch", 'b', 0, G_OPTION_ARG_NONE, &batch_mode_arg, "Batch export all .md files in directory to PDF", NULL },
  { "landscape", 'l', 0, G_OPTION_ARG_NONE, &landscape_arg, "Use landscape orientation for PDF export", NULL },
  { "watch", 'W', 0, G_OPTION_ARG_NONE, &watch_arg, "Watch file for changes and re-export automatically", NULL },
  { "merge", 'm', 0, G_OPTION_ARG_NONE, &merge_arg, "Merge batch PDFs into a single file (requires --batch)", NULL },
  { "no-border", 0, 0, G_OPTION_ARG_NONE, &no_border_arg, "Remove borders from diagrams in PDF export", NULL },
  { NULL }
};

const int APP_MENU_ACTION_ENTRIES_LEN = 7;

const GActionEntry APP_MENU_ACTION_ENTRIES[] =
{
  { "new", new_cb, NULL, NULL, NULL },
  { "prefs", marker_prefs_cb, NULL, NULL, NULL },
  { "shortcuts", marker_shortcuts_cb, NULL, NULL, NULL },
  { "help", marker_help_cb, NULL, NULL, NULL },
  { "about", marker_about_cb, NULL, NULL, NULL },
  { "quit", marker_quit_cb, NULL, NULL, NULL },
  { "export-pdf", marker_export_pdf_cb, NULL, NULL, NULL }
};

static void
marker_init(GtkApplication* app)
{
  marker_prefs_load();

  const gchar *quit_accels[] = { "<Ctrl>q", NULL };
  gtk_application_set_accels_for_action (app, "app.quit", quit_accels);

  /* Quick export to PDF with the same file name (#53) */
  const gchar *export_accels[] = { "<Ctrl>e", NULL };
  gtk_application_set_accels_for_action (app, "app.export-pdf", export_accels);

  if (gtk_application_prefers_app_menu(app))
  {
    GtkBuilder* builder =
      gtk_builder_new_from_resource("/com/github/fabiocolacio/marker/ui/marker-appmenu.ui");

    GMenuModel* app_menu =
      G_MENU_MODEL(gtk_builder_get_object(builder, "app_menu"));
    gtk_application_set_app_menu(app, app_menu);
    g_action_map_add_action_entries(G_ACTION_MAP(app),
                                    APP_MENU_ACTION_ENTRIES,
                                    APP_MENU_ACTION_ENTRIES_LEN,
                                    app);

    g_object_unref(builder);
  }

  g_object_set(gtk_settings_get_default(),
               "gtk-application-prefer-dark-theme",
               marker_prefs_get_use_dark_theme(),
               NULL);
}

static void
activate(GtkApplication* app)
{
  marker_init (app);
  marker_create_new_window();
}

static void
marker_watch_changed_cb (GFileMonitor      *monitor,
                         GFile             *file,
                         GFile             *other_file,
                         GFileMonitorEvent  event_type,
                         gpointer           user_data)
{
  if (event_type != G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT)
    return;
  gchar **paths = (gchar **) user_data;
  GDateTime *now = g_date_time_new_now_local ();
  g_autofree gchar *ts = g_date_time_format (now, "%H:%M:%S");
  g_date_time_unref (now);
  g_print ("[%s] Re-exporting %s\n", ts, paths[0]);
  marker_exporter_export (paths[0], paths[1]);
}

static void
marker_open(GtkApplication* app,
            GFile**         files,
            gint            num_files,
            const gchar*    hint)
{
  g_application_hold (G_APPLICATION (app));
  marker_init(app);

  /* Apply CLI flags */
  marker_exporter_set_landscape (landscape_arg);

  /* Batch export: convert all .md files in directory to PDF (#18) */
  if (batch_mode_arg) {
    g_autofree gchar *dir_path = g_file_get_path (files[0]);
    GDir *dir = g_dir_open (dir_path, 0, NULL);
    if (!dir) {
      g_printerr ("marker: cannot open directory: %s\n", dir_path);
      exit (1);
    }
    const gchar *name;
    /* Resolve output dir to absolute path (chdir in exporter breaks relative paths) */
    g_autofree gchar *out_dir = NULL;
    if (outfile_arg) {
      GFile *f = g_file_new_for_commandline_arg (outfile_arg);
      out_dir = g_file_get_path (f);
      g_object_unref (f);
    } else {
      out_dir = g_strdup (dir_path);
    }

    /* Count .md files first for progress (#41) */
    guint total = 0, current = 0;
    while ((name = g_dir_read_name (dir)) != NULL) {
      if (g_str_has_suffix (name, ".md")) total++;
    }
    g_dir_rewind (dir);
    g_mkdir_with_parents (out_dir, 0755);

    while ((name = g_dir_read_name (dir)) != NULL) {
      if (!g_str_has_suffix (name, ".md"))
        continue;
      current++;
      g_autofree gchar *infile = g_build_filename (dir_path, name, NULL);
      g_autofree gchar *basename = g_strdup (name);
      basename[strlen(basename) - 3] = '\0';
      g_autofree gchar *outfile = g_build_filename (out_dir, g_strdup_printf ("%s.pdf", basename), NULL);
      g_print ("[%u/%u] Exporting %s\n", current, total, name);
      marker_exporter_export (infile, outfile);
    }
    g_dir_close (dir);

    /* Merge all PDFs into one if --merge (#36) */
    if (merge_arg && outfile_arg) {
      g_autofree gchar *merge_out = g_strdup (outfile_arg);
      /* Collect all generated PDFs */
      GDir *pdf_dir = g_dir_open (out_dir, 0, NULL);
      if (pdf_dir) {
        GPtrArray *pdf_files = g_ptr_array_new_with_free_func (g_free);
        const gchar *pdf_name;
        while ((pdf_name = g_dir_read_name (pdf_dir)) != NULL) {
          if (g_str_has_suffix (pdf_name, ".pdf"))
            g_ptr_array_add (pdf_files, g_build_filename (out_dir, pdf_name, NULL));
        }
        g_dir_close (pdf_dir);

        if (pdf_files->len > 0) {
          /* Build pdfunite command argv */
          gchar **argv = g_new0 (gchar*, pdf_files->len + 3);
          argv[0] = "pdfunite";
          for (guint i = 0; i < pdf_files->len; i++)
            argv[i + 1] = g_ptr_array_index (pdf_files, i);
          argv[pdf_files->len + 1] = merge_out;

          g_print ("Merging %u PDFs → %s\n", pdf_files->len, merge_out);
          GError *error = NULL;
          gint status;
          g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                        NULL, NULL, NULL, NULL, &status, &error);
          if (error) {
            g_printerr ("merge failed: %s\n", error->message);
            g_error_free (error);
          }
          g_free (argv);
        }
        g_ptr_array_unref (pdf_files);
      }
    }

    exit (0);
  }

  /* Single file export */
  if (outfile_arg != NULL) {
    g_autoptr (GFile) outfile = g_file_new_for_commandline_arg (outfile_arg);
    g_autofree gchar *outfile_path = g_file_get_path (outfile);
    g_autofree gchar *infile_path = g_file_get_path (files[0]);

    /* Watch mode: re-export on file changes (#19) */
    if (watch_arg) {
      g_print ("Watching %s → %s (Ctrl+C to stop)\n", infile_path, outfile_path);
      marker_exporter_export (infile_path, outfile_path);

      GFileMonitor *monitor = g_file_monitor_file (files[0], G_FILE_MONITOR_NONE, NULL, NULL);
      GMainLoop *loop = g_main_loop_new (NULL, FALSE);

      /* Store paths for the callback */
      gchar *watch_data[] = { infile_path, outfile_path };
      g_signal_connect (monitor, "changed",
        G_CALLBACK (marker_watch_changed_cb), watch_data);
      g_main_loop_run (loop);
      g_main_loop_unref (loop);
    } else {
      marker_exporter_export (infile_path, outfile_path);
    }
    exit (0);
  }
 
  for (int i = 0; i < num_files; ++i)
  {
    GFile* file = files[i];
    g_object_ref(file);
    marker_open_file(file);
  }
  g_application_release (G_APPLICATION (app));
}

void
new_cb(GSimpleAction* action,
       GVariant*      parameter,
       gpointer       user_data)
{
  marker_create_new_window();
}

/* Build "<same-stem>.pdf" next to the source file. */
static gchar*
pdf_path_for (const gchar* md_path)
{
  g_autofree gchar *base = g_path_get_basename (md_path);
  g_autofree gchar *stem = g_strdup (base);
  char *dot = g_strrstr (stem, ".");

  if (dot != NULL && dot != stem) {
    *dot = '\0';
  }
  return g_strdup_printf ("%s.pdf", stem);
}

/* Pending quick-export job, run from the main loop (#52) */
typedef struct {
  gchar *src;
  gchar *out;
  MarkerPreview *preview;
  GtkWindow *window;
} ExportJob;

static gboolean marker_export_pdf_idle_cb (gpointer data);

static void
marker_export_start (const gchar     *src,
                     const gchar     *out,
                     MarkerPreview   *preview,
                     GtkWindow       *window)
{
  ExportJob *job = g_new0 (ExportJob, 1);
  job->src = g_strdup (src);
  job->out = g_strdup (out);
  job->preview = preview ? g_object_ref (preview) : NULL;
  job->window = window;
  g_idle_add (marker_export_pdf_idle_cb, job);
}

/* Single response handler for the result notice: opens the folder when
   requested, then destroys the dialog. The directory is stored on the dialog
   with g_object_set_data_full, so it is released exactly once even if the
   response signal fires more than once. */
static void
marker_export_report_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  (void) data;

  if (response == GTK_RESPONSE_ACCEPT) {
    const gchar *dir = g_object_get_data (G_OBJECT (dialog), "export-dir");

    if (dir != NULL) {
      const gchar *argv[] = { "xdg-open", dir, NULL };
      GError *error = NULL;

      if (!g_spawn_async (NULL, (gchar **) argv, NULL,
                          G_SPAWN_SEARCH_PATH, NULL, NULL,
                          NULL, &error)) {
        g_printerr ("marker: could not open folder %s: %s\n", dir, error->message);
        g_error_free (error);
      }
    }
  }
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
marker_export_report (GtkWindow     *window,
                      const gchar   *outfile,
                      gboolean       made)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (window,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   made ? GTK_MESSAGE_INFO : GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_NONE,
                                   made ? _("PDF created at:\n%s")
                                        : _("The PDF could not be created at:\n%s"),
                                   outfile);

  gtk_dialog_add_buttons (GTK_DIALOG (dialog), _("_Close"), GTK_RESPONSE_CLOSE, NULL);
  if (made) {
    gtk_dialog_add_button (GTK_DIALOG (dialog), _("Show _Folder"), GTK_RESPONSE_ACCEPT);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    g_object_set_data_full (G_OBJECT (dialog), "export-dir",
                            g_path_get_dirname (outfile), g_free);
  } else {
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
  }

  g_signal_connect (dialog, "response",
                    G_CALLBACK (marker_export_report_response_cb), NULL);

  /* Match the overwrite prompt, which is the dialog that actually shows up
     here: gtk_widget_show, not gtk_window_present (#52) */
  gtk_widget_show (dialog);
}

/* Data for the deferred result notice (#52) */
typedef struct {
  GtkWindow *window;
  gchar     *out;
  gboolean   made;
} ReportCtx;

static gboolean
marker_export_report_idle_cb (gpointer data)
{
  ReportCtx *rep = data;

  marker_export_report (rep->window, rep->out, rep->made);
  g_free (rep->out);
  g_free (rep);
  return G_SOURCE_REMOVE;
}

static gboolean
marker_export_pdf_idle_cb (gpointer data)
{
  ExportJob *job = data;
  gboolean made;

  marker_exporter_export_with_preview (job->src, job->out, job->preview);
  made = g_file_test (job->out, G_FILE_TEST_EXISTS);
  g_printerr ("marker: export %s -> %s (%s)\n",
              job->src, job->out, made ? "ok" : "FAILED");

  /* The exporter runs its own nested main loop and has already returned by
     now, so anything shown from here would be mapped inside a loop that no
     longer exists. Defer the notice to the outermost main loop (#52). */
  {
    ReportCtx *rep = g_new0 (ReportCtx, 1);
    rep->window = job->window;
    rep->out = g_strdup (job->out);
    rep->made = made;
    g_printerr ("marker: enqueuing report, window=%p made=%d\n",
                (void *) rep->window, made);
    g_idle_add (marker_export_report_idle_cb, rep);
  }

  if (job->preview != NULL) {
    g_object_unref (job->preview);
  }
  g_free (job->src);
  g_free (job->out);
  g_free (job);
  return G_SOURCE_REMOVE;
}

/* Overwrite confirmation. Fully async: gtk_dialog_run would nest a second
   main loop inside the one serving the click handler, which froze the
   export. The export only starts from the response callback (#52). */
typedef struct {
  gchar *src;
  gchar *out;
  MarkerPreview *preview;
  GtkWindow *window;
} ConfirmCtx;

static void
marker_overwrite_response_cb (GtkDialog *dialog, gint response, gpointer data)
{
  ConfirmCtx *ctx = data;

  g_printerr ("marker: overwrite dialog response=%d\n", response);
  if (response == GTK_RESPONSE_ACCEPT) {
    marker_export_start (ctx->src, ctx->out, ctx->preview, ctx->window);
  }
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (ctx->preview != NULL) {
    g_object_unref (ctx->preview);
  }
  g_free (ctx->src);
  g_free (ctx->out);
  g_free (ctx);
}

static void
marker_export_ask_overwrite (GtkWindow     *window,
                             const gchar   *src,
                             const gchar   *out,
                             MarkerPreview *preview)
{
  g_autofree gchar *stem = g_path_get_basename (out);
  GtkWidget *dialog;
  ConfirmCtx *ctx;

  dialog = gtk_message_dialog_new (window,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_NONE,
                                   _("\"%s\" already exists. Overwrite it?"),
                                   stem);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                          _("_Overwrite"), GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  ctx = g_new0 (ConfirmCtx, 1);
  ctx->src = g_strdup (src);
  ctx->out = g_strdup (out);
  ctx->preview = preview ? g_object_ref (preview) : NULL;
  ctx->window = window;

  g_signal_connect (dialog, "response",
                    G_CALLBACK (marker_overwrite_response_cb), ctx);
  gtk_widget_show (dialog);
}

void
marker_export_pdf_cb(GSimpleAction* action,
                     GVariant*      parameter,
                     gpointer       user_data)
{
  GtkApplication *application;
  GtkWindow *window = NULL;
  MarkerWindow *marker_window;
  MarkerEditor *editor;
  GFile *file;
  g_autofree gchar *src = NULL;
  g_autofree gchar *stem = NULL;
  g_autofree gchar *dir = NULL;
  g_autofree gchar *outfile = NULL;

  /* The action is registered twice: once with the application (app menu)
     and once with the window (gear popover). Accept either (#52). */
  if (G_IS_APPLICATION (user_data)) {
    application = GTK_APPLICATION (user_data);
    window = gtk_application_get_active_window (application);
  } else if (MARKER_IS_WINDOW (user_data)) {
    window = GTK_WINDOW (user_data);
    application = gtk_window_get_application (window);
  } else {
    g_printerr ("marker: export-pdf got an unexpected user_data\n");
    return;
  }

  if (!MARKER_IS_WINDOW (window)) {
    g_printerr ("marker: export-pdf has no active window\n");
    return;
  }
  marker_window = MARKER_WINDOW (window);
  editor = marker_window_get_active_editor (marker_window);
  if (editor == NULL) {
    g_printerr ("marker: export-pdf has no active editor\n");
    return;
  }

  file = marker_editor_get_file (editor);
  if (!G_IS_FILE (file)) {
    /* Never saved: fall back to the full export dialog (#52) */
    marker_exporter_show_export_dialog (marker_window);
    return;
  }

  src = g_file_get_path (file);
  stem = pdf_path_for (src);
  dir = g_path_get_dirname (src);
  outfile = g_build_filename (dir, stem, NULL);

  g_printerr ("marker: exporting %s to %s\n", src, outfile);

  /* Export reads the file from disk, so flush pending edits first (#52) */
  if (marker_editor_has_unsaved_changes (editor)) {
    marker_editor_save_file (editor);
  }

  if (g_file_test (outfile, G_FILE_TEST_EXISTS)) {
    marker_export_ask_overwrite (window, src, outfile,
                                 marker_editor_get_preview (editor));
    return;
  }

  marker_export_start (src, outfile, marker_editor_get_preview (editor), window);
}

void
marker_prefs_cb(GSimpleAction* action,
                GVariant*      parameter,
                gpointer       user_data)
{
  marker_prefs_show_window();
}

void
marker_help_cb(GSimpleAction* action,
               GVariant*      parameter,
               gpointer       user_data)
{
  GtkWindow *window;
  GtkApplication *application = user_data;
  GError *error = NULL;

  window = gtk_application_get_active_window (application);
  gtk_show_uri_on_window (window, "help:Marker",
                          gtk_get_current_event_time (), &error);
}

void
marker_about_cb(GSimpleAction* action,
                GVariant*      parameter,
                gpointer       user_data)
{
  const gchar* authors[] = {
    "Fabio Colacio",
    "Martino Ferrari",
    NULL
  };

  const gchar* artists[] = {
    "Fabio Colacio",
    NULL
  };

  GtkAboutDialog* dialog = GTK_ABOUT_DIALOG(gtk_about_dialog_new());

  gtk_about_dialog_set_logo_icon_name(dialog, "com.github.fabiocolacio.marker");
  gtk_about_dialog_set_program_name(dialog, "Marker");
  gtk_about_dialog_set_version(dialog, MARKER_VERSION);
  gtk_about_dialog_set_comments(dialog, _("A markdown editor for GNOME"));
  gtk_about_dialog_set_website(dialog, "https://github.com/fabiocolacio/Marker");
  gtk_about_dialog_set_website_label(dialog, _("Report bugs and ideas on github"));
  gtk_about_dialog_set_copyright(dialog, "Copyright 2017-2018 Fabio Colacio");
  gtk_about_dialog_set_license_type(dialog, GTK_LICENSE_GPL_3_0);
  gtk_about_dialog_set_authors(dialog, authors);
  gtk_about_dialog_set_artists(dialog, artists);
  gtk_about_dialog_set_translator_credits(dialog, _("translator-credits"));

  GtkWindow* window = gtk_application_get_active_window(app);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), window);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(GTK_WIDGET(dialog));
}

void
marker_quit_cb(GSimpleAction*  action,
               GVariant*       parameter,
               gpointer        user_data)
{
  marker_quit();
}

void
marker_shortcuts_cb(GSimpleAction* action,
                    GVariant*      parameter,
                    gpointer       user_data)
{
  GtkBuilder* builder =
    gtk_builder_new_from_resource("/com/github/fabiocolacio/marker/ui/marker-shortcuts-window.ui");

  GtkWidget* dialog = GTK_WIDGET(gtk_builder_get_object(builder, "shortcuts"));

  GtkWindow* parent = gtk_application_get_active_window(app);

	if (GTK_WINDOW(parent) != gtk_window_get_transient_for(GTK_WINDOW(dialog)))
	{
		gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
	}

	gtk_widget_show_all(dialog);
  gtk_window_present(GTK_WINDOW(dialog));

  g_object_unref(builder);
}

gboolean
marker_has_app_menu()
{
  if (gtk_application_get_app_menu(app))
  {
    return TRUE;
  }
  return FALSE;
}

void
marker_create_new_window()
{
  MarkerWindow *window = marker_window_new (app);
  gtk_widget_show (GTK_WIDGET (window));
}

void
marker_create_new_window_from_file (GFile *file)
{
  MarkerWindow *window = marker_window_new_from_file (app, file);
  gtk_widget_show (GTK_WIDGET (window));

  if (preview_mode_arg)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    marker_editor_set_view_mode (editor, PREVIEW_ONLY_MODE);
  }
  else if (editor_mode_arg)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    marker_editor_set_view_mode (editor, EDITOR_ONLY_MODE);
  }
  else if (dual_pane_mode_arg)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    marker_editor_set_view_mode (editor, DUAL_PANE_MODE);
  }
  else if (dual_window_mode_arg)
  {
    MarkerEditor *editor = marker_window_get_active_editor (window);
    marker_editor_set_view_mode (editor, DUAL_WINDOW_MODE);
  }
}


void
marker_open_file (GFile *file)
{
  GList *windows = gtk_application_get_windows(app);
  if (g_list_last(windows))
  {
    if (MARKER_IS_WINDOW(windows->data))
    {
      MarkerWindow *window = MARKER_WINDOW(windows->data);
      marker_window_new_editor_from_file(window, file);
      return;
    }
  }

  marker_create_new_window_from_file(file);

}

void
marker_quit()
{
  GtkApplication *app = marker_get_app();
  GList *windows = gtk_application_get_windows(app);
  for (GList *item = windows; item != NULL; item = item->next)
  {
    if (MARKER_IS_WINDOW(item->data))
    {
      MarkerWindow *window = item->data;
      marker_window_try_close (window);
    }
  }
}

#ifndef MARKER_TEST_BUILD
int
main(int    argc,
     char** argv)
{

  /* Initialize gettext support */
  bindtextdomain ("marker", LOCALE_DIR);
  bind_textdomain_codeset ("marker", "UTF-8");
  textdomain ("marker");

  app = gtk_application_new("com.github.fabiocolacio.marker",
                            G_APPLICATION_HANDLES_OPEN);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  g_signal_connect(app, "open", G_CALLBACK(marker_open), NULL);

  g_application_add_main_option_entries (G_APPLICATION(app), CLI_OPTIONS);

  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
#endif /* MARKER_TEST_BUILD */

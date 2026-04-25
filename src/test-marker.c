/*
 * test-marker.c — Basic test suite for Marker
 */

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "marker-markdown.h"
#include "marker-utils.h"
#include "marker-prefs.h"

static void
test_markdown_to_html_headings (void)
{
  char *html = marker_markdown_to_html ("# Hello", 7, NULL,
                                        MATHJS_OFF, HIGHLIGHT_OFF, MERMAID_OFF,
                                        NULL, -1);
  g_assert_nonnull (html);
  g_assert_true (strstr (html, "Hello") != NULL);
  free (html);
}

static void
test_markdown_to_html_code_block (void)
{
  const char *md = "```c\nint x = 1;\n```";
  char *html = marker_markdown_to_html (md, strlen (md), NULL,
                                        MATHJS_OFF, HIGHLIGHT_OFF, MERMAID_OFF,
                                        NULL, -1);
  g_assert_nonnull (html);
  g_assert_true (strstr (html, "<code") != NULL);
  free (html);
}

static void
test_markdown_to_html_mermaid_block (void)
{
  const char *md = "```mermaid\ngraph TD\n  A-->B\n```";
  char *html = marker_markdown_to_html (md, strlen (md), NULL,
                                        MATHJS_OFF, HIGHLIGHT_OFF, MERMAID_OFF,
                                        NULL, -1);
  g_assert_nonnull (html);
  /* Mermaid blocks should produce a div with class mermaid */
  g_assert_true (strstr (html, "mermaid") != NULL);
  free (html);
}

static void
test_read_file_nonexistent (void)
{
  long size = 0;
  g_test_expect_message (NULL, G_LOG_LEVEL_WARNING, "*cannot open*");
  gchar *contents = marker_utils_read_file ("/nonexistent/file.md", &size);
  g_test_assert_expected_messages ();
  g_assert_null (contents);
  g_assert_cmpint (size, ==, 0);
}

static void
test_read_file_valid (void)
{
  /* Write a temp file and read it back */
  const char *text = "# Test\nHello world";
  g_autofree gchar *path = g_build_filename (g_get_tmp_dir (), "marker_test.md", NULL);
  g_file_set_contents (path, text, -1, NULL);

  long size = 0;
  gchar *contents = marker_utils_read_file (path, &size);
  g_assert_nonnull (contents);
  g_assert_cmpint (size, >, 0);
  g_assert_true (strstr (contents, "Hello world") != NULL);
  g_free (contents);
  remove (path);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  marker_prefs_load ();

  g_test_add_func ("/markdown/headings", test_markdown_to_html_headings);
  g_test_add_func ("/markdown/code-block", test_markdown_to_html_code_block);
  g_test_add_func ("/markdown/mermaid-block", test_markdown_to_html_mermaid_block);
  g_test_add_func ("/utils/read-file-nonexistent", test_read_file_nonexistent);
  g_test_add_func ("/utils/read-file-valid", test_read_file_valid);

  return g_test_run ();
}

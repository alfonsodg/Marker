/*
 * test-marker.c — Basic test suite for Marker
 */

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "marker-markdown.h"
#include "marker-utils.h"
#include "marker-prefs.h"
#include "marker-mermaid.h"

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
test_mermaid_repair_unclosed_quote (void)
{
  gchar *r = marker_mermaid_repair ("graph TD\n  A[\"Hola]");
  g_assert_cmpstr (r, ==, "graph TD\n  A[\"Hola\"]\n");
  g_free (r);
}

static void
test_mermaid_repair_missing_close_bracket (void)
{
  gchar *r = marker_mermaid_repair ("graph TD\n  A[Hola");
  g_assert_cmpstr (r, ==, "graph TD\n  A[Hola]\n");
  g_free (r);
}

static void
test_mermaid_repair_missing_direction (void)
{
  gchar *r = marker_mermaid_repair ("graph\n  A-->B");
  g_assert_cmpstr (r, ==, "graph TD\n  A-->B\n");
  g_free (r);
}

static void
test_mermaid_repair_strips_bom_and_crlf (void)
{
  gchar *r = marker_mermaid_repair ("\xEF\xBB\xBFgraph TD\r\n  A-->B\r\n");
  g_assert_cmpstr (r, ==, "graph TD\n  A-->B\n");
  g_free (r);
}

static void
test_mermaid_repair_leaves_valid_intact (void)
{
  const gchar *ok = "graph TD\n  A[Hola] --> B[Mundo]\n";
  gchar *r = marker_mermaid_repair (ok);
  g_assert_cmpstr (r, ==, ok);
  g_free (r);
}

static void
test_mermaid_repair_subgraph_quoted (void)
{
  gchar *r = marker_mermaid_repair ("graph TD\n  subgraph S[\"Zona Publica\"]\n  A-->B\n  end");
  g_assert_cmpstr (r, ==, "graph TD\n  subgraph S[\"Zona Publica\"]\n  A-->B\n  end\n");
  g_free (r);
}

static void
test_mermaid_repair_handles_null (void)
{
  gchar *r = marker_mermaid_repair (NULL);
  g_assert_cmpstr (r, ==, "");
  g_free (r);
}

static void
test_mermaid_repair_document_only_touches_mermaid (void)
{
  const gchar *md =
    "# Titulo\n\n"
    "```mermaid\n"
    "graph\n"
    "  A[\"Roto]\n"
    "```\n\n"
    "```c\n"
    "int a[\"no tocar\"];\n"
    "```\n";
  gchar *r = marker_mermaid_repair_document (md);

  /* The mermaid block is repaired: direction added and quote closed */
  g_assert_nonnull (strstr (r, "graph TD"));
  g_assert_nonnull (strstr (r, "A[\"Roto\"]"));
  /* The C block must survive byte for byte */
  g_assert_nonnull (strstr (r, "int a[\"no tocar\"];"));
  g_free (r);
}

static void
test_mermaid_repair_document_leaves_plain_md (void)
{
  const gchar *md = "# Titulo\n\nUn parrafo con [corchetes] sueltos.\n";
  gchar *r = marker_mermaid_repair_document (md);
  g_assert_true (g_str_has_prefix (r, md));
  g_free (r);
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
  g_test_add_func ("/mermaid/repair-unclosed-quote", test_mermaid_repair_unclosed_quote);
  g_test_add_func ("/mermaid/repair-missing-close-bracket", test_mermaid_repair_missing_close_bracket);
  g_test_add_func ("/mermaid/repair-missing-direction", test_mermaid_repair_missing_direction);
  g_test_add_func ("/mermaid/repair-strips-bom-crlf", test_mermaid_repair_strips_bom_and_crlf);
  g_test_add_func ("/mermaid/repair-leaves-valid-intact", test_mermaid_repair_leaves_valid_intact);
  g_test_add_func ("/mermaid/repair-subgraph-quoted", test_mermaid_repair_subgraph_quoted);
  g_test_add_func ("/mermaid/repair-handles-null", test_mermaid_repair_handles_null);
  g_test_add_func ("/mermaid/repair-document-only-touches-mermaid", test_mermaid_repair_document_only_touches_mermaid);
  g_test_add_func ("/mermaid/repair-document-leaves-plain-md", test_mermaid_repair_document_leaves_plain_md);
  g_test_add_func ("/utils/read-file-nonexistent", test_read_file_nonexistent);
  g_test_add_func ("/utils/read-file-valid", test_read_file_valid);

  return g_test_run ();
}

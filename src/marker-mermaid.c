/*
 * marker-mermaid.c
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
 * License along with Marker. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>
#include <glib.h>

#include "marker-mermaid.h"

/* Directives that may legally start a Mermaid block. */
static const char* const DIRECTIVES[] = {
  "graph", "flowchart", "sequenceDiagram", "classDiagram", "stateDiagram",
  "stateDiagram-v2", "erDiagram", "journey", "gantt", "pie", "mindmap",
  "timeline", "quadrantChart", "requirementDiagram", "gitGraph",
  "C4Context", "C4Container", "C4Component", "C4Dynamic", "C4Deployment",
  "sankey-beta", "xychart-beta", "block-beta", "packet-beta", "kanban",
  "architecture-beta", "radar-beta", "treemap-beta", "zenuml",
  "info", "architecture", "block", "radar", "treemap", "packet",
  "sankey", "xychart",
  NULL
};

/* Flowchart direction keywords accepted after graph/flowchart. */
static const char* const DIRECTIONS[] = {
  "TB", "TD", "BT", "RL", "LR", NULL
};

static gboolean
is_directive (const char* line)
{
  int i;

  for (i = 0; DIRECTIVES[i] != NULL; i++) {
    if (g_str_has_prefix (line, DIRECTIVES[i])) {
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean
has_direction (const char* s)
{
  int i;

  for (i = 0; DIRECTIONS[i] != NULL; i++) {
    if (g_str_has_prefix (s, DIRECTIONS[i])) {
      return TRUE;
    }
  }
  return FALSE;
}

/*
 * Count delimiters on one Mermaid line, honouring escapes and quoted spans.
 * Brackets inside a quoted label are skipped because Mermaid treats them as
 * literal label text.
 */
static void
count_delims (const char* line,
              int* open_sq,  int* close_sq,
              int* open_par, int* close_par,
              int* quotes)
{
  gboolean in_quotes = FALSE;

  *open_sq = *close_sq = *open_par = *close_par = *quotes = 0;

  while (*line != '\0') {
    char c = *line;

    if (c == '\\' && line[1] != '\0') {
      line += 2;
      continue;
    }
    if (c == '"') {
      in_quotes = !in_quotes;
      (*quotes)++;
      line++;
      continue;
    }
    if (!in_quotes) {
      if (c == '[') {
        (*open_sq)++;
      } else if (c == ']') {
        (*close_sq)++;
      } else if (c == '(') {
        (*open_par)++;
      } else if (c == ')') {
        (*close_par)++;
      }
    }
    line++;
  }
}

/*
 * Mermaid cannot parse parentheses inside an edge label: `A -->|text (x)| B`
 * aborts with a parse error. Mermaid itself quotes labels with double quotes
 * instead, so the parentheses are simply removed from the label (#52).
 *
 * Returns: newly allocated line, or a copy of @line when nothing to strip.
 */
static gchar*
fix_edge_label_parens (const char* line)
{
  GString *out = g_string_new (NULL);
  const char *p = line;
  gboolean changed = FALSE;

  while (*p != '\0') {
    /* Edge label starts with |text| right after an arrow. */
    if (*p == '|' && p > line) {
      char prev = p[-1];

      if (prev == '-' || prev == '<' || prev == '>') {
        const char *end = strchr (p + 1, '|');

        /* Only touch a label that actually holds a parenthesis. */
        if (end != NULL) {
          gboolean has_paren = FALSE;

          for (const char *q = p + 1; q < end; q++) {
            if (*q == '(' || *q == ')') {
              has_paren = TRUE;
              break;
            }
          }
          if (has_paren) {
            g_string_append_c (out, '|');
            for (const char *q = p + 1; q < end; q++) {
              if (*q == '(' || *q == ')') {
                changed = TRUE;
                continue;
              }
              g_string_append_c (out, *q);
            }
            g_string_append_c (out, '|');
            p = end + 1;
            continue;
          }
        }
      }
    }
    g_string_append_c (out, *p);
    p++;
  }

  if (!changed) {
    g_string_free (out, TRUE);
    return g_strdup (line);
  }
  return g_string_free (out, FALSE);
}

/*
 * Balance a single line that declares a node label.
 *
 * Handles the common hand-written forms where a delimiter is missing:
 * A["Label  ->  A["Label],  A[Label  ->  A[Label].
 *
 * Lines without a node bracket, and lines whose delimiters already balance,
 * are returned verbatim so valid diagrams are never altered.
 *
 * Returns: newly allocated fixed line without trailing newline, never %NULL.
 */
static gchar*
fix_line (const char* line)
{
  int open_sq, close_sq, open_par, close_par, quotes;
  gboolean has_bracket;
  GString *out;
  gchar *stripped;

  /* Parentheses inside an edge label always break the Mermaid parser. */
  stripped = fix_edge_label_parens (line);

  has_bracket = (strchr (stripped, '[') != NULL) || (strchr (stripped, '(') != NULL);
  if (!has_bracket) {
    return stripped;
  }

  count_delims (stripped, &open_sq, &close_sq, &open_par, &close_par, &quotes);

  /* Nothing else to repair. */
  if (quotes % 2 == 0 && open_sq == close_sq && open_par == close_par) {
    return stripped;
  }

  out = g_string_new (stripped);
  g_free (stripped);

  /* Unbalanced quote: close it before any trailing closers. */
  if (quotes % 2 == 1) {
    gsize cut = out->len;
    while (cut > 0 && (out->str[cut - 1] == ']' || out->str[cut - 1] == ')')) {
      cut--;
    }
    g_string_insert_c (out, cut, '"');
  }

  /* Recount, then append the closers that are still missing. */
  count_delims (out->str, &open_sq, &close_sq, &open_par, &close_par, &quotes);

  for (int i = close_par; i < open_par; i++) {
    g_string_append_c (out, ')');
  }
  for (int i = close_sq; i < open_sq; i++) {
    g_string_append_c (out, ']');
  }

  return g_string_free (out, FALSE);
}

/* Ensure graph/flowchart declares a direction keyword. */
static gchar*
fix_directive (const char* line)
{
  g_autofree gchar *trimmed = g_strstrip (g_strdup (line));

  if (g_strcmp0 (trimmed, "graph") == 0 || g_strcmp0 (trimmed, "flowchart") == 0) {
    return g_strdup_printf ("%s TD", trimmed);
  }

  if (g_str_has_prefix (trimmed, "graph ") || g_str_has_prefix (trimmed, "flowchart ")) {
    const char *rest = strchr (trimmed, ' ') + 1;
    g_autofree gchar *head = g_strstrip (g_strdup (rest));

    if (*head == '\0' || !has_direction (head)) {
      return g_strdup_printf ("%s TD", trimmed);
    }
  }

  return g_strdup (line);
}

gchar*
marker_mermaid_repair (const gchar* src)
{
  g_autofree gchar *normalized_src = NULL;
  gchar *normalized = NULL;
  g_auto (GStrv) lines = NULL;
  GString *out;
  gboolean first_content = TRUE;
  guint i;

  if (src == NULL || *src == '\0') {
    return g_strdup ("");
  }

  /* Normalize CRLF to LF and strip a UTF-8 BOM and control characters. */
  normalized_src = g_strdup (src);
  if (g_str_has_prefix (normalized_src, "\xEF\xBB\xBF")) {
    memmove (normalized_src, normalized_src + 3, strlen (normalized_src + 3) + 1);
  }
  {
    GString *clean = g_string_new (NULL);
    for (i = 0; normalized_src[i] != '\0'; i++) {
      char c = normalized_src[i];
      if (c == '\r') {
        /* CRLF collapses to a single LF; a lone CR also becomes LF. */
        if (normalized_src[i + 1] == '\n') {
          i++;
        }
        g_string_append_c (clean, '\n');
      } else if ((guchar) c < 0x20 && c != '\n') {
        g_string_append_c (clean, ' ');
      } else {
        g_string_append_c (clean, c);
      }
    }
    normalized = g_string_free (clean, FALSE);
  }

  lines = g_strsplit (normalized, "\n", -1);
  g_free (normalized);
  out = g_string_new (NULL);

  for (i = 0; lines[i] != NULL; i++) {
    const char *raw = lines[i];
    const char *content = raw + strspn (raw, " \t");
    g_autofree gchar *trimmed = g_strstrip (g_strdup (content));
    g_autofree gchar *indent = g_strndup (raw, content - raw);

    if (*trimmed == '\0') {
      g_string_append_c (out, '\n');
      continue;
    }

    if (first_content) {
      first_content = FALSE;
      if (is_directive (trimmed)) {
        g_autofree gchar *fixed = fix_directive (trimmed);
        g_string_append (out, indent);
        g_string_append (out, fixed);
        g_string_append_c (out, '\n');
        continue;
      }
      /* Not a directive: fall through and repair as a normal line. */
    }

    {
      g_autofree gchar *fixed = fix_line (trimmed);
      g_string_append (out, indent);
      g_string_append (out, fixed);
    }
    g_string_append_c (out, '\n');
  }

  /* The loop emits one newline per source line; drop a trailing duplicate. */
  if (out->len > 1 && out->str[out->len - 1] == '\n' &&
      out->str[out->len - 2] == '\n') {
    g_string_truncate (out, out->len - 1);
  }

  return g_string_free (out, FALSE);
}

/*
 * Walk the document and repair every fenced ```mermaid block in place.
 * Blocks with any other info string, and all other content, pass through
 * untouched.
 *
 * Returns: newly allocated document, never %NULL.
 */
gchar*
marker_mermaid_repair_document (const gchar* md)
{
  g_auto (GStrv) lines = NULL;
  GString *out;
  gboolean in_mermaid = FALSE;
  GString *block = NULL;
  guint i;

  if (md == NULL || *md == '\0') {
    return g_strdup ("");
  }

  lines = g_strsplit (md, "\n", -1);
  out = g_string_new (NULL);
  block = g_string_new (NULL);

  for (i = 0; lines[i] != NULL; i++) {
    const char *raw = lines[i];
    const char *content = raw + strspn (raw, " \t");

    if (!in_mermaid) {
      /* Opening fence: ```mermaid or ~~~mermaid, allowing extra info. */
      if (g_str_has_prefix (content, "```mermaid") ||
          g_str_has_prefix (content, "~~~mermaid")) {
        in_mermaid = TRUE;
        g_string_truncate (block, 0);
        g_string_append (out, raw);
        g_string_append_c (out, '\n');
        continue;
      }
      g_string_append (out, raw);
      g_string_append_c (out, '\n');
      continue;
    }

    /* Inside a mermaid block: buffer until the closing fence. */
    if (g_str_has_prefix (content, "```") || g_str_has_prefix (content, "~~~")) {
      g_autofree gchar *fixed = marker_mermaid_repair (block->str);
      g_string_append (out, fixed);
      g_string_truncate (block, 0);
      g_string_append (out, raw);
      g_string_append_c (out, '\n');
      in_mermaid = FALSE;
      continue;
    }

    g_string_append (block, raw);
    g_string_append_c (block, '\n');
  }

  /* Unterminated block: repair what we buffered. */
  if (in_mermaid && block->len > 0) {
    g_autofree gchar *fixed = marker_mermaid_repair (block->str);
    g_string_append (out, fixed);
  }

  g_string_free (block, TRUE);
  return g_string_free (out, FALSE);
}
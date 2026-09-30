/*
 * marker-mermaid.h
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

#ifndef __MARKER_MERMAID_H__
#define __MARKER_MERMAID_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * marker_mermaid_repair:
 * @src: (nullable): raw Mermaid diagram source.
 *
 * Repairs common non-standard Mermaid syntax so the diagram renders instead of
 * failing. Fixes unbalanced quotes and brackets in node labels, missing
 * flowchart direction, trailing junk and control characters.
 *
 * Valid input is returned unchanged. Caller owns the returned string.
 *
 * Returns: (transfer full): a newly allocated repaired string, never %NULL.
 */
gchar*              marker_mermaid_repair           (const gchar*      src);

/**
 * marker_mermaid_repair_document:
 * @md: (nullable): full markdown document.
 *
 * Repairs every fenced ```mermaid block in @md and returns the result. All
 * other content, including fenced blocks with a different info string, is
 * copied through unchanged.
 *
 * Returns: (transfer full): a newly allocated document, never %NULL.
 */
gchar*              marker_mermaid_repair_document  (const gchar*      md);

G_END_DECLS

#endif /* __MARKER_MERMAID_H__ */
# <img width="30" src="data/com.github.fabiocolacio.marker.svg"/> Marker Enhanced

Enhanced fork of [Marker](https://github.com/fabiocolacio/Marker) — a markdown editor for Linux made with GTK+-3.0.

This fork adds security fixes, modern JS libraries, powerful CLI export, dark mode, and more.

[![Build](https://github.com/alfonsodg/Marker/actions/workflows/build.yml/badge.svg)](https://github.com/alfonsodg/Marker/actions/workflows/build.yml)

## What's New (vs upstream)

### Security
- **RCE fix**: replaced `system()` with `g_spawn_sync()` to prevent shell injection
- **XSS fix**: Content Security Policy + WebKit JS restrictions in preview

### Updated Libraries
- **Mermaid 10.9.1** — all diagram types: flowchart, sequence, class, state, ER, journey, gitgraph, mindmap, timeline, pie, quadrant, sankey, and more
- **KaTeX 0.16.21** — full LaTeX math rendering
- **Highlight.js 11.11.1** — syntax highlighting for 190+ languages

### CLI Export
- `marker file.md -o output.pdf` — export to PDF with Mermaid diagrams
- `marker /dir/ -b -o /output/` — batch export all .md files to PDF
- `marker /dir/ -b -o combined.pdf --merge` — merge all into single PDF
- `marker file.md -o output.pdf --watch` — auto re-export on file changes
- `marker file.md -o output.pdf --landscape` — landscape orientation
- `marker file.md -o output.pdf --no-border` — remove diagram borders

### Editor
- **Search and replace** (Replace / Replace All buttons in search bar)
- **Dark mode** preview with Mermaid diagram support
- **Drag & drop** images into editor (png, jpg, gif, svg, webp)
- **Scroll sync** between editor and preview in dual-pane mode
- **TOC sidebar** with document outline from headings
- **Persistent zoom** per editor tab
- **Keyboard shortcuts**: Ctrl+T (table), Ctrl+Shift+M (mermaid template), Ctrl+K (link)
- **Emoji support**: `:rocket:` → 🚀, `:warning:` → ⚠️, `:white_check_mark:` → ✅

### PDF Export
- **Page numbers** at bottom of each page
- **Styled tables** with borders, striping, and padding
- **Syntax highlighting** colors preserved in PDF
- **Link URLs** shown in parentheses after link text
- **Auto heading numbering** (1. Title, 1.1 Subtitle)
- **One diagram per page** in A4 format

### Code Quality
- Memory leaks fixed (GObject dispose/finalize)
- Buffer over-reads fixed (null terminators)
- 282 lines of dead code removed
- Test suite with 5 tests (`meson test`)
- CI/CD with GitHub Actions

## Features

* View and edit markdown documents
* HTML and LaTeX conversion with [scidown](https://github.com/Mandarancio/scidown/)
  * YAML headers, document classes, beamer/presentation mode
  * Table of Contents, equations, figures, references
* TeX math rendering with [KaTeX](https://katex.org/) or [MathJax](https://mathjax.org/)
* Diagrams with [Mermaid](https://mermaid.js.org/) (flowchart, sequence, class, state, ER, gantt, pie, mindmap, timeline, gitgraph, and more)
* Plotting with [charter](https://github.com/Mandarancio/charter/)
* Syntax highlighting with [highlight.js](https://highlightjs.org/)
* Integrated sketch editor
* Export: PDF, HTML, LaTeX, RTF, ODT, DOCX (via [pandoc](https://pandoc.org/))
* Custom CSS and syntax themes
* Native GTK+3 application

## Screenshots

![scrot.png](help/C/figures/scrot.png)

![scrot1.png](help/C/figures/scrot1.png)

## Installation

### Arch Linux (AUR)

```bash
# Prebuilt binary (recommended, instant install)
yay -S marker-enhanced-bin

# Build from source (compiles latest master)
yay -S marker-enhanced-git
```

### Flatpak

```bash
# Build from manifest
flatpak-builder --install build com.github.fabiocolacio.marker.yml
```

### AppImage

```bash
# Build portable AppImage
./build-appimage.sh
./Marker-Enhanced-x86_64.AppImage
```

### From Source

#### Dependencies

* meson >= 0.37.0
* gtk3-devel >= 3.20
* gtksourceview3-devel
* webkit2gtk-4.1-devel
* gtkspell3-devel
* itstool
* pandoc (optional, for RTF/ODT/DOCX export)

#### Build

```bash
git clone https://github.com/alfonsodg/Marker.git
cd Marker
git submodule update --init --recursive
meson setup builddir
ninja -C builddir
meson test -C builddir
sudo ninja -C builddir install
```

## CLI Usage

```bash
# Open file in editor
marker document.md

# Export to PDF (with Mermaid diagrams)
marker document.md -o output.pdf

# Batch export entire directory
marker /path/to/docs/ -b -o /path/to/pdfs/

# Merge all into single PDF
marker /path/to/docs/ -b -o combined.pdf --merge

# Watch mode — re-export on every save
marker document.md -o output.pdf --watch

# Landscape orientation for wide diagrams
marker document.md -o output.pdf --landscape

# Export to HTML
marker document.md -o output.html

# Editor modes
marker document.md -e    # editor only
marker document.md -p    # preview only
marker document.md -d    # dual pane
marker document.md -w    # dual window
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+T | Insert table template |
| Ctrl+K | Insert link |
| Ctrl+Shift+M | Insert Mermaid diagram template |
| Ctrl+B | Bold |
| Ctrl+I | Italic |
| Ctrl+M | Monospace |
| Ctrl+F | Find |
| Ctrl+R | Refresh preview |
| Ctrl+D | Open sketcher |
| Ctrl+N | New tab |
| Ctrl+Shift+N | New window |
| Ctrl+Q | Quit |

## Credits

- Original project: [Fabio Colacio](https://github.com/fabiocolacio/Marker)
- Enhanced fork: [Alfonso de la Guarda](https://github.com/alfonsodg/Marker)
- Markdown engine: [scidown](https://github.com/Mandarancio/scidown/) by Martino Ferrari

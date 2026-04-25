# <img width="30" src="data/com.github.fabiocolacio.marker.svg"/> Marker Enhanced

Enhanced fork of [Marker](https://github.com/fabiocolacio/Marker) — a markdown editor for Linux made with GTK+-3.0.

This fork adds security fixes, modern JS libraries, CLI batch export, dark mode, and more.

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
- `marker file.md -o output.pdf --watch` — auto re-export on file changes
- `marker file.md -o output.pdf --landscape` — landscape orientation

### Editor
- **Dark mode** preview with Mermaid diagram support
- **Drag & drop** images into editor
- **Scroll sync** between editor and preview in dual-pane mode
- **TOC sidebar** with document outline from headings
- **Table of contents** auto-generated in PDF export
- **Page numbers** in PDF export

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
# Prebuilt binary (recommended)
yay -S marker-enhanced-bin

# Build from source
yay -S marker-enhanced-git
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

## Credits

- Original project: [Fabio Colacio](https://github.com/fabiocolacio/Marker)
- Enhanced fork: [Alfonso de la Guarda](https://github.com/alfonsodg/Marker)
- Markdown engine: [scidown](https://github.com/Mandarancio/scidown/) by Martino Ferrari

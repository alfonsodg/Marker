#!/bin/bash
# Build NeoMarker AppImage
#
# linuxdeploy resolves and bundles every shared library the binary needs, so the
# result runs on a machine that has none of the build dependencies installed.
set -euo pipefail

APP=NeoMarker
APPDIR="$APP.AppDir"
BUILDDIR=builddir-appimage

# Build and stage into the AppDir
meson setup "$BUILDDIR" --prefix=/usr --buildtype=release
ninja -C "$BUILDDIR"
DESTDIR="$(pwd)/$APPDIR" meson install -C "$BUILDDIR" --no-rebuild
rm -rf "$BUILDDIR"

# Bundle the dependencies and produce the AppImage. linuxdeploy reads the
# desktop file and icon from the staged tree.
if [ ! -f linuxdeploy ]; then
  wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" -O linuxdeploy
  chmod +x linuxdeploy
fi

export APPIMAGE_EXTRACT_AND_RUN=1
export OUTPUT="NeoMarker-x86_64.AppImage"
# The appimage plugin validates the AppStream metadata over the network, which
# fails in a sandbox or offline even when the metadata is correct. The file is
# checked separately with appstream-util, which works offline.
export NO_APPSTREAM=1
./linuxdeploy \
  --appdir="$APPDIR" \
  --desktop-file="$APPDIR/usr/share/applications/io.github.alfonsodg.neomarker.desktop" \
  --icon-file="$APPDIR/usr/share/icons/hicolor/scalable/apps/io.github.alfonsodg.neomarker.svg" \
  --executable="$APPDIR/usr/bin/neomarker" \
  --output=appimage

rm -rf "$APPDIR"
echo "AppImage created: $OUTPUT"

#!/bin/bash
# Build NeoMarker AppImage
set -e

APP=NeoMarker
APPDIR="$APP.AppDir"

# Build
meson setup builddir --prefix=/usr
ninja -C builddir
DESTDIR="$(pwd)/$APPDIR" meson install -C builddir --no-rebuild
rm -rf builddir

# AppRun
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export PATH="$HERE/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
export XDG_DATA_DIRS="$HERE/usr/share:$XDG_DATA_DIRS"
export GI_TYPELIB_PATH="$HERE/usr/lib/girepository-1.0"
exec "$HERE/usr/bin/neomarker" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Desktop and icon
cp "$APPDIR/usr/share/applications/io.github.alfonsodg.neomarker.desktop" "$APPDIR/"
cp "$APPDIR/usr/share/icons/hicolor/scalable/apps/io.github.alfonsodg.neomarker.svg" "$APPDIR/"

# Download appimagetool if not present
if [ ! -f appimagetool ]; then
  wget -q "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" -O appimagetool
  chmod +x appimagetool
fi

# Build AppImage
ARCH=x86_64 ./appimagetool "$APPDIR" "NeoMarker-x86_64.AppImage"
rm -rf "$APPDIR"
echo "AppImage created: NeoMarker-x86_64.AppImage"

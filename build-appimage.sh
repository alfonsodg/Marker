#!/bin/bash
# Build Marker AppImage
set -e

APP=Marker
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
exec "$HERE/usr/bin/marker" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Desktop and icon
cp "$APPDIR/usr/share/applications/com.github.fabiocolacio.marker.desktop" "$APPDIR/"
cp "$APPDIR/usr/share/icons/hicolor/scalable/apps/com.github.fabiocolacio.marker.svg" "$APPDIR/"

# Download appimagetool if not present
if [ ! -f appimagetool ]; then
  wget -q "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" -O appimagetool
  chmod +x appimagetool
fi

# Build AppImage
ARCH=x86_64 ./appimagetool "$APPDIR" "Marker-Enhanced-x86_64.AppImage"
rm -rf "$APPDIR"
echo "AppImage created: Marker-Enhanced-x86_64.AppImage"

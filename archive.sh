#!/bin/bash
set -euo pipefail
git clone https://github.com/alfonsodg/NeoMarker
cd Marker || exit 1
git submodule update --init --recursive
rm -rf .git .gitignore .gitmodules
cd ..
zip -r neomarker.zip Marker
rm -rf Marker

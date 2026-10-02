#!/usr/bin/env bash
# Compila Pennyroyal Audio en Linux (Ubuntu/Debian/Mint) y genera el .deb
# Uso:  bash packaging/linux/build-linux.sh
set -e
cd "$(dirname "$0")/../.."

sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git pkg-config \
  libasound2-dev libjack-jackd2-dev ladspa-sdk libcurl4-openssl-dev \
  libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev \
  libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
  libglu1-mesa-dev mesa-common-dev libgtk-3-dev

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cpack --config build/CPackConfig.cmake -B dist

echo ""
echo "Listo. Instaladores en la carpeta dist/:"
ls -1 dist/*.deb dist/*.tar.gz
echo ""
echo "Para instalar:  sudo apt install ./dist/pennyroyal-audio_*_amd64.deb"

#!/usr/bin/env bash
# Build script for macOS and Linux.
#
#   ./build.sh          build everything (gc + wii)
#   ./build.sh gc       build only the GameCube dol
#   ./build.sh wii      build only the Wii dol
#   ./build.sh clean    remove all build output
set -euo pipefail

cd "$(dirname "$0")"

export DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
export DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"
export DEVKITPPC="${DEVKITPPC:-$DEVKITPRO/devkitPPC}"

missing=0
if [ ! -x "$DEVKITARM/bin/arm-none-eabi-gcc" ]; then
	echo "devkitARM not found at $DEVKITARM (install with: sudo dkp-pacman -S gba-dev)" >&2
	missing=1
fi
if [ ! -x "$DEVKITPPC/bin/powerpc-eabi-gcc" ]; then
	echo "devkitPPC not found at $DEVKITPPC (install with: sudo dkp-pacman -S gamecube-dev wii-dev)" >&2
	missing=1
fi
[ "$missing" -eq 0 ] || exit 1

if command -v nproc >/dev/null 2>&1; then
	jobs=$(nproc)
else
	jobs=$(sysctl -n hw.ncpu 2>/dev/null || echo 1)
fi

make -j"$jobs" "$@"

if [ "${1:-all}" != "clean" ]; then
	echo
	ls -l ./*.dol 2>/dev/null || true
fi

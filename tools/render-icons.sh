#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
for size in 16 32 48 64 128 256; do
  mkdir -p "resources/icons/png/$size"
  rsvg-convert -w "$size" -h "$size" resources/icons/io.github.pingskills.plainweight.svg -o "resources/icons/png/$size/io.github.pingskills.plainweight.png"
done

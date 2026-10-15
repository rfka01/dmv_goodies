#!/bin/sh
# Baut DMVKERM.COM aus src/ (JWasm, unter Linux)
set -e
cd "$(dirname "$0")/.."
mkdir -p build
python3 tools/mktab.py src/tabellen.inc
cd src
jwasm -bin -q -Fo../build/DMVKERM.COM -Fl../build/dmvkerm.lst dmvkerm.asm
cd ..
ls -l build/DMVKERM.COM

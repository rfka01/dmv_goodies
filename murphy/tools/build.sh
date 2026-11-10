#!/bin/sh
# Baut MURPHY.COM (JWasm, unter Linux)
set -e
cd "$(dirname "$0")/.."
mkdir -p build
jwasm -bin -q -Fobuild/MURPHY.COM -Flbuild/murphy.lst MURPHY.ASM
ls -l build/MURPHY.COM

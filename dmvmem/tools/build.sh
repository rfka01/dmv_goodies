#!/bin/sh
# Baut DMVMEM.COM (JWasm, unter Linux)
set -e
cd "$(dirname "$0")/.."
mkdir -p build
jwasm -bin -q -Fobuild/DMVMEM.COM -Flbuild/dmvmem.lst DMVMEM.ASM
ls -l build/DMVMEM.COM

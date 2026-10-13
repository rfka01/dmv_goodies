#!/bin/sh
# Baut beide Fassungen von MS-DOS Kermit 3.16 aus ../src unter Linux:
#   KERMIT.EXE   NCR Decision Mate V (-Ddmv)
#   KERMIBM.EXE  IBM-PC ohne Netzwerk (wie das Original mit no_network)
# Werkzeuge: jwasm und jwlink (Open-Watcom-Abkoemmlinge), optional upx
# fuer KERMIBM.EXE. Aufruf: sh tools/build.sh [Zielverzeichnis]
set -e
HIER=$(cd "$(dirname "$0")/.." && pwd)
ZIEL=${1:-$HIER/build}
ARB=$(mktemp -d)
trap 'rm -rf "$ARB"' EXIT
# die Quellen schreiben ihre Includes klein: unter Linux kleine Namen
for f in "$HIER"/src/*; do
    cp "$f" "$ARB/$(basename "$f" | tr 'A-Z' 'a-z')"
done
COMMON="msscmd msscom mssfil mssker mssrcv mssscp msssen mssser mssset msssho msster msuibm msxibm msyibm mszibm"
mkdir -p "$ZIEL"

baue() {   # $1 = Name, $2 = Schalter, $3 = zusaetzliche Module
    out=$ARB/$1
    mkdir -p "$out"
    objs=""
    for m in $COMMON $3; do
        (cd "$ARB" && jwasm -Cx -Zm -Zp1 -q $2 -Fo"$out/$m.obj" $m.asm) \
            > "$out/$m.err" 2>&1 || { cat "$out/$m.err"; exit 1; }
        if grep -qi "error" "$out/$m.err"; then cat "$out/$m.err"; exit 1; fi
        objs="$objs,$out/$m.obj"
    done
    jwlink format dos name "$ZIEL/$1" option map="$ZIEL/$1.map",quiet \
        file "${objs#,}" > "$out/link.err" 2>&1 || { cat "$out/link.err"; exit 1; }
    ls -l "$ZIEL/$1"
}

baue KERMIT.EXE "-Ddmv" "msxdmv"
baue KERMIBM.EXE "-Dno_network" "msgibm"
if command -v upx >/dev/null 2>&1; then
    upx --best -q "$ZIEL/KERMIBM.EXE" >/dev/null && ls -l "$ZIEL/KERMIBM.EXE"
fi

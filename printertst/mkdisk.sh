#!/bin/sh
# Build the printer test disk ../Images/DMVPRN.IMG from the files in this directory:
# the .PRN test files (mkprn.py), the batch files (mkbat.py) and LIESMICH.TXT.
set -e
cd "$(dirname "$0")"
OUT=$(mktemp -d)
python3 mkprn.py "$OUT"
python3 mkbat.py "$OUT"
cp LIESMICH.TXT "$OUT/"
python3 mkimg.py ../Images/DMVPRN.IMG DRUCKTEST $(ls "$OUT"/* | sort)
rm -rf "$OUT"

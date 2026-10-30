#!/usr/bin/env python3
"""mkfont.py - GFONT.H fuer DMVGRAF aus Hopplers Schrift.

    mkfont.py PFAD/dmv_games/hoppler/tools [GFONT.H]

Die Schrift ist die von HOPPLER (rfka01/dmv_games): 8 x 16, Bit 0 = linker
Punkt, Index 0-94 = Zeichen 32-126, 95-101 = Umlaute in Codepage 437
(84 94 81 8E 99 9A E1).
"""
import sys

sys.path.insert(0, sys.argv[1])
from font_def import F, UML, cell  # noqa: E402

zeilen = [cell(F[chr(c)]) for c in range(32, 127)]
codes = list(UML.keys())
for k in codes:
    nm, art, cap = UML[k]
    zeilen.append(cell(art, cap))

ziel = sys.argv[2] if len(sys.argv) > 2 else 'GFONT.H'
out = ['/* GFONT.H - erzeugt von tools/mkfont.py aus Hopplers Schrift',
       '   (rfka01/dmv_games, hoppler/tools/font_def.py), nicht von Hand aendern.',
       '   8 x 16, Zeile 0 oben, Bit 0 = linker Punkt. Index 0-94 = Zeichen 32-126,',
       '   95-101 = Umlaute in Codepage 437. MIT */',
       'char fntuml[7] = {' + ', '.join('0x%02X' % k for k in codes) + '};',
       'char fntbit[%d] = {' % (len(zeilen) * 16)]
for z in zeilen:
    out.append('  ' + ','.join('0x%02X' % v for v in z) + ',')
out.append('};')
with open(ziel, 'w', newline='\n') as f:
    f.write('\n'.join(out) + '\n')
print(ziel)

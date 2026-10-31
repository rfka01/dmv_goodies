#!/usr/bin/env python3
"""bau68.py - C-Programme mit DMVGRAF fuer CP/M-68K uebersetzen (gcc).

  bau68.py [-I dir] [-D NAME=WERT] -o PROG.68K quelle.c ... quelle.S ... [x.o ...] [DMVGRAF.A]

Uebersetzt mit m68k-linux-gnu-gcc fuer den 68000 (der 68008 der K234 kennt
nichts darueber hinaus), bindet mit GCRT68.S (muss als erste Quelle stehen,
'start' vorn; es bringt auch die 32-Bit-Rechenhilfen mit, denn libgcc
enthaelt 68020-Befehle) und wandelt mit elf2cpm.py in eine verschiebbare
.68K-Datei. libgcc wird nicht gebunden (sie enthaelt 68020-Befehle);
alles uebrige uebersetzt gcc mit -m68000; -mstrict-align und
-fno-store-merging, weil gcc sonst benachbarte Bytes zu Wort- oder
Langwortzugriffen auf ungerade Adressen zusammenfasst (Adressfehler auf dem
68000/68008).

Fertig uebersetzte Objekte (.O) und Archive (.A) werden mitgebunden.
Endet -o auf .A, entsteht statt eines Programms ein Archiv aus den
Quellen (so entsteht lib/cpm68k/DMVGRAF.A); auf .O ein einzelnes Objekt.

Braucht gcc-m68k-linux-gnu und binutils-m68k-linux-gnu.
"""
import argparse
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import elf2cpm

CC = 'm68k-linux-gnu-gcc'
CFLAGS = ['-m68000', '-mstrict-align', '-fno-store-merging', '-O2', '-fomit-frame-pointer', '-fno-pic', '-fno-pie',
          '-ffreestanding', '-fno-builtin', '-fno-common',
          '-fno-asynchronous-unwind-tables', '-std=gnu89', '-w']


def q_bibliothek(ziel, objs):
    """-o X.A: Archiv aus den Objekten, -o X.O: ein einzelnes Objekt"""
    import shutil
    if ziel.upper().endswith('.A'):
        if os.path.exists(ziel):
            os.remove(ziel)
        r = subprocess.run(['m68k-linux-gnu-ar', 'rcsD', ziel] + objs,
                           capture_output=True, text=True)
        if r.returncode:
            sys.exit(r.stderr)
        print('%s: %d Objekte' % (ziel, len(objs)))
        return True
    if ziel.upper().endswith('.O'):
        shutil.copy(objs[0], ziel)
        print(ziel)
        return True
    return False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-I', action='append', default=[])
    ap.add_argument('-D', action='append', default=[])
    ap.add_argument('-o', required=True)
    ap.add_argument('quellen', nargs='+')
    a = ap.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        objs = []
        for i, q in enumerate(a.quellen):
            if q.upper().endswith(('.O', '.A')):   # fertig uebersetzt
                objs.append(os.path.abspath(q))
                continue
            o = os.path.join(tmp, os.path.splitext(os.path.basename(q))[0]
                             .upper() + '.O')
            art = ['-x', 'assembler-with-cpp', '-Wa,--register-prefix-optional'] \
                if q.upper().endswith('.S') else ['-x', 'c']
            cmd = [CC] + CFLAGS + ['-I' + d for d in a.I] + \
                ['-D' + d for d in a.D] + art + ['-c', q, '-o', o]
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode:
                sys.exit(r.stderr)
            objs.append(o)
        if q_bibliothek(a.o, objs):
            return
        n, bss, nrel = elf2cpm.wandle(objs, a.o)
    print('%s: Text+Daten %d, BSS %d, %d Verschiebungen' % (a.o, n, bss, nrel))


if __name__ == '__main__':
    main()

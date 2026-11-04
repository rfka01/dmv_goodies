#!/usr/bin/env python3
"""Baut eine selbststartende Diagnosediskette (360 KB, DMV-Format).

    mkimg.py [name] [zielverzeichnis]

name: Programm in dmvdiag/ ohne Endung, Vorgabe dmvdiag.

Die Version steht an EINER Stelle, in diag/VERSION (fuer ein anderes
Programm in diag/VERSION.<name>, falls vorhanden). Daraus entstehen
  - die Anzeige im Programm (nasm -DVERSION='x.yy'),
  - der Dateiname des Images: DMVDIAG-x.yy.IMG,
  - der Datentraegername: DMVDIAGx.yy (11 Zeichen, ohne Leerzeichen).
Ausgabe nach dmvdiag/build/ (von .gitignore ausgenommen).
Jede Aenderung am Programm oder Image bekommt eine neue Version.

Ablauf: Programm assemblieren (zweimal: es braucht seine eigene
Sektorzahl, um die Ergebnisdateien schreiben zu koennen), Bootsektor mit PROGSECS = Sektorzahl des
Programms assemblieren, Programm in die reservierten Sektoren 1..n
legen, FAT und Verzeichnis leer anlegen. DOS sieht eine leere, gueltige
360-KB-Diskette.

Die Sektorfolge im Image ist die des DMV-Treibers: Zylinder fuer
Zylinder, je Zylinder Kopf 0 und Kopf 1, je 9 Sektoren.
"""
import os
import re
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
PROG_ADR = 0x4000
PROG_MAX = 0xA000          # ab A000h Textpuffer der Ergebnisdateien (results.inc)


def nasm(src, out, *defs):
    subprocess.check_call(['nasm', '-f', 'bin', *defs, '-o', out, src],
                          cwd=HERE)


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else 'dmvdiag'
    outdir = sys.argv[2] if len(sys.argv) > 2 else \
        os.path.join(HERE, 'build')
    vfile = os.path.join(HERE, 'VERSION.' + name)   # eigene Version, z. B. VERSION.dmvfrage
    if not os.path.exists(vfile):
        vfile = os.path.join(HERE, 'VERSION')
    ver = open(vfile).read().strip()
    if not re.fullmatch(r'\d+\.\d\d', ver):
        sys.exit('diag/VERSION: erwartet n.nn, gefunden %r' % ver)
    os.makedirs(outdir, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp:
        progbin = os.path.join(tmp, 'prog.bin')
        t = time.localtime()
        fdate = ((t.tm_year - 1980) << 9) | (t.tm_mon << 5) | t.tm_mday
        ftime = (t.tm_hour << 11) | (t.tm_min << 5) | (t.tm_sec // 2)
        defs = ["-DVERSION='%s'" % ver, '-DFATDATE=%d' % fdate,
                '-DFATTIME=%d' % ftime]
        # zwei Durchgaenge: das Programm braucht seine eigene Sektorzahl
        # (Lage von FAT und Verzeichnis fuer die Ergebnisdateien)
        nasm(name + '.nasm', progbin, *defs, '-DPROGSECS=1')
        secs = (len(open(progbin, 'rb').read()) + 511) // 512
        nasm(name + '.nasm', progbin, *defs, '-DPROGSECS=%d' % secs)
        prog = open(progbin, 'rb').read()
        assert (len(prog) + 511) // 512 == secs
        if PROG_ADR + len(prog) > PROG_MAX:
            sys.exit('Programm zu gross: %d Bytes, erlaubt %d'
                     % (len(prog), PROG_MAX - PROG_ADR))
        secs = (len(prog) + 511) // 512
        bootbin = os.path.join(tmp, 'boot.bin')
        nasm('boot.nasm', bootbin, '-DPROGSECS=%d' % secs)
        boot = open(bootbin, 'rb').read()
    assert len(boot) == 512

    img = bytearray(b'\xF6' * 368640)
    img[0:512] = boot
    img[512:512 + len(prog)] = prog
    res = 1 + secs
    spf = 2
    for f in range(2):                       # zwei FATs
        off = (res + f * spf) * 512
        img[off:off + spf * 512] = bytes(spf * 512)
        img[off:off + 3] = b'\xFD\xFF\xFF'
    root = (res + 2 * spf) * 512
    img[root:root + 7 * 512] = bytes(7 * 512)  # 112 Eintraege
    label = (name.upper() + ver)[:11].ljust(11).encode()
    img[root:root + 11] = label
    img[root + 11] = 0x08                    # Datentraegername

    out = os.path.join(outdir, '%s-%s.IMG' % (name.upper(), ver))
    open(out, 'wb').write(img)
    open(os.path.join(outdir, '%s-%s.bin' % (name, ver)), 'wb').write(prog)
    print('%s: Version %s, Programm %d Bytes in %d Sektoren'
          % (out, ver, len(prog), secs))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""mkcpm16.py - Testdiskette fuer 16-Bit-CP/M bauen (DD320-Datendiskette).

  mkcpm16.py [zielverzeichnis]        (Vorgabe: dmvdiag/build)

Inhalt: GTEST und MTEST fuer CP/M-68K (hier mit gcc uebersetzt, ueber
dmvgraf/tools/bau68.py; braucht gcc-m68k-linux-gnu) und fuer CP/M-8000
(GTEST.Z8K, MTEST.Z8K liegen fertig hier, siehe README), dazu
LIESMICH.TXT. CP/M-86 folgt, wenn DMVGRAF dafuer eine Fassung hat.

Die Version steht in grafik/VERSION; daraus der Dateiname
CPM16TEST-x.yy.IMG. Jede Aenderung an Programm oder Abbild bekommt eine
neue Version.

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import os, re, subprocess, sys, tempfile

HIER = os.path.dirname(os.path.abspath(__file__))
GRAF = os.path.join(HIER, '..', '..', 'dmvgraf')
SRC = os.path.join(GRAF, 'src')
sys.path.insert(0, HIER)
import dd320

LIB68 = ['GSUP68.S', 'GARG68.C', 'GRAF.C', 'GBILD.C', 'GEIN.C', 'GK68.S',
         'GHWCPM.C', 'GHD68K.C', 'GHPOOL.C']


def bau68(prog, aus):
    q = [os.path.join(SRC, 'GCRT68.S'), os.path.join(HIER, prog)] + \
        [os.path.join(SRC, f) for f in LIB68]
    subprocess.check_call([sys.executable, os.path.join(GRAF, 'tools', 'bau68.py'),
                           '-I', SRC, '-D', 'GBPOOL=61440', '-o', aus] + q)
    return open(aus, 'rb').read()


def main():
    ziel = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HIER, '..', 'build')
    ver = open(os.path.join(HIER, 'VERSION')).read().strip()
    if not re.fullmatch(r'\d+\.\d\d', ver):
        sys.exit('grafik/VERSION: erwartet n.nn, gefunden %r' % ver)
    os.makedirs(ziel, exist_ok=True)
    with tempfile.TemporaryDirectory() as t:
        g68 = bau68('GTEST.C', os.path.join(t, 'GTEST.68K'))
        m68 = bau68('MTEST.C', os.path.join(t, 'MTEST.68K'))
    rd = lambda n: open(os.path.join(HIER, n), 'rb').read()
    # Reihenfolge zaehlt: die .Z8K-Dateien muessen auf Seite 0 liegen
    # (die ersten 37 Spuren). Wie CP/M-8000 Seite 1 belegt, ist nicht
    # belegt; die Belegung durch CP/M-68K ist an Disketten geprueft.
    dateien = [('LIESMICH.TXT', rd('LIESMICH.TXT')),
               ('GTEST.Z8K', rd('GTEST.Z8K')), ('MTEST.Z8K', rd('MTEST.Z8K')),
               ('GTEST.68K', g68), ('MTEST.68K', m68)]
    aus = os.path.join(ziel, 'CPM16TEST-%s.IMG' % ver)
    n = dd320.schreiben(dateien, aus)
    img = open(aus, 'rb').read()
    for name, d in dateien:             # gegenlesen
        if dd320.datei(img, name)[:len(d)] != d:
            sys.exit('%s: falsch im Abbild' % name)
    for e in dd320.eintraege(img):      # .Z8K nur auf Seite 0
        if e[0] == 0 and e[9:12] == b'Z8K' and max(e[16:32]) >= dd320.SEITE0:
            sys.exit('.Z8K reicht auf Seite 1 - Reihenfolge pruefen')
    print('%s: %d von %d Bloecken' % (aus, n, dd320.BLOECKE))


main()

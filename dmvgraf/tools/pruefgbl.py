#!/usr/bin/env python3
"""pruefgbl.py - Pruefsammlungen fuer gbldv schreiben und (am PC) pruefen.

  pruefgbl.py VERZ [--lauf PROG]

Schreibt nach VERZ:
  GLKLEIN.GBL  14 Bilder aller Formate 1-5, Breiten nicht durch 8 oder 16
               teilbar (Format 5 bis 32 x 32), kopf 0
  GLOHNE5.GBL  dieselben ohne Format 5, kopf 0
  GLBKL.GBL    nur die Formate 1, 2, 4 (DMVBKL.INC), kopf 0
  GLGROSS.GBL  40000 Byte Kopf, dann Bilder mit zusammen ueber 26 KB:
               Datei laenger als 64 KB, kopf > 32 KB
  GLKURZ.GBL   GLKLEIN.GBL ohne die letzten 100 Byte (gbldv: GE_KURZ,
               die Bilder davor bleiben)

--lauf PROG: das PC-Programm gltest (make gltest) mit jeder Sammlung
zweimal laufen lassen (gbldv und gbdef) und die Bildschirmabzuege
vergleichen.

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import argparse, os, random, subprocess, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mkgbl

KLEIN = [(13, 9, 1), (21, 7, 2), (5, 11, 3), (27, 13, 4), (17, 17, 5),
         (1, 3, 4), (31, 5, 1), (9, 21, 2), (23, 6, 3), (33, 12, 4),
         (32, 32, 5), (3, 1, 5), (47, 9, 4), (63, 14, 2)]
GROSS = [(320, 300, 2), (250, 320, 1), (61, 37, 4), (19, 30, 5), (200, 40, 3)]


def bild(z, b, h, f):
    n = mkgbl.daten(b, h, f)
    d = bytearray(z.randrange(256) for _ in range(n))
    if f == 3:                          # nur Farben 1-15 und 0 (frei)
        pass
    if f == 4:                          # Ebenen nur unter der Maske
        m = (b + 7) // 8
        for y in range(h):
            o = y * 4 * m
            for k in range(m):
                for e in (1, 2, 3):
                    d[o + e * m + k] &= d[o + k]
    return mkgbl.eintrag(b, h, f, d)


def schreiben(verz):
    z = random.Random(1103)
    kl = [bild(z, *t) for t in KLEIN]
    gr = [bild(z, *t) for t in GROSS]
    w = lambda n, d: open(os.path.join(verz, n), 'wb').write(d)
    w('GLKLEIN.GBL', b''.join(kl))
    w('GLOHNE5.GBL', b''.join(e for e, t in zip(kl, KLEIN) if t[2] != 5))
    w('GLBKL.GBL', b''.join(e for e, t in zip(kl, KLEIN) if t[2] in (1, 2, 4)))
    g = bytes(z.randrange(256) for _ in range(40000)) + b''.join(gr) + b''.join(kl[:6])
    w('GLGROSS.GBL', g)
    w('GLKURZ.GBL', b''.join(kl)[:-100])
    print('GLKLEIN %d, GLOHNE5 %d, GLGROSS %d Byte (kopf 40000)' % (
        sum(map(len, kl)), sum(len(e) for e, t in zip(kl, KLEIN) if t[2] != 5), len(g)))


def lauf(prog, verz):
    prog = os.path.abspath(prog)
    fehler = 0
    for n, k, soll in (('GLKLEIN.GBL', 0, '0 14'), ('GLOHNE5.GBL', 0, '0 11'), ('GLBKL.GBL', 0, '0 9'),
                       ('GLGROSS.GBL', 40000, '0 11'), ('GLKURZ.GBL', 0, '93 13')):
        b = []
        for m in ('', 'd'):
            ppm = 'gl_%s%s.ppm' % (n[:-4], m)
            open(os.path.join(verz, 'gl.txt'), 'w').write('bild %s\ntaste x\n' % ppm)
            r = subprocess.run([prog, n, str(k)] + ([m] if m else []), cwd=verz,
                               env=dict(os.environ, GSKRIPT='gl.txt'),
                               capture_output=True, text=True)
            aus = [l for l in (r.stdout + r.stderr).splitlines() if l.startswith('gbldv')]
            aus = aus[0] if aus else r.stderr.strip()
            b.append(open(os.path.join(verz, ppm), 'rb').read())
            print('%-12s %-2s %s' % (n, m or 'v', aus))
            if m == '' and aus != 'gbldv: ' + soll:
                print('  erwartet gbldv: %s' % soll)
                fehler += 1
        if b[0] != b[1]:
            print('  Bilder verschieden!')
            fehler += 1
    print('gut' if not fehler else '%d Fehler' % fehler)
    return fehler


ap = argparse.ArgumentParser()
ap.add_argument('verz')
ap.add_argument('--lauf')
a = ap.parse_args()
os.makedirs(a.verz, exist_ok=True)
schreiben(a.verz)
if a.lauf:
    sys.exit(1 if lauf(a.lauf, a.verz) else 0)

#!/usr/bin/env python3
"""pruefpic.py - Rundlauf fuer gepackte Vollbilder (gpic, gpicm, GPIC.INC):
Testdateien mit tools/dmvpack.py packen, mit test/PICTEST zeigen lassen
und den Bildspeicher mit den Rohdaten vergleichen - Farbe und mono, volle
und schmale Breite, xw/y ungleich 0, Speicher statt Datei, Grenzfaelle
und kaputte Dateien.

  pruefpic.py VERZ [--host pictest] [--dos PICTEST.EXE] [--com GPT.COM]
              [--m68 PICTEST.68K] [--kitt KITT.PIC] [--tut TUT.PIC]

VERZ: Arbeitsverzeichnis (wird angelegt). --host: PC-Simulation (make
pictest), --dos: MS-DOS im Pruefstand pruefdos.py (make PICTEST.EXE),
--com: dasselbe mit GPIC.INC aus MASM (test/GPT.ASM, JWasm), --m68:
CP/M-68K im Pruefstand pruef68.py. KITT.PIC und TUT.PIC (DMVDEMO) werden
mitgeprueft, wenn angegeben.

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import argparse, os, random, struct, subprocess, sys

HIER = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HIER)
import dmvpack as dp

ap = argparse.ArgumentParser()
ap.add_argument('verz')
ap.add_argument('--host')
ap.add_argument('--dos')
ap.add_argument('--com')
ap.add_argument('--m68')
ap.add_argument('--kitt')
ap.add_argument('--tut')
a = ap.parse_args()
os.makedirs(a.verz, exist_ok=True)
os.chdir(a.verz)


def muster(w, h, saat, art=0):
    """Testebene w x h Worte: Raster, Kanten, Zufall - LZSS findet etwas"""
    r = random.Random(saat)
    d = bytearray()
    for y in range(h):
        for x in range(2 * w):
            if art == 0:
                v = (0x55 if (y >> 2) & 1 else 0xAA) if (x + y // 8) % 5 else r.randrange(256)
            else:
                v = r.randrange(256) if r.random() < 0.3 else (x * 7 + y) & 0xFF
            d.append(v)
    return bytes(d)


def schreib(name, ebenen, w, h, roh=''):
    b, info = dp.schreiben(ebenen, w, h, roh)
    open(name, 'wb').write(b)
    return info


# ---- erwarteter Bildspeicher ------------------------------------------------

class Schirm:
    def __init__(s):
        s.e = [bytearray(32000) for _ in range(3)]

    def setz(s, e, xw, y, w, h, d):
        for z in range(h):
            o = (y + z) * 80 + 2 * xw
            s.e[e][o:o + 2 * w] = d[z * 2 * w:(z + 1) * 2 * w]

    def bild(s, name, ebenen, xw, y, mono):
        w, h, eb = dp.lesen(open(name, 'rb').read())
        d = dict(eb)
        if mono:
            if 'M' in d:
                s.setz(0, xw, y, w, h, d['M'])
            elif 'G' in d:
                s.setz(0, xw, y, w, h, d['G'])
            return
        if any(x in d for x in 'GRB'):
            for i, x in enumerate('GRB'):
                s.setz(i, xw, y, w, h, d.get(x, bytes(2 * w * h)))
        else:
            for i in range(3):
                s.setz(i, xw, y, w, h, d['M'])

    def ppm(s, mono):
        px = bytearray()
        for y in range(400):
            for x in range(640):
                o = y * 80 + (x >> 4) * 2
                b = x & 15
                bit = [(((e[o] | e[o + 1] << 8) >> b) & 1) for e in s.e]
                if mono:
                    px += bytes((40, 255, 90)) if bit[0] else bytes(3)
                else:
                    px += bytes((255 * bit[1], 255 * bit[0], 255 * bit[2]))
        return b'P6 640 400 255\n' + bytes(px)


def lies_ppm(n):
    b = open(n, 'rb').read()
    return b[b.index(b'255\n') + 4:]


# ---- Laeufe -----------------------------------------------------------------

open('s.txt', 'w').write('bild aus.ppm\ntaste x\n')


def lauf(art, prog, args, mono):
    if os.path.exists('aus.ppm'):
        os.remove('aus.ppm')
    if art == 'host':
        env = dict(os.environ, GSKRIPT='s.txt', GMONO='1' if mono else '0')
        r = subprocess.run([prog] + args, env=env, capture_output=True, text=True)
        o = r.stdout + r.stderr
    else:
        werk = {'dos': 'pruefdos.py', 'com': 'pruefdos.py', 'm68': 'pruef68.py'}[art]
        cmd = [sys.executable, os.path.join(HIER, werk), prog, 's.txt',
               '--arg', ' '.join(args)] + (['--mono'] if mono else [])
        r = subprocess.run(cmd, capture_output=True, text=True)
        o = r.stdout + r.stderr
    codes = None
    for z in o.splitlines():
        if 'gpic:' in z:
            codes = [int(x) for x in z.split('gpic:')[1].split()]
    return codes, (lies_ppm('aus.ppm') if os.path.exists('aus.ppm') else None), o


fehler = 0
faelle = []        # (Name, [(Datei, xw, y, art)], erwartete Rueckgaben, nur_aussen)

# KITT: nur M, alt roh 32000, alt roh 128000 (aus DMVDEMO, sonst Ersatz)
if a.kitt:
    kraw = open(a.kitt, 'rb').read()
else:
    kraw = b''.join(muster(40, 400, i, 1) for i in range(4))
kg = {x: kraw[i * 32000:(i + 1) * 32000] for i, x in enumerate('GRBM')} \
    if len(kraw) == 128000 else {'M': kraw}
schreib('KITTM.PIC', [('M', kg['M'])], 40, 400)
open('KITTALT.PIC', 'wb').write(kraw)
open('MALT.PIC', 'wb').write(kg['M'])
faelle.append(('KITT nur M, volle Breite', [('KITTM.PIC', 0, 0, '0')], [0], 0))
faelle.append(('KITT M, Anzeige dunkel', [('KITTM.PIC', 0, 0, '1')], [0], 0))
faelle.append(('alte Rohdatei', [('KITTALT.PIC', 0, 0, '0')], [0], 0))
faelle.append(('alte Rohdatei 32000', [('MALT.PIC', 0, 0, '0')], [0], 0))

# schmal, vier Ebenen, R roh, xw/y ungleich 0
w, h = 24, 200
eb4 = [(x, muster(w, h, 10 + i, i & 1)) for i, x in enumerate('GRBM')]
schreib('SCHMAL.PIC', eb4, w, h, roh='R')
faelle.append(('schmal 24 x 200 bei 5/77, R roh', [('SCHMAL.PIC', 5, 77, '0')], [0], 0))

# aus dem Speicher (klein)
schreib('KLEIN.PIC', [(x, muster(10, 60, 30 + i)) for i, x in enumerate('GRBM')], 10, 60)
faelle.append(('aus dem Speicher 10 x 60 bei 29/333', [('KLEIN.PIC', 29, 333, '0S')], [0], 0))

# nur G und B ueber ein volles Bild: R wird dort geloescht
schreib('GB.PIC', [('G', muster(12, 50, 40)), ('B', muster(12, 50, 41))], 12, 50)
faelle.append(('G und B ueber KITT (R loeschen)', [('KITTALT.PIC', 0, 0, '0'),
               ('GB.PIC', 20, 300, '0')], [0, 0], 0))

# Grenzfaelle: Abstand 4095, Abstand 1 Laenge 18, Ende mitten im Flagbyte
r = random.Random(7)
blk = bytes(r.randrange(256) for _ in range(4095))
d = blk + blk[:3500] + bytes([0x33] * 405)
d = d[:8000]
info = schreib('GRENZ.PIC', [('M', d)], 40, 100)
p = open('GRENZ.PIC', 'rb').read()[16:]
dists, i, flags = [], 0, 0
while i < len(p):
    f = p[i]; i += 1
    for bit in range(8):
        if i >= len(p):
            break
        if f >> bit & 1:
            dists.append((p[i] | (p[i + 1] >> 4) << 8, (p[i + 1] & 15) + 3)); i += 2
        else:
            i += 1
print('Grenzfaelle im Strom: Abstand 4095 %s, Abstand 1/Laenge 18 %s'
      % ('ja' if any(x == 4095 for x, _ in dists) else 'NEIN',
         'ja' if (1, 18) in dists else 'NEIN'))
faelle.append(('Grenzfaelle LZSS', [('GRENZ.PIC', 0, 150, '0')], [0], 0))

if a.tut:
    faelle.append(('TUT', [(a.tut, 0, 0, '0')], [0], 0))

# kaputte Dateien: Rueckgabe pruefen, nichts ausserhalb des Bildes
b = open('SCHMAL.PIC', 'rb').read()
open('KURZ.PIC', 'wb').write(b[:len(b) // 2])
open('VERS.PIC', 'wb').write(b[:4] + b'\x02' + b[5:])
# Verweis vor den Anfang: Flag 1, Abstand 5
open('VERW.PIC', 'wb').write(b'DMVP' + struct.pack('<BBHHH', 1, 1, 4, 4, 0) +
                             struct.pack('<BBH', ord('M'), 1, 3) + bytes([1, 5, 0]))
# Ebenenlaenge um eins zu gross (ein Byte zu viel in der Ebene)
schreib('KLEINM.PIC', [('M', muster(10, 60, 33))], 10, 60)
x = bytearray(open('KLEINM.PIC', 'rb').read())
l0 = x[14] | x[15] << 8
x[14:16] = struct.pack('<H', l0 + 1)
x = x + b'\0'
open('LANG.PIC', 'wb').write(bytes(x))
faelle.append(('zu kurz', [('KURZ.PIC', 5, 77, '0')], [93], 1))
faelle.append(('Version 2', [('VERS.PIC', 5, 77, '0')], [94], 1))
faelle.append(('Verweis vor den Anfang', [('VERW.PIC', 3, 3, '0')], [97], 1))
faelle.append(('Ebenenlaenge stimmt nicht', [('LANG.PIC', 29, 333, '0')], [93], 1))
faelle.append(('ragt ueber den Schirm', [('SCHMAL.PIC', 20, 0, '0')], [96], 1))
faelle.append(('Datei fehlt', [('FEHLT.PIC', 0, 0, '0')], [92], 1))

laeufer = [(n, p) for n, p in (('host', a.host), ('dos', a.dos), ('com', a.com),
                               ('m68', a.m68)) if p]
for name, liste, erw, aussen in faelle:
    for mono in (False, True):
        s = Schirm()
        args = []
        for (f, xw, y, art) in liste:
            args += [f, str(xw), str(y), art]
            if not aussen:
                s.bild(f, None, xw, y, mono)
        soll = s.ppm(mono)[15:]
        for art, prog in laeufer:
            ar = list(args)
            if art == 'com':            # GPIC.INC kennt die Karte nicht
                for k in range(3, len(ar), 4):
                    ar[k] = str(int(ar[k].rstrip('Ss')) | (2 if mono else 4))
            codes, ist, o = lauf(art, prog, ar, mono)
            gut = codes == erw and ist is not None
            if gut and not aussen:
                gut = ist == soll
            elif gut and aussen:        # ausserhalb des Bildes alles schwarz
                (f, xw, y, _), = liste
                try:
                    w, h, _ = dp.lesen(open(f, 'rb').read()) if f not in (
                        'KURZ.PIC', 'VERW.PIC', 'LANG.PIC') else (
                        (24, 200, 0) if f == 'KURZ.PIC' else
                        (4, 4, 0) if f == 'VERW.PIC' else (10, 60, 0))
                except Exception:
                    w, h = 0, 0
                for yy in range(400):
                    for xx in range(640):
                        if xw * 16 <= xx < (xw + w) * 16 and y <= yy < y + h:
                            continue
                        if ist[3 * (yy * 640 + xx):3 * (yy * 640 + xx) + 3] != b'\0\0\0':
                            gut = False
                            break
                    if not gut:
                        break
            print('%-5s %-4s %-40s %s' % (art, 'mono' if mono else 'farb', name,
                  'gut' if gut else 'FEHLER %s %s' % (codes, o[-300:] if codes is None else '')))
            if not gut:
                fehler += 1
print('Fehler:', fehler)
sys.exit(1 if fehler else 0)

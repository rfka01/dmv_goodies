#!/usr/bin/env python3
"""dmvpack.py - Vollbilder fuer die NCR DMV packen (.PIC mit Kennung DMVP,
LZSS) und zur Kontrolle wieder entpacken. Format: README.md, Abschnitt
"Gepackte Vollbilder".

Packen:
  dmvpack.py ein.raw aus.PIC                 Rohdatei 128 000 Byte (G R B M)
                                             oder 32 000 Byte (M), 40 x 400
  dmvpack.py bild.png aus.PIC [--mono m.png] PNG in den 8 DMV-Farben; die
                                             Mono-Ebene aus einem zweiten PNG
                                             (hell = gesetzt)
  Optionen: --ebenen GRBM   welche Ebenen in die Datei kommen (Vorgabe: alle
                            vorhandenen; bei einer Rohdatei 32 000: M)
            --roh G         diese Ebenen ungepackt (geschieht auch von selbst,
                            wenn LZSS groesser wuerde)
            --breite W --hoehe H   Groesse in Worten und Zeilen (Rohdatei
                            oder PNG kleiner als 640 x 400)
Kontrolle:
  dmvpack.py --entpacken x.PIC aus.raw|aus.png

Nach dem Packen entpackt das Werkzeug selbst, vergleicht byteweise und
zeigt die Groessen. Braucht Python 3, fuer PNG Pillow.

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import argparse, struct, sys

FARBEN = {(0, 0, 0): 0, (0, 255, 0): 1, (255, 0, 0): 2, (255, 255, 0): 3,
          (0, 0, 255): 4, (0, 255, 255): 5, (255, 0, 255): 6,
          (255, 255, 255): 7}           # Bits: 1 Gruen, 2 Rot, 4 Blau
EBIT = {'G': 1, 'R': 2, 'B': 4}


# ---- LZSS -------------------------------------------------------------------

def packen(d, tiefe=64):
    """gierig mit Nachschau um ein Byte (lazy matching); Fenster 4 KB,
    Laengen 3-18, Abstand 1-4095"""
    n = len(d)
    kette = {}                          # 3-Byte-Schluessel -> Positionen
    aus = bytearray()

    def treffer(i):
        if i + 3 > n:
            return 0, 0
        bl, bd = 0, 0
        k = d[i:i + 3]
        for j in reversed(kette.get(k, ())[-tiefe:]):
            dist = i - j
            if dist > 4095:
                break
            l = 3
            while l < 18 and i + l < n and d[j + l] == d[i + l]:
                l += 1
            if l > bl:
                bl, bd = l, dist
                if l == 18:
                    break
        return bl, bd

    def merke(i):
        if i + 3 <= n:
            kette.setdefault(d[i:i + 3], []).append(i)

    i = 0
    flag = 0
    fpos = -1
    bit = 8
    while i < n:
        if bit == 8:
            fpos = len(aus)
            aus.append(0)
            bit = 0
        l, dist = treffer(i)
        if l >= 3:
            l2, _ = treffer(i + 1)
            if l2 > l:                  # lieber erst ein Literal
                l = 0
        if l >= 3:
            aus[fpos] |= 1 << bit
            aus.append(dist & 0xFF)
            aus.append(((dist >> 8) << 4) | (l - 3))
            for k in range(l):
                merke(i + k)
            i += l
        else:
            aus.append(d[i])
            merke(i)
            i += 1
        bit += 1
    return bytes(aus)


def entpacken(p, laenge):
    """wie GPIC.INC; Fehler als ValueError"""
    aus = bytearray()
    i = 0
    while len(aus) < laenge:
        if i >= len(p):
            raise ValueError('zu kurz')
        f = p[i]
        i += 1
        for bit in range(8):
            if len(aus) >= laenge:
                break
            if f >> bit & 1:
                if i + 2 > len(p):
                    raise ValueError('zu kurz')
                dist = p[i] | (p[i + 1] >> 4) << 8
                l = (p[i + 1] & 15) + 3
                i += 2
                if dist == 0 or dist > len(aus):
                    raise ValueError('Verweis vor den Anfang')
                for _ in range(l):
                    if len(aus) >= laenge:
                        break
                    aus.append(aus[-dist])
            else:
                if i >= len(p):
                    raise ValueError('zu kurz')
                aus.append(p[i])
                i += 1
    if i != len(p):
        raise ValueError('Ebenenlaenge stimmt nicht (%d von %d Byte gebraucht)'
                         % (i, len(p)))
    return bytes(aus)


# ---- Datei ------------------------------------------------------------------

def schreiben(ebenen, breite, hoehe, roh=''):
    """ebenen: Liste (Art, Rohdaten); -> Dateiinhalt, Tabelle der Groessen"""
    kopf = b'DMVP' + struct.pack('<BBHHH', 1, len(ebenen), breite, hoehe, 0)
    tab, daten, info = b'', b'', []
    for art, d in ebenen:
        assert len(d) == breite * 2 * hoehe
        p = None if art in roh else packen(d)
        if p is None or len(p) >= len(d):
            v, p = 0, d
        else:
            v = 1
        tab += struct.pack('<BBH', ord(art), v, len(p))
        daten += p
        info.append((art, v, len(d), len(p)))
    return kopf + tab + daten, info


def lesen(b):
    """Dateiinhalt -> breite, hoehe, [(Art, Rohdaten)] (auch alte Rohdateien)"""
    if b[:4] != b'DMVP':
        if len(b) == 128000:
            return 40, 400, [(a, b[i * 32000:(i + 1) * 32000])
                             for i, a in enumerate('GRBM')]
        if len(b) == 32000:
            return 40, 400, [('M', b)]
        raise ValueError('unbekanntes Format')
    ver, n, w, h, _ = struct.unpack('<BBHHH', b[4:12])
    if ver != 1 or not 1 <= n <= 4 or not 1 <= w <= 40 or not 1 <= h <= 400:
        raise ValueError('unbekanntes Format oder Version')
    pos = 12 + 4 * n
    aus = []
    for k in range(n):
        art, v, l = struct.unpack('<BBH', b[12 + 4 * k:16 + 4 * k])
        p = b[pos:pos + l]
        if len(p) < l:
            raise ValueError('zu kurz')
        pos += l
        if v == 0:
            if l != w * 2 * h:
                raise ValueError('Ebenenlaenge stimmt nicht')
            d = p
        elif v == 1:
            d = entpacken(p, w * 2 * h)
        else:
            raise ValueError('unbekanntes Verfahren')
        aus.append((chr(art), d))
    return w, h, aus


# ---- PNG --------------------------------------------------------------------

def ebene_aus_bits(bits, w, h):
    """bits[y][x] (0/1), x < 16 w -> Wortstrom (Low-Byte zuerst, Bit 0 links)"""
    out = bytearray()
    for y in range(h):
        for wx in range(w):
            v = 0
            for b in range(16):
                if bits[y][wx * 16 + b]:
                    v |= 1 << b
            out += struct.pack('<H', v)
    return bytes(out)


def png_lesen(name, w, h, mono=False):
    from PIL import Image
    im = Image.open(name).convert('RGB')
    if im.size != (16 * w, h):
        sys.exit('%s: %d x %d, erwartet %d x %d' % (name, im.size[0],
                 im.size[1], 16 * w, h))
    px = im.load()
    if mono:
        bits = [[1 if sum(px[x, y]) >= 384 else 0 for x in range(16 * w)]
                for y in range(h)]
        return ebene_aus_bits(bits, w, h)
    feld = []
    for y in range(h):
        z = []
        for x in range(16 * w):
            c = px[x, y]
            if c not in FARBEN:
                sys.exit('%s: Farbe %s bei %d,%d ist keine DMV-Farbe'
                         % (name, c, x, y))
            z.append(FARBEN[c])
        feld.append(z)
    return {a: ebene_aus_bits([[1 if p & EBIT[a] else 0 for p in z]
                               for z in feld], w, h) for a in 'GRB'}


def png_schreiben(name, w, h, ebenen):
    from PIL import Image
    d = dict(ebenen)
    im = Image.new('RGB', (16 * w, h))
    px = im.load()
    farbe = any(a in d for a in 'GRB')
    for y in range(h):
        for wx in range(w):
            o = 2 * (y * w + wx)
            for b in range(16):
                def bit(a):
                    v = d[a][o] | d[a][o + 1] << 8
                    return v >> b & 1
                if farbe:
                    g = bit('G') if 'G' in d else 0
                    r = bit('R') if 'R' in d else 0
                    bl = bit('B') if 'B' in d else 0
                    px[wx * 16 + b, y] = (255 * r, 255 * g, 255 * bl)
                else:
                    m = 255 * bit('M')
                    px[wx * 16 + b, y] = (m, m, m)
    im.save(name)


def main():
    ap = argparse.ArgumentParser(description='DMV-Vollbilder packen (.PIC, DMVP)')
    ap.add_argument('ein')
    ap.add_argument('aus')
    ap.add_argument('--mono')
    ap.add_argument('--ebenen')
    ap.add_argument('--roh', default='')
    ap.add_argument('--breite', type=int, default=40)
    ap.add_argument('--hoehe', type=int, default=400)
    ap.add_argument('--entpacken', action='store_true')
    a = ap.parse_args()
    if a.entpacken:
        w, h, eb = lesen(open(a.ein, 'rb').read())
        if a.aus.lower().endswith('.png'):
            png_schreiben(a.aus, w, h, eb)
        else:
            open(a.aus, 'wb').write(b''.join(d for _, d in eb))
        print('%s: %d x %d Worte, Ebenen %s' % (a.ein, w, h,
              ''.join(x for x, _ in eb)))
        return
    w, h = a.breite, a.hoehe
    el = w * 2 * h
    vorh = {}
    if a.ein.lower().endswith('.png'):
        vorh = png_lesen(a.ein, w, h)
        if a.mono:
            vorh['M'] = png_lesen(a.mono, w, h, mono=True)
    else:
        b = open(a.ein, 'rb').read()
        if len(b) == 4 * el:
            vorh = {x: b[i * el:(i + 1) * el] for i, x in enumerate('GRBM')}
        elif len(b) == el:
            vorh = {'M': b}
        else:
            sys.exit('%s: %d Byte, erwartet %d oder %d' % (a.ein, len(b), el,
                     4 * el))
    arten = (a.ebenen or ''.join(x for x in 'GRBM' if x in vorh)).upper()
    for x in arten:
        if x not in vorh:
            sys.exit('Ebene %s fehlt in der Eingabe' % x)
    if not arten:
        sys.exit('keine Ebenen')
    if any(x in arten for x in 'GRB') and 'M' not in arten:
        print('Hinweis: ohne Ebene M zeigt eine Mono-DMV die Ebene G')
    datei, info = schreiben([(x, vorh[x]) for x in arten], w, h,
                            a.roh.upper())
    w2, h2, zur = lesen(datei)               # Selbstpruefung
    if (w2, h2) != (w, h) or [(x, vorh[x]) for x in arten] != zur:
        sys.exit('Selbstpruefung FEHLGESCHLAGEN')
    open(a.aus, 'wb').write(datei)
    print('Ebene  Verfahren  ungepackt   gepackt  Anteil')
    for x, v, l, p in info:
        print('  %s    %-8s  %9d %9d  %4d %%' % (x, 'LZSS' if v else 'roh',
              l, p, round(100 * p / l)))
    print('%s: %d Byte (%d x %d Worte), Selbstpruefung gut'
          % (a.aus, len(datei), w, h))


if __name__ == '__main__':
    main()

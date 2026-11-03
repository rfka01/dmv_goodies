#!/usr/bin/env python3
"""mkgbl.py - Bildsammlung fuer gbldv schreiben (DMVGRAF).

  mkgbl.py AUS.GBL quelle [quelle ...] [--kopf n] [--liste]

quelle:
  bild.png            PNG (mit Durchsichtigkeit) -> Format 4
  bild.png:1          dasselbe als 1 Bit je Punkt (Format 1, Bit 7 links;
  bild.png:2          Format 2, Bit 0 links): gesetzt ist jeder deckende,
                      nicht schwarze Punkt
  roh.bin:b:h:f       Rohdaten genau wie fuer gbdef (b Punkte breit, h
                      Zeilen, Format f 1-5; Format 5 als 16-Bit-Worte mit
                      dem niederwertigen Byte zuerst)

PNG nach Format 4: deckend ist ein Punkt mit Alpha >= 128; die Ebenen
Gruen, Rot, Blau sind gesetzt, wenn der Anteil >= 128 ist (schwarz =
deckendes Schwarz).

Jedes Bild bekommt einen Kopf von 6 Byte: Breite, Hoehe (je 2 Byte,
niederwertiges zuerst), Format, 0; dann die Daten. --kopf n stellt n
Nullbytes voran (fuer gbldv(..., kopf) hinter einem eigenen Dateikopf).
--liste zeigt eine vorhandene Sammlung.

Als Modul: daten(b, h, fmt) -> Laenge, eintrag(b, h, fmt, d) -> Bytes,
png4(datei) / png1(datei, fmt) -> (b, h, d), lesen(bytes, kopf).

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import argparse, struct, sys


def daten(b, h, fmt):
    """Laenge der Bilddaten wie gbdef sie liest"""
    n = (b + 7) // 8
    if fmt in (1, 2):
        return h * n
    if fmt == 3:
        return h * ((b + 1) // 2)
    if fmt == 4:
        return 4 * h * n
    if fmt == 5:
        return 12 * h * ((b + 15) // 16)
    raise ValueError('Format %d' % fmt)


def eintrag(b, h, fmt, d):
    if not (1 <= b <= 640 and 1 <= h <= 400):
        raise ValueError('Groesse %d x %d' % (b, h))
    if len(d) != daten(b, h, fmt):
        raise ValueError('%d Byte, Format %d braucht %d' % (len(d), fmt, daten(b, h, fmt)))
    return struct.pack('<HHBB', b, h, fmt, 0) + bytes(d)


def _png(datei):
    from PIL import Image
    im = Image.open(datei).convert('RGBA')
    return im.size[0], im.size[1], im.load()


def png4(datei):
    b, h, p = _png(datei)
    n = (b + 7) // 8
    d = bytearray(4 * h * n)
    for y in range(h):
        for x in range(b):
            r, g, bl, a = p[x, y]
            if a < 128:
                continue
            o = y * 4 * n + x // 8
            bit = 1 << (x & 7)
            d[o] |= bit
            if g >= 128: d[o + n] |= bit
            if r >= 128: d[o + 2 * n] |= bit
            if bl >= 128: d[o + 3 * n] |= bit
    return b, h, bytes(d)


def png1(datei, fmt):
    b, h, p = _png(datei)
    n = (b + 7) // 8
    d = bytearray(h * n)
    for y in range(h):
        for x in range(b):
            r, g, bl, a = p[x, y]
            if a >= 128 and (r >= 128 or g >= 128 or bl >= 128):
                d[y * n + x // 8] |= (0x80 >> (x & 7)) if fmt == 1 else (1 << (x & 7))
    return b, h, bytes(d)


def lesen(s, kopf=0):
    """Eintraege (b, h, fmt, daten) einer Sammlung"""
    i, r = kopf, []
    while i + 6 <= len(s):
        b, h, fmt, _ = struct.unpack_from('<HHBB', s, i)
        if not 1 <= b <= 640:
            break
        n = daten(b, h, fmt)
        r.append((b, h, fmt, s[i + 6:i + 6 + n]))
        i += 6 + n
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('aus')
    ap.add_argument('quellen', nargs='*')
    ap.add_argument('--kopf', type=int, default=0)
    ap.add_argument('--liste', action='store_true')
    a = ap.parse_args()
    if a.liste:
        s = open(a.aus, 'rb').read()
        e = lesen(s, a.kopf)
        for k, (b, h, fmt, d) in enumerate(e):
            print('%3d: %3d x %3d  Format %d  %5d Byte' % (k, b, h, fmt, len(d)))
        print('%d Bilder, %d Byte' % (len(e), len(s)))
        return
    aus = bytearray(a.kopf)
    for q in a.quellen:
        t = q.split(':')
        if t[0].lower().endswith('.png'):
            if len(t) > 1 and t[1] in ('1', '2'):
                b, h, d = png1(t[0], int(t[1]))
                fmt = int(t[1])
            else:
                b, h, d = png4(t[0])
                fmt = 4
        elif len(t) == 4:
            b, h, fmt = int(t[1]), int(t[2]), int(t[3])
            d = open(t[0], 'rb').read()[:daten(b, h, fmt)]
        else:
            sys.exit('%s: PNG oder roh.bin:b:h:f' % q)
        aus += eintrag(b, h, fmt, d)
    open(a.aus, 'wb').write(aus)
    print('%s: %d Bilder, %d Byte' % (a.aus, len(a.quellen), len(aus)))


if __name__ == '__main__':
    main()

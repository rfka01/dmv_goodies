#!/usr/bin/env python3
"""dd320.py - CP/M-Datendiskette im NCR-Format DD320 (320 KB) schreiben
und lesen, wie sie CP/M-68K und CP/M-8000 auf der DMV benutzen.

  dd320.py liste ABBILD             Verzeichnis zeigen
  dd320.py hole ABBILD NAME.TYP     Datei herausholen (in das aktuelle
                                    Verzeichnis)
  Als Modul: schreiben([(name, daten), ...], aus), datei(abbild, name)

Aufbau (aus vorhandenen Disketten erschlossen und daran geprueft):
- Abbild in Spuren zu 4 KB, Folge Zylinder 0 Seite 0, Zylinder 0 Seite 1,
  ...; je Spur 16 Sektoren zu 256 Byte.
- Logische Spuren: Seite 0 ab Zylinder 3 (Zylinder 0-2 der Seite 0 sind
  Systemspuren und bleiben leer), danach Seite 1 ab Zylinder 0; 77 Spuren.
- Bloecke zu 2 KB, Block 0-1 Verzeichnis (64 Eintraege), 8-Bit-
  Blocknummern, 16 Bloecke je Eintrag (zwei logische Extents, EXM 1),
  154 Bloecke.
- Achtung: Die Belegung von Seite 1 ist nur fuer CP/M-68K an Disketten
  geprueft. Unter CP/M-8000 waren .Z8K-Dateien auf Seite 1 in MAME "not
  executable" (CPM16TEST 1.00); vermutlich zaehlt dessen BIOS Seite 1
  anders. Seine Programme darum in die ersten 37 Spuren legen, also in
  Bloecke unter SEITE0.

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import sys

SPUREN = 77
BLOCK = 2048
BLOECKE = SPUREN * 4096 // BLOCK        # 154
EINTRAEGE = 64
SEITE0 = 37 * 4096 // BLOCK          # 74: erster Block auf Seite 1
GROESSE = 80 * 4096


def phys(lage):
    """logische Byteadresse -> Stelle im Abbild"""
    t, o = divmod(lage, 4096)
    p = 2 * (3 + t) if t < 37 else 2 * (t - 37) + 1
    return p * 4096 + o


def cpmname(n):
    n = n.upper()
    b, _, e = n.partition('.')
    if not b or len(b) > 8 or len(e) > 3:
        raise ValueError('Name %s passt nicht in 8.3' % n)
    return (b.ljust(8) + e.ljust(3)).encode('ascii')


def eintraege(abbild):
    return [bytes(abbild[phys(i * 32):phys(i * 32) + 32])
            for i in range(EINTRAEGE)]


def datei(abbild, name):
    """Inhalt einer Datei (auf ganze 128-Byte-Saetze)"""
    if isinstance(name, str):
        name = cpmname(name)
    es = sorted((e for e in eintraege(abbild) if e[0] == 0 and e[1:12] == name),
                key=lambda e: e[12])
    aus = b''
    for e in es:
        n = (e[12] & 1) * 128 + e[15]
        d = b''.join(bytes(abbild[phys(b * BLOCK):phys(b * BLOCK) + BLOCK])
                     for b in e[16:32] if b)
        aus += d[:n * 128]
    return aus


def schreiben(dateien, aus):
    """dateien: Liste (Name 'NAME.TYP' oder 11 Byte, Daten). -> belegte Bloecke"""
    img = bytearray(b'\xe5' * GROESSE)
    blk = 2
    es = []
    for name, data in dateien:
        if isinstance(name, str):
            name = cpmname(name)
        n = (len(data) + 127) // 128
        data = data + b'\x1a' * (n * 128 - len(data))
        k = 0
        while True:
            teil = data[k * 32768:(k + 1) * 32768]
            if not teil and k:
                break
            saetze = len(teil) // 128
            bl = []
            for j in range(0, len(teil), BLOCK):
                if blk >= BLOECKE:
                    raise ValueError('Diskette voll')
                p = phys(blk * BLOCK)
                img[p:p + BLOCK] = teil[j:j + BLOCK].ljust(BLOCK, b'\x1a')
                bl.append(blk)
                blk += 1
            ex = 2 * k + (1 if saetze > 128 else 0)
            rc = saetze - 128 if saetze > 128 else saetze
            es.append(bytes([0]) + name + bytes([ex, 0, 0, rc]) +
                      bytes(bl).ljust(16, b'\0'))
            k += 1
            if len(teil) < 32768:
                break
    if len(es) > EINTRAEGE:
        raise ValueError('Verzeichnis voll')
    for i, e in enumerate(es):
        p = phys(i * 32)
        img[p:p + 32] = e
    open(aus, 'wb').write(img)
    return blk


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    img = open(sys.argv[2], 'rb').read()
    if sys.argv[1] == 'liste':
        gesehen = []
        for e in eintraege(img):
            if e[0] == 0 and e[1:12] not in gesehen:
                gesehen.append(e[1:12])
        for n in gesehen:
            d = datei(img, n)
            print('%s.%s %7d' % (n[:8].decode().strip(), n[8:].decode().strip(), len(d)))
    elif sys.argv[1] == 'hole':
        d = datei(img, sys.argv[3])
        open(sys.argv[3].upper(), 'wb').write(d)
        print('%s: %d Byte' % (sys.argv[3].upper(), len(d)))


if __name__ == '__main__':
    main()

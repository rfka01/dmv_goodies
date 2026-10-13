"""Baut Images/KERMIT.IMG (360 KB, nicht startfaehig) fuer die DMV.

Aufruf aus dem Verzeichnis kermit/:  python3 tools/mkdisk.py build
(build = Ausgabe von tools/build.sh). Braucht nur Python 3.
"""
import os
import struct
import sys

HIER = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
BOOT = os.path.join(HIER, '..', 'Images', 'GOODIES.IMG')   # BPB 360 KB
ZIEL = os.path.join(HIER, '..', 'Images', 'KERMIT.IMG')


class Fat12:
    """Kleinste FAT12-Diskette: Dateien im Stammverzeichnis anlegen."""

    def __init__(s, img):
        s.d = bytearray(img)
        (s.bps, s.spc, s.res, s.nf, s.nr, s.tot, s.med,
         s.spf) = struct.unpack('<HBHBHHBH', s.d[11:24])
        s.rs = s.res + s.nf * s.spf
        s.ds = s.rs + s.nr * 32 // s.bps
        s.ncl = (s.tot - s.ds) // s.spc + 2
        s.fat = bytearray(s.spf * s.bps)
        s.fat[0:3] = bytes([s.med, 0xFF, 0xFF])
        s.n = 0

    def setz(s, c, x):
        o = c * 3 // 2
        v = s.fat[o] | (s.fat[o + 1] << 8)
        v = (v & 0x000F) | (x << 4) if c & 1 else (v & 0xF000) | x
        s.fat[o], s.fat[o + 1] = v & 0xFF, v >> 8

    def frei(s):
        c = 2
        while True:
            o = c * 3 // 2
            v = s.fat[o] | (s.fat[o + 1] << 8)
            if ((v >> 4) if c & 1 else v & 0xFFF) == 0:
                yield c
            c += 1
            if c >= s.ncl:
                raise SystemExit('Diskette voll')

    def add(s, name, data, datum=(2026, 10, 12)):
        clu = s.spc * s.bps
        n = max(1, -(-len(data) // clu))
        g = s.frei()
        ketten = [next(g) for _ in range(n)]
        for a, b in zip(ketten, ketten[1:] + [0xFFF]):
            s.setz(a, b)
        for k, c in enumerate(ketten):
            o = (s.ds + (c - 2) * s.spc) * s.bps
            teil = data[k * clu:(k + 1) * clu]
            s.d[o:o + len(teil)] = teil
        b, _, e = name.upper().partition('.')
        e11 = (b.ljust(8) + e.ljust(3)).encode()
        y, m, t = datum
        o = s.rs * s.bps + s.n * 32
        s.d[o:o + 32] = e11 + b'\x20' + bytes(10) + struct.pack(
            '<HHHI', 0, ((y - 1980) << 9) | (m << 5) | t, ketten[0], len(data))
        s.n += 1

    def bild(s):
        for k in range(s.nf):
            o = (s.res + k * s.spf) * s.bps
            s.d[o:o + len(s.fat)] = s.fat
        return bytes(s.d)


def main(build):
    boot = open(BOOT, 'rb').read(512)
    assert struct.unpack('<HBHBHHBHHH', boot[11:28])[:9] == \
        (512, 2, 1, 2, 112, 720, 0xFD, 2, 9)
    f = Fat12(boot + bytes(720 * 512 - 512))
    lies = lambda p: open(p, 'rb').read()
    f.add('KERMIT.EXE', lies(os.path.join(build, 'KERMIT.EXE')))
    f.add('KERMIBM.EXE', lies(os.path.join(build, 'KERMIBM.EXE')))
    for n in ('MSKERMIT.INI', 'KERMIT.TXT', 'KERMIT.LIC'):
        f.add(n, lies(os.path.join(HIER, n)))
    open(ZIEL, 'wb').write(f.bild())
    print(ZIEL)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(HIER, 'build'))

"""Baut DMVKERM.IMG (360 KB) und traegt DMVKERM in GOODIES.IMG ein.

Aufruf: python3 mkdisk.py <ziel-verzeichnis-images> <msk314-KERMIT.EXE>
Der Startsektor (BPB 360 KB) kommt aus GOODIES.IMG, die Diskette selbst
ist nicht startfaehig.
"""
import os, struct, sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from fat12 import Fat12                                   # noqa: E402

D = os.path.join(os.path.dirname(__file__), '..')
DAT = (2026, 10, 15)


def txt(name):
    """Text (ASCII) -> DOS-Text mit CRLF und Ctrl-Z."""
    s = open(os.path.join(D, name), encoding='ascii').read()
    s = s.replace('\r', '')
    return (s.replace('\n', '\r\n') + '\x1a').encode('ascii')


def com():
    return open(os.path.join(D, 'build', 'DMVKERM.COM'), 'rb').read()


def main(images, kermibm):
    goodies = os.path.join(images, 'GOODIES.IMG')
    roh = bytearray(open(goodies, 'rb').read(512)) + bytes(720 * 512 - 512)
    bpb = struct.unpack('<HBHBHHBHHH', roh[11:28])
    assert bpb[:9] == (512, 2, 1, 2, 112, 720, 0xFD, 2, 9), bpb
    f = Fat12(roh)
    f.fat[:] = bytes(len(f.fat))
    f.fat[0], f.fat[1], f.fat[2] = 0xFD, 0xFF, 0xFF
    f.add(b'DMVKERM COM', com(), DAT)
    f.add(b'DMVKERM TXT', txt('DMVKERM.TXT'), DAT)
    f.add(b'DMVKERM INI', txt('DMVKERM.INI')[:-1], DAT)
    f.add(b'KERMIBM EXE', open(kermibm, 'rb').read(), (1995, 5, 25))
    f.add(b'KERMIT  LIC', txt('KERMIT.LIC'), DAT)
    open(os.path.join(images, 'DMVKERM.IMG'), 'wb').write(f.image())

    g = Fat12(bytearray(open(goodies, 'rb').read()))
    for n, d in ((b'DMVKERM COM', com()), (b'DMVKERM TXT', txt('DMVKERM.TXT'))):
        if g.find(n) is not None:
            g.delete(n)
        g.add(n, d, DAT)
    open(goodies, 'wb').write(g.image())


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])

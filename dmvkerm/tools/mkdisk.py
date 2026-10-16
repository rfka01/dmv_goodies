"""Traegt DMVKERM.COM und DMVKERM.TXT in GOODIES.IMG ein.

Aufruf: python3 mkdisk.py <verzeichnis-mit-GOODIES.IMG>
(DMVKERM.IMG wird nicht mehr gebaut; der letzte Stand liegt in Images/.)
"""
import os, struct, sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from fat12 import Fat12                                   # noqa: E402

D = os.path.join(os.path.dirname(__file__), '..')
DAT = (2026, 10, 16)


def txt(name):
    """Text (ASCII) -> DOS-Text mit CRLF und Ctrl-Z."""
    s = open(os.path.join(D, name), encoding='ascii').read()
    s = s.replace('\r', '')
    return (s.replace('\n', '\r\n') + '\x1a').encode('ascii')


def com():
    return open(os.path.join(D, 'build', 'DMVKERM.COM'), 'rb').read()


def main(images):
    goodies = os.path.join(images, 'GOODIES.IMG')
    g = Fat12(bytearray(open(goodies, 'rb').read()))
    for n, d in ((b'DMVKERM COM', com()), (b'DMVKERM TXT', txt('DMVKERM.TXT'))):
        if g.find(n) is not None:
            g.delete(n)
        g.add(n, d, DAT)
    open(goodies, 'wb').write(g.image())


if __name__ == '__main__':
    main(sys.argv[1])

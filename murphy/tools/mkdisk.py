"""Traegt MURPHY.COM, MURPHY.LAW und MURPHY.TXT in GOODIES.IMG ein.

Aufruf: python3 mkdisk.py <verzeichnis-mit-GOODIES.IMG>
MURPHY.COM wird vorher mit tools/build.sh gebaut (build/MURPHY.COM).
"""
import datetime
import os
import sys
from zoneinfo import ZoneInfo

D = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
sys.path.insert(0, os.path.join(D, '..', 'dmvkerm', 'tools'))
from fat12 import Fat12                                   # noqa: E402

# Datum und Uhrzeit des Baus (deutsche Zeit) als Stempel der Dateien
JETZT = datetime.datetime.now(ZoneInfo('Europe/Berlin'))
DAT = (JETZT.year, JETZT.month, JETZT.day)
ZEIT = (JETZT.hour, JETZT.minute, JETZT.second)


def txt(name):
    """Text (ASCII) -> DOS-Text mit CRLF und Ctrl-Z."""
    s = open(os.path.join(D, name), encoding='ascii').read()
    s = s.replace('\r', '')
    return (s.replace('\n', '\r\n') + '\x1a').encode('ascii')


def main(images):
    goodies = os.path.join(images, 'GOODIES.IMG')
    g = Fat12(bytearray(open(goodies, 'rb').read()))
    com = open(os.path.join(D, 'build', 'MURPHY.COM'), 'rb').read()
    for n, d in ((b'MURPHY  COM', com),
                 (b'MURPHY  LAW', txt('MURPHY.LAW')),
                 (b'MURPHY  TXT', txt('MURPHY.TXT'))):
        if g.find(n) is not None:
            g.delete(n)
        g.add(n, d, DAT, zeit=ZEIT)
    open(goodies, 'wb').write(g.image())


if __name__ == '__main__':
    main(sys.argv[1])

"""Baut alle Programme der GOODIES-Diskette und traegt sie mit ihren Texten
in Images/GOODIES.IMG ein.

Aufruf: python3 tools/mkgoodies.py
Braucht JWasm (jwasm im Pfad). Die Texte liegen im Repository als UTF-8
und werden beim Eintragen in den Zeichensatz der DMV gewandelt
(dmvkerm/tools/dmvtext.py). Die Versionsdatei ist die eine Images/GOODIES.nnn.
"""
import datetime
import glob
import os
import subprocess
import sys
from zoneinfo import ZoneInfo

R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
sys.path.insert(0, os.path.join(R, 'dmvkerm', 'tools'))
from fat12 import Fat12                                   # noqa: E402
from dmvtext import dostext                               # noqa: E402

JETZT = datetime.datetime.now(ZoneInfo('Europe/Berlin'))
DAT = (JETZT.year, JETZT.month, JETZT.day)
ZEIT = (JETZT.hour, JETZT.minute, JETZT.second)

# Programme: Verzeichnis, Quelle; gebaut mit JWasm -Zm (wie MASM 5.10)
PROGRAMME = [('dmvdemo', 'DMVDEMO'), ('dmvrace', 'DMVRACE'), ('ecodrive', 'ECODRIVE'),
             ('maustest', 'MAUSTEST'), ('dmvkerm', 'DMVKERM'), ('murphy', 'MURPHY'),
             ('dmvmem', 'DMVMEM')]
# Dateien in der Reihenfolge der Diskette: Name auf der Diskette, Quelle, Art
DATEIEN = [('DMVDEMO.COM', 'dmvdemo', 'com'), ('KITT.PIC', 'dmvdemo/KITT.PIC', 'bin'),
           ('DMVDEMO.TXT', 'dmvdemo/LIESMICH.TXT', 'txt'), ('DMVRACE.COM', 'dmvrace', 'com'),
           ('DMVRACE.TXT', 'dmvrace/LIESMICH.TXT', 'txt'), ('ECODRIVE.COM', 'ecodrive', 'com'),
           ('ECODRIVE.TXT', 'ecodrive/ECODRIVE.TXT', 'txt'),
           ('KITT.TXT', 'dmvdemo/KITT-LICENSE.txt', 'txt'), ('MAUSTEST.COM', 'maustest', 'com'),
           ('MAUSTEST.TXT', 'maustest/MAUSTEST.TXT', 'txt'), ('DMVKERM.COM', 'dmvkerm', 'com'),
           ('DMVKERM.TXT', 'dmvkerm/DMVKERM.TXT', 'txt'), ('TUT.PIC', 'dmvdemo/TUT.PIC', 'bin'),
           ('TUT.TXT', 'dmvdemo/TUT-LICENSE.txt', 'txt'), ('GOODIES.NNN', None, 'ver'),
           ('MURPHY.COM', 'murphy', 'com'), ('MURPHY.LAW', 'murphy/MURPHY.LAW', 'txt'),
           ('MURPHY.TXT', 'murphy/MURPHY.TXT', 'txt'), ('DMVMEM.COM', 'dmvmem', 'com'),
           ('DMVMEM.TXT', 'dmvmem/DMVMEM.TXT', 'txt')]


def baue(d, n):
    os.makedirs(os.path.join(R, d, 'build'), exist_ok=True)
    ziel = os.path.join(R, d, 'build', n + '.COM')
    if d == 'dmvkerm':
        subprocess.run(['sh', 'tools/build.sh'], cwd=os.path.join(R, d), check=True,
                       stdout=subprocess.DEVNULL)
        return ziel
    subprocess.run(['jwasm', '-bin', '-q', '-Zm', '-Fo' + ziel,
                    '-Fl' + os.path.join(R, d, 'build', n.lower() + '.lst'), n + '.ASM'],
                   cwd=os.path.join(R, d), check=True)
    return ziel


def dosname(n):
    a, _, b = n.partition('.')
    return (a.ljust(8) + b.ljust(3)).encode('ascii')


def main():
    coms = {d: baue(d, n) for d, n in PROGRAMME}
    ver = glob.glob(os.path.join(R, 'Images', 'GOODIES.[0-9][0-9][0-9]'))
    assert len(ver) == 1, ver
    pfad = os.path.join(R, 'Images', 'GOODIES.IMG')
    g = Fat12(bytearray(open(pfad, 'rb').read()))
    # alte Versionsdateien weg
    for i in range(112):
        o = g.entry(i)
        n = bytes(g.d[o:o + 11])
        if n.startswith(b'GOODIES ') and n[8:].isdigit():
            g.delete(n)
    for name, quelle, art in DATEIEN:
        if art == 'ver':
            name = os.path.basename(ver[0])
            daten = dostext(ver[0], ctrlz=False)
        elif art == 'com':
            daten = open(coms[quelle], 'rb').read()
        elif art == 'txt':
            daten = dostext(os.path.join(R, quelle))
        else:
            daten = open(os.path.join(R, quelle), 'rb').read()
        n = dosname(name)
        if g.find(n) is not None:
            g.delete(n)
        g.add(n, daten, DAT, zeit=ZEIT)
        print('%-12s %6d' % (name, len(daten)))
    open(pfad, 'wb').write(g.image())


if __name__ == '__main__':
    main()

# Build the batch files of the printer test disk (MS-DOS 2.x and later: no @, no CHOICE)
import os, sys

OUT = sys.argv[1] if len(sys.argv) > 1 else '.'

# name, title, mono files, colour files (None = black only)
PRINTERS = [
    ('MX80',   'Epson MX-80',                  ['MX80.PRN'], None),
    ('MX80GT', 'Epson MX-80 Graftrax',         ['MX80.PRN', 'MX80GTRX.PRN'], None),
    ('MX80DP', 'Epson MX-80 Dots Perfect',     ['MX80DP.PRN'], None),
    ('RX80',   'Epson RX-80',                  ['RX80FONT.PRN', 'GRAFIK.PRN'], None),
    ('FX80',   'Epson FX-80',                  ['FX80FONT.PRN', 'GRAFIK.PRN'], None),
    ('JX80',   'Epson JX-80',                  ['FX80FONT.PRN', 'GRAFIK.PRN'], ['JX80FONT.PRN', 'FARBE.PRN', 'FARBGRAF.PRN']),
    ('FX85',   'Epson FX-85',                  ['FX85FONT.PRN', 'GRAFIK.PRN'], None),
    ('LX86',   'Epson LX-86',                  ['LX86FONT.PRN', 'GRAFIK.PRN'], None),
    ('LX800',  'Epson LX-800',                 ['LX800FNT.PRN', 'GRAFIK.PRN'], None),
    ('LX810L', 'Epson LX-810L / ActionPrinter 2000', ['FX80FONT.PRN', 'GRAFIK.PRN'], None),
    ('EX800',  'Epson EX-800',                 ['EX800FNT.PRN', 'GRAFIK.PRN'], ['EX800FNT.PRN', 'FARBE.PRN', 'FARBGRAF.PRN']),
    ('FX880',  'Epson FX-880',                 ['FX880FNT.PRN', 'FX880GRF.PRN'], None),
    ('AP3250', 'Epson ActionPrinter 3250 (LQ-100)', ['AP32FNT.PRN', 'AP32GRF.PRN'], None),
    ('C200GX', 'Citizen 200GX',                ['C200FNT.PRN', 'GRAFIK.PRN'], ['C200FNT.PRN', 'FARBE.PRN', 'FARBGRAF.PRN']),
    ('BX80',   'BMC BX-80',                    ['BX80FONT.PRN', 'GRAFIK.PRN'], None),
    ('GEM10X', 'Star Gemini-10X',              ['GEM10FNT.PRN', 'GRAFIK.PRN'], None),
    ('NL10',   'Star NL-10 (IBM-Kassette)',    ['NL10FONT.PRN'], None),
    ('MD16',   'Nixdorf MD 16',                ['MD16FONT.PRN', 'GRAFIK.PRN'], None),
]


def ascii(s):
    return s.replace('ä', 'ae').replace('ö', 'oe').replace('ü', 'ue').replace('ß', 'ss')


def save(name, lines):
    with open(os.path.join(OUT, name), 'wb') as f:
        f.write(ascii('\r\n'.join(lines) + '\r\n').encode('ascii'))
    print('%-12s' % name)


def copy(files):
    out = []
    for f in files:
        out += ['ECHO   ' + f, 'COPY /B ' + f + ' LPT1: >NUL']
    return out


for name, title, mono, colour in PRINTERS:
    b = ['ECHO OFF']
    if colour:
        for arg in ('FARBE', 'farbe', 'Farbe', 'F', 'f'):
            b.append('IF "%%1"=="%s" GOTO FARBE' % arg)
        b += ['ECHO ' + title + ', schwarzweiss (Farbe: ' + name + ' FARBE)'] + copy(mono)
        b += ['GOTO ENDE', ':FARBE', 'ECHO ' + title + ', Farbe'] + copy(colour) + [':ENDE']
    else:
        b += ['IF "%1"=="" GOTO SW', 'ECHO ' + title + ' druckt nur schwarz.', ':SW',
              'ECHO ' + title] + copy(mono)
    b.append('ECHO Fertig.')
    save(name + '.BAT', b)

# overview
d = ['ECHO OFF', 'ECHO Druckertest: Druckername eingeben, bei Farbdruckern',
     'ECHO mit FARBE dahinter fuer den Farbtest, z.B.  JX80 FARBE']
for name, title, mono, colour in PRINTERS:
    d.append('ECHO   %-7s %s%s' % (name, title, '  (Farbe moeglich)' if colour else ''))
d += ['ECHO   TEXT    alle Drucker: BILD.PRN und ZEILEN.PRN', ''
      'ECHO Die Dateien gehen mit COPY /B an LPT1:.']
save('DRUCKER.BAT', d)
save('TEXT.BAT', ['ECHO OFF', 'ECHO Text fuer alle Drucker'] + copy(['BILD.PRN', 'ZEILEN.PRN']) + ['ECHO Fertig.'])
save('PRNTEST.BAT', ['ECHO OFF', 'ECHO Kurztest fuer alle 9-Nadel-Drucker'] + copy(['BILD.PRN', 'GRAFIK.PRN']) + ['ECHO Fertig.'])

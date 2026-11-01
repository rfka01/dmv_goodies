# Build the batch files of the printer test disk (MS-DOS 2.x and later: no @, no CHOICE)
import os, sys

OUT = sys.argv[1] if len(sys.argv) > 1 else '.'

# name, title, mono files, colour files (None = black only)
PRINTERS = [
    ('MX80',   'Epson MX-80',                  ['MX80.PRN'], None),
    ('MX80GT', 'Epson MX-80 Graftrax',         ['MX80.PRN', 'MX80GTRX.PRN'], None),
    ('MX80DP', 'Epson MX-80 Dots Perfect',     ['MX80DP.PRN'], None),
    ('RX80',   'Epson RX-80',                  ['RX80FONT.PRN', 'GRAPHICS.PRN'], None),
    ('FX80',   'Epson FX-80',                  ['FX80FONT.PRN', 'GRAPHICS.PRN'], None),
    ('JX80',   'Epson JX-80',                  ['FX80FONT.PRN', 'GRAPHICS.PRN'], ['JX80FONT.PRN', 'COLOUR.PRN', 'COLGRAPH.PRN']),
    ('FX85',   'Epson FX-85',                  ['FX85FONT.PRN', 'GRAPHICS.PRN'], None),
    ('LX86',   'Epson LX-86',                  ['LX86FONT.PRN', 'GRAPHICS.PRN'], None),
    ('LX800',  'Epson LX-800',                 ['LX800FNT.PRN', 'GRAPHICS.PRN'], None),
    ('LX810L', 'Epson LX-810L / ActionPrinter 2000', ['FX80FONT.PRN', 'GRAPHICS.PRN'], None),
    ('EX800',  'Epson EX-800',                 ['EX800FNT.PRN', 'GRAPHICS.PRN'], ['EX800FNT.PRN', 'COLOUR.PRN', 'COLGRAPH.PRN']),
    ('FX880',  'Epson FX-880',                 ['FX880FNT.PRN', 'FX880GRF.PRN'], None),
    ('AP3250', 'Epson ActionPrinter 3250 (LQ-100)', ['AP32FNT.PRN', 'AP32GRF.PRN'], None),
    ('C200GX', 'Citizen 200GX',                ['C200FNT.PRN', 'GRAPHICS.PRN'], ['C200FNT.PRN', 'COLOUR.PRN', 'COLGRAPH.PRN']),
    ('BX80',   'BMC BX-80',                    ['BX80FONT.PRN', 'GRAPHICS.PRN'], None),
    ('GEM10X', 'Star Gemini-10X',              ['GEM10FNT.PRN', 'GRAPHICS.PRN'], None),
    ('NL10',   'Star NL-10 (IBM cartridge)',    ['NL10FONT.PRN'], None),
    ('MD16',   'Nixdorf MD 16',                ['MD16FONT.PRN', 'GRAPHICS.PRN'], None),
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
        for arg in ('COLOUR', 'colour', 'Colour', 'COLOR', 'color', 'Color', 'C', 'c'):
            b.append('IF "%%1"=="%s" GOTO COLOUR' % arg)
        b += ['ECHO ' + title + ', black and white (colour: ' + name + ' COLOUR)'] + copy(mono)
        b += ['GOTO END', ':COLOUR', 'ECHO ' + title + ', colour'] + copy(colour) + [':END']
    else:
        b += ['IF "%1"=="" GOTO BW', 'ECHO ' + title + ' prints black only.', ':BW',
              'ECHO ' + title] + copy(mono)
    b.append('ECHO Done.')
    save(name + '.BAT', b)

# overview
d = ['ECHO OFF', 'ECHO Printer test: type the printer name, on colour printers',
     'ECHO add COLOUR for the colour test, e.g.  JX80 COLOUR']
for name, title, mono, colour in PRINTERS:
    d.append('ECHO   %-7s %s%s' % (name, title, '  (colour possible)' if colour else ''))
d += ['ECHO   TEXT    all printers: PICTURE.PRN and LINES.PRN',
      'ECHO The files go to LPT1: with COPY /B.']
save('PRINTERS.BAT', d)
save('TEXT.BAT', ['ECHO OFF', 'ECHO Text for all printers'] + copy(['PICTURE.PRN', 'LINES.PRN']) + ['ECHO Done.'])
save('PRNTEST.BAT', ['ECHO OFF', 'ECHO Short test for all 9-pin printers'] + copy(['PICTURE.PRN', 'GRAPHICS.PRN']) + ['ECHO Done.'])

# Build the printer test files for the DMV test disk.
#   PICTURE.PRN  ASCII picture (printer.txt), plain text (all printers)
#   LINES.PRN    100 numbered lines (all printers)
#   GRAPHICS.PRN bit image graphics, ESC L 120 dpi (9-pin Epson compatibles)
#   COLOUR.PRN   colour text and bars, Epson ESC r (JX-80, EX-800, Citizen 200GX)
#   COLGRAPH.PRN colour bit image graphics, Epson ESC r
#   ....FONT.PRN fonts and attributes per printer (see README.TXT)
#   MX80*.PRN    Epson MX-80 with the original, Graftrax and Dots Perfect ROMs
#   FX880*.PRN, AP32*.PRN  Epson FX-880, ActionPrinter 3250
#
# usage: python3 mkprn.py <output directory>
import math, os, sys

E = b'\x1b'
OUT = sys.argv[1] if len(sys.argv) > 1 else '.'
HERE = os.path.dirname(os.path.abspath(__file__))


def save(name, data):
    with open(os.path.join(OUT, name), 'wb') as f:
        f.write(data)
    print('%-12s %6d bytes' % (name, len(data)))


def t(s):
    return s.encode('latin-1')


# ---------------------------------------------------------------- picture

def picture():
    lines = open(os.path.join(HERE, 'printer.txt'), encoding='ascii').read().rstrip('\n').split('\n')
    return t('\r\n'.join(lines) + '\r\n\x0c')


# ---------------------------------------------------------------- fonts

SAMPLE = 'The quick brown fox jumps over the lazy dog 0123456789'


def fonts_epson(title, colour=False, nlq=False, italic_set=True, elite=True, prop=True, master=True, intl=True):
    d = E + b'@'
    d += E + b'E' + t(title) + E + b'F' + b'\r\n\r\n'
    d += t('Pica 10 cpi:      ' + SAMPLE + '\r\n')
    if elite:
        d += E + b'M' + t('Elite 12 cpi:     ' + SAMPLE + '\r\n') + E + b'P'
    d += b'\x0f' + t('Condensed 17 cpi: ' + SAMPLE + ' ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n') + b'\x12'
    if prop:
        d += E + b'p\x01' + t('Proportional:     Illinois Minimum Wilhelm mmm iii\r\n') + E + b'p\x00'
    if nlq:
        d += E + b'x\x01' + t('NLQ:              ' + SAMPLE + '\r\n') + E + b'x\x00'
    d += b'\r\n'
    d += E + b'E' + t('Emphasized') + E + b'F' + t('  ') + E + b'G' + t('Double strike') + E + b'H' + t('  ')
    d += E + b'-\x01' + t('Underlined') + E + b'-\x00' + t('  ') + E + b'4' + t('Italic') + E + b'5' + b'\r\n'
    d += E + b'W\x01' + t('Double width') + E + b'W\x00' + t('  ') + b'\x0e' + t('SO wide') + b'\x14' + t('  ')
    d += b'\x0f' + E + b'W\x01' + t('Condensed wide') + E + b'W\x00' + b'\x12' + b'\r\n'
    d += t('H') + E + b'S\x01' + t('2') + E + b'T' + t('O, x') + E + b'S\x00' + t('2') + E + b'T'
    d += t(' + y') + E + b'S\x00' + t('2') + E + b'T' + t('   Sub- and superscript\r\n')
    d += E + b'E' + E + b'4' + E + b'-\x01' + t('Bold italic underlined') + E + b'-\x00' + E + b'5' + E + b'F' + b'\r\n'
    d += b'\r\n'
    # Epson master select ESC ! n
    for n, name in () if not master else ((0x00, 'Pica'), (0x01, 'Elite'), (0x04, 'Condensed'), (0x08, 'Emphasized'),
                    (0x10, 'Double strike'), (0x20, 'Double width'), (0x40, 'Italic'), (0x80, 'Underline'),
                    (0x38, 'Emph+DStrike+Wide')):
        d += E + b'!' + bytes([n]) + t('ESC ! %02Xh %s' % (n, name)) + E + b'!\x00' + b'\r\n'
    if master:
        d += b'\r\n'
    # international character sets ESC R n
    for n, name in () if not intl else enumerate(('USA', 'France', 'Germany', 'England', 'Denmark', 'Sweden', 'Italy', 'Spain')):
        d += t('ESC R %d %-8s: ' % (n, name)) + E + b'R' + bytes([n]) + t('#$@[\\]^`{|}~') + E + b'R\x00' + b'\r\n'
    if italic_set:
        d += b'\r\n' + t('Italic character set (A0h-FEh):\r\n') + bytes(range(0xa0, 0xff)) + b'\r\n'
    if colour:
        d += b'\r\n'
        for n, name in enumerate(('Black', 'Red', 'Blue', 'Violet', 'Yellow', 'Orange', 'Green')):
            d += E + b'r' + bytes([n]) + E + b'E' + t('ESC r %d %-7s ' % (n, name)) + E + b'F'
            d += t('\xdb' * 0 + 'MMMMMMMMMMMMMMMMMMMM ' + SAMPLE[:23]) + b'\r\n'
        d += E + b'r\x00'
    d += b'\x0c'
    return d


def fonts_ibm(title):
    # IBM Proprinter / Graphics Printer command set (Star NL-10 with IBM cartridge)
    d = b'\x12' + E + b'F' + E + b'H' + E + b'-\x00' + E + b'W\x00' + E + b'T'
    d += E + b'E' + t(title) + E + b'F' + b'\r\n\r\n'
    d += t('Pica 10 cpi:      ' + SAMPLE + '\r\n')
    d += E + b'x\x01' + t('NLQ (ESC x 1):    ' + SAMPLE + '\r\n') + E + b'x\x00'
    d += b'\x0f' + t('Condensed 17 cpi: ' + SAMPLE + ' ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n') + b'\x12'
    d += b'\r\n'
    d += E + b'E' + t('Emphasized') + E + b'F' + t('  ') + E + b'G' + t('Double strike') + E + b'H' + t('  ')
    d += E + b'-\x01' + t('Underlined') + E + b'-\x00' + b'\r\n'
    d += E + b'W\x01' + t('Double width') + E + b'W\x00' + t('  ') + b'\x0e' + t('SO wide') + b'\x14' + b'\r\n'
    d += t('H') + E + b'S\x01' + t('2') + E + b'T' + t('O, x') + E + b'S\x00' + t('2') + E + b'T'
    d += t('   Sub- and superscript\r\n')
    d += E + b'E' + E + b'-\x01' + t('Bold underlined') + E + b'-\x00' + E + b'F' + b'\r\n'
    d += E + b'x\x01' + E + b'W\x01' + t('NLQ wide') + E + b'W\x00' + t('  ') + E + b'E' + t('NLQ bold') + E + b'F' + E + b'x\x00' + b'\r\n'
    d += b'\r\n'
    d += t('IBM character set 2 (ESC 6), 80h-FFh:\r\n') + E + b'6'
    for lo in range(0x80, 0x100, 0x20):
        d += t('%02Xh: ' % lo) + bytes(range(lo, lo + 0x20)) + b'\r\n'
    d += E + b'7'
    d += b'\r\n'
    d += b'\xc9' + b'\xcd' * 30 + b'\xbb\r\n'
    d += b'\xba' + t('  Box drawing with IBM chars  ') + b'\xba\r\n'
    d += b'\xc8' + b'\xcd' * 30 + b'\xbc\r\n'
    d += b'\x0c'
    return d


# ---------------------------------------------------------------- graphics

def mandel_gray(w, h, x0=-2.2, x1=0.9, y0=-1.25, y1=1.25, maxit=96):
    img = []
    for j in range(h):
        row = []
        ci = y0 + (y1 - y0) * j / (h - 1)
        for i in range(w):
            cr = x0 + (x1 - x0) * i / (w - 1)
            zr = zi = 0.0
            n = 0
            while n < maxit and zr * zr + zi * zi < 4.0:
                zr, zi = zr * zr - zi * zi + cr, 2 * zr * zi + ci
                n += 1
            if n >= maxit:
                v = 0.0
            else:
                mu = n + 1 - math.log(math.log(math.sqrt(zr * zr + zi * zi) + 1e-9) + 1e-9) / math.log(2)
                v = min(1.0, max(0.0, 1.0 - math.sqrt(mu / maxit)))
                v = 0.15 + 0.85 * v
            row.append(v)   # 0 = black, 1 = white
        img.append(row)
    return img


def dither(img):
    # Floyd-Steinberg, returns 1 = dot
    h, w = len(img), len(img[0])
    a = [r[:] for r in img]
    out = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            old = a[y][x]
            new = 1.0 if old >= 0.5 else 0.0
            out[y][x] = 0 if new else 1
            e = old - new
            if x + 1 < w: a[y][x + 1] += e * 7 / 16
            if y + 1 < h:
                if x > 0: a[y + 1][x - 1] += e * 3 / 16
                a[y + 1][x] += e * 5 / 16
                if x + 1 < w: a[y + 1][x + 1] += e * 1 / 16
    return out


def bands(bitmap, cmd=b'L'):
    # 8-dot bands, bit 7 = top needle; ESC 3 24 = 24/216" = 8 dots at 1/72"
    h, w = len(bitmap), len(bitmap[0])
    d = b''
    for y in range(0, h, 8):
        cols = bytearray()
        for x in range(w):
            v = 0
            for k in range(8):
                if y + k < h and bitmap[y + k][x]:
                    v |= 0x80 >> k
            cols.append(v)
        while cols and cols[-1] == 0:
            cols.pop()
        if cols:
            d += E + cmd + bytes([len(cols) & 255, len(cols) >> 8]) + bytes(cols)
        d += b'\r\n'
    return d


def graphics_page(title_cmds):
    W, H = 900, 400                     # 7.5" x 5.6" at 120 x 72 dpi
    img = mandel_gray(W, H)
    bm = dither(img)
    d = title_cmds
    d += t('Bit image graphics, ESC L (120 x 72 dpi), Floyd-Steinberg dithered\r\n\r\n')
    d += E + b'3\x18' + bands(bm) + E + b'2' + b'\r\n'
    # grey wedge, 16 steps, 120 dpi
    wedge = []
    for y in range(32):
        wedge.append([1.0 - (x // (W // 16)) / 15.0 for x in range(W)])
    d += t('Grey wedge:\r\n') + E + b'3\x18' + bands(dither(wedge)) + E + b'2'
    # line art: sine waves and a frame, 60 dpi ESC K
    w60, h60 = 450, 48
    art = [[0] * w60 for _ in range(h60)]
    for x in range(w60):
        art[0][x] = art[h60 - 1][x] = 1
        for ph in (0.0, 2.1, 4.2):
            y = int((h60 - 1) / 2 + (h60 / 2 - 3) * math.sin(x / 24.0 + ph))
            art[y][x] = 1
    for y in range(h60):
        art[y][0] = art[y][w60 - 1] = 1
    d += t('\r\nLine graphics, ESC K (60 x 72 dpi):\r\n') + E + b'3\x18' + bands(art, b'K') + E + b'2'
    d += b'\x0c'
    return d


def jx80_colour_page():
    W, H = 900, 240
    maxit = 64
    # colour Mandelbrot: palette of the JX-80 ribbon colours, mixed on paper
    pal = [  # (red, blue, yellow, black) planes
        (0, 0, 1, 0),   # yellow
        (1, 0, 1, 0),   # orange
        (1, 0, 0, 0),   # red
        (1, 1, 0, 0),   # violet
        (0, 1, 0, 0),   # blue
        (0, 1, 1, 0),   # green
    ]
    planes = [[[0] * W for _ in range(H)] for _ in range(4)]
    for j in range(H):
        ci = -1.25 + 2.5 * j / (H - 1)
        for i in range(W):
            cr = -2.2 + 3.1 * i / (W - 1)
            zr = zi = 0.0
            n = 0
            while n < maxit and zr * zr + zi * zi < 4.0:
                zr, zi = zr * zr - zi * zi + cr, 2 * zr * zi + ci
                n += 1
            if n >= maxit:
                planes[3][j][i] = 1            # black inside
            elif n > 2:
                p = pal[(n - 3) % len(pal)]
                # thin out the outer bands so the page stays light
                if n > 6 or ((i + j) & 1) == 0:
                    for k in range(3):
                        planes[k][j][i] = p[k]
    d = E + b'@' + E + b'E' + t('Colour graphics (Epson ESC r)') + E + b'F' + b'\r\n'
    d += t('ESC L 120 dpi, one pass per ribbon colour: yellow, magenta, cyan, black\r\n\r\n')
    d += E + b'3\x18'
    order = ((2, 4), (0, 1), (1, 2), (3, 0))     # (plane, ESC r colour)
    for y in range(0, H, 8):
        for plane, colour in order:
            band = bands([planes[plane][y + k] if y + k < H else [0] * W for k in range(8)])
            band = band[:-2]                   # strip the CR LF, overprint the band
            if band:
                d += E + b'r' + bytes([colour]) + band + b'\r'
        d += b'\n'
    d += E + b'r\x00' + E + b'2' + b'\r\n'
    d += t('Colour bars:\r\n')
    for n, name in enumerate(('Black', 'Magenta', 'Cyan', 'Violet', 'Yellow', 'Orange', 'Green')):
        d += E + b'r' + bytes([n]) + E + b'L' + bytes([120, 0]) + b'\xff' * 120 + E + b'r\x00' + t(' ' + name + '  ')
        if n % 3 == 2:
            d += b'\r\n'
    d += E + b'r\x00' + b'\r\n\x0c'
    return d


save('PICTURE.PRN', picture())
save('FX80FONT.PRN', fonts_epson('Epson FX-80: fonts and print attributes'))
save('JX80FONT.PRN', fonts_epson('Epson JX-80: fonts, print attributes and colours', colour=True))
save('MD16FONT.PRN', fonts_epson('Nixdorf MD 16: fonts and print attributes', nlq=True, italic_set=False) [:-1]
     + E + b'k\x00' + E + b'x\x01' + t('NLQ Roman: ' + SAMPLE + '\r\n')
     + E + b'k\x01' + t('NLQ Script: ' + SAMPLE + '\r\n') + E + b'x\x00' + b'\x0c')
save('NL10FONT.PRN', fonts_ibm('Star NL-10 (IBM interface): fonts and print attributes'))
save('LX800FNT.PRN', fonts_epson('Epson LX-800: fonts and print attributes', nlq=True))
save('EX800FNT.PRN', fonts_epson('Epson EX-800: fonts and print attributes', nlq=True)[:-1]
     + E + b'x\x01' + E + b'k\x00' + t('NLQ Roman:        ' + SAMPLE + '\r\n')
     + E + b'k\x01' + t('NLQ Sans Serif:   ' + SAMPLE + '\r\n') + E + b'k\x00' + E + b'x\x00' + b'\x0c')


def lines_page():
    # 100 numbered lines and a form feed: page breaks, sheet changes, green bar paper
    d = E + b'@'
    for i in range(1, 101):
        d += t('Line %03d  ' % i + SAMPLE + '\r\n')
    return d + b'\x0c'


save('LINES.PRN', lines_page())
save('GRAPHICS.PRN', graphics_page(E + b'E' + t('Graphics test page') + E + b'F' + b'\r\n'))
save('COLGRAPH.PRN', jx80_colour_page())


# ---------------------------------------------------------------- Epson MX-80

def mx80_base():
    # the original MX-80 command set: the print mode applies to a whole line,
    # so every mode is switched on at the start of its line and off on the next
    d = E + b'@'
    d += t('Epson MX-80: basic command set\r\n') + E + b'E' + t('(mode changes take effect for the whole line)') + b'\r\n' + E + b'F'
    d += b'\r\n'
    d += t('Normal 10 cpi:        ' + SAMPLE + '\r\n')
    d += b'\x0f' + t('Condensed 16.5 cpi (SI): ' + SAMPLE + ' ABCDEFGHIJKLMNOPQRSTUVWXYZ') + b'\r\n' + b'\x12'
    d += b'\x0e' + t('Enlarged (SO)') + b'\r\n'
    d += b'\x0f\x0e' + t('Condensed enlarged (SI SO)') + b'\r\n' + b'\x12'
    d += E + b'E' + t('Emphasized (ESC E):   ' + SAMPLE) + b'\r\n' + E + b'F'
    d += E + b'G' + t('Double strike (ESC G): ' + SAMPLE) + b'\r\n' + E + b'H'
    d += E + b'E' + E + b'G' + t('Emphasized + double strike') + b'\r\n' + E + b'F' + E + b'H'
    d += b'\r\n'
    d += t('Line spacing:\r\n')
    for cmd, name in ((E + b'0', 'ESC 0  1/8"'), (E + b'1', 'ESC 1  7/72"'), (E + b'A\x0c', 'ESC A 12  12/72"'), (E + b'2', 'ESC 2  1/6"')):
        d += cmd
        for k in range(3):
            d += t('%s  line %d' % (name, k + 1)) + b'\r\n'
    d += b'\r\n'
    d += t('Characters 20h-7Eh:\r\n') + bytes(range(0x20, 0x50)) + b'\r\n' + bytes(range(0x50, 0x7f)) + b'\r\n'
    d += b'\r\n'
    # 80 column ruler
    d += t(''.join(str(i // 10 % 10) if i % 10 == 0 else ' ' for i in range(1, 81))) + b'\r\n'
    d += t(''.join(str(i % 10) for i in range(1, 81))) + b'\r\n'
    d += b'\x0c'
    return d


def mx80_graftrax():
    # Graftrax: bit image graphics, ESC A line spacing, ESC R, italics
    # (8/72" with ESC A 8: these ROMs have no ESC 3)
    d = E + b'@'
    d += t('Epson MX-80 Graftrax: graphics and extended commands\r\n\r\n')
    # Mandelbrot, ESC L 120 dpi, 8" = 960 dots max
    W, H = 840, 240
    d += t('Bit image ESC L (120 x 72 dpi):\r\n') + E + b'A\x08' + bands(dither(mandel_gray(W, H))) + E + b'2'
    # line art, ESC K 60 dpi
    w60, h60 = 420, 40
    art = [[0] * w60 for _ in range(h60)]
    for x in range(w60):
        art[0][x] = art[h60 - 1][x] = 1
        for ph in (0.0, 2.1, 4.2):
            art[int((h60 - 1) / 2 + (h60 / 2 - 3) * math.sin(x / 20.0 + ph))][x] = 1
    for y in range(h60):
        art[y][0] = art[y][w60 - 1] = 1
    d += t('\r\nBit image ESC K (60 x 72 dpi):\r\n') + E + b'A\x08' + bands(art, b'K') + E + b'2'
    # alignment: vertical bars in both print directions
    d += t('\r\nBidirectional alignment (bars must line up):\r\n')
    for k in range(6):
        d += t(('|' + ' ' * 9) * 7 + '|') + b'\r\n'
    d += b'\r\n'
    d += t('Line spacing ESC A n (n/72"):\r\n')
    for n in (10, 12, 18):
        d += E + b'A' + bytes([n])
        for k in range(2):
            d += t('ESC A %d  line %d' % (n, k + 1)) + b'\r\n'
    d += E + b'2' + b'\r\n'
    for n, name in enumerate(('USA', 'France', 'Germany', 'England', 'Denmark', 'Sweden', 'Italy', 'Spain')):
        d += t('ESC R %d %-8s: ' % (n, name)) + E + b'R' + bytes([n]) + t('#$@[\\]^`{|}~') + E + b'R\x00' + b'\r\n'
    d += b'\r\n' + E + b'4' + t('ESC 4 italics: ' + SAMPLE) + b'\r\n' + E + b'5'
    d += b'\x0c'
    return d


def mx80_dots_perfect():
    # Dots Perfect: FX-80 style attributes within a line, NLQ selected with DIP SW2-2
    d = E + b'@'
    d += E + b'E' + t('Epson MX-80 with Dots Perfect') + E + b'F' + b'\r\n'
    d += t('(near letter quality: DIP switch SW2-2 ON, then switch the printer off and on)\r\n\r\n')
    d += t('Pica 10 cpi:      ' + SAMPLE + '\r\n')
    d += b'\x0f' + t('Condensed:        ' + SAMPLE + ' ABCDEFGHIJKLMNOPQRSTUVWXYZ') + b'\x12' + b'\r\n'
    d += b'\r\n'
    d += E + b'E' + t('Emphasized') + E + b'F' + t('  ') + E + b'G' + t('Double strike') + E + b'H' + t('  ')
    d += E + b'-\x01' + t('Underlined') + E + b'-\x00' + t('  ') + E + b'4' + t('Italic') + E + b'5' + b'\r\n'
    d += E + b'W\x01' + t('Double width') + E + b'W\x00' + t('  ') + b'\x0e' + t('SO wide') + b'\x14' + t('  ')
    d += b'\x0f' + E + b'W\x01' + t('Condensed wide') + E + b'W\x00' + b'\x12' + b'\r\n'
    d += t('H') + E + b'S\x01' + t('2') + E + b'T' + t('O, x') + E + b'S\x00' + t('2') + E + b'T'
    d += t(' + y') + E + b'S\x00' + t('2') + E + b'T' + t('   Sub- and superscript\r\n')
    d += E + b'E' + E + b'4' + E + b'-\x01' + t('Bold italic underlined') + E + b'-\x00' + E + b'5' + E + b'F' + b'\r\n'
    d += b'\r\n' + t('Italic character set (A0h-FEh):\r\n') + bytes(range(0xa0, 0xff)) + b'\r\n'
    d += b'\r\n'
    W, H = 840, 160
    d += t('Bit image ESC L (120 x 72 dpi):\r\n') + E + b'3\x18' + bands(dither(mandel_gray(W, H))) + E + b'2'
    d += b'\x0c'
    return d


save('MX80.PRN', mx80_base())
save('MX80GTRX.PRN', mx80_graftrax())
save('MX80DP.PRN', mx80_dots_perfect())


# ---------------------------------------------------------------- Epson FX-880, Citizen 200GX

def line_art(w, h, period):
    art = [[0] * w for _ in range(h)]
    for x in range(w):
        art[0][x] = art[h - 1][x] = 1
        for ph in (0.0, 2.1, 4.2):
            art[int((h - 1) / 2 + (h / 2 - 3) * math.sin(x / period + ph))][x] = 1
    for y in range(h):
        art[y][0] = art[y][w - 1] = 1
    return art


def bands_star(bitmap, m):
    # ESC * m n1 n2: 8-pin bit image in mode m
    h, w = len(bitmap), len(bitmap[0])
    d = b''
    for y0 in range(0, h, 8):
        cols = []
        for x in range(w):
            v = 0
            for b in range(8):
                y = y0 + b
                if y < h and bitmap[y][x]:
                    v |= 0x80 >> b
            cols.append(v)
        d += E + b'*' + bytes([m, w & 255, w >> 8]) + bytes(cols) + b'\r\n'
    return d


def bands_caret(bitmap, m):
    # ESC ^ m n1 n2: 9-pin bit image, two bytes per column (second byte bit 7 = pin 9)
    h, w = len(bitmap), len(bitmap[0])
    d = b''
    for y0 in range(0, h, 9):
        cols = b''
        for x in range(w):
            v = 0
            for b in range(9):
                y = y0 + b
                if y < h and bitmap[y][x]:
                    v |= 0x100 >> b
            cols += bytes([v >> 1, (v & 1) << 7])
        d += E + b'^' + bytes([m, w & 255, w >> 8]) + cols + b'\r\n'
    return d


def fx880_graphics():
    d = E + b'@' + E + b'E' + t('Epson FX-880: bit image modes') + E + b'F' + b'\r\n\r\n'
    # one inch wide line art in every ESC * density; modes 2 and 3 print no adjacent dots
    modes = ((0, 60, 'single density'), (1, 120, 'double density'), (2, 120, 'high speed double density'),
             (3, 240, 'quadruple density'), (4, 80, 'CRT I'), (5, 72, 'plotter 1:1'), (6, 90, 'CRT II'))
    for m, dpi, name in modes:
        d += t('ESC * %d  %3d dpi  %s:' % (m, dpi, name)) + b'\r\n'
        d += E + b'3\x18' + bands_star(line_art(2 * dpi, 16, dpi / 12.0), m) + E + b'2' + b'\r\n'
    d += t('ESC ^ 0  60 dpi, 9 pins:') + b'\r\n' + E + b'3\x1b' + bands_caret(line_art(240, 18, 10.0), 0) + E + b'2' + b'\r\n'
    d += t('ESC ^ 1 120 dpi, 9 pins:') + b'\r\n' + E + b'3\x1b' + bands_caret(line_art(480, 18, 20.0), 1) + E + b'2'
    d += b'\r\n'
    # dithered picture at 120 dpi
    W, H = 720, 240
    d += t('ESC * 1, Floyd-Steinberg dithered:\r\n') + E + b'3\x18' + bands(dither(mandel_gray(W, H))) + E + b'2'
    d += b'\r\n' + t('Bidirectional alignment (bars must line up):\r\n')
    for k in range(6):
        d += t(('|' + ' ' * 9) * 7 + '|') + b'\r\n'
    d += b'\x0c'
    return d


save('FX880FNT.PRN', fonts_epson('Epson FX-880: fonts and print attributes', nlq=True)[:-1]
     + E + b'x\x01' + E + b'k\x00' + t('LQ Roman:         ' + SAMPLE + '\r\n')
     + E + b'k\x01' + t('LQ Sans Serif:    ' + SAMPLE + '\r\n') + E + b'k\x00' + E + b'x\x00' + b'\x0c')
save('FX880GRF.PRN', fx880_graphics())
save('C200FNT.PRN', fonts_epson('Citizen 200GX: fonts and print attributes', nlq=True)[:-1]
     + E + b'x\x01' + E + b'k\x00' + t('NLQ Roman:        ' + SAMPLE + '\r\n')
     + E + b'k\x01' + t('NLQ Sans Serif:   ' + SAMPLE + '\r\n')
     + E + b'k\x02' + t('NLQ Courier:      ' + SAMPLE + '\r\n') + E + b'k\x00' + E + b'x\x00' + b'\x0c')


# ---------------------------------------------------------------- Epson ActionPrinter 3250 / LQ-100 (ESC/P 2)

def bands24(bitmap, m):
    # 24-pin bit image ESC * m, three bytes per column, 24/180" per band
    h, w = len(bitmap), len(bitmap[0])
    d = b''
    for y0 in range(0, h, 24):
        cols = bytearray()
        for x in range(w):
            v = 0
            for k in range(24):
                y = y0 + k
                v = (v << 1) | (1 if y < h and bitmap[y][x] else 0)
            cols += bytes([(v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff])
        d += E + b'*' + bytes([m, w & 0xff, w >> 8]) + bytes(cols) + b'\r\n'
    return d


def ap3250_fonts():
    d = E + b'@' + E + b'x\x01' + E + b'E' + t('Epson ActionPrinter 3250 (LQ-100): ESC/P 2 fonts') + E + b'F' + b'\r\n\r\n'
    for n, name in ((0, 'Roman'), (1, 'Sans Serif'), (2, 'Courier'), (3, 'Prestige'), (4, 'Script')):
        d += E + b'k' + bytes([n]) + t('ESC k %d %-11s ' % (n, name + ':') + SAMPLE + '\r\n')
    d += E + b'k\x00' + E + b'x\x00' + t('Draft:              ' + SAMPLE + '\r\n')
    d += b'\x0f' + t('Draft condensed:    ' + SAMPLE + ' ABCDEFGHIJKLMNOPQRSTUVWXYZ\r\n') + b'\x12' + E + b'x\x01'
    d += E + b'M' + t('LQ Roman 12 cpi:    ' + SAMPLE + '\r\n') + E + b'P'
    d += E + b'g' + t('LQ Roman 15 cpi:    ' + SAMPLE + '\r\n') + E + b'P'
    d += E + b'p\x01' + t('Proportional:       Illinois Minimum Wilhelm mmm iii\r\n') + E + b'p\x00'
    d += b'\r\n'
    # ESC/P 2 scalable fonts: ESC X m nL nH, size in half points; ESC ( - for scoring
    d += t('Scalable fonts (ESC X, 8 to 32 point):\r\n')
    for face, name in ((0, 'Roman'), (1, 'Sans Serif')):
        d += E + b'k' + bytes([face])
        for pt in (8, 10, 12, 16, 20, 24, 32):
            d += E + b'+' + bytes([pt * 6]) + E + b'X\x01' + bytes([pt * 2, 0]) + t('%s %d pt  ' % (name, pt)) + b'\r\n'
        d += E + b'X\x00\x00\x00' + E + b'P' + E + b'2'
    d += E + b'k\x00' + b'\r\n'
    d += E + b'E' + t('Emphasized') + E + b'F' + t('  ') + E + b'G' + t('Double strike') + E + b'H' + t('  ')
    d += E + b'-\x01' + t('Underlined') + E + b'-\x00' + t('  ') + E + b'4' + t('Italic') + E + b'5' + t('  ')
    d += E + b'q\x01' + t('Outline') + E + b'q\x02' + t('  Shadow') + E + b'q\x00' + b'\r\n'
    d += E + b'W\x01' + t('Double width') + E + b'W\x00' + t('  ') + E + b'w\x01' + t('Double height') + E + b'w\x00' + b'\r\n'
    d += t('H') + E + b'S\x01' + t('2') + E + b'T' + t('O, x') + E + b'S\x00' + t('2') + E + b'T' + t('   Sub- and superscript\r\n')
    d += b'\x0c'
    return d


def ap3250_graphics():
    d = E + b'@' + E + b'E' + t('Epson ActionPrinter 3250 (LQ-100): 24-pin bit image modes') + E + b'F' + b'\r\n\r\n'
    for m, dpi, name in ((32, 60, 'single density'), (33, 120, 'double density'), (38, 90, 'CRT III'),
                         (39, 180, 'triple density'), (40, 360, 'hex density')):
        d += t('ESC * %d  %3d dpi  %s:' % (m, dpi, name)) + b'\r\n'
        d += E + b'3\x18' + bands24(line_art(2 * dpi, 24, dpi / 12.0), m) + E + b'2' + b'\r\n'
    W, H = 1080, 360
    d += t('ESC * 39, 180 dpi, Floyd-Steinberg dithered:\r\n') + E + b'3\x18' + bands24(dither(mandel_gray(W, H)), 39) + E + b'2'
    d += b'\r\n' + t('Bidirectional alignment (bars must line up):\r\n')
    for k in range(6):
        d += t(('|' + ' ' * 9) * 7 + '|') + b'\r\n'
    d += b'\x0c'
    return d


save('AP32FNT.PRN', ap3250_fonts())
save('AP32GRF.PRN', ap3250_graphics())


# ---------------------------------------------------------------- additions November 2026

save('FX85FONT.PRN', fonts_epson('Epson FX-85: fonts and print attributes', nlq=True))
save('LX86FONT.PRN', fonts_epson('Epson LX-86: fonts and print attributes', nlq=True))
save('RX80FONT.PRN', fonts_epson('Epson RX-80: fonts and print attributes', elite=False, prop=False, master=False))
save('BX80FONT.PRN', fonts_epson('BMC BX-80: fonts and print attributes', elite=False, prop=False, master=False, intl=False))
save('GEM10FNT.PRN', fonts_epson('Star Gemini-10X: fonts and print attributes', italic_set=False, elite=False, prop=False, master=False, intl=False))


def colour_text():
    # Epson ESC r: 0 black, 1 magenta (red), 2 cyan (blue), 3 violet, 4 yellow, 5 orange, 6 green
    d = E + b'@' + E + b'E' + t('Colour printing (Epson ESC r)') + E + b'F' + b'\r\n\r\n'
    names = ('Black', 'Magenta (red)', 'Cyan (blue)', 'Violet', 'Yellow', 'Orange', 'Green')
    for n, name in enumerate(names):
        d += E + b'r' + bytes([n]) + t('ESC r %d  %-14s ' % (n, name))
        d += E + b'E' + t(SAMPLE[:30]) + E + b'F' + b'\r\n'
    d += E + b'r\x00' + b'\r\n'
    # rainbow line: one word per colour
    for n, word in enumerate(('BLACK ', 'MAGENTA ', 'CYAN ', 'VIOLET ', 'YELLOW ', 'ORANGE ', 'GREEN')):
        d += E + b'r' + bytes([n]) + E + b'E' + t(word.strip()) + E + b'F' + t(' ')
    d += E + b'r\x00' + b'\r\n\r\n'
    # colour bars, bit image ESC L
    for n, name in enumerate(names):
        d += E + b'r' + bytes([n]) + E + b'L' + bytes([240, 0]) + b'\xff' * 240 + E + b'r\x00' + t('  ' + name) + b'\r\n'
    d += E + b'r\x00' + b'\x0c'
    return d


save('COLOUR.PRN', colour_text())

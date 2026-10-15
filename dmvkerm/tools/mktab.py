"""Erzeugt src/tabellen.inc fuer DMVKERM.

  glyph   256 Byte  empfangenes Zeichen (CP437) -> Byte an die DMV-Konsole.
                    Der Zeichengenerator 76161 hat 128 Glyphen und
                    uebergeht Bit 7; gesendet wird Glyphe OR 80h, damit
                    NCRs Sprachtabelle unter DOS 2.11 nicht eingreift.
                    Akzentglyphen liegen auf 00h..1Fh, Rahmenzeichen
                    werden + - | .
  tastw   256 Worte Rohcode des 8741 -> Taste. Niederwertiges Byte das
                    Zeichen, hoeherwertiges 0, oder 1 und ein Tastencode
                    (K_xxx) fuer Cursor- und Funktionstasten.
  tastde  Tripel    Abweichungen der deutschen Tastatur (Rohcode, Wort)
  tastdos Tripel    Zeichen, die DOS schon vorgelesen und umgesetzt hat
"""
import sys
GLYPH = {
    'Ä': 0x00, 'Æ': 0x01, 'Å': 0x02, '£': 0x03, 'Ñ': 0x05, 'Ö': 0x06,
    'Ø': 0x07, 'ç': 0x08, 'Ü': 0x09, 'à': 0x0A, 'è': 0x0B, '´': 0x0C,
    '°': 0x0D, '¨': 0x0F, 'ä': 0x10, 'æ': 0x11, 'å': 0x12, '¤': 0x13,
    'é': 0x14, 'ñ': 0x15, 'ö': 0x16, 'ø': 0x17, 'ò': 0x18, 'ü': 0x19,
    'ù': 0x1A, 'ì': 0x1B, '§': 0x1C, '¿': 0x1D, 'ß': 0x1E, '¡': 0x1F,
}
ERSATZ = dict(zip('âáêëïîíôóûúÿÇÉ¢¥ƒªº«»±÷·²µ≥≤≈■¬∞πΣσΩ', 'aaeeiiioouuyCEcYfao<>+/.2u><~#-8nSsO'))

def glyphe(z):
    if z in GLYPH: return GLYPH[z]
    if z in ERSATZ: return ord(ERSATZ[z])
    o = ord(z)
    if 0x20 <= o < 0x7F: return o
    if z in '│║┃': return ord('|')
    if z in '─━': return ord('-')
    if z in '═': return ord('=')
    if z in '░▒▓█▄▌▐▀': return ord('#')
    if 0x2500 <= o <= 0x257F: return ord('+')
    return ord('?')

def glyphtab():
    t = []
    for b in range(256):
        if b < 0x20 or b == 0x7F: g = 0x20
        else: g = glyphe(bytes([b]).decode('cp437'))
        t.append(g | 0x80 if g != 0x20 else 0x20)
    return t

# Tastencodes (hoeherwertiges Byte 1, niederwertiges der Code)
K = dict(HOCH=1, RUNTER=2, RECHTS=3, LINKS=4, POS1=5)
for i in range(20): K['F%d' % (i + 1)] = 0x10 + i

def tastatur():
    w = [0] * 256
    for c in range(0x80): w[c] = c
    cur = {1: 'POS1', 2: 'LINKS', 3: 'RUNTER', 4: 'HOCH', 5: 'RECHTS'}
    for n, k in cur.items():
        w[0x80 + n] = 0x100 | K[k]
        w[0x90 + n] = 0x100 | K[k]
    w[0x86] = 0x7F                 # CLR = DEL
    w[0x96] = 0x1D                 # Ctrl-CLR: Fluchttaste
    w[0x88] = 0x0D                 # Return
    w[0x98] = 0x0A                 # Ctrl-Return
    w[0x8A] = ord('.')
    w[0x8B] = 0x08                 # Rueckschritt
    w[0x9B] = 0x08
    for i in range(20):
        for b in (0xE0, 0xC0, 0xA0):
            w[b + i] = 0x100 | K['F%d' % (i + 1)]
    w[0xB3] = 0                    # Ctrl-F20: Kaltstart der Hardware
    kps = {0xD4: '4', 0xD5: '5', 0xD6: '6', 0xD7: '7', 0xD8: '8', 0xD9: '9',
           0xDA: '*', 0xDB: '+', 0xDC: '1', 0xDD: '2', 0xDE: '3', 0xDF: '/',
           0xF4: '-'}
    for r, z in kps.items(): w[r] = ord(z)
    return w

DEUTSCH = [(0x7B, 0x84), (0x7C, 0x94), (0x7D, 0x81), (0x5B, 0x8E),
           (0x5C, 0x99), (0x5D, 0x9A), (0x7E, 0xE1), (0x40, 0x40),
           (0xD7, 0x5B), (0xD8, 0x5D), (0xD9, 0x5C), (0xD4, 0x7B),
           (0xD5, 0x7D), (0xD6, 0x7C), (0xDC, 0x40), (0x8A, 0x2C)]
DOSK = [(0x90, 0x84), (0x96, 0x94), (0x99, 0x81), (0x80, 0x8E), (0x86, 0x99),
        (0x89, 0x9A), (0x9E, 0xE1), (0x9C, 0x40), (0x8C, 0x1D)]

def main():
    o = ['; tabellen.inc - von tools/mktab.py erzeugt, nicht von Hand aendern', '']
    for k, v in K.items(): o.append('K_%s\tequ\t%03Xh' % (k, v))
    g = glyphtab(); o.append('glyph\tlabel\tbyte')
    for i in range(0, 256, 16): o.append('\tdb\t' + ','.join('%03Xh' % x for x in g[i:i+16]))
    w = tastatur(); o.append('tastw\tlabel\tword')
    for i in range(0, 256, 8): o.append('\tdw\t' + ','.join('%05Xh' % x for x in w[i:i+8]))
    for name, tab in (('tastde', DEUTSCH), ('tastdos', DOSK)):
        o.append('%s\tlabel\tbyte' % name)
        for r, v in tab: o.append('\tdb\t%03Xh\n\tdw\t%05Xh' % (r, v))
        o.append('%sn\tequ\t%d' % (name, len(tab)))
    open(sys.argv[1], 'w').write('\n'.join(o) + '\n')
main()

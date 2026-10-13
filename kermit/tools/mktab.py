"""Erzeugt msxdmvt.inc: Umsetztabellen fuer das DMV-Kermit.

  dmvglyph  256 Byte  Zeichen im Bildspeicher (Codepage 437, so wie Kermit
                      ihn fuellt) -> Byte, das an die DMV-Konsole geht.
                      Der Zeichengenerator 76161 hat 128 Glyphen und
                      ignoriert Bit 7; ausgegeben wird darum Glyphe OR 80h.
                      Damit greift NCRs Sprachtabelle (CRTACTTBL) unter
                      DOS 2.11 nicht, die z.B. auf der deutschen Fassung
                      '[' als 'Ae' zeigt.
  dmvkeyw   256 Worte Rohcode des 8741 -> Scan*256 + ASCII (0 = verwerfen)
  dmvkeyf   256 Byte  Umschaltzustand zum Rohcode (02h Shift, 04h Ctrl)
  dmvkeyde  Paare     Abweichungen fuer die deutsche Tastatur
  dmvdosk   Paare     Abweichungen fuer Zeichen, die DOS schon vorgelesen hat
"""
import sys

# --- Zeichengenerator 76161: Akzentglyphen auf 00h..1Fh -----------------
# aus NCRs CRTTBL (basinit.asm, DOS 2.11): welche Glyphe die einzelnen
# Landesfassungen an die Stelle der ISO-646-Zeichen setzen.
GLYPH = {
    'Ä': 0x00, 'Æ': 0x01, 'Å': 0x02, '£': 0x03, 'Ñ': 0x05, 'Ö': 0x06,
    'Ø': 0x07, 'ç': 0x08, 'Ü': 0x09, 'à': 0x0A, 'è': 0x0B, '´': 0x0C,
    '°': 0x0D, '¨': 0x0F, 'ä': 0x10, 'æ': 0x11, 'å': 0x12, '¤': 0x13,
    'é': 0x14, 'ñ': 0x15, 'ö': 0x16, 'ø': 0x17, 'ò': 0x18, 'ü': 0x19,
    'ù': 0x1A, 'ì': 0x1B, '§': 0x1C, '¿': 0x1D, 'ß': 0x1E, '¡': 0x1F,
}
ERSATZ = {
    'â': 'a', 'á': 'a', 'ê': 'e', 'ë': 'e', 'ï': 'i', 'î': 'i', 'í': 'i',
    'ô': 'o', 'ó': 'o', 'û': 'u', 'ú': 'u', 'ÿ': 'y', 'Ç': 'C', 'É': 'E',
    '¢': 'c', '¥': 'Y', '₧': 'P', 'ƒ': 'f', 'ª': 'a', 'º': 'o', '½': '?',
    '¼': '?', '«': '<', '»': '>', '±': '+', '÷': '/', '∙': '.', '·': '.',
    '²': '2', 'µ': 'u', '√': 'v', 'ⁿ': 'n', '≥': '>', '≤': '<', '≈': '~',
    '■': '#', '⌐': '-', '¬': '-', '∞': '8', 'α': 'a', 'Γ': 'G', 'π': 'n',
    'Σ': 'S', 'σ': 's', 'τ': 't', 'Φ': 'F', 'Θ': 'O', 'Ω': 'O', 'δ': 'd',
    'φ': 'f', 'ε': 'e', '∩': 'n', '≡': '=', '⌠': '(', '⌡': ')',
}


def glyphe(z):
    if z in GLYPH:
        return GLYPH[z]
    if z in ERSATZ:
        return ord(ERSATZ[z])
    o = ord(z)
    if 0x20 <= o < 0x7F:
        return o
    # Rahmen und Flaechen
    if z in '│║╎┃': return ord('|')
    if z in '─━': return ord('-')
    if z in '═': return ord('=')
    if z in '░▒▓█▄▌▐▀': return ord('#')
    if 0x2500 <= o <= 0x257F: return ord('+')
    return ord('?')


def cp437():
    t = []
    for b in range(256):
        if b < 0x20:
            # Steuerzeichenbereich: im Bildspeicher nur Leerzeichen, NUL
            # und die beiden, die CP437 als Zeichen kennt (§ und ¶)
            z = {0x15: '§', 0x14: '¶'}.get(b, ' ')
            t.append(glyphe(z) if z != '¶' else ord('P'))
        elif b == 0x7F:
            t.append(ord('?'))
        else:
            t.append(glyphe(bytes([b]).decode('cp437')))
    return [g | 0x80 if g != 0x20 else 0x20 for g in t]


# --- Tastatur ------------------------------------------------------------
SHIFT, CTRL = 0x02, 0x04
FSCAN = [0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,
         0x85, 0x86] + [0xF0 + i for i in range(8)]       # F1..F20
CUR = {1: 0x47, 2: 0x4B, 3: 0x50, 4: 0x48, 5: 0x4D}       # POS1 links ...
KPCTRL = {0xB4: 0xD4, 0xB5: 0xD5, 0xB6: 0xD6, 0xB7: 0xD7, 0xB8: 0xD8,
          0xB9: 0xD9, 0xBA: 0xDA, 0xBB: 0xDB, 0xBC: 0xD1, 0xBD: 0xD2,
          0xBE: 0xD3, 0xBF: 0xDF, 0xF5: 0xDD}
KPSHIFT = {0xD4: '4', 0xD5: '5', 0xD6: '6', 0xD7: '7', 0xD8: '8',
           0xD9: '9', 0xDA: '*', 0xDB: '+', 0xDC: '1', 0xDD: '2',
           0xDE: '3', 0xDF: '/', 0xF4: '-'}


def tastatur():
    w = [0] * 256
    f = [0] * 256
    for c in range(0x80):
        w[c] = c                                 # ASCII wie getippt
    for n, sc in CUR.items():
        w[0x80 + n] = sc << 8
        w[0x90 + n] = sc << 8; f[0x90 + n] = CTRL
    w[0x86] = (0x53 << 8) | 0x7F                 # CLR = DEL
    w[0x96] = 0x1D                               # Ctrl-CLR = Ctrl-] (Escape)
    w[0x88] = (0x1C << 8) | 0x0D                 # Return / Enter
    w[0x98] = (0x1C << 8) | 0x0A; f[0x98] = CTRL
    w[0x8A] = ord('.')                           # Dezimaltaste
    w[0x8B] = (0x0E << 8) | 0x08                 # Rueckschritt
    w[0x9B] = (0x0E << 8) | 0x08; f[0x9B] = CTRL
    for i in range(20):
        w[0xE0 + i] = FSCAN[i] << 8
        w[0xC0 + i] = FSCAN[i] << 8; f[0xC0 + i] = SHIFT
        w[0xA0 + i] = FSCAN[i] << 8; f[0xA0 + i] = CTRL
    w[0xB3] = 0                                  # Ctrl-F20: Kaltstart (HW)
    for r, sc in KPCTRL.items():
        w[r] = sc << 8; f[r] = CTRL
    for r, z in KPSHIFT.items():
        w[r] = ord(z)
    return w, f


DEUTSCH = [   # Rohcode -> Wort
    (0x7B, 0x84), (0x7C, 0x94), (0x7D, 0x81),    # ae oe ue (CP437)
    (0x5B, 0x8E), (0x5C, 0x99), (0x5D, 0x9A),    # Ae Oe Ue
    (0x7E, 0xE1),                                # sz
    (0x40, 0x40),                                # Shift-3 (§) -> @
    (0xD7, 0x5B), (0xD8, 0x5D), (0xD9, 0x5C),    # Ziffernblock wie DOS 3.3:
    (0xD4, 0x7B), (0xD5, 0x7D), (0xD6, 0x7C),    #  [ ] \ { } |
    (0xDC, 0x40),                                #  @
    (0x8A, 0x2C),                                # Dezimaltaste = Komma
]
DOSK = [      # von DOS vorgelesen (DMVCON 3.3 / NCR 2.11)
    (0x90, 0x84), (0x96, 0x94), (0x99, 0x81), (0x80, 0x8E), (0x86, 0x99),
    (0x89, 0x9A), (0x9E, 0xE1), (0x9C, 0x40), (0x8C, 0x1D),
]


def asm():
    g = cp437()
    w, f = tastatur()
    out = ['; msxdmvt.inc - von tools/mktab.py erzeugt, nicht von Hand aendern',
           '']
    out.append('dmvglyph label byte')
    for i in range(0, 256, 16):
        out.append('\tdb\t' + ','.join('%03Xh' % x for x in g[i:i + 16]))
    out.append('dmvkeyw label word')
    for i in range(0, 256, 8):
        out.append('\tdw\t' + ','.join('%05Xh' % x for x in w[i:i + 8]))
    out.append('dmvkeyf label byte')
    for i in range(0, 256, 16):
        out.append('\tdb\t' + ','.join('%03Xh' % x for x in f[i:i + 16]))
    for name, tab in (('dmvkeyde', DEUTSCH), ('dmvdosk', DOSK)):
        out.append('%s label byte' % name)
        for r, v in tab:
            out.append('\tdb\t%03Xh\n\tdw\t%05Xh' % (r, v))
        out.append('%sn equ ($-%s)/3' % (name, name))
    return '\n'.join(out) + '\n'


if __name__ == '__main__':
    open(sys.argv[1], 'w').write(asm())

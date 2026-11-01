#!/usr/bin/env python3
"""pruefcpm.py - Pruefstand fuer CP/M-80 auf der DMV (Z80): ein .COM-Programm
unter einem nachgebildeten CP/M 2.2.

  pruefcpm.py VERZ PROG.COM [--tasten "..."] [--skript s.txt] [--mono]
              [--nomaus] [--arg "..."] [--bdos E406] [--log]

- BDOS (0005h) und BIOS (Sprungtafel) werden in Python erledigt:
  Konsole, Dateien (FCB, sequentiell und wahlfrei) im Verzeichnis VERZ.
- --tasten: Eingaben fuer die Konsole (Text; \\r Enter, \\e ESC, ^X
  Steuerzeichen), etwa um Turbo Pascal 3 (TURBO.COM) zu bedienen.
- --skript: wie GHWSIM/pruefdos (maus x y k / taste c / bild datei.ppm);
  je Abfrage der Konsole ohne Warten eine Zeile.
- 7220 an A0h/A1h/A2h ueber simlib.so (make simlib.so), K806 an 30h-33h,
  8741 an 40h/41h.
- --bdos: Lage des BDOS (Hex, Vorgabe E406 wie ein 56-KB-System); die TPA
  reicht bis dorthin.

Braucht das Python-Paket z80 (pip install z80).

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import argparse, ctypes, os, re, sys
import z80

HIER = os.path.dirname(os.path.abspath(__file__))

ap = argparse.ArgumentParser()
ap.add_argument('verz'); ap.add_argument('prog')
ap.add_argument('--tasten', default='')
ap.add_argument('--skript')
ap.add_argument('--mono', action='store_true')
ap.add_argument('--nomaus', action='store_true')
ap.add_argument('--arg', default='')
ap.add_argument('--bdos', default='E406')
ap.add_argument('--max', type=int, default=4000)
ap.add_argument('--log', action='store_true')
a = ap.parse_args()

sim = None
if a.skript:
    sim = ctypes.CDLL(os.path.join(HIER, 'simlib.so'))

def tasten(s):
    q = []; i = 0
    while i < len(s):
        c = s[i]
        if c == '\\' and i + 1 < len(s):
            q.append({'r': 13, 'e': 27, 'n': 10}.get(s[i + 1], ord(s[i + 1]))); i += 2; continue
        if c == '^' and i + 1 < len(s):
            q.append(ord(s[i + 1].upper()) - 64); i += 2; continue
        q.append(ord(c)); i += 1
    return q

q = tasten(a.tasten)
skript = open(a.skript).read().splitlines() if a.skript else []
BDOS = int(a.bdos, 16)
BIOS = (BDOS + 0x0E00) & 0xFF00         # Sprungtafel wie im echten System
m = z80.Z80Machine()
mem = bytearray(0x10000)
prog = open(os.path.join(a.verz, a.prog), 'rb').read()
mem[0x100:0x100 + len(prog)] = prog
mem[0:3] = bytes([0xC3, (BIOS + 3) & 0xFF, (BIOS + 3) >> 8])     # JP WBOOT
mem[5:8] = bytes([0xC3, BDOS & 0xFF, BDOS >> 8])                 # JP BDOS
for i in range(17):
    mem[BIOS + 3 * i] = 0xC9                                     # RET
mem[BDOS] = 0xC9
tail = (' ' + a.arg.upper()) if a.arg else ''
mem[0x80] = len(tail); mem[0x81:0x81 + len(tail)] = tail.encode()
mem[0x5C:0x5C + 12] = b'\0' + b' ' * 11
m.set_memory_block(0, bytes(mem))
m.pc = 0x100
m.sp = BDOS - 2
m.set_breakpoint(BDOS)
for i in range(17):
    m.set_breakpoint(BIOS + 3 * i)

st = dict(ende=False, dma=0x80, n2=0, leer=0, sx=320, sy=200, sb=0)
out = bytearray()
dateien = {}            # Dateiname -> Datei (Programme kopieren FCBs umher)

def rd(adr, n): return bytes(m.memory[adr:adr + n])
def wr(adr, b): m.set_memory_block(adr, bytes(b))

def zeile():
    taste = 0
    if not skript:
        return 0x1B
    z = skript.pop(0).split()
    if z and z[0] == 'maus':
        st['sx'], st['sy'], st['sb'] = int(z[1]), int(z[2]), int(z[3])
    elif z and z[0] == 'taste':
        w = z[1]; taste = int(w, 16) if w.startswith('0x') else ord(w[0])
    elif z and z[0] == 'bild':
        gdc_flush(); sim.w_bild(z[1].encode())
    return taste

def frage():            # Konsole ohne Warten -> Taste oder 0
    if a.skript:
        st['n2'] += 1
        if st['n2'] > a.max:
            st['ende'] = True; return 0x1B
        if st.get('vor'):
            t = st['vor']; return t
        st['vor'] = zeile()
        return st['vor']
    if q:
        return q[0]
    st['leer'] += 1
    if st['leer'] > 20000:
        st['ende'] = True
    return 0

def hole():             # Konsole mit Warten
    if a.skript:
        while not st['ende']:
            t = frage()
            st['vor'] = 0
            if t: return t
        return 0x1B
    if q:
        return q.pop(0)
    st['ende'] = True
    return 0x1A

# ---- Dateien ----
def fcbname(f):
    b = rd(f, 12)
    n = bytes(c & 0x7F for c in b[1:9]).decode('latin-1').strip()
    e = bytes(c & 0x7F for c in b[9:12]).decode('latin-1').strip()
    return n + ('.' + e if e else '')

def pfad(n):
    for x in os.listdir(a.verz):
        if x.upper() == n.upper():
            return os.path.join(a.verz, x)
    return os.path.join(a.verz, n.upper())

def satz(f):            # aktueller Satz aus EX, S2, CR
    b = rd(f, 33)
    return (b[12] & 0x1F) * 128 + (b[14] & 0x3F) * 4096 + b[32]

def setz_satz(f, n):
    wr(f + 12, bytes([(n // 128) & 0x1F])); wr(f + 14, bytes([(n // 4096) & 0x3F]))
    wr(f + 32, bytes([n % 128]))

def zufall(f):
    b = rd(f, 36)
    return b[33] | (b[34] << 8) | (b[35] << 16)

def lese(f, n):
    h = dateien.get(fcbname(f))
    if not h: return 9
    h.seek(n * 128); b = h.read(128)
    if not b: return 1
    wr(st['dma'], b.ljust(128, b'\x1a')); return 0

def schreibe(f, n):
    h = dateien.get(fcbname(f))
    if not h: return 9
    h.seek(n * 128); h.write(rd(st['dma'], 128)); return 0

suche = []
def such(f, erst):
    global suche
    if erst:
        mu = rd(f + 1, 11)
        suche = []
        for x in sorted(os.listdir(a.verz)):
            n, _, e = x.upper().partition('.')
            if len(n) > 8 or len(e) > 3: continue
            nm = (n.ljust(8) + e.ljust(3)).encode()
            if all(p == ord('?') or p == c for p, c in zip(mu, nm)):
                suche.append(nm)
    if not suche: return 0xFF
    nm = suche.pop(0)
    wr(st['dma'], b'\0' + nm + b'\0' * 20)
    return 0

def bdos(c, de):
    r = 0
    if c == 0:
        st['ende'] = True
    elif c == 1:
        r = hole(); out.append(r)
    elif c == 2:
        out.append(de & 0xFF)
    elif c == 6:
        e = de & 0xFF
        if e == 0xFF:
            r = frage()
            if r:
                if a.skript: st['vor'] = 0
                else: q.pop(0)
        elif e == 0xFE:
            r = 0xFF if frage() else 0
        else:
            out.append(e)
    elif c == 9:
        p = de
        while m.memory[p] != 0x24:
            out.append(m.memory[p]); p += 1
    elif c == 10:
        mx = m.memory[de]; s = bytearray()
        while True:
            t = hole()
            if t in (13, 10) or st['ende']: break
            if t == 8: s = s[:-1]; continue
            if len(s) < mx: s.append(t)
        wr(de + 1, bytes([len(s)]) + bytes(s))
    elif c == 11:
        r = 0xFF if frage() else 0
    elif c == 12:
        r = 0x22
    elif c in (13, 14):
        r = 0
    elif c == 15:                       # Oeffnen
        p = pfad(fcbname(de))
        if os.path.exists(p):
            alt = dateien.pop(fcbname(de), None)
            if alt: alt.close()
            dateien[fcbname(de)] = open(p, 'r+b'); setz_satz(de, 0); r = 0
        else:
            r = 0xFF
    elif c == 16:
        h = dateien.pop(fcbname(de), None)
        if h: h.close()
    elif c == 17:
        r = such(de, True)
    elif c == 18:
        r = such(de, False)
    elif c == 19:
        try: os.remove(pfad(fcbname(de)))
        except OSError: r = 0xFF
    elif c == 20:
        n = satz(de); r = lese(de, n)
        if r == 0: setz_satz(de, n + 1)
    elif c == 21:
        n = satz(de); r = schreibe(de, n)
        if r == 0: setz_satz(de, n + 1)
    elif c == 22:                       # Anlegen
        dateien[fcbname(de)] = open(pfad(fcbname(de)), 'w+b'); setz_satz(de, 0)
    elif c == 23:
        n2 = de + 16
        try: os.rename(pfad(fcbname(de)), pfad(fcbname(n2)))
        except OSError: r = 0xFF
    elif c == 24:
        r = 1
    elif c == 25:
        r = 0
    elif c == 26:
        st['dma'] = de
    elif c == 32:
        r = 0
    elif c == 33:
        r = lese(de, zufall(de))
    elif c in (34, 40):
        r = schreibe(de, zufall(de))
    elif c == 35:
        h = dateien.get(fcbname(de))
        if h:
            h.seek(0, 2); n = (h.tell() + 127) // 128
        else:
            p = pfad(fcbname(de)); n = (os.path.getsize(p) + 127) // 128 if os.path.exists(p) else 0
        wr(de + 33, bytes([n & 0xFF, (n >> 8) & 0xFF, n >> 16]))
    elif c == 36:
        n = satz(de); wr(de + 33, bytes([n & 0xFF, (n >> 8) & 0xFF, n >> 16]))
    else:
        if a.log: print('BDOS %d nicht nachgebildet' % c)
    return r

def bios(i):
    if i in (0, 1):                     # Kalt-/Warmstart
        st['ende'] = True
    elif i == 2:
        m.a = 0xFF if frage() else 0
    elif i == 3:
        m.a = hole()
    elif i == 4:
        out.append(m.c)

# ---- 7220, K806, 8741 (wie pruefdos.py) ----
gdc = dict(eintrag=bytearray(), lese=bytearray(), vs=0, n=0)
def gdc_flush():
    e = gdc['eintrag']
    if e and sim:
        l = bytes([len(e)]) + bytes(e) + b'\0'
        if e[0] & 0xE4 == 0xA0 or e[0] == 0xE0:
            buf = ctypes.create_string_buffer(1024)
            nl = sim.w_rdat(l, buf)
            gdc['lese'] = bytearray(buf.raw[:nl])
        else:
            sim.w_liste(l)
        gdc['n'] += 1
    gdc['eintrag'] = bytearray()

maus = dict(aus=bytearray())
def ein(port):
    p = port & 0xFF
    v = 0xFF
    if p == 0xA0:
        gdc['vs'] += 1
        v = 0x04 | (0x20 if (gdc['vs'] // 7) & 1 else 0) | (1 if gdc['lese'] else 0)
    elif p == 0xA1:
        gdc_flush()
        v = gdc['lese'].pop(0) if gdc['lese'] else 0
    elif p in (0x40, 0x41):
        v = 0
    elif 0x30 <= p <= 0x33 and not a.nomaus and a.skript:
        if p == 0x30: v = maus['aus'].pop(0) if maus['aus'] else 0
        elif p == 0x31: v = 1 if maus['aus'] else 0
        elif p == 0x33: v = 0x80
        else: v = 0
    return v

def aus(port, v):
    p = port & 0xFF
    v &= 0xFF
    if p == 0xA1:
        gdc_flush(); gdc['eintrag'] = bytearray([v])
        if v & 0xE4 == 0xA0 or v == 0xE0:
            gdc_flush()
    elif p in (0xA0, 0xA2):
        gdc['eintrag'].append(v)
    elif p == 0x31 and v == 0 and not a.nomaus:
        y = 399 - st['sy']
        maus['aus'] = bytearray([st['sx'] & 0xFF, st['sx'] >> 8, y & 0xFF, y >> 8, st['sb']])

m.set_input_callback(ein)
m.set_output_callback(aus)
if sim:
    sim.w_init(0 if a.mono else 1)
    sim.w_graf(1)

n = 0
while not st['ende'] and n < 20000:
    m.ticks_to_stop = 2000000
    ev = m.run()
    n += 1
    pc = m.pc
    if pc == BDOS or BIOS <= pc < BIOS + 51:
        if pc == BDOS:
            r = bdos(m.c, m.de)
            m.a = r & 0xFF; m.l = r & 0xFF; m.h = 0; m.b = 0
        else:
            bios((pc - BIOS) // 3)
        sp = m.sp                       # RET
        m.pc = m.memory[sp] | (m.memory[sp + 1] << 8)
        m.sp = sp + 2
for h in dateien.values():
    h.close()
gdc_flush()
s = out.decode('latin-1')
s = re.sub(r'\x1b[=][\s\S][\s\S]', '\n', s)
s = re.sub(r'\x1b.', '', s)
print(s[-3000:] if a.skript else s)
if a.skript:
    print('GDC-Eintraege:', gdc['n'])

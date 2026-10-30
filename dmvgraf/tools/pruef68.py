#!/usr/bin/env python3
"""pruef68.py - Pruefstand: ein .68K-Programm mit DMVGRAF unter einem
nachgebildeten CP/M-68K auf der DMV (68000 in Unicorn).

  pruef68.py PROG.68K skript.txt [--block] [--mono] [--nomaus] [--arg "1234"]

- BDOS (TRAP #2): Konsole, BDOS 6 mit Eingaben aus dem Skript (je Aufruf
  eine Zeile, wie GHWSIM: maus x y k / taste c / bild datei.ppm), Dateien
  im aktuellen Verzeichnis, BDOS 62 (Supervisorzustand), BDOS 50.
- 7220 an FFF00h + A0h/A1h/A2h ueber simlib.so: dieselbe Nachbildung wie
  GHWSIM.C (make simlib.so); K806 an 30h-33h, 8741 an 40h/41h.
- --block: BIOS 64-69 wie unser CP/M-8000, Kennung "DMVB" vor dem
  TRAP-3-Einsprung; sonst wie NCRs CP/M-68K 1.2 ohne DMV-BIOS-Block (ein
  BIOS-Aufruf ueber BDOS 50 haelt dann mit einer Meldung an).

Die Bilder lassen sich mit denen der PC-Simulation vergleichen (gleiches
Skript, gleiche Spielnummer).

Braucht das Python-Paket unicorn (2.x).

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import argparse, ctypes, os, struct, sys
from unicorn import *
from unicorn.m68k_const import *

HIER = os.path.dirname(os.path.abspath(__file__))
sim = ctypes.CDLL(os.path.join(HIER, 'simlib.so'))

ap = argparse.ArgumentParser()
ap.add_argument('prog'); ap.add_argument('skript')
ap.add_argument('--block', action='store_true')
ap.add_argument('--mono', action='store_true')
ap.add_argument('--arg', default='')
ap.add_argument('--nomaus', action='store_true')
ap.add_argument('--max', type=int, default=4000)
a = ap.parse_args()

mu = Uc(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN)
mu.ctl_set_cpu_model(UC_CPU_M68K_M68000)
mu.mem_map(0, 0x100000)
BASE = 0x10000
d = open(a.prog, 'rb').read()
mag, tl, dl, bl = struct.unpack('>HIII', d[:14])
assert mag == 0x601A
text = bytearray(d[28:28 + tl])
rel = struct.unpack('>%dH' % (tl // 2), d[28 + tl:28 + tl + tl])
for i, r in enumerate(rel):
    if r == 5:
        v = struct.unpack('>I', text[2 * i:2 * i + 4])[0] + BASE
        text[2 * i:2 * i + 4] = struct.pack('>I', v)
mu.mem_write(BASE, bytes(text))
bp = BASE - 0x100
tail = (' ' + a.arg.upper()) if a.arg else ''
bpage = bytearray(0x100)
struct.pack_into('>IIII', bpage, 0, BASE, 0xF0000, BASE, tl)
bpage[0x80] = len(tail); bpage[0x81:0x81 + len(tail)] = tail.encode()
mu.mem_write(bp, bytes(bpage))
sp = 0xF0000
sp -= 4; mu.mem_write(sp, struct.pack('>I', bp))
sp -= 4; mu.mem_write(sp, struct.pack('>I', 0x400))   # Ruecksprung (nie benutzt)
mu.mem_write(0x400, b'\x4e\x72\x27\x00')               # stop
# BDOS: jede TRAP-#2-Stelle im Programm bekommt einen Codehaken, der den
# Aufruf erledigt und dahinter weitermacht (Unicorns Ausnahmehaken fuehrt
# TRAP beim 68000 zweimal aus); andere Ausnahmen halten an
# TRAP-3-Vektor auf 600h; mit --block steht davor die Kennung "DMVB"
mu.mem_write(0x8C, struct.pack('>I', 0x600))
mu.mem_write(0x5FC, b'DMVB' if a.block else b'NCR?')
mu.reg_write(UC_M68K_REG_A7, 0xFF000)                 # Systemstapel
mu.reg_write(UC_M68K_REG_SR, 0x0000)                   # Benutzerzustand
mu.reg_write(UC_M68K_REG_A7, sp)

skript = open(a.skript).read().splitlines()
st = dict(sx=320, sy=200, sb=0, dma=0x80, fcb=None, ende=False, n2=0, gr=0)
datei = [None]
out = []

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

# ---- 7220 an den Anschluessen: Eintraege sammeln, RDAT beantworten ----
gdc = dict(eintrag=bytearray(), lese=bytearray(), vs=0, n=0)
def gdc_flush():
    e = gdc['eintrag']
    if e:
        l = bytes([len(e)]) + bytes(e) + b'\0'
        if e[0] & 0xE4 == 0xA0 or e[0] == 0xE0:    # RDAT, CURD
            buf = ctypes.create_string_buffer(1024)
            nl = sim.w_rdat(l, buf)
            gdc['lese'] = bytearray(buf.raw[:nl])
        else:
            sim.w_liste(l)
        gdc['eintrag'] = bytearray()
        gdc['n'] += 1

maus = dict(port=None, aus=bytearray(), cmd=None, par=bytearray())
def maus_antwort(c):
    if c == 0:
        y = 399 - st['sy']
        maus['aus'] = bytearray([st['sx'] & 0xFF, st['sx'] >> 8, y & 0xFF, y >> 8, st['sb']])
    elif c == 4:
        pass

def io_r(uc, acc, addr, size, value, ud):
    p = addr & 0xFF
    v = 0xFF
    if p == 0xA0:
        gdc['vs'] += 1
        v = 0x04 | (0x20 if (gdc['vs'] // 7) & 1 else 0) | (1 if gdc['lese'] else 0)
    elif p == 0xA1:
        gdc_flush()
        v = gdc['lese'].pop(0) if gdc['lese'] else 0
    elif p == 0x41:
        v = 0x00
    elif p == 0x40:
        v = 0x00
    elif 0x30 <= p <= 0x33 and not a.nomaus:
        if p == 0x30:
            v = maus['aus'].pop(0) if maus['aus'] else 0
        elif p == 0x31:
            v = 1 if maus['aus'] else 0
        elif p == 0x33:
            v = 0x80
        else:
            v = 0
    mu.mem_write(addr, bytes([v]))

def io_w(uc, acc, addr, size, value, ud):
    p = addr & 0xFF
    v = value & 0xFF
    if p == 0xA1:
        gdc_flush(); gdc['eintrag'] = bytearray([v])
        if v & 0xE4 == 0xA0 or v == 0xE0:
            gdc_flush()
    elif p == 0xA0 or p == 0xA2:
        gdc['eintrag'].append(v)
    elif p == 0x31 and not a.nomaus:
        maus_antwort(v)

def mem_hook(uc, acc, addr, size, value, ud):
    if acc == UC_MEM_READ:
        io_r(uc, acc, addr, size, value, ud)
    else:
        io_w(uc, acc, addr, size, value, ud)
mu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, mem_hook, begin=0xFFF00, end=0xFFFFF)

# Der 68000 kennt keine Wort- und Langwortzugriffe auf ungerade Adressen
# (Adressfehler); Unicorn fuehrt sie still aus. Hier halten sie an.
def ungerade(uc, acc, addr, size, value, ud):
    if size > 1 and addr & 1:
        print('Adressfehler: %d Byte %s %X bei PC %X' % (size,
              'schreiben' if acc == UC_MEM_WRITE else 'lesen', addr,
              uc.reg_read(UC_M68K_REG_PC)))
        uc.emu_stop(); st['ende'] = True
mu.hook_add(UC_HOOK_MEM_READ | UC_HOOK_MEM_WRITE, ungerade)

def rd(adr, n): return bytes(mu.mem_read(adr, n))
def uc_stop():
    mu.emu_stop(); st['ende'] = True
def fcbname(adr):
    f = rd(adr, 12)
    n = f[1:9].decode().strip(); e = f[9:12].decode().strip()
    return n + ('.' + e if e else '')

def bios(fn, p1, p2):
    if not a.block:
        # NCRs CP/M-68K 1.2 kennt keine DMV-Funktionen und prueft die
        # Nummer nicht: jeder Aufruf hier waere dort ein Absturz
        print('BIOS %d ohne DMV-Block gerufen' % fn)
        uc_stop()
        return 0
    if fn == 64:
        return (0x44 << 24) | (1 << 16) | (0 if a.mono else 1) | (0 if a.nomaus else 2) | 8 | 16
    if fn == 65:
        gdc_flush(); sim.w_graf(p1 & 1); st['gr'] = p1 & 1; return 0
    if fn == 66:
        l = bytearray(); q = p2
        while True:
            n = rd(q, 1)[0]
            l += rd(q, n + 1); q += n + 1
            if n == 0: break
        gdc_flush(); sim.w_liste(bytes(l)); return 0
    if fn == 67:
        if p1 & 0xFFFF == 0:
            gdc['vs'] += 1; return 0x04 | (0x20 if (gdc['vs'] // 7) & 1 else 0)
        if p1 & 0xFFFF == 1: return 0
        m = struct.unpack('>H', rd(p2, 2))[0]; q = p2 + 2; l = bytearray()
        while True:
            n = rd(q, 1)[0]; l += rd(q, n + 1); q += n + 1
            if n == 0: break
        buf = ctypes.create_string_buffer(1024)
        gdc_flush(); sim.w_rdat(bytes(l), buf)
        mu.mem_write(q, buf.raw[:m]); return 0
    if fn == 68:
        if a.nomaus: return 1
        p = rd(p2, 1)[0]; q = p2 + 1 + p; an = rd(q, 1)[0]
        if p1 & 0xFF == 0:
            y = 399 - st['sy']
            mu.mem_write(q + 1, bytes([st['sx'] & 0xFF, st['sx'] >> 8, y & 0xFF, y >> 8, st['sb']])[:an])
        return 0
    if fn == 69: return 0
    return 0

def ausn(uc, intno, ud):
    print('Ausnahme %d bei %X' % (intno, uc.reg_read(UC_M68K_REG_PC))); uc.emu_stop(); st['ende'] = True

def trap(uc, adr, size, ud):
    if rd(adr, 2) != b'\x4e\x42':
        return
    uc.reg_write(UC_M68K_REG_PC, adr + 2)
    d0 = uc.reg_read(UC_M68K_REG_D0) & 0xFFFF
    d1 = uc.reg_read(UC_M68K_REG_D1)
    r = 0
    if d0 == 0:
        uc.emu_stop(); st['ende'] = True; return
    elif d0 == 2:
        out.append(chr(d1 & 0xFF))
    elif d0 == 6:                       # FEh fragen, FFh warten (wie DRs BDOS)
        if d1 & 0xFF == 0xFE:
            st['n2'] += 1
            if st['n2'] > a.max: uc.emu_stop(); st['ende'] = True; return
            st['taste'] = zeile()
            r = 0xFF if st['taste'] else 0
        elif d1 & 0xFF == 0xFF:
            r = st.get('taste') or 0
            while not r and not st['ende']:
                st['n2'] += 1
                if st['n2'] > a.max: uc.emu_stop(); st['ende'] = True; return
                r = zeile()
            st['taste'] = 0
    elif d0 == 26:
        st['dma'] = d1
    elif d0 == 15:
        try:
            datei[0] = open(os.path.join(os.getcwd(), fcbname(d1)), 'rb'); r = 0
        except OSError:
            r = 0xFF
    elif d0 == 20:
        b = datei[0].read(128) if datei[0] else b''
        if b:
            mu.mem_write(st['dma'], b.ljust(128, b'\x1a')); r = 0
        else:
            r = 1
    elif d0 == 16:
        if datei[0]: datei[0].close()
        r = 0
    elif d0 == 50:
        fn, p1, p2 = struct.unpack('>HII', rd(d1, 10))
        r = bios(fn, p1, p2)
    elif d0 == 62:
        uc.reg_write(UC_M68K_REG_SR, uc.reg_read(UC_M68K_REG_SR) | 0x2000)
    uc.reg_write(UC_M68K_REG_D0, r & 0xFFFFFFFF)
    uc.emu_stop()          # mit neuem PC weiter

mu.hook_add(UC_HOOK_INTR, ausn)
for i in range(0, len(text) - 1, 2):
    if text[i:i + 2] == b'\x4e\x42':
        mu.hook_add(UC_HOOK_CODE, trap, begin=BASE + i, end=BASE + i + 1)
sim.w_init(0 if a.mono else 1)
# ohne Block: Grafik schaltet das Programm selbst ein (PRAM ueber die
# Anschluesse); die Nachbildung zeichnet ab dem ersten Eintrag
if not a.block:
    sim.w_graf(1)
pc = BASE
n = 0
while not st['ende']:
    try:
        mu.emu_start(pc, 0x404, count=2000000)
    except UcError as e:
        print('Fehler', e, hex(mu.reg_read(UC_M68K_REG_PC))); break
    pc = mu.reg_read(UC_M68K_REG_PC)
    if pc in (0x400, 0x404): break
    n += 1
gdc_flush()
print(''.join(out)[-400:])
print('GDC-Eintraege ueber Anschluesse:', gdc['n'])

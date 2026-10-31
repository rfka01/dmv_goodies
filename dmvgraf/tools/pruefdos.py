#!/usr/bin/env python3
"""pruefdos.py - Pruefstand: eine DOS-EXE mit DMVGRAF (gcc-ia16, small)
unter einem nachgebildeten MS-DOS auf der DMV (8086 in Unicorn).

  pruefdos.py PROG.EXE skript.txt [--mono] [--nomaus] [--arg "1234"]

- INT 21h: 02h Zeichen, 06h DL=FFh Taste ohne Warten (je Aufruf eine
  Skriptzeile wie GHWSIM: maus x y k / taste c / bild datei.ppm),
  3Dh/3Fh/3Eh Dateien im aktuellen Verzeichnis, 48h Speicher, 4Ch Ende.
- 7220 an A0h/A1h/A2h ueber simlib.so (dieselbe Nachbildung wie
  GHWSIM.C, make simlib.so); K806 an 30h-33h, 8741 an 40h/41h.

Die Bilder lassen sich mit denen der PC-Simulation vergleichen (gleiches
Skript, gleiche Spielnummer). Braucht das Python-Paket unicorn (2.x).

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import argparse, ctypes, os, struct, sys
from unicorn import *
from unicorn.x86_const import *

HIER = os.path.dirname(os.path.abspath(__file__))
sim = ctypes.CDLL(os.path.join(HIER, 'simlib.so'))

ap = argparse.ArgumentParser()
ap.add_argument('prog'); ap.add_argument('skript')
ap.add_argument('--mono', action='store_true')
ap.add_argument('--arg', default='')
ap.add_argument('--nomaus', action='store_true')
ap.add_argument('--max', type=int, default=4000)
ap.add_argument('--speicher', type=int, default=2, help='freie 64-KB-Bloecke')
a = ap.parse_args()

mu = Uc(UC_ARCH_X86, UC_MODE_16)
mu.mem_map(0, 0x100000)
d = open(a.prog, 'rb').read()
(mag, cblp, cp, crlc, cpar, mina, maxa, ss, sp, cs_, ip, cs, lfa, ov) = \
    struct.unpack('<2sHHHHHHHHHHHHH', d[:28])
assert mag == b'MZ' and crlc == 0
gr = (cp - 1) * 512 + (cblp or 512) if cp else len(d)
bild = d[cpar * 16:gr]
PSP = 0x0FF0
LAD = PSP + 0x10
mu.mem_write(LAD * 16, bild)
tail = (' ' + a.arg.upper()) if a.arg else ''
psp = bytearray(0x100)
psp[0:2] = b'\xcd\x20'
psp[0x80] = len(tail); psp[0x81:0x81 + len(tail)] = tail.encode()
psp[0x81 + len(tail)] = 13
mu.mem_write(PSP * 16, bytes(psp))
frei = LAD + len(bild) // 16 + mina + 1
mu.reg_write(UC_X86_REG_CS, LAD + cs); mu.reg_write(UC_X86_REG_IP, ip)
mu.reg_write(UC_X86_REG_SS, LAD + ss); mu.reg_write(UC_X86_REG_SP, sp)
mu.reg_write(UC_X86_REG_DS, PSP); mu.reg_write(UC_X86_REG_ES, PSP)

skript = open(a.skript).read().splitlines()
st = dict(sx=320, sy=200, sb=0, ende=False, n2=0, frei=frei, bl=0)
dateien = {}
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

# ---- 7220: Eintraege sammeln, RDAT/CURD beantworten (wie pruef68.py) ----
gdc = dict(eintrag=bytearray(), lese=bytearray(), vs=0, n=0)
def gdc_flush():
    e = gdc['eintrag']
    if e:
        l = bytes([len(e)]) + bytes(e) + b'\0'
        if e[0] & 0xE4 == 0xA0 or e[0] == 0xE0:
            buf = ctypes.create_string_buffer(1024)
            nl = sim.w_rdat(l, buf)
            gdc['lese'] = bytearray(buf.raw[:nl])
        else:
            sim.w_liste(l)
        gdc['eintrag'] = bytearray()
        gdc['n'] += 1

maus = dict(aus=bytearray())
def maus_antwort(c):
    if c == 0:
        y = 399 - st['sy']
        maus['aus'] = bytearray([st['sx'] & 0xFF, st['sx'] >> 8, y & 0xFF, y >> 8, st['sb']])

def port_in(uc, port, size, ud):
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
    elif 0x30 <= p <= 0x33 and not a.nomaus:
        if p == 0x30:
            v = maus['aus'].pop(0) if maus['aus'] else 0
        elif p == 0x31:
            v = 1 if maus['aus'] else 0
        elif p == 0x33:
            v = 0x80
        else:
            v = 0
    return v

def port_out(uc, port, size, value, ud):
    p = port & 0xFF
    v = value & 0xFF
    if p == 0xA1:
        gdc_flush(); gdc['eintrag'] = bytearray([v])
        if v & 0xE4 == 0xA0 or v == 0xE0:
            gdc_flush()
    elif p in (0xA0, 0xA2):
        gdc['eintrag'].append(v)
    elif p == 0x31 and not a.nomaus:
        maus_antwort(v)

def R(r): return mu.reg_read(r)
def W(r, v): mu.reg_write(r, v & 0xFFFF)
def flag(bit, an):
    f = R(UC_X86_REG_EFLAGS)
    mu.reg_write(UC_X86_REG_EFLAGS, (f | (1 << bit)) if an else (f & ~(1 << bit)))
def stop():
    mu.emu_stop(); st['ende'] = True
def lin(seg, off): return (seg << 4) + off

def intr(uc, no, ud):
    if no != 0x21:
        print('INT %02Xh bei %04X:%04X' % (no, R(UC_X86_REG_CS), R(UC_X86_REG_IP)))
        stop(); return
    ax = R(UC_X86_REG_AX); ah = ax >> 8
    bx, cx, dx, ds = R(UC_X86_REG_BX), R(UC_X86_REG_CX), R(UC_X86_REG_DX), R(UC_X86_REG_DS)
    flag(0, 0)
    if ah == 0x4C:
        stop(); return
    elif ah == 0x02:
        out.append(chr(dx & 0xFF))
    elif ah == 0x06 and dx & 0xFF == 0xFF:
        st['n2'] += 1
        if st['n2'] > a.max:
            stop(); return
        t = zeile()
        W(UC_X86_REG_AX, (ax & 0xFF00) | t)
        flag(6, t == 0)
    elif ah == 0x3D:
        n = bytes(mu.mem_read(lin(ds, dx), 80)).split(b'\0')[0].decode()
        try:
            h = 5 + len(dateien); dateien[h] = open(n, 'rb'); W(UC_X86_REG_AX, h)
        except OSError:
            W(UC_X86_REG_AX, 2); flag(0, 1)
    elif ah == 0x3F:
        b = dateien[bx].read(cx) if bx in dateien else b''
        mu.mem_write(lin(ds, dx), b); W(UC_X86_REG_AX, len(b))
    elif ah == 0x3E:
        if bx in dateien:
            dateien.pop(bx).close()
    elif ah == 0x48:
        if st['bl'] < a.speicher and bx <= 0x1000:
            W(UC_X86_REG_AX, st['frei']); st['frei'] += 0x1000; st['bl'] += 1
        else:
            W(UC_X86_REG_AX, 8); W(UC_X86_REG_BX, 0); flag(0, 1)
    else:
        print('INT 21h AH=%02Xh nicht nachgebildet' % ah)
        stop()

mu.hook_add(UC_HOOK_INSN, port_in, None, 1, 0, UC_X86_INS_IN)
mu.hook_add(UC_HOOK_INSN, port_out, None, 1, 0, UC_X86_INS_OUT)
mu.hook_add(UC_HOOK_INTR, intr)
sim.w_init(0 if a.mono else 1)
sim.w_graf(1)       # Grafik schaltet das Programm selbst ein (PRAM)
while not st['ende']:
    try:
        mu.emu_start(lin(R(UC_X86_REG_CS), R(UC_X86_REG_IP)), 0x100000,
                     count=2000000)
    except UcError as e:
        print('Fehler', e, '%04X:%04X' % (R(UC_X86_REG_CS), R(UC_X86_REG_IP))); break
    if os.environ.get('GPROT'):
        print('PC %04X:%04X GDC %d Tasten %d' % (R(UC_X86_REG_CS), R(UC_X86_REG_IP), gdc['n'], st['n2']), flush=True)
gdc_flush()
print(''.join(out)[-400:])
print('GDC-Eintraege:', gdc['n'])

#!/usr/bin/env python3
"""tp3.py - Turbo Pascal 3.01A (MS-DOS) am PC ohne Bildschirm benutzen:
TURBO.COM laeuft in einem 8086 (Unicorn) mit nachgebildetem INT 21h, die
Tasten kommen aus einer Liste, die Bildschirmausgabe wird als Text
gezeigt.

  tp3.py VERZ PROG.PAS            PROG.PAS zu PROG.COM uebersetzen
  tp3.py VERZ "tasten" [--log]    beliebige Tasten

VERZ enthaelt TURBO.COM (und TURBO.MSG fuer Fehlertexte) und die
Quellen; TURBO.COM gehoert Borland und liegt nicht im Repository. Beim
Uebersetzen wird "mAximum free dynamic memory" auf 0400h Absaetze gesetzt
(16 KB Heap und Stapel), damit DOS Speicher fuer den Bildspeicher von
DMVGRAF behaelt. Tasten: Text; \\r Enter, \\e ESC, ^X Steuerzeichen.

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import os, sys, struct
from unicorn import *
from unicorn.x86_const import *

dirn = sys.argv[1]; keys = sys.argv[2]
if keys.upper().endswith('.PAS'):
    keys = 'YW%s\\rOCA0400\\rQC' % keys
LOG = '--log' in sys.argv
q = []
i = 0
while i < len(keys):
    c = keys[i]
    if c == '\\' and i + 1 < len(keys):
        n = keys[i+1]; q.append({'r': 13, 'e': 27, 'n': 10}.get(n, ord(n))); i += 2; continue
    if c == '^' and i + 1 < len(keys):
        q.append(ord(keys[i+1].upper()) - 64); i += 2; continue
    q.append(ord(c)); i += 1

mu = Uc(UC_ARCH_X86, UC_MODE_16)
mu.mem_map(0, 0x100000)
PSP = 0x1000
mu.mem_write(PSP*16 + 0x100, open(os.path.join(dirn, 'TURBO.COM'), 'rb').read())
psp = bytearray(256); psp[0:2] = b'\xcd\x20'; struct.pack_into('<H', psp, 2, 0x9FFF); psp[0x80] = 0; psp[0x81] = 13
mu.mem_write(PSP*16, bytes(psp))
for r in (UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS): mu.reg_write(r, PSP)
mu.reg_write(UC_X86_REG_SP, 0xFFFE); mu.reg_write(UC_X86_REG_IP, 0x100)
mu.mem_write(PSP*16 + 0xFFFE, b'\0\0')
out = bytearray()
st = dict(ende=False, dta=(PSP, 0x80), frei=0x3000, fh={}, nh=5, find=None)
def R(r): return mu.reg_read(r)
def W(r, v): mu.reg_write(r, v & 0xFFFF)
def lin(s, o): return (s << 4) + o
def flag(bit, an):
    f = R(UC_X86_REG_EFLAGS); mu.reg_write(UC_X86_REG_EFLAGS, (f | (1 << bit)) if an else (f & ~(1 << bit)))
def cstr(a): return bytes(mu.mem_read(a, 128)).split(b'\0')[0].decode('latin-1')
def pfad(n):
    n = n.strip()
    if len(n) > 1 and n[1] == ':': n = n[2:]
    n = n.lstrip('\\')
    # Gross-/Kleinschreibung egal
    for f in os.listdir(dirn):
        if f.upper() == n.upper(): return os.path.join(dirn, f)
    return os.path.join(dirn, n.upper())
def stop(): mu.emu_stop(); st['ende'] = True
def taste():
    if not q: stop(); return None
    return q.pop(0)
def fcbname(a):
    f = bytes(mu.mem_read(a, 12)); n = f[1:9].decode('latin-1').strip(); e = f[9:12].decode('latin-1').strip()
    return n + ('.' + e if e else '')
def intr(uc, no, ud):
    ax = R(UC_X86_REG_AX); ah = ax >> 8; al = ax & 0xFF
    bx, cx, dx, ds, es = R(UC_X86_REG_BX), R(UC_X86_REG_CX), R(UC_X86_REG_DX), R(UC_X86_REG_DS), R(UC_X86_REG_ES)
    if no == 0x20: stop(); return
    if no != 0x21:
        if LOG: print('INT %02X AX=%04X' % (no, ax))
        return
    flag(0, 0)
    if LOG and ah not in (2, 6, 0x0B, 0x3F, 0x40): print('21/%02X' % ah, hex(bx), hex(cx), hex(dx))
    if ah in (0x4C, 0x00): stop(); return
    elif ah == 0x02: out.append(dx & 0xFF)
    elif ah == 0x06:
        if dx & 0xFF == 0xFF:
            if q: W(UC_X86_REG_AX, (ax & 0xFF00) | q.pop(0)); flag(6, 0)
            else:
                st['leer'] = st.get('leer', 0) + 1
                if st['leer'] > 20000:
                    stop(); return
                W(UC_X86_REG_AX, ax & 0xFF00); flag(6, 1)
        else: out.append(dx & 0xFF)
    elif ah in (0x01, 0x07, 0x08):
        t = taste()
        if t is None: return
        W(UC_X86_REG_AX, (ax & 0xFF00) | t)
        if ah == 1: out.append(t)
    elif ah == 0x0B:
        if not q:
            st['leer'] = st.get('leer', 0) + 1
            if st['leer'] > 20000:
                stop(); return
        W(UC_X86_REG_AX, (ax & 0xFF00) | (0xFF if q else 0))
    elif ah == 0x09:
        a = lin(ds, dx)
        while True:
            c = mu.mem_read(a, 1)[0]
            if c == 0x24: break
            out.append(c); a += 1
    elif ah == 0x0A:
        a = lin(ds, dx); mx = mu.mem_read(a, 1)[0]; s = bytearray()
        while True:
            t = taste()
            if t is None: return
            if t == 13: break
            if t == 8:
                s = s[:-1]; continue
            if len(s) < mx - 1: s.append(t)
        mu.mem_write(a + 1, bytes([len(s)]) + bytes(s) + b'\r')
    elif ah == 0x0E: W(UC_X86_REG_AX, (ax & 0xFF00) | 3)
    elif ah == 0x19: W(UC_X86_REG_AX, ax & 0xFF00)
    elif ah == 0x1A: st['dta'] = (ds, dx)
    elif ah == 0x2A: W(UC_X86_REG_CX, 2026); W(UC_X86_REG_DX, 0x0B01)
    elif ah == 0x2C: W(UC_X86_REG_CX, 0x0C00); W(UC_X86_REG_DX, 0)
    elif ah == 0x30: W(UC_X86_REG_AX, 0x1E03)
    elif ah == 0x35: W(UC_X86_REG_BX, 0); mu.reg_write(UC_X86_REG_ES, 0)
    elif ah == 0x25: pass
    elif ah == 0x4A: pass
    elif ah == 0x48:
        W(UC_X86_REG_AX, st['frei']); st['frei'] += bx
    elif ah == 0x47: mu.mem_write(lin(ds, R(UC_X86_REG_SI)), b'\0')
    elif ah in (0x3C, 0x3D):
        n = cstr(lin(ds, dx)); p = pfad(n)
        try:
            f = open(p, 'w+b' if ah == 0x3C else ('r+b' if al & 3 else 'rb'))
            h = st['nh']; st['nh'] += 1; st['fh'][h] = f; W(UC_X86_REG_AX, h)
        except OSError:
            W(UC_X86_REG_AX, 2); flag(0, 1)
    elif ah == 0x3E:
        f = st['fh'].pop(bx, None)
        if f: f.close()
    elif ah == 0x3F:
        if bx == 0:
            t = taste()
            if t is None: return
            mu.mem_write(lin(ds, dx), bytes([t])); W(UC_X86_REG_AX, 1)
        else:
            b = st['fh'][bx].read(cx); mu.mem_write(lin(ds, dx), b); W(UC_X86_REG_AX, len(b))
    elif ah == 0x40:
        b = bytes(mu.mem_read(lin(ds, dx), cx))
        if bx in (1, 2): out.extend(b)
        elif bx in st['fh']:
            f = st['fh'][bx]
            if cx == 0: f.truncate()
            else: f.write(b)
        W(UC_X86_REG_AX, cx)
    elif ah == 0x41:
        try: os.remove(pfad(cstr(lin(ds, dx))))
        except OSError: W(UC_X86_REG_AX, 2); flag(0, 1)
    elif ah == 0x42:
        f = st['fh'][bx]; off = (cx << 16) | dx
        if off & 0x80000000: off -= 1 << 32
        f.seek(off, al); p = f.tell(); W(UC_X86_REG_AX, p & 0xFFFF); W(UC_X86_REG_DX, p >> 16)
    elif ah == 0x43: W(UC_X86_REG_CX, 0)
    elif ah == 0x44: W(UC_X86_REG_DX, 0x80 if bx < 3 else 0)
    elif ah == 0x56:
        try: os.rename(pfad(cstr(lin(ds, dx))), pfad(cstr(lin(es, R(UC_X86_REG_DI)))))
        except OSError: W(UC_X86_REG_AX, 2); flag(0, 1)
    elif ah in (0x4E, 0x4F):
        W(UC_X86_REG_AX, 18); flag(0, 1)
    elif ah in (0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x21, 0x22, 0x23, 0x24, 0x29):
        if LOG: print('FCB-Aufruf %02X' % ah, fcbname(lin(ds, dx)) if ah != 0x29 else '')
        if ah == 0x29:
            W(UC_X86_REG_AX, ax & 0xFF00)
        else:
            W(UC_X86_REG_AX, (ax & 0xFF00) | 0xFF)
    else:
        print('INT 21h AH=%02X nicht nachgebildet' % ah); stop()
mu.hook_add(UC_HOOK_INTR, intr)
n = 0
while not st['ende'] and n < 2000:
    try:
        mu.emu_start(lin(R(UC_X86_REG_CS), R(UC_X86_REG_IP)), 0x100000, count=5000000)
    except UcError as e:
        print('Fehler', e, '%04X:%04X' % (R(UC_X86_REG_CS), R(UC_X86_REG_IP))); break
    n += 1
s = out.decode('latin-1')
import re
s = re.sub(r'\x1b[=][\s\S][\s\S]', '\n', s)   # Cursorpositionierung ADM -> Zeilenwechsel
s = re.sub(r'\x1b.', '', s)
print(s)

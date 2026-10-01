#!/usr/bin/env python3
"""Test harness for ECODRIVE.COM: 8086 via Unicorn, a uPD7220 model on
ports A0h/A1h (MAME semantics, optional real-chip RDAT count, mono/colour
video RAM decoding as in the new dmv.cpp) and the DOS 3.3 tables the TSR
touches (List of Lists, DPB chain, CDS, device chain, buffers)."""
import random, struct, sys
from unicorn import *
from unicorn.x86_const import *

COM = open(sys.argv[1] if len(sys.argv) > 1 else 'ECODRIVE.COM', 'rb').read()

# ---------------------------------------------------------------- GDC model
class GDC:
    XD = [0, 1, 1, 1, 0, -1, -1, -1]
    def __init__(self, colour=True, real_rdat=False):
        self.colour, self.real_rdat = colour, real_rdat
        self.vram = [0] * 0xC000
        self.ead, self.mask, self.dir, self.dc = 0, 0xFFFF, 0, 0
        self.cmd, self.par, self.fifo = None, [], []
        self.log = []
    def rd(self, a):
        a &= 0xFFFF
        if not self.colour: return self.vram[a & 0x3FFF]
        return self.vram[a] if a < 0xC000 else 0xFFFF
    def wr(self, a, v):
        a &= 0xFFFF
        if not self.colour: self.vram[a & 0x3FFF] = v
        elif a < 0xC000: self.vram[a] = v
    def status(self):
        s = 0
        if self.fifo: s |= 1
        else: s |= 4
        return s
    def read_data(self):
        return self.fifo.pop(0) if self.fifo else 0
    def command(self, c):
        self.fifo = []                     # a command turns the FIFO around
        self.cmd, self.par = c, []
        self.log.append(c)
        if c == 0xE0:                      # CURD
            self.fifo = [self.ead & 255, (self.ead >> 8) & 255, (self.ead >> 16) & 3,
                         self.mask & 255, self.mask >> 8]
        elif (c & 0xE4) == 0xA0:           # RDAT word
            n = self.dc + 1 if self.real_rdat else self.dc
            for _ in range(n):
                w = self.rd(self.ead)
                self.fifo += [w & 255, w >> 8]
                self.ead = (self.ead + self.XD[self.dir]) & 0x3FFFF
            self.dc = 0
    def param(self, p):
        self.par.append(p)
        c, n = self.cmd, len(self.par)
        if c == 0x49:                      # CSRW
            if n >= 2: self.ead = self.par[0] | self.par[1] << 8
            if n == 3:
                self.ead |= (p & 3) << 16
                self.mask = 1 << (p >> 4)
        elif c == 0x4C:                    # FIGS
            if n == 1: self.dir = p & 7
            if n == 3: self.dc = self.par[1] | (self.par[2] & 0x3F) << 8
        elif c == 0x4A:                    # MASK
            if n == 2: self.mask = self.par[0] | self.par[1] << 8
        elif c is not None and (c & 0xE4) == 0x20:   # WDAT word
            if n == 2:
                d = (self.par[0] | self.par[1] << 8) & self.mask
                for _ in range(self.dc + 1):
                    cur = self.rd(self.ead)
                    self.wr(self.ead, (cur & ~self.mask & 0xFFFF) | d)
                    self.ead = (self.ead + self.XD[self.dir]) & 0x3FFFF
                self.dc, self.par = 0, []

# ---------------------------------------------------------------- machine
DOSSEG, LOL = 0x0080, 0x0026
BIOS = 0xF000
def lin(s, o): return (s << 4) + o

class Machine:
    def __init__(self, gdc):
        self.g = gdc
        self.u = Uc(UC_ARCH_X86, UC_MODE_16)
        self.u.mem_map(0, 0x100000)
        self.out, self.keys, self.exec_reached, self.freed = '', [], [], []
        self.on_exec, self.exec_fail = None, False
        self.stopped = None
        u = self.u
        # DOS stubs: INT 21h -> INT E1h (Python) ; IRET.  INT 2Fh -> IRET.
        u.mem_write(lin(BIOS, 0x100), b'\xCD\xE1\xCF')
        u.mem_write(lin(BIOS, 0x110), b'\xCF')
        self.setvec(0x21, BIOS, 0x100)
        self.setvec(0x2F, BIOS, 0x110)
        self.build_dos()
        u.hook_add(UC_HOOK_INTR, self.intr)
        u.hook_add(UC_HOOK_INSN, self.port_in, None, 1, 0, UC_X86_INS_IN)
        u.hook_add(UC_HOOK_INSN, self.port_out, None, 1, 0, UC_X86_INS_OUT)

    # helpers
    def w8(self, a, v): self.u.mem_write(a, bytes([v & 255]))
    def w16(self, a, v): self.u.mem_write(a, struct.pack('<H', v & 0xFFFF))
    def r8(self, a): return self.u.mem_read(a, 1)[0]
    def r16(self, a): return struct.unpack('<H', self.u.mem_read(a, 2))[0]
    def rfar(self, a): return self.r16(a + 2), self.r16(a)
    def setvec(self, n, s, o): self.w16(n * 4, o); self.w16(n * 4 + 2, s)
    def getvec(self, n): return self.r16(n * 4 + 2), self.r16(n * 4)
    def L(self, off): return lin(DOSSEG, LOL + off)

    def build_dos(self):
        D = lambda o: lin(DOSSEG, o)
        dpbs = [0x200, 0x230, 0x260]
        for i, o in enumerate(dpbs):
            self.w8(D(o), i)
            nxt = dpbs[i + 1] if i + 1 < 3 else None
            self.w16(D(o + 0x18), nxt if nxt else 0xFFFF)
            self.w16(D(o + 0x1A), DOSSEG if nxt else 0xFFFF)
        self.w16(self.L(0), 0x200); self.w16(self.L(2), DOSSEG)
        self.w16(self.L(0x10), 512)
        bufs = [0x400, 0x420, 0x440]
        for i, o in enumerate(bufs):
            self.w8(D(o + 4), [0, 2, 0xFF][i])
            nxt = bufs[i + 1] if i + 1 < 3 else 0xFFFF
            self.w16(D(o), nxt); self.w16(D(o + 2), DOSSEG)
        self.w16(self.L(0x12), 0x400); self.w16(self.L(0x14), DOSSEG)
        self.w16(self.L(0x16), 0x500); self.w16(self.L(0x18), DOSSEG)
        for d in range(5):
            self.w16(D(0x500 + d * 0x51 + 0x43), 0x4000 if d < 3 else 0)
        self.w8(self.L(0x20), 3); self.w8(self.L(0x21), 5)
        self.w16(self.L(0x22), 0x700); self.w16(self.L(0x24), DOSSEG)   # NUL -> CON
        self.w16(D(0x700), 0xFFFF); self.w16(D(0x702), 0xFFFF)

    # ports
    def port_in(self, u, port, size, data):
        if port == 0xA0: return self.g.status()
        if port == 0xA1: return self.g.read_data()
        raise Exception('IN %x' % port)
    def port_out(self, u, port, size, value, data):
        if port == 0xA0: self.g.param(value & 255)
        elif port == 0xA1: self.g.command(value & 255)
        else: raise Exception('OUT %x' % port)

    # fake DOS
    def setcf(self, cf):
        ss, sp = self.u.reg_read(UC_X86_REG_SS), self.u.reg_read(UC_X86_REG_SP)
        a = lin(ss, sp + 4)
        f = self.r16(a)
        self.w16(a, (f | 1) if cf else (f & ~1))
    def intr(self, u, intno, data):
        if intno != 0xE1:
            # real-mode INT n: push FLAGS, CS, IP and jump through the IVT
            if intno not in (0x21, 0x2F):
                raise Exception('INT %x at %x:%x' % (intno, u.reg_read(UC_X86_REG_CS), u.reg_read(UC_X86_REG_IP)))
            ss, sp = u.reg_read(UC_X86_REG_SS), u.reg_read(UC_X86_REG_SP)
            fl = u.reg_read(UC_X86_REG_EFLAGS) & 0xFFFF
            for v in (fl, u.reg_read(UC_X86_REG_CS), u.reg_read(UC_X86_REG_IP)):
                sp = (sp - 2) & 0xFFFF
                self.w16(lin(ss, sp), v)
            u.reg_write(UC_X86_REG_SP, sp)
            u.reg_write(UC_X86_REG_EFLAGS, fl & ~0x0300)
            s, o = self.getvec(intno)
            u.reg_write(UC_X86_REG_CS, s)
            u.reg_write(UC_X86_REG_IP, o)
            return
        ax = u.reg_read(UC_X86_REG_AX); ah, al = ax >> 8, ax & 255
        R = lambda r: u.reg_read(r)
        ds, dx = R(UC_X86_REG_DS), R(UC_X86_REG_DX)
        self.setcf(False)
        if ax == 0x3000: u.reg_write(UC_X86_REG_AX, 0x1E03)
        elif ax == 0x4452: u.reg_write(UC_X86_REG_AX, 1); self.setcf(True)
        elif ah == 0x52:
            u.reg_write(UC_X86_REG_ES, DOSSEG); u.reg_write(UC_X86_REG_BX, LOL)
        elif ah == 0x53: self.dpb_from_bpb()
        elif ah == 0x35:
            s, o = self.getvec(al)
            u.reg_write(UC_X86_REG_ES, s); u.reg_write(UC_X86_REG_BX, o)
        elif ah == 0x25: self.setvec(al, ds, dx)
        elif ah == 0x49: self.freed.append(R(UC_X86_REG_ES))
        elif ah == 0x09:
            a = lin(ds, dx); s = b''
            while self.r8(a) != 0x24: s += bytes([self.r8(a)]); a += 1
            self.out += s.decode('latin1')
        elif ah == 0x02: self.out += chr(dx & 255)
        elif ah == 0x08: u.reg_write(UC_X86_REG_AX, (ax & 0xFF00) | ord(self.keys.pop(0)))
        elif ah == 0x19: u.reg_write(UC_X86_REG_AX, (ax & 0xFF00) | 2)   # C:
        elif ah == 0x0D: pass
        elif ah == 0x2C: u.reg_write(UC_X86_REG_CX, 0x1415); u.reg_write(UC_X86_REG_DX, 0x2B00)   # 20:21:43
        elif ah == 0x2A: u.reg_write(UC_X86_REG_CX, 2026); u.reg_write(UC_X86_REG_DX, 0x0A01)     # 1.10.2026
        elif ax == 0x4B00:
            a = lin(ds, dx); s = b''
            while self.r8(a): s += bytes([self.r8(a)]); a += 1
            if self.exec_fail:
                u.reg_write(UC_X86_REG_AX, 2); self.setcf(True); return
            self.exec_reached.append(s.decode())
            if self.on_exec: self.on_exec()
            for r in (UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI,
                      UC_X86_REG_DI, UC_X86_REG_BP):                     # DOS leaves junk
                u.reg_write(r, 0xDEAD)
            u.reg_write(UC_X86_REG_DS, 0x1234); u.reg_write(UC_X86_REG_ES, 0x4321)
            u.reg_write(UC_X86_REG_AX, 0)
        elif ah in (0x31, 0x4C):
            self.stopped = (ah, al, dx); u.emu_stop()
        else: raise Exception('DOS %04x' % ax)
    def dpb_from_bpb(self):
        u = self.u
        b = lin(u.reg_read(UC_X86_REG_DS), u.reg_read(UC_X86_REG_SI))
        d = lin(u.reg_read(UC_X86_REG_ES), u.reg_read(UC_X86_REG_BP))
        bps, spc, res, nf, root, tot, med, spf = struct.unpack('<HBHBHHBH', bytes(u.mem_read(b, 13)))
        self.w16(d + 2, bps); self.w8(d + 4, spc - 1); self.w8(d + 5, spc.bit_length() - 1)
        self.w16(d + 6, res); self.w8(d + 8, nf); self.w16(d + 9, root)
        dirsec = res + nf * spf; data = dirsec + root * 32 // bps
        self.w16(d + 0x0B, data); self.w16(d + 0x0D, (tot - data) // spc + 1)
        self.w8(d + 0x0F, spf); self.w16(d + 0x10, dirsec); self.w8(d + 0x16, med)

    def run_com(self, seg, args=''):
        u = self.u
        u.mem_write(lin(seg, 0), b'\xCD\x20' + bytes(254))
        self.w16(lin(seg, 0x2C), 0x9000)
        cl = (' ' + args).encode() if args else b''
        self.w8(lin(seg, 0x80), len(cl)); u.mem_write(lin(seg, 0x81), cl + b'\r')
        u.mem_write(lin(seg, 0x100), COM)
        for r in (UC_X86_REG_CS, UC_X86_REG_DS, UC_X86_REG_ES, UC_X86_REG_SS):
            u.reg_write(r, seg)
        u.reg_write(UC_X86_REG_SP, 0xFFFE)
        self.out, self.stopped = '', None
        u.emu_start(lin(seg, 0x100), 0xFFFFFFFF, count=50_000_000)
        return self.stopped

    def call_far(self, seg, off, regs, stubseg=0x7000):
        """CALL FAR seg:off from a stub, return when it comes back."""
        u = self.u
        code = b'\x9A' + struct.pack('<HH', off, seg) + b'\x90'
        u.mem_write(lin(stubseg, 0), code)
        for r, v in regs.items(): u.reg_write(r, v)
        u.reg_write(UC_X86_REG_CS, stubseg); u.reg_write(UC_X86_REG_SS, 0x7800)
        u.reg_write(UC_X86_REG_SP, 0xFFF0)
        u.emu_start(lin(stubseg, 0), lin(stubseg, 5), count=50_000_000)

    def int_call(self, n, regs, stubseg=0x7100):
        u = self.u
        u.mem_write(lin(stubseg, 0), bytes([0xCD, n, 0x90]))
        for r, v in regs.items(): u.reg_write(r, v)
        u.reg_write(UC_X86_REG_CS, stubseg); u.reg_write(UC_X86_REG_SS, 0x7800)
        u.reg_write(UC_X86_REG_SP, 0xFFF0)
        u.emu_start(lin(stubseg, 0), lin(stubseg, 2), count=50_000_000)
        return {k: u.reg_read(k) for k in (UC_X86_REG_AX, UC_X86_REG_BX, UC_X86_REG_CX,
                                           UC_X86_REG_DX, UC_X86_REG_ES, UC_X86_REG_EFLAGS)}

    def request(self, rseg, cmd, start=0, count=0, buf=(0x6000, 0)):
        """Call the device driver of the TSR loaded at rseg."""
        hdr = lin(rseg, 0x103)
        strat, intr = self.r16(hdr + 6), self.r16(hdr + 8)
        rq = lin(0x7400, 0)
        self.u.mem_write(rq, bytes(32))
        self.w8(rq, 22); self.w8(rq + 1, 0); self.w8(rq + 2, cmd)
        self.w16(rq + 0x0E, buf[1]); self.w16(rq + 0x10, buf[0])
        self.w16(rq + 0x12, count); self.w16(rq + 0x14, start)
        self.call_far(rseg, strat, {UC_X86_REG_ES: 0x7400, UC_X86_REG_BX: 0})
        self.call_far(rseg, intr, {})
        return self.r16(rq + 3), self.r16(rq + 0x12), rq

# ---------------------------------------------------------------- tests
fails = 0
def check(cond, what):
    global fails
    print(('  ok   ' if cond else '  FAIL ') + what)
    if not cond: fails += 1

def scenario(real_rdat):
    print('== colour board, RDAT reads %s words' % ('DC+1' if real_rdat else 'DC (MAME)'))
    g = GDC(colour=True, real_rdat=real_rdat)
    for i in range(0x4000): g.vram[i] = 0x0E00 | (0x41 + i % 26)       # green text
    for i in range(0x4000, 0xC000): g.vram[i] = random.randrange(65536)  # garbage
    green = g.vram[:0x4000]
    g.ead, g.mask, g.dir = 0x0123, 0x0001, 2                            # console state
    m = Machine(g)
    R = 0x1000
    st = m.run_com(R)
    print('   ' + m.out.replace('\r\n', '\n   ').rstrip())
    check(st and st[0] == 0x31, 'stays resident')
    check('Laufwerk D:' in m.out, 'gets drive D:')
    check(g.vram[:0x4000] == green, 'green plane untouched')
    check((g.ead, g.mask) == (0x0123, 0x0001), 'console cursor/mask restored')
    check(m.r8(m.L(0x20)) == 4, 'block device count 3 -> 4')
    cds = lin(DOSSEG, 0x500 + 3 * 0x51)
    check(bytes(m.u.mem_read(cds, 4)) == b'D:\\\x00' and m.r16(cds + 0x43) == 0x4000, 'CDS D: filled')
    check(m.rfar(cds + 0x45) == (R, m.r16(lin(DOSSEG, 0x260 + 0x18))), 'CDS points at our DPB')
    dpb = lin(*m.rfar(lin(DOSSEG, 0x260 + 0x18)))
    check(m.r8(dpb) == 3 and m.r16(dpb + 0x0D) == 125 and m.r16(dpb + 0x0B) == 4, 'DPB: drive 3, 124 clusters, data at 4')
    check(m.rfar(dpb + 0x12) == (R, 0x103), 'DPB points at device header')
    check(m.rfar(m.L(0x22)) == (R, 0x103) and m.rfar(lin(R, 0x103)) == (DOSSEG, 0x700), 'device chain NUL -> ECODRIVE -> CON')
    check(m.getvec(0x21)[0] == R and m.getvec(0x2F)[0] == R, 'INT 21h/2Fh hooked')
    check(0x9000 in m.freed, 'environment freed')
    check(all(w == 0 for w in g.vram[0x4000 + 3 * 256:0xC000]), 'data sectors cleared')
    boot = bytes(sum(([w & 255, w >> 8] for w in g.vram[0x4000:0x4100]), []))
    check(boot[3:11] == b'ECODRIVE' and boot[510:512] == b'\x55\xAA', 'boot sector')
    fat = g.vram[0x4100]
    check(fat == 0xFFF8, 'FAT media bytes')
    root = bytes(sum(([w & 255, w >> 8] for w in g.vram[0x4200:0x4210]), []))
    check(root[:11] == b'ECODRIVE   ' and root[11] == 8, 'volume label')
    tm, dt = struct.unpack('<HH', root[22:26])
    check(tm == (20 << 11 | 21 << 5 | 21) and dt == (46 << 9 | 10 << 5 | 1), 'label dated 1.10.2026 20:21:42')

    # driver: media check, build BPB
    s, _, rq = m.request(R, 1)
    check(s == 0x0100 and m.r8(rq + 0x0E) == 1, 'media check: not changed')
    s, _, rq = m.request(R, 2)
    check(s == 0x0100 and m.rfar(rq + 0x12)[0] == R, 'build BPB')

    # write and read back
    data = bytes(random.randrange(256) for _ in range(8 * 512))
    m.u.mem_write(lin(0x6000, 0), data)
    g.ead, g.mask = 0x0456, 0x8000
    s, n, _ = m.request(R, 8, start=3, count=8)
    check(s == 0x0100 and n == 8, 'write 8 sectors')
    check((g.ead, g.mask) == (0x0456, 0x8000), 'console cursor/mask restored after write')
    w = g.vram[0x4000 + 3 * 256: 0x4000 + 11 * 256]
    check(bytes(sum(([x & 255, x >> 8] for x in w), [])) == data, 'data landed in the red plane')
    m.u.mem_write(lin(0x6000, 0), bytes(8 * 512))
    s, n, _ = m.request(R, 4, start=3, count=8)
    check(s == 0x0100 and n == 8 and bytes(m.u.mem_read(lin(0x6000, 0), 8 * 512)) == data, 'read 8 sectors back')
    check(g.vram[:0x4000] == green, 'green plane still untouched')

    # across the red/blue boundary and the last sector
    data2 = bytes(random.randrange(256) for _ in range(4 * 512))
    m.u.mem_write(lin(0x6000, 0), data2)
    s, n, _ = m.request(R, 9, start=62, count=4)
    check(s == 0x0100 and n == 4, 'write with verify across red/blue')
    m.u.mem_write(lin(0x6000, 0), bytes(4 * 512))
    s, n, _ = m.request(R, 4, start=62, count=4)
    check(bytes(m.u.mem_read(lin(0x6000, 0), 4 * 512)) == data2, 'read across red/blue')
    m.u.mem_write(lin(0x6000, 0), data2[:512])
    s, n, _ = m.request(R, 8, start=127, count=1)
    check(s == 0x0100 and g.vram[0xBF00] == data2[0] | data2[1] << 8, 'last sector at BF00h')
    s, n, _ = m.request(R, 4, start=127, count=2)
    check(s == 0x8108 and n == 0, 'beyond the end: sector not found')

    # damage by a "graphics program"
    g.vram[0x4000 + 5 * 256 + 17] ^= 0x0400
    s, n, _ = m.request(R, 4, start=3, count=8)
    check(s == 0x8104 and n == 2, 'damaged sector 5: CRC error after 2 good sectors')
    s, n, _ = m.request(R, 4, start=6, count=2)
    check(s == 0x0100, 'undamaged sectors still readable')
    g.vram[0x4000 + 5 * 256 + 17] ^= 0x0400                             # repair it again

    # INT 2Fh
    r = m.int_call(0x2F, {UC_X86_REG_AX: 0xEC00, UC_X86_REG_BX: 0})
    check(r[UC_X86_REG_AX] & 255 == 0xFF and r[UC_X86_REG_BX] == 0x4543 and r[UC_X86_REG_DX] == 6
          and r[UC_X86_REG_ES] == R, 'INT 2Fh EC00h')
    r = m.int_call(0x2F, {UC_X86_REG_AX: 0xEC01})
    check(r[UC_X86_REG_AX] & 255 == 3, 'INT 2Fh EC01h: drive D:')
    r = m.int_call(0x2F, {UC_X86_REG_AX: 0x1600})
    check(r[UC_X86_REG_AX] == 0x1600, 'other INT 2Fh calls passed on')

    # EXEC watch
    def exe(path, keys=''):
        m.u.mem_write(lin(0x7200, 0), path.encode() + b'\0')
        m.keys, m.exec_reached, m.out = list(keys), [], ''
        r = m.int_call(0x21, {UC_X86_REG_AX: 0x4B00, UC_X86_REG_DS: 0x7200, UC_X86_REG_DX: 0})
        return bool(m.exec_reached), r[UC_X86_REG_EFLAGS] & 1, r[UC_X86_REG_AX], m.out
    ran, cf, ax, out = exe('C:\\GEM\\GEMVDI.EXE', 'xn')
    check(not ran and cf and ax == 5 and 'J/N' in out, 'GEMVDI: N refuses with error 5')
    lines = out.split('\r\n')
    check(max(len(l) for l in lines) < 80 and 'zerst\x96ren.' in out, 'prompt wrapped below 80 columns, DMV oe (96h)')
    ran, cf, ax, out = exe('C:\\GEM\\GEMVDI.EXE', 'j')
    check(ran and not cf, 'GEMVDI: J starts it')
    check('besch' not in out, 'planes untouched: no damage report')
    s, n, rq = m.request(R, 1)
    check(m.r8(rq + 0x0E) == 1, 'media check still: not changed')
    ran, *_ = exe('a:dmvdem88.com', 'n')
    check(not ran, 'DMVDEM88 matches DMVDEM*')
    ran, cf, ax, out = exe('C:\\GEMX.EXE')
    check(ran and out == '', 'GEMX.EXE passes without asking')
    ran, cf, ax, out = exe('GEM.EXE', 'n')
    check(not ran, 'GEM.EXE without path asks')
    r = m.int_call(0x21, {UC_X86_REG_AX: 0x1900})
    check(r[UC_X86_REG_AX] & 255 == 2, 'other INT 21h calls passed on')

    # a failed EXEC is not checked
    m.exec_fail = True
    ran, cf, ax, out = exe('GEM.EXE', 'j')
    check(cf and ax == 2 and 'besch' not in out, 'failed EXEC: error passed back, no check')
    m.exec_fail = False

    # graphics program clears red and blue
    def wipe():
        for i in range(0x4000, 0xC000): g.vram[i] = 0
    m.on_exec = wipe
    g.ead, g.mask = 0x0777, 0x0010
    ran, cf, ax, out = exe('A:DMVDEMO.COM', 'j')
    m.on_exec = None
    print('   ' + out.strip().replace('\r\n', '\n   '))
    check(ran and not cf and ax == 0, 'DMVDEMO ran, EXEC result passed back')
    check('16 von 128 Sektoren besch\x90digt. Laufwerk D: gesperrt.' in out
          and 'ECODRIVE /F' in out, 'damage report: 16 sectors, drive D:')
    check(max(len(l) for l in out.split('\r\n')) < 80, 'report lines below 80 columns')
    check((g.ead, g.mask) == (0x0777, 0x0010), 'console cursor/mask restored after check')
    for k in range(2):
        s, n, rq = m.request(R, 1)
        check(m.r8(rq + 0x0E) == 0xFF, 'locked: media check says changed (%d)' % (k + 1))
    s, n, _ = m.request(R, 4, start=10, count=1)
    check(s == 0x8102 and n == 0, 'locked: read gives not ready')
    s, n, _ = m.request(R, 8, start=10, count=1)
    check(s == 0x8102, 'locked: write gives not ready')
    ran, cf, ax, out = exe('A:DMVDEMO.COM', 'j')
    check('besch' not in out, 'no second report while locked')

    # set it up again
    m.w8(lin(DOSSEG, 0x440 + 4), 3)                                     # a buffer of D:
    st = m.run_com(0x2800, '/F')
    print('   ' + m.out.strip())
    check(st[0] == 0x4C and st[1] == 0 and 'neu eingerichtet' in m.out and 'J/N' not in m.out,
          '/F after damage: no question, done')
    check(m.r8(lin(DOSSEG, 0x440 + 4)) == 0xFF, '/F: D: buffers dropped')
    s, n, rq = m.request(R, 1)
    check(m.r8(rq + 0x0E) == 0xFF, '/F: media check says changed once')
    s, n, rq = m.request(R, 1)
    check(m.r8(rq + 0x0E) == 1, '/F: then not changed')
    s, n, _ = m.request(R, 4, start=0, count=3)
    check(s == 0x0100, '/F: boot, FAT and directory readable')
    m.u.mem_write(lin(0x6000, 0), data[:512])
    s, n, _ = m.request(R, 9, start=40, count=1)
    s2, n, _ = m.request(R, 4, start=40, count=1)
    check(s == 0x0100 and s2 == 0x0100 and bytes(m.u.mem_read(lin(0x6000, 0), 512)) == data[:512],
          '/F: write and read work again')
    m.keys = list('xn')
    st = m.run_com(0x2800, '/F')
    check(st[0] == 0x4C and st[1] == 1 and 'J/N' in m.out and 'Abgebrochen' in m.out, '/F intact: N cancels')
    s, n, _ = m.request(R, 4, start=40, count=1)
    check(s == 0x0100 and bytes(m.u.mem_read(lin(0x6000, 0), 512)) == data[:512], '/F cancelled: data kept')
    m.keys = list('j')
    st = m.run_com(0x2800, '/F')
    check(st[0] == 0x4C and st[1] == 0 and 'neu eingerichtet' in m.out, '/F intact: J sets it up')
    m.request(R, 1)
    s, n, _ = m.request(R, 4, start=40, count=1)
    check(s == 0x0100 and bytes(m.u.mem_read(lin(0x6000, 0), 512)) == bytes(512), '/F: old data gone')

    # second load is refused
    st = m.run_com(0x3000)
    check(st[0] == 0x4C and 'bereits geladen' in m.out, 'second load refused')

    # unload: N cancels, J removes
    m.keys = list('n')
    st = m.run_com(0x2000, '/U')
    check(st[0] == 0x4C and st[1] == 1 and 'gehen beim Entfernen verloren' in m.out
          and 'Abgebrochen' in m.out, '/U: asks, N cancels')
    check(m.r8(m.L(0x20)) == 4 and m.getvec(0x21)[0] == R, '/U cancelled: still loaded')
    m.w8(lin(DOSSEG, 0x440 + 4), 3)                                     # a buffer of D:
    m.keys = list('j')
    st = m.run_com(0x2000, '/U')
    print('   ' + m.out.strip().replace('\r\n', '\n   '))
    check(st[0] == 0x4C and st[1] == 0, 'unload ok')
    check(m.r8(m.L(0x20)) == 3, 'block device count back to 3')
    check(m.r16(cds + 0x43) == 0, 'CDS D: cleared')
    check(m.r16(lin(DOSSEG, 0x260 + 0x18)) == 0xFFFF, 'DPB chain ends at C: again')
    check(m.rfar(m.L(0x22)) == (DOSSEG, 0x700), 'device chain NUL -> CON again')
    check(m.getvec(0x21) == (BIOS, 0x100) and m.getvec(0x2F) == (BIOS, 0x110), 'vectors restored')
    check(m.r8(lin(DOSSEG, 0x440 + 4)) == 0xFF and m.r8(lin(DOSSEG, 0x400 + 4)) == 0, 'D: buffers dropped, others kept')
    check(R in m.freed, 'resident memory freed')
    st = m.run_com(0x2000, '/U')
    check(st[0] == 0x4C and 'nicht geladen' in m.out, 'second unload: not loaded')
    st = m.run_com(R)
    check(st[0] == 0x31 and 'Laufwerk D:' in m.out, 'loads again after unload')

def mono():
    print('== monochrome board')
    g = GDC(colour=False)
    for i in range(0x4000): g.vram[i] = 0x0E00 | 0x41
    g.ead, g.mask = 0x0010, 0x0002
    m = Machine(g)
    st = m.run_com(0x1000)
    print('   ' + m.out.strip())
    check(st[0] == 0x4C and st[1] == 1 and 'Keine Farbgrafikkarte gefunden (nur 32KB Bildspeicher vorhanden).' in m.out, 'refused on mono')
    check(all(w == 0x0E41 for w in g.vram[:0x4000]), 'green plane (word 0) put back')
    check((g.ead, g.mask) == (0x0010, 0x0002), 'console cursor/mask restored')
    check(m.r8(m.L(0x20)) == 3 and m.getvec(0x21) == (BIOS, 0x100), 'DOS untouched')

def misc():
    print('== command line')
    g = GDC()
    m = Machine(g)
    st = m.run_com(0x1000, '/X:ws /x:Turbo*')
    check(st[0] == 0x31, '/X names accepted')
    m.u.mem_write(lin(0x7200, 0), b'B:WS.COM\0')
    m.keys, m.exec_reached = ['n'], []
    m.int_call(0x21, {UC_X86_REG_AX: 0x4B00, UC_X86_REG_DS: 0x7200, UC_X86_REG_DX: 0})
    check(not m.exec_reached, '/X:ws watches WS.COM')
    m.u.mem_write(lin(0x7200, 0), b'TURBO3.COM\0')
    m.keys, m.exec_reached = ['n'], []
    m.int_call(0x21, {UC_X86_REG_AX: 0x4B00, UC_X86_REG_DS: 0x7200, UC_X86_REG_DX: 0})
    check(not m.exec_reached, '/X:Turbo* watches TURBO3.COM')
    m2 = Machine(GDC())
    st = m2.run_com(0x1000, '/Q')
    check(st[0] == 0x4C and 'Parameter' in m2.out, 'bad switch refused')
    m3 = Machine(GDC())
    m3.w8(m3.L(0x21), 3)                                                 # LASTDRIVE=C
    st = m3.run_com(0x1000)
    check(st[0] == 0x4C and 'LASTDRIVE' in m3.out, 'no free drive letter')

def charset():
    print('== message characters')
    ok = {0x0D, 0x0A, 0x90, 0x96, 0x99, 0x80, 0x86, 0x89, 0x9E}
    import re
    bad = []
    for mo in re.finditer(rb"[\x20-\x7e\x80-\xff]{12,}\$", COM):
        for b in mo.group(0):
            if b >= 0x80 and b not in ok: bad.append((mo.group(0)[:30], hex(b)))
    check(not bad, 'messages use only ASCII and DMV umlauts %s' % bad[:3])

random.seed(1)
charset()
scenario(False)
scenario(True)
mono()
misc()
print('\n%d failure(s)' % fails)
sys.exit(1 if fails else 0)

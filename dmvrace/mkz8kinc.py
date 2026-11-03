#!/usr/bin/env python3
"""mkz8kinc.py SRC.8KN OUT.INC NAME  -  Z8001-Quelle mit libasm (asm -C Z8001)
uebersetzen und als db-Zeilen fuer MASM ausgeben (NAMELEN, NAMECODE)."""
import sys, subprocess, tempfile, os
src, out, name = sys.argv[1:4]
asm = os.environ.get('LIBASM_ASM', 'asm')
d = tempfile.mkdtemp(); h = os.path.join(d, 'a.hex')
r = subprocess.run([asm, '-C', 'Z8001', '-o', h, src], capture_output=True, text=True)
if r.returncode or r.stdout.strip() or r.stderr.strip():
    print(r.stdout, r.stderr); sys.exit(1)
mem = {}
for l in open(h):
    l = l.strip()
    if l.startswith(':') and int(l[7:9], 16) == 0:
        n = int(l[1:3], 16); a = int(l[3:7], 16)
        for i in range(n): mem[a + i] = int(l[9 + 2*i:11 + 2*i], 16)
data = bytes(mem.get(i, 0) for i in range(max(mem) + 1))
L = ['; %s - erzeugt von mkz8kinc.py aus %s (libasm, asm -C Z8001)' % (os.path.basename(out), os.path.basename(src)),
     '; Z8001-Code, gehoert nach P+100h (P = Koprozessor-Block)',
     '%sLEN  EQU     %d' % (name, len(data)), '%sCODE LABEL   BYTE' % name]
for i in range(0, len(data), 12):
    L.append('        db      ' + ','.join('0%02Xh' % c for c in data[i:i+12]))
open(out, 'w', newline='\r\n').write('\n'.join(L) + '\n')
print(out, len(data), 'Bytes')

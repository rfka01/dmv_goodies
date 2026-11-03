#!/usr/bin/env python3
"""mk68inc.py SRC.S OUT.INC NAME  -  68000-Quelle mit GNU as uebersetzen und
als db-Zeilen fuer MASM ausgeben (NAMELEN, NAMECODE)."""
import sys, subprocess, tempfile, os
src, out, name = sys.argv[1:4]
d = tempfile.mkdtemp()
o = os.path.join(d, 'a.o'); b = os.path.join(d, 'a.bin')
subprocess.check_call(['m68k-linux-gnu-as', '-m68000', '--register-prefix-optional', '-o', o, src])
subprocess.check_call(['m68k-linux-gnu-objcopy', '-O', 'binary', '-j', '.text', o, b])
data = open(b, 'rb').read()
L = ['; %s - erzeugt von mk68inc.py aus %s (m68k-linux-gnu-as -m68000)' % (os.path.basename(out), os.path.basename(src)),
     '; 68008-Code, gehoert nach P+100h (P = 68008-Block)',
     '%sLEN  EQU     %d' % (name, len(data)), '%sCODE LABEL   BYTE' % name]
for i in range(0, len(data), 12):
    L.append('        db      ' + ','.join('0%02Xh' % c for c in data[i:i+12]))
open(out, 'w', newline='\r\n').write('\n'.join(L) + '\n')
print(out, len(data), 'Bytes')

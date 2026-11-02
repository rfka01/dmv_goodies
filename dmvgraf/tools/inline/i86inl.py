"""i86inl.py DATEI.ASM SYM1,SYM2,... -> TP3-inline (8086), mit NASM. Die Symbole
sind globale Pascal-Variablen; sie werden zweimal mit verschiedenen
Platzhaltern uebersetzt, wo sich Worte unterscheiden, steht eine
Variablenadresse (>name+versatz).
Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT"""
import sys, subprocess, os, tempfile
src, syms = sys.argv[1], sys.argv[2].split(',')
def bau(base):
    d = tempfile.mkdtemp()
    hdr = 'cpu 8086\n' + ''.join('%%define %s 0x%04X\n' % (s, base + 0x40 * i) for i, s in enumerate(syms))
    a = os.path.join(d, 'a.asm'); open(a, 'w').write('bits 16\n' + hdr + open(src).read())
    subprocess.check_call(['nasm', '-f', 'bin', '-o', a + '.bin', '-l', a + '.lst', a])
    return open(a + '.bin', 'rb').read(), open(a + '.lst').read()
A, lst = bau(0x1000)
B, _ = bau(0x3011)
assert len(A) == len(B)
ref = {}
i = 0
while i < len(A):
    if A[i] != B[i]:
        v = A[i] | (A[i + 1] << 8); k = (v - 0x1000) // 0x40; o = (v - 0x1000) % 0x40
        ref[i] = '>%s' % syms[k] + ('+%d' % o if o else ''); i += 2
    else: i += 1
# Listing: Zeilen mit Adresse, Bytes, Quelltext
out = []
for l in lst.split('\n'):
    p = l.split(None, 3)
    if len(p) >= 3 and len(p[1]) == 8 and all(c in '0123456789ABCDEF' for c in p[1]):
        adr = int(p[1], 16); hx = p[2].replace('[', '').replace(']', '').replace('(', '').replace(')', '')
        n = len(hx) // 2; txt = p[3].strip() if len(p) > 3 else ''
        if ';' in txt and not txt.startswith(';'): pass
        toks = []; j = adr
        while j < adr + n:
            if j in ref: toks.append(ref[j]); j += 2
            else: toks.append('$%02X' % A[j]); j += 1
        out.append(['/'.join(toks), txt.split(';')[0].replace('<1>','').strip()])
    elif len(p) >= 2 and p[-1].endswith(':') and len(p) == 2:
        out.append([None, p[-1]])
# Fortsetzungszeilen der Listing (lange Bytefolgen) zusammenfuegen ignoriert: nasm bricht ab 10 Bytes um
res = ['  inline(']
idx = [k for k, x in enumerate(out) if x[0]]
for k, (t, c) in enumerate(out):
    if t is None: res.append('    { %s }' % c); continue
    res.append('    %-34s{ %s }' % (t + ('' if k == idx[-1] else '/'), c))
res.append('  );')
tot = sum(len(x[0].split('/')) + sum(1 for y in x[0].split('/') if y.startswith('>')) for x in out if x[0])
assert tot == len(A), (tot, len(A))
print('\n'.join(res))

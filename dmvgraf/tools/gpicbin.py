#!/usr/bin/env python3
"""gpicbin.py - den Kern von src/GPIC.INC (MASM) fuer die Bibliothek
uebersetzen: JWasm -Zm -bin mit GP_NURKERN, ab Versatz 0, und als
src/GPIC86.S (GNU as, .byte) mit dem Aufruf aus C ablegen. So gibt es eine
Quelle fuer DMVDEMO (MASM) und fuer DMVGRAF.

  gpicbin.py [--jwasm PFAD]       (Vorgabe: jwasm im Suchpfad oder $JWASM)

Nur noetig, wenn sich src/GPIC.INC aendert; src/GPIC86.S liegt fertig im
Repository, der Bau der Bibliothek braucht kein JWasm.

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
"""
import argparse, os, shutil, subprocess, sys, tempfile

HIER = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HIER, '..', 'src')

RAHMEN = '''; GPICB.ASM - nur der Kern von GPIC.INC, ab Versatz 0
GP_NURKERN EQU 1
_TEXT SEGMENT BYTE PUBLIC 'CODE'
        ASSUME CS:_TEXT
        ORG 0
INCLUDE GPIC.INC
_TEXT ENDS
        END
'''

KOPF = '''# GPIC86.S - gepackte Vollbilder (gpic, gpicm) fuer MS-DOS.
# ERZEUGT von tools/gpicbin.py aus src/GPIC.INC (JWasm, %d Byte) -
# nicht von Hand aendern, sondern GPIC.INC aendern und neu erzeugen.
#
#   int gpicw(char *quelle, int xw, int y, int art, int laenge, char *arbeit)
#   -> 0 oder Fehlernummer. arbeit: GP_GROESSE (5311) Byte.
#
# Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT

        .arch   i8086
        .code16
        .intel_syntax noprefix

        .section .text.gpicw,"ax"
        .global gpicw
gpicw:  push    bp
        mov     bp, sp
        push    si
        push    di
        push    es
        mov     dx, [bp+4]
        mov     bx, [bp+6]
        mov     cx, [bp+8]
        mov     ax, [bp+10]
        mov     si, [bp+12]
        mov     di, [bp+14]
        push    ds
        pop     es
        call    gp_picw
        jc      1f
        xor     ax, ax
1:      pop     es
        pop     di
        pop     si
        pop     bp
        ret

# Kern, lageunabhaengig (gp_picw)
gp_picw:
'''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--jwasm', default=os.environ.get('JWASM') or
                    shutil.which('jwasm') or 'jwasm')
    a = ap.parse_args()
    with tempfile.TemporaryDirectory() as t:
        shutil.copy(os.path.join(SRC, 'GPIC.INC'), t)
        open(os.path.join(t, 'GPICB.ASM'), 'w').write(RAHMEN)
        r = subprocess.run([a.jwasm, '-Zm', '-bin', 'GPICB.ASM'], cwd=t,
                           capture_output=True, text=True)
        b = os.path.join(t, 'GPICB.BIN')
        if r.returncode or not os.path.exists(b):
            sys.exit(r.stdout + r.stderr)
        k = open(b, 'rb').read()
    z = [KOPF % len(k)]
    for i in range(0, len(k), 12):
        z.append('        .byte   ' + ', '.join('0x%02X' % c for c in k[i:i + 12]) + '\n')
    open(os.path.join(SRC, 'GPIC86.S'), 'w').write(''.join(z))
    print('src/GPIC86.S: %d Byte Kern' % len(k))


main()

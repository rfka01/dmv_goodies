#!/usr/bin/env python3
"""gpicbin.py - den Kern von src/GPIC.INC (MASM) fuer die Bibliothek
uebersetzen: JWasm -Zm -bin mit GP_NURKERN, ab Versatz 0, und als
src/GPIC86.S (GNU as, .byte) mit dem Aufruf aus C ablegen. So gibt es eine
Quelle fuer DMVDEMO (MASM) und fuer DMVGRAF.

  gpicbin.py [--jwasm PFAD]       (Vorgabe: jwasm im Suchpfad oder $JWASM)

Schreibt ausserdem pascal/DMVPIC.INC: derselbe Kern als inline fuer
Turbo Pascal 3 unter MS-DOS.

Nur noetig, wenn sich src/GPIC.INC aendert; src/GPIC86.S und
pascal/DMVPIC.INC liegen fertig im Repository, der Bau braucht kein JWasm.

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


PASKOPF = """{ DMVPIC.INC - DMVGRAF fuer Turbo Pascal 3 unter MS-DOS: gepackte
  Vollbilder (.PIC, Kennung DMVP, LZSS), wie gpic/gpicm in C.
  ERZEUGT von tools/gpicbin.py aus src/GPIC.INC (Kern %d Byte) - nicht von
  Hand aendern. Nach DMVGRAF.INC und einer Bildfassung
  (DMVBILD.INC oder DMVBKL.INC) einbinden, nur mit DMVHWD.INC.

  gpic(name, xw, y, art)              Datei an Wortspalte xw (0-39), Zeile y
  gpicm(daten, laenge, xw, y, art)    dasselbe aus dem Speicher
  art: 1 Anzeige dunkel, 2 Mono, 4 Farbe (ohne 2/4: wie die Karte).
  gerr: 0, GE_DATEI, GE_KURZ, GE_FORM, GE_LAGE, GE_VERW (97), GE_HW.
  Braucht 5,3 KB Daten (Arbeitsbereich).

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

const GE_VERW = 97;
var hwPw: array[0..5311] of byte;      { GP_GROESSE: Fenster, Puffer }
    hwPn: array[0..80] of char;        { Dateiname ASCIIZ }
    hwPart, hwPxw, hwPy, hwPlen, hwPqo, hwPqs, hwPerg: integer;

procedure hwPic;                        { Kern rufen: Register aus hwP* }
begin
  inline(
    $A1/>hwPart/$8B/$1E/>hwPxw/$8B/$0E/>hwPy/$8B/$36/>hwPlen/
    $BF/>hwPw/$8B/$16/>hwPqo/
    $1E/$06/$1E/$07/$FF/$36/>hwPqs/$1F/
    $E8/$03/$00/$E9/$%02X/$%02X/
"""

PASFUSS = """/
    $07/$1F/$72/$02/$31/$C0/$A3/>hwPerg);
end;

procedure qpicw(art: integer);
begin
  gerr := qstart;
  if gerr <> 0 then exit;
  if (art and 6) = 0 then
    if qneb = 1 then art := art or 2 else art := art or 4;
  if qzeig then qzhid;
  if (art and 1) <> 0 then qcmd($0C);  { dunkel ueber die Liste, nicht im }
  qflush;                               { Kern (MAME, siehe README) }
  hwPart := art and $86;
  hwPic;
  hwWst := false;                       { MASK/FIGS hat der Kern gesetzt }
  if (art and 1) <> 0 then begin qcmd($6B); qcmd($0D); qflush; end;
  gerr := hwPerg;
end;

procedure gpic(name: GStr; xw, y, art: integer);
var i: integer;
begin
  for i := 1 to Length(name) do hwPn[i - 1] := name[i];
  hwPn[Length(name)] := #0;
  hwPxw := xw; hwPy := y; hwPlen := 0;
  hwPqo := Ofs(hwPn); hwPqs := Seg(hwPn);
  qpicw(art and 7);
end;

procedure gpicm(var daten; laenge, xw, y, art: integer);
begin
  hwPxw := xw; hwPy := y; hwPlen := laenge;
  hwPqo := Ofs(daten); hwPqs := Seg(daten);
  qpicw((art and 7) or $80);
end;
"""


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
    zeilen = []
    for i in range(0, len(k), 14):
        zeilen.append('    ' + '/'.join('$%02X' % c for c in k[i:i + 14]))
    pas = PASKOPF % (len(k), len(k) & 0xFF, len(k) >> 8) + '/\r\n'.join(zeilen) + PASFUSS
    open(os.path.join(SRC, '..', 'pascal', 'DMVPIC.INC'), 'w', newline='').write(
        pas.replace('\r\n', '\n').replace('\n', '\r\n'))
    print('pascal/DMVPIC.INC')


main()

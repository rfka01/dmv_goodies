#!/usr/bin/env python3
"""baudos.py - C-Programme mit DMVGRAF fuer MS-DOS auf der DMV uebersetzen
(gcc-ia16, Speichermodell small) und als EXE-Datei schreiben.

  baudos.py [-I dir] [-D NAME=WERT] -o PROG.EXE GCRTDOS.S|GCRTDOS.O quelle.c ... [x.o ...] [DMVGRAF.A]

GCRTDOS.S muss als erste Quelle stehen ('start' vorn, Abschnitt
.text.start). Gebunden wird mit einem eigenen Skript: Code ab 0 im
Codesegment, .rodata, .data und .bss ab 0 im Datensegment, das im
EXE-Kopf auf volle 64 KB gebracht wird (SS = Datensegment, SP = FFFEh;
der Stapel waechst vom Ende herab). DOS gibt dem Programm genau so viel
Speicher; den Rest holt DMVGRAF fuer den Bildspeicher mit INT 21h/48h.
Keine Verschiebungen: das Startstueck nimmt DS aus SS.

Mit libgcc von gcc-ia16 (32-Bit-Rechnen); sonst keine Bibliothek.

Fertig uebersetzte Objekte (.O) und Archive (.A) werden mitgebunden.
Endet -o auf .A, entsteht statt eines Programms ein Archiv aus den
Quellen (so entsteht lib/dos/DMVGRAF.A); auf .O ein einzelnes Objekt.
"""
import argparse
import os
import struct
import subprocess
import sys
import tempfile

PFAD = os.environ.get('IA16', '/opt/ia16/bin')
CC = os.path.join(PFAD, 'ia16-elf-gcc')
LD = os.path.join(PFAD, 'ia16-elf-ld')
OBJCOPY = os.path.join(PFAD, 'ia16-elf-objcopy')
NM = os.path.join(PFAD, 'ia16-elf-nm')
AR = os.path.join(PFAD, 'ia16-elf-ar')
CFLAGS = ['-march=i8086', '-mcmodel=small', '-Os', '-ffreestanding',
          '-fno-builtin', '-fno-common', '-std=gnu89', '-w']

SKRIPT = '''OUTPUT_FORMAT("elf32-i386")
ENTRY(start)
SECTIONS
{
  .text 0 : { *(.text.start) *(.text .text.*) }
  .data 0 : AT(ALIGN(LOADADDR(.text) + SIZEOF(.text), 16))
  { *(.rodata .rodata.*) *(.data .data.*) . = ALIGN(2); }
  .bss : { __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(2); _end = .; }
  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
'''


def bibliothek(ziel, objs, ar):
    """-o X.A: Archiv aus den Objekten, -o X.O: ein einzelnes Objekt"""
    import shutil
    if ziel.upper().endswith('.A'):
        if os.path.exists(ziel):
            os.remove(ziel)
        lauf([ar, 'rcsD', ziel] + objs)
        print('%s: %d Objekte' % (ziel, len(objs)))
        return True
    if ziel.upper().endswith('.O'):
        shutil.copy(objs[0], ziel)
        print(ziel)
        return True
    return False


def lauf(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode:
        sys.exit(' '.join(cmd) + '\n' + r.stderr)
    return r.stdout


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-I', action='append', default=[])
    ap.add_argument('-D', action='append', default=[])
    ap.add_argument('-o', required=True)
    ap.add_argument('--map', action='store_true')
    ap.add_argument('quellen', nargs='+')
    a = ap.parse_args()
    with tempfile.TemporaryDirectory() as tmp:
        objs = []
        for i, q in enumerate(a.quellen):
            if q.upper().endswith(('.O', '.A')):   # fertig uebersetzt
                objs.append(os.path.abspath(q))
                continue
            o = os.path.join(tmp, os.path.splitext(os.path.basename(q))[0]
                             .upper() + '.O')
            art = ['-x', 'assembler'] if q.upper().endswith('.S') \
                else ['-x', 'c']
            lauf([CC] + CFLAGS + ['-I' + d for d in a.I] +
                 ['-D' + d for d in a.D] + art + ['-c', q, '-o', o])
            objs.append(o)
        if bibliothek(a.o, objs, AR):
            return
        libgcc = lauf([CC] + CFLAGS + ['-print-libgcc-file-name']).strip()
        ld = os.path.join(tmp, 'p.ld')
        open(ld, 'w').write(SKRIPT)
        elf = os.path.join(tmp, 'p.elf')
        cmd = [LD, '-T', ld, '-o', elf] + objs + \
            ([libgcc] if os.path.isabs(libgcc) else [])
        if a.map:
            cmd += ['-Map', a.o + '.map']
        lauf(cmd)
        sym = {}
        for z in lauf([NM, elf]).splitlines():
            t = z.split()
            if len(t) == 3:
                sym[t[2]] = int(t[0], 16)
        tb = os.path.join(tmp, 't.bin')
        db = os.path.join(tmp, 'd.bin')
        lauf([OBJCOPY, '-O', 'binary', '-j', '.text', elf, tb])
        lauf([OBJCOPY, '-O', 'binary', '-j', '.data', elf, db])
        text = open(tb, 'rb').read()
        daten = open(db, 'rb').read()
    ende = sym['_end']
    if len(text) > 0x10000:
        sys.exit('Code groesser als 64 KB')
    if ende > 0xF000:
        sys.exit('Daten und BSS %d Byte: zu wenig Platz fuer den Stapel'
                 % ende)
    tpar = (len(text) + 15) // 16
    bild = text.ljust(tpar * 16, b'\0') + daten
    groesse = 32 + len(bild)
    noetig = tpar * 16 + 0x10000            # Datensegment ganz
    mina = (noetig - len(bild) + 15) // 16
    kopf = struct.pack('<2sHHHHHHHHHHHHH', b'MZ', groesse % 512,
                       (groesse + 511) // 512, 0, 2, mina, mina,
                       tpar, 0xFFFE, 0, sym['start'], 0, 0x1C, 0)
    kopf = kopf.ljust(32, b'\0')
    open(a.o, 'wb').write(kopf + bild)
    print('%s: Code %d, Daten %d, BSS %d, Stapel %d' %
          (a.o, len(text), len(daten), ende - len(daten), 0x10000 - ende))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""elf2cpm.py - ein mit GNU ld gebundenes 68000-Programm in eine
verschiebbare CP/M-68K-Programmdatei (.68K) umwandeln.

Abschrift aus rfka01/dmv_os (os/cpm/cpm68k/src/BAU/elf2cpm.py), fuer
gcc-Programme erweitert: .rodata kommt zu den Daten, Abschnitte mit
Punkt-Namen (-ffunction-sections u. ae.) werden mitgenommen, Archive
(libgcc.a) koennen hinter den Objekten stehen.

Das Programm wird zweimal gebunden, an Adresse 0 und an 10000h. Jedes
Langwort, das sich dabei genau um 10000h unterscheidet, ist eine
Adresse im Programm und bekommt Verschiebeinformation. Text und Daten
landen zusammen im Textsegment; BSS folgt, wie der CP/M-Lader es legt.

Verschiebewort je Programmwort (CP/M-68K Programmer's Guide):
    0 nicht verschieben, 2 textbezogen, 5 = erstes Wort eines Langworts,
    dessen zweites Wort die Art traegt.

    elf2cpm.py PROG.o AUSGABE.68K   (bindet selbst, braucht m68k-linux-gnu-ld)
"""
import os
import struct
import subprocess
import sys
import tempfile

LD = 'm68k-linux-gnu-ld'
OBJCOPY = 'm68k-linux-gnu-objcopy'
NM = 'm68k-linux-gnu-nm'
VERSATZ = 0x10000


def binde(objs, basis, tmp):
    elf = os.path.join(tmp, 'p%x.elf' % basis)
    skript = os.path.join(tmp, 'p.ld')
    with open(skript, 'w') as h:
        h.write('''OUTPUT_FORMAT("elf32-m68k")
ENTRY(start)
SECTIONS
{
    . = 0x%X;
    .text : { *(.text) *(.text.*) }
    .data : { *(.rodata) *(.rodata.*) *(.data) *(.data.*) }
    . = ALIGN(2);
    __tdend = .;
    .bss : { *(.bss) *(.bss.*) *(COMMON) . = ALIGN(2); }
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
    __bend = .;
}
''' % basis)
    r = subprocess.run([LD, '-T', skript, '-o', elf] + objs, capture_output=True, text=True)
    if r.returncode:
        sys.exit(r.stderr)
    b = os.path.join(tmp, 'p%x.bin' % basis)
    subprocess.run([OBJCOPY, '-O', 'binary', '-j', '.text', '-j', '.data', elf, b], check=True)
    sym = {}
    for z in subprocess.run([NM, elf], capture_output=True, text=True).stdout.splitlines():
        t = z.split()
        if len(t) == 3:
            sym[t[2]] = int(t[0], 16)
    return open(b, 'rb').read(), sym


def wandle(objs, ausgabe):
    with tempfile.TemporaryDirectory() as tmp:
        b0, s0 = binde(objs, 0, tmp)
        b1, s1 = binde(objs, VERSATZ, tmp)
    if s0.get('start') != 0:
        sys.exit('start muss am Anfang des Textes stehen')
    if len(b0) != len(b1):
        sys.exit('ungleiche Laenge')
    text = b0.ljust(s0['__tdend'], b'\0')
    if len(text) % 2:
        text += b'\0'
    t1 = b1.ljust(len(text), b'\0')
    bss = s0['__bend'] - s0['__tdend']
    rel = [0] * (len(text) // 2)
    i = 0
    while i + 4 <= len(text):
        a = struct.unpack('>I', text[i:i + 4])[0]
        b = struct.unpack('>I', t1[i:i + 4])[0]
        if b - a == VERSATZ:
            rel[i // 2] = 5
            rel[i // 2 + 1] = 2
            i += 4
            continue
        if text[i:i + 2] != t1[i:i + 2]:
            sys.exit('Wort bei %X aendert sich, ist aber kein Langwort' % i)
        i += 2
    kopf = struct.pack('>HIIIIIIH', 0x601A, len(text), 0, bss, 0, 0, 0, 0)
    with open(ausgabe, 'wb') as h:
        h.write(kopf + text + struct.pack('>%dH' % len(rel), *rel))
    return len(text), bss, sum(1 for r in rel if r == 5)


if __name__ == '__main__':
    print(wandle(sys.argv[1:-1], sys.argv[-1]))

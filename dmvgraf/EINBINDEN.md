# DMVGRAF aus verschiedenen Sprachen aufrufen

*English summary at the end.*

DMVGRAF ist in C geschrieben und liegt für drei Systeme fertig übersetzt in [`lib/`](lib/). Dieses Dokument beschreibt, wie ein Programm die Funktionen aufruft: aus C, aus Assembler und was für andere Sprachen gilt. Was die einzelnen Funktionen tun, steht in [`README.md`](README.md), die Liste mit Parametern in [`src/DMVGRAF.H`](src/DMVGRAF.H). Zu jeder Sprache gibt es in [`beispiele/`](beispiele/) dasselbe kleine Programm: Grafik ein, eine Linie, auf eine Taste warten, Grafik aus.

## Übersicht

| Sprache | MS-DOS | CP/M-68K | CP/M-8000 |
|---|---|---|---|
| C | gcc-ia16 | gcc (m68k) | DRs `zcc` |
| Assembler | GNU as (ia16), NASM (`-f elf32`) | GNU as (m68k) | DRs `asz8k` |
| Microsoft-Werkzeuge (MASM, MS-C, Turbo C, BASCOM) | nicht direkt (OMF), siehe unten | – | – |
| DRs C für CP/M-68K (Alcyon) | – | nicht direkt (16-Bit-`int`), siehe unten | – |
| Turbo Pascal 3 | nicht direkt, eigene Fassung geplant | – | – |

Geprüft im Prüfstand: die Beispiele in C und Assembler unter MS-DOS (gcc-ia16, GNU as, NASM) und CP/M-68K (gcc, GNU as). `DEMO.8KN` für CP/M-8000 ist mit `asz8k` assembliert und mit `ld8k` gebunden, aber noch nicht gelaufen.

## Was für alle Sprachen gilt

- **Einsprung:** Das Startstück (`GCRTDOS.O` bzw. `GCRT68.O`, unter CP/M-8000 DRs `STARTUP.O`) ruft `main(argc, argv)` auf. Ein Assemblerprogramm stellt also eine Funktion `main` bereit und kehrt mit `ret`/`rts` zurück. Unter MS-DOS wird der Rückgabewert zum Errorlevel.
- **Alle Funktionen liefern `int`**, 0 heißt gut, sonst ein Fehlercode (`GE_…` in `DMVGRAF.H`). Ausnahmen sind die Abfragen `gkey`, `mpos`, `grnd`: Sie liefern einen Wert.
- **Zeichenketten** enden mit einem Nullbyte (`gtext`, `gputs`, Dateinamen für `gbload`).
- **Rückgabe über Zeiger:** `ginfo`, `mpos`, `where` und `gbinfo` schreiben an Adressen, die das Programm übergibt. Unter MS-DOS sind es 16-Bit-Adressen im Datensegment des Programms.
- **Reihenfolge:** zuerst `ginit` (oder `ginfo`, das nur abfragt), am Ende `gexit`, sonst bleibt der Bildschirm im Grafikbetrieb.

## Aufrufkonventionen

| | MS-DOS (gcc-ia16, small) | CP/M-68K (gcc) | CP/M-8000 (zcc -m1) |
|---|---|---|---|
| Name im Objekt | wie in C (`ginit`) | wie in C (`ginit`) | mit Unterstrich (`_ginit`) |
| Argumente | auf dem Stapel, das letzte zuerst; das erste liegt bei `[sp+2]` nach dem `call` | auf dem Stapel, das letzte zuerst; das erste bei `4(sp)` nach dem `jsr` | auf dem Stapel, das letzte zuerst; das erste bei `2(r15)` nach dem `call` |
| Größe je Argument | 2 Byte (`int`, Zeiger) | **4 Byte** (`int` ist 32 Bit, Zeiger 32 Bit) | 2 Byte (`int`, Zeiger) |
| Stapel abräumen | der Aufrufer (`add sp, 2·n`) | der Aufrufer (`addq.l #4·n, sp` bzw. `lea`) | der Aufrufer (`inc r15, #2·n`) |
| Ergebnis | `AX` | `d0` | `r7` |
| bleibt erhalten | `SI`, `DI`, `BP`, `DS`, `SS`, `ES` | `d2`–`d7`, `a2`–`a6` | `r8`–`r14` |
| darf sich ändern | `AX`, `BX`, `CX`, `DX`, Flaggen | `d0`, `d1`, `a0`, `a1` | `r0`–`r7` |
| Vorbedingung | `DS` = `SS` = Datensegment, Richtungsflagge gelöscht | – | nicht segmentierter Normalmodus |

## C

Die Kopfdatei `src/DMVGRAF.H` einbinden und gegen die Bibliothek binden (Befehle auch in [`lib/README.md`](lib/README.md)):

    # MS-DOS
    python3 tools/baudos.py -I src -o DEMO.EXE lib/dos/GCRTDOS.O beispiele/DEMO.C lib/dos/DMVGRAF.A
    # CP/M-68K
    python3 tools/bau68.py -I src -o DEMO.68K lib/cpm68k/GCRT68.O beispiele/DEMO.C lib/cpm68k/DMVGRAF.A
    # CP/M-8000 (auf der DMV oder im Simulator)
    zcc -c -m1 demo.c
    ld8k -w -s -o demo.z8k startup.o demo.o dmvgraf.a -lcpm

DMVGRAF selbst ist K&R-C, ein Programm darf auch ANSI-C sein. `DMVGRAF.H` deklariert die Funktionen ohne Parameterliste; wer Prototypen will, schreibt sie sich nach den Kommentaren dort.

## Assembler

### MS-DOS: NASM oder GNU as

NASM erzeugt mit `-f elf32` Objekte, die der Binder von gcc-ia16 versteht. `cpu 8086` sorgt dafür, dass keine Befehle des 80186 hineinrutschen (etwa `push 50`). Beispiel [`beispiele/DEMO.ASM`](beispiele/DEMO.ASM), Kern davon:

            extern  ginit, gexit, gpoint, gline, gkey
    main:   call    ginit
            mov     ax, 50          ; gpoint(100, 50): Argumente von rechts
            push    ax
            mov     ax, 100
            push    ax
            call    gpoint
            add     sp, 4           ; der Aufrufer raeumt den Stapel ab

Übersetzen und binden:

    nasm -f elf32 beispiele/DEMO.ASM -o demo.o
    python3 tools/baudos.py -o DEMO.EXE lib/dos/GCRTDOS.O demo.o lib/dos/DMVGRAF.A

Mit GNU as (`ia16-elf-as`) geht es genauso (`.code16`, `.intel_syntax noprefix`, `.arch i8086`); `baudos.py` assembliert eine Quelle mit der Endung `.S` selbst.

Eigene Daten legt ein Assemblerprogramm in `section .data` bzw. `.bss`; sie landen im selben Datensegment wie die von DMVGRAF. `DS` darf sich zwischen zwei Aufrufen nicht ändern.

### CP/M-68K: GNU as

Jedes Argument ist ein Langwort. [`beispiele/DEMO68.S`](beispiele/DEMO68.S):

    main:   jsr     ginit
            move.l  #50,-(sp)       | gpoint(100, 50): jedes Argument ein Langwort
            move.l  #100,-(sp)
            jsr     gpoint
            addq.l  #8,sp

    python3 tools/bau68.py -o DEMO.68K lib/cpm68k/GCRT68.O beispiele/DEMO68.S lib/cpm68k/DMVGRAF.A

Der 68008 verträgt keine Wort- oder Langwortzugriffe auf ungerade Adressen; Daten, die DMVGRAF als `int` liest, müssen auf geraden Adressen liegen.

### CP/M-8000: asz8k

Namen mit Unterstrich, jeder fremde Name als `.global` angemeldet, Argumente als Worte. [`beispiele/DEMO.8KN`](beispiele/DEMO.8KN):

    _main:  call    _ginit
            push    @r15,#50        ;* gpoint(100, 50): Argumente von rechts
            push    @r15,#100
            call    _gpoint
            inc     r15,#4

    asz8k -o demo.o demo.8kn
    ld8k -w -s -o demo.z8k startup.o demo.o dmvgraf.a -lcpm

## Sprachen ohne direkten Weg

- **Microsoft-Werkzeuge unter DOS (MASM, MS-C, Turbo C, BASCOM, LINK):** Sie arbeiten mit OMF-Objekten; `lib/dos/DMVGRAF.A` ist ein ELF-Archiv für den Binder von gcc-ia16. Mit MASM geschriebene Teile lassen sich darum nicht dazubinden; NASM oder GNU as sind der Ersatz. Für eine OMF-Fassung müsste DMVGRAF mit einem Compiler übersetzt werden, der OMF schreibt (etwa OpenWatcom), und `GCRTDOS.S` müsste in dessen Konvention neu entstehen.
- **DRs C-Compiler für CP/M-68K (Alcyon):** Dort hat `int` 16 Bit, die Bibliothek erwartet 32 Bit je Argument. Ein Alcyon-Programm müsste jeden Aufruf über eine kleine Assembler-Zwischenstufe führen, die die Worte zu Langworten erweitert. Einfacher ist es, das Programm mit gcc zu übersetzen.
- **Turbo Pascal 3:** TP3 hat keinen Binder; `external` lädt nur eine Binärdatei an eine feste Stelle. Dafür ist eine eigene Pascal-Fassung von DMVGRAF geplant, als Include-Datei wie HOPPLERs `DMVGFX.INC`, mit denselben Namen.
- **Ein Weg für alle diese Sprachen** wäre ein residenter DMVGRAF-Treiber unter DOS, der über einen Software-Interrupt gerufen wird (wie NCRs GSX). Den gibt es noch nicht; sag Bescheid, wenn er gebraucht wird.

## English summary

DMVGRAF is written in C and ships ready-built in [`lib/`](lib/) for MS-DOS (gcc-ia16, small model), CP/M-68K (gcc) and CP/M-8000 (DR's zcc). [`beispiele/`](beispiele/) holds the same small program in C, NASM, GNU as (68000) and asz8k.

- **Entry:** the start-up module calls `main(argc, argv)`; all functions return `int` (0 = good).
- **Calling conventions:** MS-DOS: arguments on the stack, last first, 2 bytes each, caller pops, result in `AX`, `SI`/`DI`/`BP`/`DS`/`SS`/`ES` preserved, `DS = SS` required. CP/M-68K: 4 bytes per argument (32-bit `int`), result in `d0`, `d2`–`d7`/`a2`–`a6` preserved. CP/M-8000: names with a leading underscore, 2 bytes per argument from `2(r15)`, result in `r7`, `r8`–`r14` preserved.
- **Assembler:** NASM with `-f elf32` and `cpu 8086` or GNU as under MS-DOS, GNU as under CP/M-68K, asz8k under CP/M-8000.
- **Not directly:** Microsoft OMF tools (MASM, MS-C, Turbo C, BASCOM), DR's Alcyon C on CP/M-68K (16-bit `int`), and Turbo Pascal 3, which has no linker. A Pascal include version is planned.
- **Tested:** the C and assembler examples ran in the test benches under MS-DOS and CP/M-68K; `DEMO.8KN` assembles and links under CP/M-8000 but has not been run.

Code: Claude (Anthropic), idea and prompts: rfka01. Licence: MIT.

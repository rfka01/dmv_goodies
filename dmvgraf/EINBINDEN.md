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
| Turbo Pascal 3 | eigene Fassung in `pascal/` | – | – |

Geprüft im Prüfstand: die Beispiele in C und Assembler unter MS-DOS (gcc-ia16, GNU as, NASM; NASM auch im Speichermodell tiny) und CP/M-68K (gcc, GNU as). `DEMO.8KN` für CP/M-8000 ist mit `asz8k` assembliert und mit `ld8k` gebunden, aber noch nicht gelaufen.

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
    # MS-DOS, Speichermodell tiny (.COM)
    python3 tools/baudos.py --tiny -I src -o DEMO.COM lib/dos-tiny/GCRTCOM.O beispiele/DEMO.C lib/dos-tiny/DMVGRAF.A
    # CP/M-68K
    python3 tools/bau68.py -I src -o DEMO.68K lib/cpm68k/GCRT68.O beispiele/DEMO.C lib/cpm68k/DMVGRAF.A
    # CP/M-8000 (auf der DMV oder im Simulator)
    zcc -c -m1 demo.c
    ld8k -w -s -o demo.z8k startup.o demo.o dmvgraf.a -lcpm

Gepackte Vollbilder (`gpic`, `gpicm`) sind in `src/DMVPIC.H` vereinbart; wer sie braucht, bindet diese Datei zusätzlich ein:

    #include "DMVGRAF.H"
    #include "DMVPIC.H"
    ...
    if (gpic("TITEL.PIC", 0, 0, 1))         /* Anzeige dunkel, wie die Karte */
            gputs("TITEL.PIC fehlt\n");

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
    # oder tiny: Code, Daten und Stapel in einem Segment ab 100h
    python3 tools/baudos.py --tiny -o DEMO.COM lib/dos-tiny/GCRTCOM.O demo.o lib/dos-tiny/DMVGRAF.A

Im Speichermodell tiny gilt CS = DS = ES = SS; ein Programm, das sich selbst verschieben oder Briefkästen für Z80 und 68008 anlegen will, hat so alles in einem Segment. Der Binder nimmt in beiden Modellen nur die Funktionen, die gebraucht werden. Unter MS-DOS gibt es zusätzlich `ghinfo`, `ghcpu`, die Zeitbasis (`gtinit`, `gtus` …) und `gcorun` (siehe `README.md`); `ghcpu()` liefert das Ergebnis in AX und eignet sich für den Start eines Assemblerprogramms.

Mit GNU as (`ia16-elf-as`) geht es genauso (`.code16`, `.intel_syntax noprefix`, `.arch i8086`); `baudos.py` assembliert eine Quelle mit der Endung `.S` selbst.

Eigene Daten legt ein Assemblerprogramm in `section .data` bzw. `.bss`; sie landen im selben Datensegment wie die von DMVGRAF. `DS` darf sich zwischen zwei Aufrufen nicht ändern.

### MS-DOS: MASM 5.1 (gepackte Vollbilder)

Die Bibliothek selbst lässt sich nicht an MASM-Programme binden (siehe unten), die gepackten Vollbilder aber schon: `src/GPIC.INC` ist eigenständig (MASM 5.1, auch JWasm `-Zm`; alle Namen mit `gp_`, nur eigene Port-Konstanten). Beispiel und Prüfprogramm: [`test/GPT.ASM`](test/GPT.ASM).

            mov     dx, OFFSET name         ; DS:DX Dateiname (ASCIIZ)
            mov     al, 1 + 4               ; dunkel schreiben, Farbe
            xor     bx, bx                  ; xw = 0
            xor     cx, cx                  ; y = 0
            call    gp_pic                  ; CF = 1: AX = Fehlernummer
            ...
            INCLUDE GPIC.INC                ; am Ende des Codes (mit gp_puf, 5311 Byte)

`gp_picw` nimmt statt `gp_puf` einen Arbeitsbereich des Programms in ES:DI. Art Bit 7: Daten im Speicher (DS:DX, Länge in SI). Ohne Art-Bit 1 (mono) oder 2 (Farbe) gilt Farbe – das Programm kennt seine Karte selbst.

### MS-DOS: MASM 5.1 (Hardware erkennen, Zeitbasis, Koprozessor)

`src/GZEIT.INC` und `src/GHW.INC` sind dieselben Quellen, aus denen der Kern der DOS-Bibliothek entsteht (MASM 5.1, auch JWasm `-Zm`; Namen `gh_`/`gz_`, Konstanten `GH_`/`GHI_`/`GZ_`). `GZEIT.INC` zuerst einbinden, wenn `gh_takt`, `gh_bild`, `gh_uhr` oder `gh_info` gebraucht werden. DS muss auf die Daten zeigen (wie in einem .COM-Programm). Alle Register bleiben, Ergebnisse in AX, Fehler in CF. Beispiel und Prüfprogramm: [`test/GHWT.ASM`](test/GHWT.ASM).

            INCLUDE GZEIT.INC
            INCLUDE GHW.INC
            ...
            call    gh_cpuk                 ; AX = CPU | FPU << 2 | Karte << 4
            mov     ax, GH_MITCOPRO
            call    gh_info                 ; gh_tab wie ghinfo
            mov     ax, gh_tab[GHI_COPRO*2] ; 1 keiner, 2 K234, 3 K238, ...
            mov     ax, 0100h               ; deutsch, gleich ausgeben
            call    gh_zeile                ; "Erkannt: K235 mit V20, ..."

MASM unterscheidet Groß und Klein nicht; darum heißen die Indizes der Tabelle `GHI_ROM` … `GHI_UHR` (in C `GH_ROM` …), weil `gh_rom`, `gh_cpu` usw. die Prozeduren sind.

| Prozedur | Eingabe | Ergebnis |
|---|---|---|
| `gh_dmv` | – | AX 1, CF = 1: keine DMV |
| `gh_rom` | – | AL `C`/`M`, 0 und CF bei Programm oder Stapel unter 2000h |
| `gh_romv`, `gh_cpu`, `gh_87`, `gh_k235`, `gh_baenke`, `gh_spur` | – | AX wie die Spalte in `gh_tab` |
| `gh_cpuk` | – | AX wie `ghcpu` |
| `gh_schirm` | – | AX 1 Mono, 2 Farbe; nur im Grafikbetrieb |
| `gh_maus` | – | AX 30h (K806) oder 0 |
| `gh_takt`, `gh_bild`, `gh_uhr`, `gh_info` | `gh_info`: AX = `GH_MITCOPRO` \| `GH_MITUHR` | wie `ghinfo` |
| `gh_copro` | – | AX = `gh_tab[GHI_COPRO*2]` |
| `gh_coblk` | BX Absätze | AX Segment, CF = 1 keiner |
| `gh_vec` | AX Segment P | `gh_stz`: Startzustand (68008: SSP P+1000h, PC P+100h; Z8001 bei K238) |
| `gh_corun` | DS:SI Startzustand, CX Länge (≤ 40) | AX 0, 1 kein Weg, 2 Z80 nicht zurück (CF); `gh_res` Byte 0:0000, `gh_c0a`/`gh_c0b` Kanal 0 davor und danach |
| `gh_corz` | AX:SI Z80-Code, CX Länge, BX Segment für CX+8 Byte Sicherung | wie `gh_corun` |
| `gh_zhit` | AX Segment, CX Byte | CF = 1: liegt über dem Warteplatz F10Eh–F11Ch |
| `gh_zeile` | AL 0 deutsch, 1 englisch; AH 1: ausgeben | `gh_ztext` (`$`, dann 0) |
| `gz_lern` | – | Kanal 2 gelernt, Uhr 0; CF = 1: er stand (jetzt Modus 2) |
| `gz_us` | – | DX:AX µs seit `gz_lern`/`gz_start` |
| `gz_cal0` | AL 1: anstoßen, wenn er steht | AX µs je Takt Kanal 0 (`gz_c0per`, `gz_stoss`) |
| `gz_calv` | – | AX µs je Bild (`gz_vper`) |
| `gz_hz` | AX µs | AX in 1/100 Hz |

Messdauer der Eichung: `gz_dur` in Einheiten von 65 536 µs (Vorgabe 4). Rohwerte: `gh_spd`, `gh_aad`, `gh_sw`, `gh_cw`, `gh_romz`, `gz_n`, `gz_nmax`, `gz_snap`. Gegenüber `HWDET.INC`: `hcpu` → `gh_cpu`, `h87` → `gh_87`, `hpic` → `gh_k235`, `hprobe` → `gh_copro`, `hrunco` → `gh_corun`, `hvec68` → `gh_vec`, `hcoblk` → `gh_coblk`, `hzhit` → `gh_zhit`, `hwline` → `gh_zeile` mit AX = 0100h; aus `PITCAL.INC`: `c2learn` → `gz_lern`, `c2start`/`c2tick` → `gz_start`/`gz_tick`, `cal0` → `gz_cal0` (AL 0: ohne Anstoß), `calv` → `gz_calv`, `calhz` → `gz_hz`, `c_dur` → `gz_dur`. `caledge` setzt die Uhr nicht mehr zurück.

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

- **Microsoft-Werkzeuge unter DOS (MASM, MS-C, Turbo C, BASCOM, LINK):** Sie arbeiten mit OMF-Objekten (Ausnahme: die gepackten Vollbilder als `GPIC.INC`, siehe oben); `lib/dos/DMVGRAF.A` ist ein ELF-Archiv für den Binder von gcc-ia16. Mit MASM geschriebene Teile lassen sich darum nicht dazubinden; NASM oder GNU as sind der Ersatz. Für eine OMF-Fassung müsste DMVGRAF mit einem Compiler übersetzt werden, der OMF schreibt (etwa OpenWatcom), und `GCRTDOS.S` müsste in dessen Konvention neu entstehen.
- **DRs C-Compiler für CP/M-68K (Alcyon):** Dort hat `int` 16 Bit, die Bibliothek erwartet 32 Bit je Argument. Ein Alcyon-Programm müsste jeden Aufruf über eine kleine Assembler-Zwischenstufe führen, die die Worte zu Langworten erweitert. Einfacher ist es, das Programm mit gcc zu übersetzen.
- **Turbo Pascal 3:** TP3 hat keinen Binder; `external` lädt nur eine Binärdatei an eine feste Stelle. Dafür gibt es eine eigene Pascal-Fassung von DMVGRAF als Include-Dateien mit denselben Namen, siehe [`pascal/README.md`](pascal/README.md) (MS-DOS und CP/M-80; in Teilen einzubinden, die inneren Schleifen in Maschinensprache als `inline`).
- **Ein Weg für alle diese Sprachen** wäre ein residenter DMVGRAF-Treiber unter DOS, der über einen Software-Interrupt gerufen wird (wie NCRs GSX). Den gibt es noch nicht; sag Bescheid, wenn er gebraucht wird.

## English summary

DMVGRAF is written in C and ships ready-built in [`lib/`](lib/) for MS-DOS (gcc-ia16, small model), CP/M-68K (gcc) and CP/M-8000 (DR's zcc). [`beispiele/`](beispiele/) holds the same small program in C, NASM, GNU as (68000) and asz8k.

- **Entry:** the start-up module calls `main(argc, argv)`; all functions return `int` (0 = good).
- **Calling conventions:** MS-DOS: arguments on the stack, last first, 2 bytes each, caller pops, result in `AX`, `SI`/`DI`/`BP`/`DS`/`SS`/`ES` preserved, `DS = SS` required. CP/M-68K: 4 bytes per argument (32-bit `int`), result in `d0`, `d2`–`d7`/`a2`–`a6` preserved. CP/M-8000: names with a leading underscore, 2 bytes per argument from `2(r15)`, result in `r7`, `r8`–`r14` preserved.
- **Assembler:** NASM with `-f elf32` and `cpu 8086` or GNU as under MS-DOS, GNU as under CP/M-68K, asz8k under CP/M-8000.
- **Not directly:** Microsoft OMF tools (MASM, MS-C, Turbo C, BASCOM), DR's Alcyon C on CP/M-68K (16-bit `int`), and Turbo Pascal 3, which has no linker – for TP3 there is a Pascal include version in `pascal/`.
- **Hardware detection, time base, coprocessor:** for MASM programs `src/GZEIT.INC` and `src/GHW.INC`, the same sources as the library kernel; results in `gh_tab` in `ghinfo` order (indices `GHI_…`), registers preserved, CF on error. Example: `test/GHWT.ASM`.
- **Packed full-screen pictures** (`gpic`, `gpicm`): declared in `src/DMVPIC.H`. For MASM programs (DMVDEMO) the same 8086 kernel is a standalone include, `src/GPIC.INC` (MASM 5.1 / JWasm `-Zm`): DS:DX file name, AL art, BX xw, CX y, `call gp_pic`; CF/AX on error. Example: `test/GPT.ASM`.
- **Tested:** the C and assembler examples ran in the test benches under MS-DOS and CP/M-68K; `DEMO.8KN` assembles and links under CP/M-8000 but has not been run.

Code: Claude (Anthropic), idea and prompts: rfka01. Licence: MIT.

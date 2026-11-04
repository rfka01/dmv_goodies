# DMVGRAF

*Deutsch weiter unten.*

## English

A graphics, image, mouse and sound library for the **NCR Decision Mate V**, linked into the program like NCRGRAF (CP/M-80, MS-BASIC) and TurboGraf (Turbo Pascal 3). It uses the same names and parameters wherever NCRGRAF or TurboGraf already had a function, so old programs carry over. It is K&R C for **CP/M-8000** (K238 Z8001 card, with a Z8000 assembler kernel) **CP/M-68K** (K234 68008 card, compiled with gcc, also on NCR's own CP/M-68K 1.2 without the DMV BIOS block) and **MS-DOS** (8088/V20, compiled with gcc-ia16), with a simulation for the PC. Ready-made libraries for all three are in [`lib/`](lib/); [`EINBINDEN.md`](EINBINDEN.md) explains how to call them from C and assembler. For **Turbo Pascal 3** under MS-DOS and CP/M-80 there is a Pascal translation as include files in [`pascal/`](pascal/).

- Lines, rectangles, filled boxes with 16 patterns (own patterns, two colours, anchored to the screen so adjoining boxes match), circles and arcs drawn by the µPD7220 itself, in eight colours and four drawing modes
- Text with the 8 × 16 font from HOPPLER (with German umlauts), in eight directions, zoom 1–16
- Images in five formats (1 bit, 4-bit colour numbers as in Nels Anderson's TIL tile sets, DMV planes, SET/RESET sprites as in HOPPLER), stored outside the program in the free segments of the K208 (BIOS 72)
- Overlapping images composed off-screen and written in one go (`GLAYER`, `GLPUT`, `GLDRAW`), with a shaded variant for selections
- Mouse pointer with saved background (`GZEIG`): moving it reads and writes only the small box around old and new position
- Small figures ("sprites", 8 or 16 pixels, zoom 1–16) drawn and magnified by the µPD7220 itself (`gsprdef`, `gsprset` with mask in format 5, `gspr`)
- Mouse (K806), keyboard, frame wait, tone via the keyboard 8741
- Image collections of mixed sizes and formats in one file (`gbldv`, each image with its own 6-byte header; writer `tools/mkgbl.py`)
- Packed full-screen pictures (`.PIC`, LZSS, `gpic`, `gpicm`, header `DMVPIC.H`) written straight into the image store; the same 8086 kernel as a standalone MASM include (`src/GPIC.INC`) and a packer `tools/dmvpack.py`
- MS-DOS only: hardware report (`ghinfo`, `ghcpu`, `ghzeile`), time base from the 8253 (`gtinit`, `gtus`), running a code block on the coprocessor (`gcorun`, `gcoblk`, K235), and a tiny-model (.COM) library. Detection, coprocessor and time base are one 8086 kernel (`src/GHW.INC`, `src/GZEIT.INC`, MASM) shared with DMVDEMO and DMVRACE

Status 0.3 (2.11.2026): a fast image path like HOPPLER's `BlitAsm` (word-aligned, unzoomed images go straight from the image store to the 7220), sprites, patterns anchored to the screen and in two colours, hardware probes after the hardware detection guide (empty bus echoes its address, K806 via F1, screen probe that restores memory), MS-DOS hardware report, time base and coprocessor run, tiny model. Since then: masked format-5 sprites (`gsprset`), packed full-screen pictures (`gpic`, also from Turbo Pascal 3), image collections (`gbldv`), a C tone queue that no longer waits for the 8741. All screens unchanged or as intended in the test benches. Tested in MAME (3.11.2026, MS-DOS): `gpic`/`gpicm` from C, MASM and Pascal, `gbldv` from C and Pascal, the tone queue, `gcorun` with a K234 (ITEST K: the 68008 did the sum). On 4.11.2026: detection, coprocessor and time base as one MASM kernel (`GHW.INC`, `GZEIT.INC`) shared with DMVDEMO and DMVRACE, checked in the test bench for all hardware cases, not yet in MAME; horizontal text about 40 % faster (GTEST tested in MAME).

Status 0.2: CP/M-8000 tested in MAME with Mahjongg (rfka01/dmv_games). CP/M-68K in a test bench (`tools/pruef68.py`, 68000 in Unicorn): the same screens as the PC simulation, with and without the BIOS block, colour and mono; Mahjongg tested in MAME under our CP/M-68K 1.3 v005 and NCR's 1.2. MS-DOS: the same screens as the PC simulation in its test bench (`tools/pruefdos.py`, 8086 in Unicorn), Mahjongg tested in MAME under MS-DOS 3.30.

### Hardware detection, time base, coprocessor (MS-DOS)

One source for every 8086 program: `src/GHW.INC` and `src/GZEIT.INC` (MASM 5.1 / JWasm `-Zm`), included by DMVDEMO, DMVRACE and PITTEST and turned into the library kernel `src/GHW86.S` by `tools/gpicbin.py`; the C calls (`ghinfo`, `ghcpu`, `gtinit`, `gtus`, `gtbild`, `gt0per`, `gt0`, `gcorun`) are unchanged, new are `ghzeile`, `gcoblk` and `gteich`. Rules (also for the CP/M versions): DMV check first, read only; the empty bus echoes the low address byte; firmware letter at 0FF9h with interrupts off; CPU by `PUSH 0` and `AAD 16`, which must agree; 8087 by a 5A5Ah-preset control word; K235 by port 13h bit 1 and the 8259 mask register; the screen probe only in graphics mode (or with the write address EAD saved, as `ginit` does); channel 0 and frame rate calibrated separately and only on request (0.26 s each). Coprocessor only on a K235: the Z80 waits at F10Eh, starts the K234/K238 with `OUT DFh` (never D8h; the real K234 did not start with D8h), `DI` first, F10Eh–F11Ch and the overwritten vectors are restored; the bilingual 40-byte probe from DMVDIAG tells K234 (36h) from K238 (5Ah). The test bench emulates 8088/V20, 8087, K235/K230, no coprocessor, K234 (a real 68008 run in Unicorn), K238 (probe only), a Z80 that does not come back and a stopped channel 0; C and MASM give the same table.

### Packed full-screen pictures (.PIC)

`gpic(file, xw, y, art)` loads a `.PIC` file straight into the image store (`gpicm(data, length, xw, y, art)` from memory); `xw` is the left column in words (0–39), `y` the top line; `art` bit 0 blanks the display while writing, bit 1 forces mono, bit 2 colour. Declared in `src/DMVPIC.H`. MS-DOS uses an 8086 kernel that streams each byte to the 7220 (waits for an empty FIFO, then 16 bytes unchecked); the other systems use the same logic in C. For MASM programs such as DMVDEMO the kernel is `src/GPIC.INC` (MASM 5.1 / JWasm `-Zm`, all names `gp_`; DS:DX file name, AL art, BX xw, CX y; CF/AX error). `tools/dmvpack.py` packs raw 128 000/32 000-byte files or 8-colour PNGs and checks its output. File format: see "Dateiformat" below; KITT (mono plane only) packs from 32 000 to 12 231 bytes.

## Deutsch

Eine Bibliothek für Grafik, Bilder, Maus und Klang auf der **NCR Decision Mate V**, die ins Programm gebunden wird – wie NCRGRAF (CP/M-80, MS-BASIC) und TurboGraf (Turbo Pascal 3). Was es dort schon gibt, heißt und verhält sich wie dort; so laufen alte Programme mit wenig Aufwand weiter. Sie ist K&R-C für **CP/M-8000** (Z8001-Karte K238, mit einem Kern in Z8000-Assembler) **CP/M-68K** (68008-Karte K234, mit gcc übersetzt, auch auf NCRs eigenem CP/M-68K 1.2 ohne DMV-BIOS-Block) und **MS-DOS** (8088/V20, mit gcc-ia16 übersetzt), dazu eine Simulation für den PC. Fertig übersetzte Bibliotheken für alle drei liegen in [`lib/`](lib/); wie man sie aus C und Assembler aufruft, steht in [`EINBINDEN.md`](EINBINDEN.md). Für **Turbo Pascal 3** unter MS-DOS und CP/M-80 gibt es eine Übersetzung nach Pascal als Include-Dateien in [`pascal/`](pascal/).

**Stand 0.3** (2.11.2026): schneller Weg für Bilder wie HOPPLERs `BlitAsm`, Figuren über GCHRD (`gsprdef`, `gspr`), Muster am Schirm verankert, zweifarbig und selbst festgelegt (`gpdef`, `gbcol`), Hardwareproben nach der Anleitung „Hardware erkennen“ (Echo des leeren Busses, K806 über F1, Bildschirmprobe mit Wiederherstellen, DMV-Prüfung vor `ginit`), unter MS-DOS Hardwareauskunft (`ghinfo`, `ghcpu`), Zeitbasis (`gtinit` …), Koprozessor (`gcorun`) und eine Bibliothek im Speichermodell tiny. Danach: Figuren im Format 5 mit Maske (`gsprset`), Anzahl der Figuren frei, `gbload` auch für Format 5, gepackte Vollbilder (`gpic`, auch aus Turbo Pascal 3), Bildsammlungen (`gbldv`), Tonwarteschlange in C ohne Warten auf den 8741. In den Prüfständen dieselben Bilder wie die PC-Simulation. In MAME getestet (3.11.2026, MS-DOS): `gpic`/`gpicm` in C, MASM und Pascal, `gbldv` in C und Pascal (`DMVBILD.INC`, `DMVBKL.INC`), die Tonwarteschlange, `gcorun` mit K234 (ITEST K: der 68008 hat gerechnet). Am 4.11.2026: Erkennung, Koprozessor und Zeitbasis als gemeinsamer MASM-Kern (`GHW.INC`, `GZEIT.INC`) für DMVGRAF, DMVDEMO und DMVRACE, im Prüfstand mit allen Hardwarefällen geprüft, in MAME noch nicht; waagrechter Text rund 40 % schneller (GTEST in MAME getestet).

**Stand 0.2** (1.11.2026): CP/M-8000 mit Mahjongg (rfka01/dmv_games) in MAME getestet, direkt am 7220 und über BIOS 66/67; der Z8000-Kern erzeugt dieselben GDC-Listen wie die C-Fassung. CP/M-68K im Prüfstand (`tools/pruef68.py`, 68000 in Unicorn): dieselben Bilder wie die PC-Simulation, mit und ohne BIOS-Block, Farbe und mono. Mahjongg in MAME getestet unter unserem CP/M-68K 1.3 v005 und unter NCRs 1.2. MS-DOS im Prüfstand (`tools/pruefdos.py`, 8086 in Unicorn) wie die PC-Simulation, Mahjongg in MAME unter MS-DOS 3.30 getestet.

![Figuren](screenshots/figuren.png)
![Text](screenshots/text.png)
![Bilder](screenshots/bilder.png)
![Figuren über GCHRD](screenshots/figuren-gchrd.png)
![Muster](screenshots/muster.png)

*Bilder aus der PC-Simulation (`test/GTEST.C`, `test/STEST.C`, `test/PTEST.C`).*

### Voraussetzungen unter CP/M-8000

- CP/M-8000 mit dem DMV-BIOS-Block (Funktionen 64–69, `docs/DMV-Schnittstellen.md` in rfka01/dmv_os, Abschnitt 9).
- Für Bilder: **BIOS 72** (freie Segmente, ab CP/M-8000 v015) und eine K208 (512 KB, Segmente 4–6). Ohne sie gibt `GBDEF`/`GBLOAD` den Fehler 91 (kein Bildspeicher); Programme, die nur zeichnen, laufen trotzdem. Wer keine K208 hat, bindet statt `GHSEG.C` die Datei `GHPOOL.C` (Bildspeicher als Feld im Programm, Größe dort einstellen).

### Voraussetzungen unter CP/M-68K

- 68008-Karte K234 (die Anschlüsse liegen beim 68008 auf FFF00h + Port), 256 KB reichen.
- Mit DMV-BIOS-Block (unser CP/M-68K, sobald es 64–69 hat): Maus und Ton über den BIOS, der 7220 direkt (Rückfall `gdirekt(0)`: BIOS 66/67).
- Ohne Block (NCRs CP/M-68K 1.2, unser CP/M-68K bis v003): DMVGRAF macht alles selbst wie der BIOS unter CP/M-8000 – Grafikbetrieb ein und aus (danach löscht die Konsole mit ^Z den Schirm), Farbe erkennt es an der Spiegelung des Bildspeichers der Monochromkarte, die K806 auf 30h nach der Anleitung „Hardware erkennen“ (siehe Hardware), Ton über den 8741 wie BIOS 69. Die Tastatur bleibt bei BDOS 6.
- Auf die Anschlüsse geht DMVGRAF im Supervisorzustand (BDOS 62, `GSUP68.S`), weil nicht gemessen ist, ob die K234 sie im Benutzerzustand durchlässt.
- Bildspeicher: `GHPOOL.C` mit `-DGBPOOL=61440` (60 KB im Programm).

### Voraussetzungen unter MS-DOS

- 8088/V20 der K230/K231/K235, DOS ab 2.11. Das Programm spricht 7220 (A0h–A2h), K806 (30h) und den Tastatur-8741 (40h/41h) selbst an, mit denselben Abläufen wie unter NCRs CP/M-68K; Grafik ein und aus wie NCRs GRAPH-Bibliothek unter DOS (danach löscht die Konsole mit ^Z den Schirm). Tastatur, Text und Dateien über INT 21h.
- Speichermodell small (64 KB Code, 64 KB Daten und Stapel, `.EXE`) oder tiny (alles in 64 KB, `.COM`, `lib/dos-tiny/`); Bilder in Speicherblöcken von DOS (INT 21h/48h, bis zu zwei zu 64 KB). Der Binder lässt ungebrauchte Funktionen weg.
- Vor dem Einschalten prüft DMVGRAF nur lesend, ob es eine DMV ist (Statusbits des 7220 wechseln, nie FFh); sonst liefert `ginit` den Fehler 99.

### Dateien

| Datei | Inhalt |
|---|---|
| `src/DMVGRAF.H` | Schnittstelle für Programme |
| `src/GRAF.C` | Zustand, GDC-Liste, Figuren, Text |
| `src/GBILD.C` | Bilder und Zusammensetzen |
| `src/GEIN.C` | Maus, Tastatur, Zeit, Auskunft, Ton |
| `src/GKERN.C` | Kern in C (Simulation, CP/M-68K) |
| `src/GZ8K.8KN` | derselbe Kern in Z8000-Assembler (CP/M-8000) |
| `src/GHWCPM.C` | Hardwareschicht CP/M-8000/CP/M-68K: BIOS-Block über BDOS 50, Tastatur über BDOS 6, Dateien über FCB |
| `src/GHSEG.C`, `src/GSC1.8KN` | Bildspeicher in freien Segmenten (BIOS 72), Kopie über DRs System Call 1 |
| `src/GHPOOL.C` | Bildspeicher als Feld im Programm |
| `src/GHDIR.C`, `src/GDIR.8KN` | CP/M-8000: GDC-Listen direkt an den 7220 (Systemmodus über BDOS 62, zurück mit `LDCTL FCW`) |
| `src/GHD68K.C`, `src/GSUP68.S` | CP/M-68K: 7220 direkt, ohne BIOS-Block auch Grafikbetrieb, Farbe, K806 und Ton; Supervisorzustand über BDOS 62 |
| `src/GCRT68.S`, `src/GARG68.C` | CP/M-68K: Startstück, BDOS-Aufruf, Kommandozeile, 32-Bit-Rechnen (statt libgcc, die 68020-Befehle enthält) |
| `src/GHWDOS.C`, `src/GCRTDOS.S` | MS-DOS: Hardwareschicht (7220, K806, 8741 direkt, Tastatur/Text/Dateien über INT 21h, Bildspeicher von DOS); Startstück (small, mit `-DTINY` tiny), Anschlüsse, Kopieren in fremde Segmente, GDC-Liste, Proben (Firmware, Prozessor, 8087, 8253) |
| `src/GHW.INC`, `src/GZEIT.INC`, `src/GHW86.S` | nur MS-DOS: Erkennung, Koprozessor und Zeitbasis in MASM, gemeinsam mit DMVDEMO/DMVRACE; `GHW86.S` ist der daraus erzeugte Kern der Bibliothek |
| `src/GINFO.C`, `src/GZEIT.C` | nur MS-DOS: C-Aufrufe dazu (`ghinfo`, `gcorun`, `gtinit` …) |
| `test/KTEST.C`, `test/GHWT.ASM`, `test/ITEST.C` | Koprozessor und Hardwaretabelle aus C und MASM (Vergleich), Hardwareauskunft |
| `src/GHDNUL.C` | andere Systeme: kein direkter Weg, alles über BIOS 66/67 |
| `src/GHWSIM.C` | Hardwareschicht für die Simulation am PC |
| `src/GINTERN.H`, `src/GFONT.H` | intern; Schrift aus HOPPLER (`tools/mkfont.py`) |
| `src/GKONF.H` | Bauoptionen, normal leer; `#define GKLEIN` lässt Figuren, Format 5, Bildsammlungen, `gbget` und den DOS-Schnellweg weg (für knappe CP/M-8000-Programme) |
| `test/GTEST.C` | Prüf- und Vorführprogramm (drei Bilder, danach Mauszeiger) |
| `test/STEST.C`, `test/PTEST.C`, `test/BTEST.C`, `test/ITEST.C` | Prüfprogramme: Figuren über GCHRD, Muster, schneller Weg der Bilder, Hardwareauskunft (nur MS-DOS) |
| `tools/inline/` | Quellen der Maschinensprache in der Pascal-Fassung (Z80 und 8086) |
| `tools/bau68.py`, `tools/elf2cpm.py` | CP/M-68K: mit `m68k-linux-gnu-gcc` übersetzen, `.68K` erzeugen (`elf2cpm.py` aus rfka01/dmv_os, um `.rodata` erweitert) |
| `tools/pruef68.py`, `tools/simlib.c` | Prüfstand für `.68K`-Programme (Unicorn, GDC-Nachbildung aus `GHWSIM.C`) |
| `tools/baudos.py`, `tools/pruefdos.py` | MS-DOS: mit gcc-ia16 übersetzen und EXE schreiben; Prüfstand (8086 in Unicorn, nachgebildetes INT 21h) |
| `lib/` | fertige Bibliotheken für MS-DOS, CP/M-68K und CP/M-8000 (`lib/README.md`) |
| `pascal/` | Fassung für Turbo Pascal 3 unter MS-DOS und CP/M-80 (siehe `pascal/README.md`) |
| `tools/mkgbl.py`, `tools/pruefgbl.py` | Bildsammlung für `gbldv` aus PNG oder Rohdaten schreiben; Prüfsammlungen schreiben und am PC (`make gltest`) mit `gbdef` vergleichen |
| `tools/pruefcpm.py` | Prüfstand für CP/M-80 (Z80, Paket `z80`, nachgebildetes CP/M 2.2); bedient auch das `TURBO.COM` der CP/M-Fassung von TP3 |
| `tools/tp3.py` | Turbo Pascal 3.01A (`TURBO.COM`, nicht im Repository) am PC ohne Bildschirm: übersetzt eine `.PAS`-Datei zu `.COM` |
| `EINBINDEN.md`, `beispiele/` | Aufruf aus C und Assembler je System, Aufrufkonventionen, was für andere Sprachen gilt; dasselbe kleine Programm in C, NASM, GNU as (68000) und asz8k |

### Übersetzen

Unter CP/M-8000 mit DRs `zcc`, `asz8k` und `ld8k` (Quellen, `STARTUP.O` und `LIBCPM.A` auf einer Diskette): `SUBMIT GTEST` (`test/GTEST.SUB`) baut `GTEST.Z8K`. Ein eigenes Programm bindet `GRAF.O GBILD.O GEIN.O GHWCPM.O GHSEG.O GHDIR.O GDIR.O GSC1.O GZ8K.O` dazu; wer keine Bilder braucht, lässt `GBILD.O` weg, sofern er keine Bildfunktion aufruft. `GKONF.H` muss mit auf der Diskette sein. **Platz:** Zilogs `ld8k` bindet jede in `DMVGRAF.H` erklärte Funktion ein, und ein Programm ohne Segmente hat 64 KB für Code, Daten und Stapel. Wird es zu groß (`too big for non-seg load`), schreibt man `#define GKLEIN` in `GKONF.H` und übersetzt Programm und DMVGRAF damit neu (so Mahjongg 0.11).

Unter CP/M-68K: am PC mit `gcc-m68k-linux-gnu` und `binutils-m68k-linux-gnu` `make GTEST.68K` (`make MTEST.68K` für den Maustest); ein eigenes Programm übersetzt `tools/bau68.py` (`GCRT68.S` zuerst, dann Programm, `GRAF.C GBILD.C GEIN.C GKERN.C GHWCPM.C GHD68K.C GHPOOL.C GSUP68.S GARG68.C`). Prüfstand: `make simlib.so`, dann `python3 tools/pruef68.py GTEST.68K test/GTEST.TXT [--block] [--mono]`.

Unter MS-DOS: am PC mit gcc-ia16 (`ia16-elf-gcc` in `/opt/ia16/bin`, sonst `IA16=…`) `make GTEST.EXE` und `make MTEST.EXE`, im Speichermodell tiny `make GTEST.COM` (`baudos.py` erkennt es an `.COM` oder `--tiny`); ein eigenes Programm wie in `lib/README.md`. Prüfstand: `make simlib.so`, dann `python3 tools/pruefdos.py GTEST.EXE test/GTEST.TXT [--mono]`.

Fertige Bibliotheken: `make lib` (MS-DOS small und tiny, CP/M-68K), Gebrauch in `lib/README.md`.

Simulation am PC: `make gtest`, dann `GSKRIPT=test/GTEST.TXT ./gtest`. Die Simulation führt die GDC-Listen so aus wie der µPD7220 im Mischbetrieb (nach MAMEs `upd7220.cpp`) und schreibt auf Befehl Bildschirmabzüge (PPM). Skriptzeilen: `maus x y knopf` (Bildzeilen von oben), `taste c` oder `taste 0xNN`, `bild datei.ppm`. Umgebung: `GMONO=1` monochrom, `GMAUS=0` ohne Maus, `GPROT=1` jeden GDC-Befehl melden, `GSUMME=1` Prüfsumme der Listen.

### Grundsätze

- **Koordinaten wie NCRGRAF:** x 0–639 nach rechts, y 0–399 nach **oben**, (0,0) links unten. `GORIG(1)` lässt y von oben zählen.
- **Ein Zeichenpunkt** (`GPOINT`): Linien, Rechtecke, Flächen, Text und Bilder beginnen dort. Figuren erstrecken sich vom Zeichenpunkt in Richtung +x und +y: bei y nach oben ist der Zeichenpunkt die linke untere Ecke, bei `GORIG(1)` die linke obere.
- **Farbe** 0–7 (1 blau, 2 grün, 4 rot) wie TurboGraf; auf der Mono-DMV gilt jede Farbe außer 0 als hell.
- **Richtungen** 0–7 in 45°-Schritten gegen den Uhrzeiger (bei y nach unten gespiegelt).
- Alle Funktionen liefern `int`: 0 gut, sonst ein Fehlercode (1–83 wie NCRGRAF).
- Externe Namen höchstens 6 Zeichen (DRs Binder unter CP/M-8000).

### Funktionen wie NCRGRAF / TurboGraf

| Name | Parameter | Wirkung |
|---|---|---|
| `ginit` | – | Grafik ein, Schirm leer; Muster 0, Zoom 0, Modus 1, Farbe 7 |
| `gexit` | – | zurück in den Textbetrieb |
| `gpoint` | x, y | Zeichenpunkt setzen |
| `gline` | x, y | Linie vom Zeichenpunkt nach (x,y); danach steht der Zeichenpunkt dort |
| `gcircl` | r | Kreis um den Zeichenpunkt |
| `garc` | r, anf, ende | Bogen von `anf` bis `ende` Grad (0° = rechts, gegen den Uhrzeiger) |
| `grect` | m, n, d | Rechteck m × n in Richtung d |
| `gbox` | m, n, d | gefülltes Rechteck mit dem Muster, Größe × (Zoom + 1) |
| `gtext` | s, d | Text in Richtung d; Feld 8·(Z+1) × 16·(Z+1); der Zeichenpunkt rückt hinter den Text |
| `gmode` | m | 0 ersetzen (Hintergrund von Text und Muster schwarz), 1 umkehren (XOR), 2 schwarz zeichnen, 3 in der Farbe zeichnen |
| `gpatt` | p | Füll- und Linienmuster 0–15 (die ersten zwei Musterzeilen sind das Linienmuster) |
| `gpdef` | p, m | Muster p (0–15) durch 8 eigene Zeilen m ersetzen (von oben, Bit 0 links) |
| `gbcol` | c | Hintergrundfarbe der Muster in `gbox`: 0–7 zweifarbig und deckend (Musterpunkte in der Farbe, die übrigen in c, Betriebsart egal); −1 (Vorgabe) nur die Musterpunkte |
| `gzoom` | z | Zoom 0–15 für Text, Flächen und Bilder |
| `gcolor` | c | Farbe 0–7 |
| `gclear` | – | Schirm leer, Grafik bleibt an |
| `where` | &x, &y | Zeichenpunkt lesen |
| `gorig` | o | 0 y nach oben (Vorgabe), 1 y nach unten |

Muster: 0 voll, 1 50 %, 2 25 %, 3 75 %, 4 waagrechte Linien, 5 senkrechte, 6 `/`, 7 `\`, 8 Gitter, 9 Kreuz, 10 Ziegel, 11 12,5 %, 12 breite waagrechte, 13 breite senkrechte, 14 großes Brett, 15 leer.

**Muster am Schirm verankert:** In Flächen der Richtung 0 ohne Zoom bekommt der Punkt (x, y) Bit x mod 8 der Musterzeile y mod 8 (Bildpunkte, y von oben), gleich wo die Fläche beginnt. Aneinanderstoßende Flächen und ein wiederhergestellter Hintergrund passen so ohne Naht. Der 7220 beginnt eine Fläche unten links und zeichnet die Zeilen abwechselnd von links und von rechts; DMVGRAF dreht die Musterzeilen vorher entsprechend. In den anderen Richtungen und mit Zoom beginnt das Muster wie bisher an der Ecke.

`gmode` folgt NCRGRAF (Vorgabe nach `ginit`: 1 umkehren). Text in Richtung 0 geht wortweise (schnell, Zoom in Software), in den anderen Richtungen als Grafikzeichen des 7220.

### Bilder

| Name | Parameter | Wirkung |
|---|---|---|
| `gbdef` | nr, b, h, format, daten | Bild nr (0–63) aus dem Programmspeicher anlegen |
| `gbload` | nr0, datei, b, h, anz, format, kopf | `anz` Bilder b × h aus einer Datei ab nr0 anlegen; vorher `kopf` Byte überspringen |
| `gbldv` | nr0, datei, anz, kopf | Bildsammlung: Bilder verschiedener Größe und Formate, jedes mit eigenem Kopf (siehe unten); `anz` ≤ 0 bis zum Dateiende; geladen: `gbanz` |
| `gbmap` | farbe, a, b, muster | Farbnummer 0–15 (Format 3) als DMV-Farbe a oder als Raster aus a und b |
| `gbput` | nr, art | Bild am Zeichenpunkt zeichnen, mit Zoom |
| `gbget` | nr, b, h | Schirmausschnitt am Zeichenpunkt als Bild ablegen |
| `gbfree` | nr | Bild freigeben (der Speicher dahinter rückt nach) |
| `gbinfo` | nr, int r[6] | Breite, Höhe und sichtbarer Bereich x0, y0, x1, y1 (Bildpunkte, y von oben) |

**Formate:**

| | Inhalt | Beispiel |
|---|---|---|
| 1 `GB_BIT7` | 1 Bit je Punkt, Bit 7 links, je Zeile (b+7)/8 Byte | DMVRACE-Wagen |
| 2 `GB_BIT0` | 1 Bit je Punkt, Bit 0 links | eigene Schrift, Masken |
| 3 `GB_NIB` | 4 Bit je Punkt (Farbnummer 0–15), hohes Halbbyte links | TIL-Steinsätze, EGA-Rohdaten |
| 4 `GB_EBN` | je Zeile Maske, Grün, Rot, Blau, Bit 0 links | Zeiger, interne Form |
| 5 `GB_SETR` | `int`-Worte: je Ebene (Grün, Rot, Blau) SET-, dann RESET-Worte aller Zeilen, Bit 0 links | HOPPLER-Figuren (`Spr16`); in Dateien 16-Bit-Worte, niederwertiges Byte zuerst |

- 1-Bit-Bilder zeichnen in der aktuellen Farbe.
- **Format 5 aus einer Datei:** `gbload` liest je Bild alle Worte in einen Puffer und legt das Bild dann wie `gbdef` an. In C reicht der Puffer für 384 Worte (Figuren bis 32 × 32), in Pascal (`DMVBILD.INC`, nicht in der kleinen Fassung `DMVBKL.INC`) kommt er vom Heap (Bilder bis 64 × 85). Sonst `GE_PLATZ`.
- **Bildsammlung (`gbldv`):** Jedes Bild beginnt mit 6 Byte Kopf: Breite b (2 Byte, niederwertiges zuerst), Höhe h (2 Byte), Format (1–5), 0; danach die Daten genau wie für `gbdef` (Format 5 als 16-Bit-Worte, niederwertiges Byte zuerst), ohne Auffüllung bis zum nächsten Bild. Eine Breite außerhalb 1–640 (auch das ^Z am Ende eines CP/M-Satzes) beendet die Sammlung. `gbanz` sagt, wie viele Bilder angelegt wurden; bei einem Fehler (`GE_KURZ`, `GE_PLATZ`, `GE_FORM`, `GE_NR`) bleiben die Bilder davor erhalten. Mit `anz` > 0 ist ein zu frühes Ende `GE_KURZ`. `kopf` ist in C `unsigned` (bis 65535; Zahlen über 32767 als `(unsigned)40000` schreiben), in Pascal `real`. Schreiben: `tools/mkgbl.py AUS.GBL bild.png roh.bin:b:h:f …` (PNG mit Durchsichtigkeit → Format 4, `bild.png:1`/`:2` → 1 Bit je Punkt); `mkgbl.py AUS.GBL --liste` zeigt den Inhalt.
- **Format 3:** `gbmap` setzt jede Farbnummer auf eine DMV-Farbe a, oder mit `muster` 1 (Schachbrett), 2 (jeder vierte Punkt) oder 3 (jede zweite Zeile) auf ein Raster aus a und b. **a = −1 macht die Farbnummer durchsichtig;** `muster` sagt dann, wie die Form entsteht: 0 jeder Punkt für sich, 1 je Zeile vom ersten bis zum letzten sichtbaren Punkt gefüllt (Lücken in Farbe b), 2 wie 1, um einen Punkt nach allen Seiten erweitert (Rand in Farbe b). Vorgabe wie Mahjongg: 0 durchsichtig, Hellgrau → Weiß, Dunkelgrau → Raster Schwarz/Blau, Braun → Raster Gelb/Rot, die übrigen auf die nächste DMV-Farbe; mono: Hellgrau und Weiß hell, Dunkelgrau im Raster, alles andere dunkel. Eine TIL-Datei ist `gbload(0, "X.TIL", 40, 40, 42, GB_NIB, 0)`.
- **Arten** für `gbput` und `glput`: 0 `GB_DECK` deckend (das ganze Rechteck), 1 `GB_DURCH` durchsichtig, 2 `GB_SCHAT` durchsichtig und schattiert (Weißes im Raster Weiß/Blau, mono Helles im 50-%-Raster – für Auswahlen), 3 `GB_XOR` umkehren.
- `gbput` zeichnet wortweise über Setzen und Löschen je Ebene; Nachbarpunkte bleiben unberührt. Bilder werden am Schirmrand abgeschnitten.
- **Schneller Weg** wie HOPPLERs `BlitAsm` (MS-DOS in C und Pascal, CP/M-80 in Pascal): Liegt das Bild auf einer Wortgrenze (x durch 16 teilbar), ohne Zoom und ganz auf dem Schirm, und ist die Art deckend (dann auch die Breite durch 16 teilbar), durchsichtig oder umkehren, geht es ohne GDC-Liste direkt aus dem Bildspeicher an den 7220: einmal MASK und FIGS, dann je Zeile und Ebene nur CURS, WDAT und die Worte. Das Ergebnis ist dasselbe (`test/BTEST.C` zeichnet links mit, rechts ohne), nur schneller: ein Bild 64 × 47 mit Maske und drei Ebenen durchsichtig in der Pascal-Fassung unter DOS 32 000 statt 136 000 Befehle (`BlitAsm` 23 000).
- `gbget` liest mit RDAT über BIOS 67. MAMEs 7220 liefert dabei genau DC Worte, der echte DC+1; DMVGRAF fordert DC = Zahl der Worte an und liest so viele.

### Figuren über GCHRD

Kleine Figuren, die der 7220 selbst aus seinem Parameter-RAM zeichnet und vergrößert (Grafikzeichen, je 8 × 8 Punkte). Schneller als Bilder und gut für bewegte Figuren in Spielen; die Muster bleiben im Programm.

| Name | Parameter | Wirkung |
|---|---|---|
| `gsprdef` | nr, b, h, muster | Figur anlegen; b und h je 8 oder 16; muster: je Ebene (grün, rot, blau) h Zeilen zu b/8 Byte, Bit 0 links. Maske ist das ODER der Ebenen, schwarze Punkte sind also durchsichtig. |
| `gsprset` | nr, b, h, worte | Figur im Format 5 wie `gbdef … GB_SETR`: `int`-Worte je Ebene SET, dann RESET, ein Wort je Zeile, Bit 0 links. Durchsichtig ist nur, was in keiner Ebene gesetzt oder gelöscht wird; Schwarz (RESET in allen Ebenen) deckt – Umrisse, Pupillen, Beine wie in HOPPLER. |
| `gspr` | nr, x, y, zoom, art | an x, y zeichnen (Ecke wie `gbput`), Zoom 0–15; art `GB_XOR` umkehren (zweimal = weg), `GB_DURCH` nur die Figur, `GB_DECK` das ganze Feld. Ragt die Figur über den Schirm: `GE_LAGE` (96). |

- Die Daten beider Arten bleiben im Programm und müssen erhalten bleiben (bei `gsprset` etwa ein Feld, in das das Programm die Figuren aus seiner Datei liest).
- Anzahl: in C 32 Figuren (0–31, beim Übersetzen der Bibliothek `-DGSMAX=n`), in Pascal legt das Programm sie fest: `const SPMAX = 20;` vor `{$I DMVSPR.INC}` ergibt die Figuren 0–20.
- `GB_DURCH` zeichnet je Ebene erst die zu löschenden, dann die zu setzenden Punkte über GCHRD; leere Durchgänge fallen weg. HOPPLERs Drachenlibelle 16 × 16 mit Zoom 1 braucht in der Pascal-Fassung unter DOS 30 000 Befehle (HOPPLERs `Sprite16` 37 000); `test/STEST.C` zeichnet sie im zweiten Bild links mit `gspr`, rechts mit `gbput` – beide Hälften gleich.
- Auf der Mono-DMV gilt ein Punkt als gesetzt, wenn er in einer der drei Ebenen gesetzt ist.

### Zusammensetzen

| Name | Parameter | Wirkung |
|---|---|---|
| `glayer` | x0, y0, x1, y1 | Ausschnitt beginnen (Ecken einschließlich) |
| `glput` | nr, x, y, art | Bild eintragen, von hinten nach vorn; Bilder außerhalb fallen weg |
| `gldraw` | – | Ausschnitt im Speicher zusammensetzen und ersetzend zeichnen |

Für Brettspiele und alles, wo sich Bilder überdecken: nichts flackert, und jeder Punkt geht nur einmal zum 7220. Der Ausschnitt wird waagrecht auf ganze Worte (16 Punkte) erweitert; der Hintergrund ist schwarz. Bis zu 200 Einträge; `glput` kennt keinen Zoom.

### Maus, Tastatur, Zeit, Auskunft, Ton

| Name | Parameter | Wirkung |
|---|---|---|
| `ginfo` | &farbe, &maus, &syn, &covox | Farb-/Mono-DMV, K806, Synthesizer-Firmware, Covox (die letzten beiden noch immer 0) |
| `mpos` | &x, &y, &k | Mausposition in den gewählten Koordinaten, Knöpfe (Bit 0 links, 1 rechts, 2 Mitte); 1 gelesen |
| `mset` | x, y | Mausposition setzen (K806 Befehl 04h) |
| `gzeig` | nr, x, y | Mauszeiger: Bild nr (höchstens 32 × 32 Punkte) durchsichtig an x, y; DMVGRAF sichert den Hintergrund (aus dem Bildspeicher gelesen) und stellt ihn beim Versetzen wieder her. Andere Ausgaben nehmen den Zeiger vorher weg, `mpos`, `gkey` und `gwait` zeigen ihn wieder. nr < 0 nimmt ihn weg |
| `gdirekt` | an | GDC-Listen direkt an den 7220 (1, Vorgabe unter CP/M-8000) oder über BIOS 66/67 (0); Ergebnis: vorherige Einstellung, −1 wenn es keinen direkten Weg gibt |
| `gkey` | – | Taste oder 0, wartet nicht |
| `gwait` | n | n Bildwechsel abwarten |
| `grnd` | – | Zufallszahl 0–32767 für den Start (aus der Lage des Bildaufbaus und der Maus) |
| `ton` | h, l | Ton h (1–42 Halbtöne ab 110 Hz, 0 Pause), l × 20 ms, über den Tastatur-8741; wartet nicht, 1 = der 8741 nimmt gerade nichts an |
| `tonq`, `tonclr`, `tonidle` | h, l / – / – | Tonwarteschlange, siehe unten |
| `gputs` | s | Text auf die Konsole (im Textbetrieb) |

`gkey`, `grnd` und `gputs` sind neu gegenüber NCRGRAF (BASIC hatte `INKEY$` und `PRINT`).

**Tonwarteschlange** (wie HOPPLERs `SndPump`, in C und Pascal): `tonq(h, l)` stellt einen Ton in die Warteschlange (31 Plätze, voll: `GE_PLATZ`), `tonclr()` verwirft Wartendes, `tonidle()` ist 1, wenn alles an den 8741 geschickt ist. Gepumpt wird in `gkey`, `gwait`, `tonq` und `tonidle`: Sobald der 8741 wieder etwas annimmt, geht der nächste Ton hinaus; das Programm wartet nie auf den Ton.

### Nur MS-DOS: Hardwareauskunft, Zeitbasis, Koprozessor

Nach der Anleitung „NCR Decision Mate V – Hardware erkennen“ (2.11.2026) und den PITTEST-Messungen. Ein Wert 0 heißt immer „unbekannt“ oder „nicht da“, nie geraten.

**Eine Fassung für alle 8086-Programme:** Die Arbeit tun `src/GHW.INC` (Erkennung, Koprozessor) und `src/GZEIT.INC` (Zeitbasis), MASM 5.1 und JWasm `-Zm`. DMVDEMO, DMVRACE und PITTEST binden sie aus MASM ein (siehe [EINBINDEN.md](EINBINDEN.md)); für die Bibliothek macht `tools/gpicbin.py` daraus `src/GHW86.S`, und `GINFO.C`, `GZEIT.C` und `ghinit` in `GHWDOS.C` rufen nur noch diesen Kern. Die C-Aufrufe sind dieselben wie vorher. CP/M-68K und CP/M-8000 behalten ihre C-Fassung und richten sich nach den Regeln unten.

`ghinfo(r)` füllt `int r[16]`; vorher in `r[15]` `GH_MITCOPRO` (Koprozessor probieren, nur K235) und/oder `GH_MITUHR` (K803, dauert 1,2 s) setzen, sonst bleiben diese Einträge 0. Der Kern füllt dieselbe Tabelle (`gh_tab`), die Hülle kopiert sie nur.

| Index | Inhalt |
|---|---|
| `GH_ROM` | 1 M.07.00, 2 C.07.00 (CRC-32 über die 8 KB nach `OUT 11h`), 3 M.05.00, 4 M.06.00, 5 C.06.00 (nach der Kennung bei 0FF7h) |
| `GH_SCHIRM` | 1 Mono, 2 Farbe, 3 widersprüchlich (Ebenenprobe und Kennbuchstabe 0FF9h passen nicht zusammen) |
| `GH_KARTE` | 1 K235 (8259 antwortet), 2 K230/K231, 0 keine 16-Bit-Karte (13h Bit 1) |
| `GH_CPU` | 1 8088, 2 V20, 3 die Proben `PUSH 0` und `AAD 16` widersprechen sich |
| `GH_FPU` | 0 keiner, 1 8087 (Steuerwort 03FFh), 2 anderer (etwa MAME: 037Fh) |
| `GH_COPRO` | 0 nicht geprüft (oder keine K235), 1 keiner, 2 K234, 3 K238, 4 der Z80 kam nicht zurück, 5 nicht probiert (Kern oder Stapel liegen über dem Warteplatz F10Eh–F11Ch) |
| `GH_BAENKE` | Erweiterungsbänke (Muster je Bank bei n:FFF0, erst alle schreiben, dann lesen, danach zurück) |
| `GH_SPUR_A` | 40 oder 80 (0:FE08) |
| `GH_TAKT` | Prozessortakt in 10 kHz (1600 × AAM gegen Kanal 2; V20 15 Takte, 8088 vorläufig 77) |
| `GH_BILD` | Bildfrequenz in 1/100 Hz (Bildwechsel über 0,26 s) |
| `GH_UHR` | 1 = K803 auf 4B läuft |

`ghcpu()` ist die Kurzform für den Start: `GH_CPU | GH_FPU << 2 | GH_KARTE << 4`. `ghzeile(p, engl)` schreibt nach `ghinfo` die Zeile „Erkannt: K235 mit V20, 8087, K234 (68008)“ (Wortwahl wie DMVDEMO; mit `engl` = 1 „Found: K235 with V20, …“) nach `p` (80 Byte).

#### Regeln (für alle Fassungen)

- **Reihenfolge:** DMV-Prüfung (nur lesen: Bit 5 und 6 an A0h wechseln, FIFO leer kommt vor, nie FFh) vor allem anderen. Leerer Bus liefert das untere Adressbyte zurück; das gilt für jede Probe auf einen freien Steckplatz.
- **Kennbuchstabe** 0FF9h (`OUT 11h`, lesen, `OUT 10h`, Unterbrechungen gesperrt): C Farbe, M Mono. Nur wenn weder Programm noch Stapel unter 2000h liegen.
- **CPU:** `PUSH 0` (6Ah; 8088: JPE) senkt SP nur auf dem V20, `AAD 16` auf 0102h gibt 12h (8088) oder 0Ch (V20, rechnet immer mit 10). Beide müssen übereinstimmen.
- **8087:** Status- und Steuerwort mit 5A5Ah vorbelegen, FNINIT, FNSTSW, FNSTCW; Steuerwort ≠ 5A5Ah heißt vorhanden.
- **K235:** 13h Bit 1 = 0 und der 8259 auf 90h/91h nimmt AAh und 55h ins Maskenregister an (alter Wert zurück).
- **Bildschirmprobe nur im Grafikbetrieb** (oder wie `ginit` mit gesicherter Schreibstelle EAD): A5C3h nach 3FF0h, 3C5Ah nach 7FF0h, 7FF0h dann 3FF0h zurücklesen, alte Worte zurück. Im Textbetrieb verstellt das Schreiben sonst die Schreibstelle der DOS-Konsole (Zeichenmüll in Zeile 0). Für eine Meldung vor dem Grafikbetrieb den Kennbuchstaben nehmen.
- **K806** über F1 (Statusbit 3 = A0 des letzten Schreibens) und genau fünf Datenbytes; sonst wird nichts beschrieben.
- **Unterbrechungen:** beim Kennbuchstaben, bei der Eichung und beim Koprozessorlauf gesperrt; danach gilt der alte Zustand, nicht einfach STI.
- **Zeitbasis nur, wenn sie gebraucht wird:** Kanal 2 lernen ist kurz (bis 8 Neuladungen), Kanal 0 und Bildwechsel werden getrennt und erst auf Anfrage geeicht.

#### Zeitbasis

`gtinit()` lernt Kanal 2 des 8253 kennen (unter DOS Modus 3, Startwert 5000 oder 27463, 1 µs je Schritt, wird nie umprogrammiert; ohne DOS steht er, dann setzt `gtinit` Modus 2 mit vollem Umfang) und setzt die Uhr auf 0; Ergebnis 1, wenn Kanal 2 stand. `gtus()` liefert µs seit `gtinit` als `long` und muss öfter gerufen werden als einmal je Halbperiode von Kanal 2 (5 bzw. 27 ms). `gtbild()` (µs je Bild, plausibel 10 000–40 000) und `gt0per()` (µs je Takt von Kanal 0, plausibel 5000–62 500; steht er, wird er über Port 14h angestoßen und nach 10 Bildwechseln noch einmal gemessen) eichen beim ersten Aufruf über 0,26 s; `gteich(art, n)` eicht neu über n × 65 536 µs (art 0 Kanal 0, 1 Bild). Außerhalb der Plausibilität: 0. `gt0()` Kanal 0 roh. Früher eichte `gtinit` beides gleich mit (über 1 s).

#### Koprozessor (nur K235)

Auf der K230/K231 ist jedes `OUT D8h`–`DFh` eine Prozessorumschaltung; ohne K235 wird nichts probiert.

- **Warteplatz des Z80:** ab F10Eh `OUT (D0h),A / NOP` (dort wartet der Z80, solange der 8088 läuft), ab F111h `DI / OUT (DFh),A / LD A,5Ah / LD (F11Ch),A / JP F10Eh`. F10Eh–F11Ch wird gesichert und zurückgelegt. DI zuerst: sonst sähe ein anstehender Z80-Interrupt (IM 1) bei 0038h die 8088-Vektoren als Code. Die Marke 5Ah in F11Ch zeigt, dass der Z80 zurückkam.
- **Start nur mit `OUT DFh`, nie D8h** (damit lief die echte K234 nicht an). Der 68008 gibt mit `move.b d0,$FFDF.w` zurück und hält mit `stop`, der Z8001 mit `OUTB DFh` und `HALT`; danach gibt der Z80 mit `OUT D0h` an den 8088 zurück.
- **Startzustand ab 0:0000:** 68008 SSP und PC (Big Endian); Z8001 Wort 0, FCW C000h, PC als (80h | Segment) << 8 und Versatz. Die überschriebenen 8088-Vektoren werden gesichert und zurückgelegt.
- **Probe:** die zweisprachige 40-Byte-Probe aus DMVDIAG; der 68008 schreibt 36h nach 0:0000, der Z8001 5Ah. Vorher: liegen Kern oder Stapel über dem Warteplatz, wird nicht probiert (`GH_COPRO` 5).
- **Kurze Probe mit dem echten Kern:** wer den Koprozessor rechnen lässt, macht nach der Erkennung einen kurzen Lauf mit seinem Kern (DMVDEMO: eine Zeile bei Tiefe 1); das zeigt, ob Briefkasten und Kern stimmen.

`gcorun(art, seg, start, len, &us)` lässt ein Programm auf dem Z80 (art 1), dem 68008 der K234 (2) oder dem Z8001 der K238 (3) laufen; der 8088 steht so lange, die Unterbrechungen sind gesperrt. Z80: `len` Byte Code ab `seg:start`; DMVGRAF legt ihn in Bank 0 unter das eigene Programm (2000h–F100h), hängt den Rückweg an (der Code läuft bis zu seinem Ende durch, kein eigener Rücksprung) und stellt den Speicher danach wieder her. 68008/Z8001: ab `seg:start` 8 Byte Startzustand, die nach 0:0000 kommen. `us`: Laufzeit über Kanal 0 (auf ein Bild genau). Ergebnis 0 gut, 1 kein Weg (keine K235, kein Platz, Warteplatz belegt), 2 der Z80 kam nicht zurück. `gcoblk(absaetze, &seg)` holt einen DOS-Block, der nicht über dem Warteplatz liegt und für den Z8001 ganz in einem 64-KB-Segment (bis zu vier Versuche). Lange Rechnungen in Stücken laufen lassen: nach mehr als 15 s Sperre ist der Diskettenmotor aus.

**Prüfung:** `tools/pruefdos.py` bildet 8088 und V20, mit und ohne 8087, K235/K230/keine Karte, keinen Koprozessor, K234 (der 68008 rechnet in Unicorn), K238 (nur die Probe), einen Z80, der nicht zurückkommt, stehenden Kanal 0, Farbe und Mono nach. `test/KTEST.C` (C) und `test/GHWT.ASM` (MASM) liefern in allen Fällen dieselbe `ghinfo`-Tabelle, die Summe des 68008 stimmt, Vektoren und Warteplatz sind danach unverändert; liegt das Programm über dem Warteplatz, wird nicht probiert. `ITEST` zeigt die Tabelle und die Zeile. Den Takt misst der Prüfstand nicht (seine Uhr läuft nur bei Portzugriffen).

### Gepackte Vollbilder (.PIC)

Für Titelbilder, Fotos und Bildschirmabzüge: ein Vollbild oder ein Ausschnitt in Ebenen, mit LZSS gepackt, wird ohne Puffer für eine ganze Ebene direkt in den Bildspeicher geschrieben. Erbeten von DMVDEMO (`claude/DMVGRAF-LZSS-Anforderungen.md`).

| Name | Parameter | Wirkung |
|---|---|---|
| `gpic` | datei, xw, y, art | `.PIC`-Datei an Wortspalte `xw` (0–39), Zeile `y` (0–399) |
| `gpicm` | daten, laenge, xw, y, art | dasselbe aus dem Speicher (Bilder im Programm) |

- Vereinbart in **`src/DMVPIC.H`**, nicht in `DMVGRAF.H`: Der Binder von CP/M-8000 nimmt zu jeder vereinbarten Funktion den Code dazu.
- `art`: Bit 0 Anzeige während des Schreibens dunkel (BCTRL 0Ch, danach 0Dh), Bit 1 Mono erzwingen, Bit 2 Farbe erzwingen; ohne beides gilt die erkannte Karte.
- Geschrieben wird: Farbkarte – G, R, B in ihre Ebenen, fehlende Farbebenen gelöscht; hat die Datei nur M, kommt M in alle drei (weiß). Mono – M in die Grün-Ebene, ohne M die Ebene G. Nicht benötigte Ebenen werden übersprungen.
- Rückgabe: 0, `GE_DATEI` (92, Datei fehlt), `GE_KURZ` (93, zu kurz oder Ebenenlänge stimmt nicht), `GE_FORM` (94, Kennung oder Version unbekannt), `GE_LAGE` (96, ragt über den Schirm), **`GE_VERW` (97, Verweis vor den Anfang der Ebene)**, `GE_HW`. Bei einem Fehler wird nichts außerhalb des Bildes geschrieben.
- **MS-DOS:** Kern in 8086-Maschinensprache (`src/GPIC.INC`, in die Bibliothek übernommen als `src/GPIC86.S`): Arbeitsbereich 5,3 KB (Fenster 4 KB, Lesepuffer 1 KB, mit INT 21h nachgefüllt, Zeilenpuffer 80 Byte), jedes Byte geht sofort an den 7220. Je Zeile CURS und WDAT, bei voller Breite ein einziger Strom. **FIFO in Blöcken:** Statt vor jedem Byte auf „nicht voll“ zu warten, wartet der Kern auf „FIFO leer“ (Status Bit 2) und schreibt dann 16 Byte ohne Abfrage; nach Befehlen beginnt ein neuer Block. Ein Bild nur mit M wird für den Farbschirm einmal entpackt und Zeile für Zeile in alle drei Ebenen geschrieben.
- **Dunkelschalten:** `gpic` schaltet in der Bibliothek selbst um den Kern herum (GDC-Liste), `GPIC.INC` vor dem Öffnen der Datei und nach dem Schließen (6Bh, 0Dh). In MAME blieb die Anzeige dunkel, solange der Kern erst nach dem Öffnen dunkel schaltete.
- **Andere Systeme** (CP/M-68K, CP/M-8000, PC-Simulation): dieselbe Arbeit in C über die GDC-Liste, Zeile für Zeile.
- **Turbo Pascal 3 unter MS-DOS:** `pascal/DMVPIC.INC`, derselbe Kern als `inline` (CP/M-80: keiner).
- **Geschwindigkeit** (Befehle im Prüfstand; MAME braucht für 1,28 Mio. etwa 6 s): KITT nur M auf dem Farbschirm 1,03 Mio. (etwa 5 s), alle vier Ebenen gepackt 1,59 Mio., alte Rohdatei mit drei Ebenen 0,91 Mio. Die erbetenen 2 s sind auf dem 8088 nicht zu erreichen; schon das reine Ausgeben von 96 000 Byte an den 7220 kostet fast 1 s.

**Aus MASM** (DMVDEMO): `INCLUDE GPIC.INC` (MASM 5.1, auch JWasm `-Zm`), alle Namen mit `gp_`, keine Variablen des Programms. `call gp_pic` mit DS:DX = Dateiname (ASCIIZ), AL = art, BX = xw, CX = y; zurück CF = 1 und AX = Fehlernummer, sonst AX = 0; die übrigen Register bleiben. Der Arbeitsbereich `gp_puf` (`GP_GROESSE` = 5311 Byte) steht am Ende von `GPIC.INC`; `gp_picw` nimmt stattdessen ES:DI (etwa aus FREEMEM). Art Bit 7: Quelle im Speicher, DS:DX = Daten, SI = Länge. Ohne Bit 1/2 gilt Farbe. `tools/gpicbin.py` erzeugt aus derselben Quelle `src/GPIC86.S` für die Bibliothek (braucht JWasm, nur wenn sich `GPIC.INC` ändert).

**Packen:** `tools/dmvpack.py` (Python 3, für PNG Pillow): `dmvpack.py KITT.PIC KITTM.PIC --ebenen M`, `dmvpack.py bild.png bild.PIC --mono bildm.png`, `--roh G` (Ebene ungepackt; geschieht auch von selbst, wenn LZSS größer würde), `--breite`/`--hoehe` für kleinere Bilder, `--entpacken x.PIC x.png` zur Kontrolle. Nach dem Packen entpackt es selbst und vergleicht byteweise. Gierig mit Nachschau um ein Byte, Fenster 4 KB, je Stelle die letzten 64 Treffer. KITT, nur M: 32 000 → 12 231 Byte (38 %).

**Prüfung:** `tools/pruefpic.py` packt Testbilder, zeigt sie mit `test/PICTEST.C` (C) bzw. `test/GPT.ASM` (MASM, `GPIC.INC`) und vergleicht den Bildspeicher mit den Rohdaten: Farbe und mono, volle und schmale Breite, `xw`/`y` ungleich 0, aus dem Speicher, alte Rohdateien (128 000 und 32 000 Byte), rohe neben gepackten Ebenen, G und B ohne R (R wird gelöscht), Abstand 4095, Abstand 1 mit Länge 18, Ende mitten im Flagbyte, kaputte Dateien (zu kurz, Version, Verweis vor den Anfang, Ebenenlänge, Lage, fehlende Datei). Alles gleich in der PC-Simulation, unter MS-DOS (small, tiny, MASM-Fassung) und CP/M-68K.

#### Dateiformat

Alle Zahlen Little Endian, Endung `.PIC`. Andere Programme (DMVSNAP, Spiele) können es ohne DMVGRAF lesen.

| Versatz | Länge | Inhalt |
|---|---|---|
| 0 | 4 | Kennung `DMVP` |
| 4 | 1 | Version 1 |
| 5 | 1 | n = Zahl der Ebeneneinträge, 1–4 |
| 6 | 2 | Breite in Worten (1–40) |
| 8 | 2 | Höhe in Zeilen (1–400) |
| 10 | 2 | 0 |
| 12 | 4·n | je Eintrag: Art (`G` Grün 0000h, `R` Rot 4000h, `B` Blau 8000h, `M` Mono-Fassung), Verfahren (0 roh, 1 LZSS), Länge der gepackten Daten (2 Byte) |

Die Daten folgen in der Reihenfolge der Tabelle; jede Ebene steht für sich. Entpackt: Zeile für Zeile von oben, je Zeile `Breite` Worte im Format des 7220 (Low-Byte zuerst, Bit 0 = linker Punkt) – genau der Wortstrom nach CURS, MASK FFFFh, FIGS und WDAT.

**LZSS:** Ein Flagbyte gilt für die nächsten acht Einträge, Bit 0 zuerst. Bit 0 = ein Literalbyte folgt; Bit 1 = ein Verweis aus zwei Byte: Byte 1 = Abstand Bits 0–7, Byte 2 = Abstand Bits 8–11 im hohen Halbbyte, Länge − 3 im niedrigen. Abstand 1–4095 zurück, Länge 3–18; Byte für Byte kopiert, Überlappen erlaubt (Abstand < Länge wiederholt). Das Fenster beginnt bei jeder Ebene leer; Verweise vor den Anfang sind unzulässig. Die Ebene endet nach `Breite · 2 · Höhe` Byte; der Rest eines Flagbytes wird nicht gebraucht, die gepackte Länge muss genau aufgehen.

**Alte Dateien:** ohne Kennung `DMVP` gilt eine Datei mit genau 128 000 Byte als roh, 40 × 400, Ebenen G, R, B, M; mit 32 000 Byte als eine Ebene M.

### Geplant

- **`gprint`** (p, l): Bildschirm drucken wie NCRGRAF, mit den Druckerarten von DMVSNAP (0 Epson FX, 1 IBM Proprinter, 2 HP PCL).
- **Synthesizer:** `synavl`, `synon`, `synoff`, `synfrq`, `syninc`, `synnoi`, `synclk` (Namen wie `DMVSYN.INC` aus DMVTRAK). Die Synthesizer-Firmware für den Tastatur-8741 (`dmv_mb_8741_32678_synth.bin`, rfka01/dmv_firmware) ist **keine NCR-Firmware, sondern ein Eigenbau** auf Grundlage des Original-ROM 32678, und sie ist **optional**: in MAME eine eigene BIOS-Auswahl, auf einer echten DMV ein neu gebranntes 8741. DMVGRAF erkennt sie und fällt ohne sie auf `ton` zurück. Unter CP/M laufen die Aufrufe über BIOS 73.
- **Covox** am Druckerausgang K210: `smplay`, `smstop` (unter MS-DOS mit K235 im Hintergrund, unter CP/M-8000 nur blockierend).
- ein BGI-Treiber für Turbo Pascal 4.0–5.5 und ein Weg für die Microsoft-Werkzeuge (siehe `EINBINDEN.md`).

### Hardware

Tastatur und Ton (8741) und Maus (K806) laufen über den DMV-BIOS-Block, wo es ihn gibt (ohne ihn, unter NCRs CP/M-68K, spricht `GHD68K.C` sie selbst an, mit denselben Abläufen wie der BIOS); der 7220 gehört im Grafikbetrieb dem Programm (`docs/DMV-Schnittstellen.md`, Abschnitt 9, „Wem welche Hardware gehört“). Diese Fassung baut alles als GDC-Befehlsliste (Aufbau wie BIOS 66) und gibt sie selbst an den 7220 aus (CP/M-8000 `GDIR.8KN`, CP/M-68K `GHD68K.C`: BDOS 62 holt den System- bzw. Supervisorzustand, vor jedem Byte wird gewartet, bis das FIFO nicht voll ist, wie im BIOS); `gdirekt(0)` schaltet auf BIOS 66/67 um. Unter CP/M-8000 sind beide Wege mit Mahjongg in MAME getestet. Gezeichnet wird wie in HOPPLERs `DMVGFX.INC` und DMVDEMOs `GDCLIB.INC`: Mischbetrieb, 40 Worte je Bildzeile, Bit 0 = linker Punkt, GD-Bit für alles, was senkrecht weiterschreitet, nach jedem CURS mit Punktangabe eine neue MASK, vor Wortfolgen FIGS mit DC = 0 (der 7220 schreibt das erste Wort nach WDAT DC+1-mal). Die Pascal-Fassung schickt bei aufeinanderfolgenden Wortfolgen nur CURS ohne Punktangabe und WDAT; MASK und FIGS gelten dann noch (wie HOPPLERs `BlitAsm`).

**Proben** nach der Anleitung „Hardware erkennen“: Ein leerer Steckplatz liefert das untere Byte seiner Portadresse zurück (MAME: FFh), die K806 erkennt DMVGRAF daran, dass nach einem Befehl an 31h das Statusbit 3 (F1) gesetzt ist und genau fünf Datenbytes kommen; sonst schreibt es nichts auf die Steckplätze. Farbe oder Mono: A5C3h nach Wort 3FF0h, 3C5Ah nach 7FF0h, in der Reihenfolge 7FF0h, 3FF0h zurücklesen, die alten Worte wiederherstellen; unter MS-DOS wird das mit dem Kennbuchstaben der Firmware (0FF9h, `OUT 11h`) verglichen.

### Lizenz

MIT, siehe [`../LICENSE`](../LICENSE). Code: Claude (Anthropic) · Idee und Prompts: rfka01

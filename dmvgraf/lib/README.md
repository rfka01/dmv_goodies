# DMVGRAF – fertige Bibliotheken / ready-made libraries

*English below.*

DMVGRAF 0.2, übersetzt zum Einbinden in eigene Programme. Die Schnittstelle ist `src/DMVGRAF.H`; was die Funktionen tun, steht in `../README.md`.

| Verzeichnis | System | Inhalt | Werkzeuge zum Binden |
|---|---|---|---|
| `dos/` | MS-DOS auf der DMV (8088/V20), Speichermodell small | `GCRTDOS.O` (Startstück), `DMVGRAF.A` | gcc-ia16 (`ia16-elf-gcc`, binutils-ia16), `tools/baudos.py` |
| `cpm68k/` | CP/M-68K auf der K234 (mit und ohne DMV-BIOS-Block) | `GCRT68.O` (Startstück), `DMVGRAF.A` | `m68k-linux-gnu-gcc`, `tools/bau68.py` |
| `cpm8000/` | CP/M-8000 auf der K238 (DMV-BIOS-Block, Bilder in der K208) | `DMVGRAF.A` (Archiv für `ld8k`) | DRs `zcc`, `ld8k`, `STARTUP.O`, `LIBCPM.A` |

## Ein Programm binden

Aufruf aus Assembler, Aufrufkonventionen und andere Sprachen: [`../EINBINDEN.md`](../EINBINDEN.md).

**MS-DOS** (am PC):

    python3 tools/baudos.py -I src -o PROG.EXE lib/dos/GCRTDOS.O prog.c lib/dos/DMVGRAF.A

Das Programm beginnt mit `main(argc, argv)`. Die EXE-Datei hat 64 KB Daten und Stapel; die Bilder legt DMVGRAF in Speicherblöcken ab, die es bei DOS anfordert (bis zu zwei zu 64 KB).

**CP/M-68K** (am PC):

    python3 tools/bau68.py -I src -o PROG.68K lib/cpm68k/GCRT68.O prog.c lib/cpm68k/DMVGRAF.A

Der Bildspeicher ist ein Feld von 60 KB im Programm (`GHPOOL.C` mit `GBPOOL=61440`). Wer eine andere Größe will, übersetzt `src/GHPOOL.C` selbst mit `-D GBPOOL=…` und nennt es vor `DMVGRAF.A`.

**CP/M-8000** (auf der DMV oder im Simulator, mit DRs Werkzeugen):

    zcc -c -m1 prog.c
    ld8k -w -s -o prog.z8k startup.o prog.o dmvgraf.a -lcpm

`STARTUP.O` und `LIBCPM.A` gehören zu DRs C-Compiler und sind nicht dabei; ein so gebundenes Programm enthält DRs C-Laufzeit. Das Archiv selbst enthält nur DMVGRAF (mit `zcc`/`asz8k` übersetzt, mit `ar8k` zusammengestellt).

## Stand

- MS-DOS: neu, bisher nur im Prüfstand (`tools/pruefdos.py`, 8086 in Unicorn): GTEST zeigt in Farbe und mono dieselben Bilder wie die PC-Simulation, MTEST erkennt Farbe und Maus. Noch nicht in MAME.
- CP/M-68K: im Prüfstand wie die PC-Simulation; in MAME starten Mahjongg und MTEST.
- CP/M-8000: mit Mahjongg in MAME getestet.

Neu bauen: `make lib` (DOS und CP/M-68K); das CP/M-8000-Archiv mit `ar8k rv dmvgraf.a graf.o gbild.o gein.o ghwcpm.o ghseg.o ghdir.o gdir.o gsc1.o gz8k.o`.

## English

DMVGRAF 0.2, compiled for linking into your own programs. The interface is `src/DMVGRAF.H`; the functions are described in `../README.md`.

- `dos/` – MS-DOS on the DMV (8088/V20), small model: `GCRTDOS.O` start-up and `DMVGRAF.A`, for gcc-ia16. Link with `tools/baudos.py -I src -o PROG.EXE lib/dos/GCRTDOS.O prog.c lib/dos/DMVGRAF.A`. Images go into memory blocks requested from DOS (up to two of 64 KB).
- `cpm68k/` – CP/M-68K on the K234, with or without the DMV BIOS block: `GCRT68.O` and `DMVGRAF.A`, for `m68k-linux-gnu-gcc`. Link with `tools/bau68.py -I src -o PROG.68K lib/cpm68k/GCRT68.O prog.c lib/cpm68k/DMVGRAF.A`. Image store: a 60 KB array in the program (compile `src/GHPOOL.C` yourself for another size and put it before the archive).
- `cpm8000/` – CP/M-8000 on the K238: `DMVGRAF.A` for DR's `ld8k`: `ld8k -w -s -o prog.z8k startup.o prog.o dmvgraf.a -lcpm`. DR's `STARTUP.O` and `LIBCPM.A` are not included; a program linked this way contains DR's C runtime.

Status: MS-DOS is new and has only run in the test bench (`tools/pruefdos.py`): GTEST gives the same screens as the PC simulation in colour and mono, MTEST detects colour and mouse; not yet in MAME. CP/M-68K matches the PC simulation in its test bench and starts in MAME. CP/M-8000 is tested in MAME with Mahjongg.

Rebuild with `make lib` (DOS and CP/M-68K).

Code: Claude (Anthropic), idea and prompts: rfka01. Licence: MIT.

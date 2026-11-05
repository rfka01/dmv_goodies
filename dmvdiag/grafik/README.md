# GTEST, MTEST – Grafiktest für die DMV (DMVGRAF)

*Deutsch weiter unten.*

## English

Two small test programs for the graphics library **DMVGRAF** (`../../dmvgraf`), maintained here:

- **GTEST** – three screens (figures, text, images), next screen with a key; on the third a mouse pointer follows the mouse, ESC quits. It is also DMVGRAF's main regression test (PC simulation, test benches).
- **MTEST** – text mode: what DMVGRAF detects (colour, mouse, path to the 7220), then every mouse movement; any key quits.

**16-bit CP/M test disk:** `python3 mkcpm16.py` → `../build/CPM16TEST-x.yy.IMG`, a 320 KB data disk (NCR DD320, no system tracks) with `GTEST.68K`, `MTEST.68K` (built here with gcc, needs `gcc-m68k-linux-gnu`), `GTEST.Z8K`, `MTEST.Z8K` and `LIESMICH.TXT`. Boot CP/M-68K or CP/M-8000 from A:, put the disk in B:, type `GTEST` or `MTEST`. CP/M-86 will follow when DMVGRAF has a CP/M-86 version. The version is in `VERSION`.

The `.Z8K` files are checked in ready-made: they can only be built with DR's `zcc`/`ld8k` under CP/M-8000 (`SUBMIT GTEST`, `SUBMIT MTEST` with the DMVGRAF sources, `GKONF.H`, `STARTUP.O` and `LIBCPM.A` on one disk) and contain Digital Research's C runtime. `dd320.py` writes and reads DD320 disks (`dd320.py liste IMG`, `dd320.py hole IMG NAME.TYP`).

## Deutsch

Zwei kleine Prüfprogramme für die Grafikbibliothek **DMVGRAF** (`../../dmvgraf`), gepflegt hier:

- **GTEST** – drei Bilder (Figuren, Text, Bilder), weiter mit einer Taste; im dritten folgt ein Zeiger der Maus, ESC beendet. Zugleich der Haupttest von DMVGRAF (PC-Simulation `make gtest` in `dmvgraf`, Prüfstände für CP/M-68K und MS-DOS mit `GTEST.TXT`; die Pascal-Gegenstücke liegen in `dmvgraf/pascal`).
- **MTEST** – im Textbetrieb: was DMVGRAF erkennt (Farbe, Maus, Weg zum 7220), danach jede Mausbewegung; Ende mit einer Taste.

**Testdiskette für 16-Bit-CP/M:** `python3 mkcpm16.py` → `../build/CPM16TEST-x.yy.IMG`, eine 320-KB-Datendiskette (NCR DD320, ohne Systemspuren) mit `GTEST.68K`, `MTEST.68K` (hier mit gcc übersetzt, braucht `gcc-m68k-linux-gnu`), `GTEST.Z8K`, `MTEST.Z8K` und `LIESMICH.TXT`. CP/M-68K oder CP/M-8000 von A: starten, Diskette in B:, `GTEST` oder `MTEST` eingeben. CP/M-86 kommt dazu, sobald DMVGRAF eine CP/M-86-Fassung hat. Die Version steht in `VERSION`; jede Änderung bekommt eine neue.

Die `.Z8K`-Dateien liegen fertig im Verzeichnis: Übersetzen geht nur mit DRs `zcc`/`ld8k` unter CP/M-8000 (`SUBMIT GTEST` bzw. `SUBMIT MTEST`, dazu die Quellen aus `dmvgraf/src` samt `GKONF.H`, `STARTUP.O` und `LIBCPM.A` auf einer Diskette), und sie enthalten die C-Laufzeit von Digital Research. Unter MS-DOS: `make GTEST.EXE` / `make MTEST.EXE` in `dmvgraf`. `dd320.py` schreibt und liest DD320-Disketten (`dd320.py liste ABBILD`, `dd320.py hole ABBILD NAME.TYP`).

Im Prüfstand von DMVGRAF zeigt `GTEST.68K` von der Diskette dieselben Bilder wie die PC-Simulation; `MTEST.68K` meldet Farbe und Maus und folgt den Mausbewegungen. Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT

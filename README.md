# DMV Goodies

Programs for the **NCR Decision Mate V** (DMV) under MS-DOS · Programme für die **NCR Decision Mate V** unter MS-DOS

| | |
|---|---|
| [`ecodrive/`](ecodrive/) | RAM disk in the unused red/blue video memory · RAM-Disk im ungenutzten Rot/Blau-Bildspeicher |
| [`dmvdemo/`](dmvdemo/) | Graphics demo for the µPD7220 · Grafikdemo für den µPD7220 |
| [`dmvrace/`](dmvrace/) | Processor race Z80 / 8088 / 8087 / 68008 · Prozessor-Rennen |
| [`mousetst/`](mousetst/) | Graphical test for the K806 mouse adapter · Grafischer Test für den Maus-Adapter K806 |
| [`printertst/`](printertst/) | Printer test files for the printers emulated in MAME (Epson MX-80, FX-80, JX-80, LX-800, EX-800, Star NL-10, Nixdorf MD 16) · Druckertestdateien für die in MAME emulierten Drucker |
| [`dmvkerm/`](dmvkerm/) | small Kermit and terminal for the K211/K212/K801/K215 serial modules · kleines Kermit und Terminal für die seriellen Module K211/K212/K801/K215 |
| [`dmvgraf/`](dmvgraf/) | C graphics library in the style of NCRGRAF/TurboGraf for CP/M-8000 (K238), CP/M-68K (K234) and MS-DOS, with PC simulation and ready-made libraries in `dmvgraf/lib/` · C-Grafikbibliothek nach Art von NCRGRAF/TurboGraf für CP/M-8000 (K238), CP/M-68K (K234) und MS-DOS, mit PC-Simulation und fertigen Bibliotheken in `dmvgraf/lib/` |
| [`Images/`](Images/) | `GOODIES.IMG`: 360 KB DMV disk with all programs, documentation and KITT.PIC · DMV-Diskette mit allen Programmen, Beschreibungen und KITT.PIC; `DMVPRN.IMG`: printer test disk · Druckertest-Diskette; `DMVKERM.IMG`: DMVKERM 1.1 with Kermit-MS 2.32 for IBM PCs · DMVKERM 1.1 mit Kermit-MS 2.32 für IBM-PCs |

The directories hold the sources, the disk image holds the ready-to-run files. · In den Verzeichnissen liegen die Quellen, auf dem Diskettenabbild die fertigen Programme.

## English

**ECODRIVE** – The colour graphics board has 96 KB video memory in three planes; text mode and green graphics only use the green one. ECODRIVE puts a 62 KB drive into the red and blue planes. It loads and unloads as a TSR, checks every sector against a checksum, asks before graphics programs are started and locks the drive if one of them damaged it.

**DMVDEMO** – Thirteen screens that show what the µPD7220 can do on the DMV: dithered colours, zoom, two display partitions, line and area drawing, a plasma, a Mandelbrot spiral, a KITT photo with scanner animation and the front LEDs, and an anaglyph at the end. The Mandelbrot screen is computed by the fastest available processor: 68008 (K234), 8087 or 8088/V20 (switches `/6`, `/7`, `/8`).

**DMVRACE** – A race between the processors a DMV can hold: Z80 on the mainboard, 8088/V20, 8088/V20 with 8087 and the 68008 on the K234. Every track section is a benchmark (square roots, Sierpinski, trial division, sieve, Mandelbrot); each processor really computes it and shows its own result picture. Three laps: the first is measured section by section, the second and third run in real time. With the keyboard firmware that has the race engine, a drag race follows: two snails (8741 keyboard controller, Z80) against a greyhound (8088/V20).

**MOUSETST** – A coordinate cross with the origin in the middle of the screen that traces the position of the K806 mouse adapter in all directions, with scales from 1 to 8 counts per pixel and limits at the screen edge or free. It finds the K806 on any IFSEL address and shows it. The NCR diagnostic program's expected counts for 5 inches are listed in the description.

**DMVKERM** – a small Kermit for the DMV, written from scratch in assembler (one 22 KB `.COM` file): terminal with a VT102 subset, file transfer with Kermit (client and server, long packets, CRC) and XMODEM (CRC, 1K blocks), scripts for dialling a mailbox, over the K211/K212/K213, K801 and K215 serial modules, under NCR MS-DOS 2.11 and the DMV ports of MS-DOS 3.30 and DR-DOS 3.41 (tested in MAME under 3.30 against MS-Kermit on an IBM XT). It finds the modules and their interrupt by itself; CTRL-CLR is the escape key. It is on `GOODIES.IMG`; the separate disk `DMVKERM.IMG` also holds the original Kermit-MS 2.32 for IBM-compatible PCs for the other end of the cable.

**DMVGRAF** – A C graphics library for the µPD7220 in the style of NCRGRAF and TurboGraf: points, lines, circles, arcs, boxes, text, patterns, images in several formats (also stored in the free segments of a K208), off-screen composition of overlapping images, a mouse pointer with saved background, mouse, keyboard and tone. For CP/M-8000 on the K238 (tested in MAME), CP/M-68K on the K234 and MS-DOS (all three tested in MAME with Mahjongg); a PC simulation renders the screen to PPM files, and `dmvgraf/lib/` holds ready-made libraries for all three systems; `dmvgraf/pascal/` has a version for Turbo Pascal 3. Used by Mahjongg in [rfka01/dmv_games](https://github.com/rfka01/dmv_games).

## Deutsch

**ECODRIVE** – Die Farbgrafikkarte hat 96 KB Bildspeicher in drei Ebenen; Textbetrieb und grüne Grafik nutzen nur die grüne. ECODRIVE legt in Rot und Blau ein Laufwerk mit 62 KB ab. Es lädt und entfernt sich als TSR, prüft jeden Sektor gegen eine Prüfsumme, fragt vor dem Start von Grafikprogrammen nach und sperrt das Laufwerk, wenn eines davon es beschädigt hat.

**DMVDEMO** – Dreizehn Bilder zeigen, was der µPD7220 auf der DMV kann: gerasterte Mischfarben, Zoom, zwei Bildbereiche, Linien und Flächen, ein Plasma, eine Mandelbrot-Spirale, ein KITT-Foto mit Scanner-Animation und Front-LEDs und zum Schluss eine Anaglyphe. Das Mandelbrot-Bild rechnet der schnellste vorhandene Prozessor: 68008 (K234), 8087 oder 8088/V20 (Schalter `/6`, `/7`, `/8`).

**DMVRACE** – Ein Rennen der Prozessoren, die in einer DMV stecken können: Z80 der Hauptplatine, 8088/V20, 8088/V20 mit 8087 und der 68008 auf der K234. Jeder Streckenabschnitt ist ein Test (Wurzeln, Sierpinski, Probedivision, Sieb, Mandelbrot); jeder Prozessor rechnet ihn tatsächlich und zeigt sein eigenes Ergebnisbild. Drei Runden: Die erste wird abschnittsweise gemessen, die zweite und dritte laufen in Echtzeit. Mit der Tastatur-Firmware mit Rennwerk folgt ein Drag-Race: zwei Schnecken (8741 der Tastatur, Z80) gegen einen Windhund (8088/V20).

**MOUSETST** – Ein Koordinatenkreuz mit dem Nullpunkt in der Bildmitte, auf dem die Position des Maus-Adapters K806 in alle Richtungen als Spur erscheint, mit Maßstäben von 1 bis 8 Zählschritten pro Bildpunkt und Begrenzung am Bildrand oder frei. Es findet den K806 auf jeder IFSEL-Adresse und zeigt sie an. Die von der NCR-Diagnose erwarteten Werte für 5 Zoll stehen in der Beschreibung.

**DMVKERM** – ein kleines Kermit für die DMV, von Grund auf in Assembler geschrieben (eine `.COM`-Datei mit 22 KB): Terminal mit VT102-Teilmenge, Dateiübertragung mit Kermit (Client und Server, lange Pakete, CRC) und XMODEM (CRC, 1-KB-Blöcke), Skripte für die Einwahl in eine Mailbox, über die seriellen Module K211/K212/K213, K801 und K215, unter NCR MS-DOS 2.11 und den DMV-Ports von MS-DOS 3.30 und DR-DOS 3.41 (in MAME unter 3.30 gegen MS-Kermit auf einem IBM XT getestet). Es findet die Module und ihren Interrupt selbst; CTRL-CLR ist die Fluchttaste. Es steht auf `GOODIES.IMG`; auf der eigenen Diskette `DMVKERM.IMG` liegt zusätzlich das originale Kermit-MS 2.32 für IBM-kompatible PCs für die Gegenseite.

**DMVGRAF** – Eine C-Grafikbibliothek für den µPD7220 nach Art von NCRGRAF und TurboGraf: Punkte, Linien, Kreise, Bögen, Rechtecke, Text, Muster, Bilder in mehreren Formaten (auch in den freien Segmenten einer K208), Zusammensetzen überlappender Bilder abseits des Schirms, ein Mauszeiger mit gesichertem Hintergrund, Maus, Tastatur und Ton. Für CP/M-8000 auf der K238 (in MAME getestet), CP/M-68K auf der K234 und MS-DOS (alle drei mit Mahjongg in MAME getestet); eine PC-Simulation schreibt den Schirm als PPM-Datei, und in `dmvgraf/lib/` liegen fertige Bibliotheken für alle drei Systeme; `dmvgraf/pascal/` enthält eine Fassung für Turbo Pascal 3. Mahjongg in [rfka01/dmv_games](https://github.com/rfka01/dmv_games) baut darauf.

## Requirements · Voraussetzungen

NCR Decision Mate V with 8088/V20 card (K230/K231/K235). DMVDEMO and DMVRACE run under MS-DOS 2.0 or later (tested with 3.30), MOUSETST under MS-DOS 2.0 or later with a K806 mouse adapter, ECODRIVE needs MS-DOS 3.1–3.3. Colour graphics board for ECODRIVE and the colour screens. · DMVDEMO und DMVRACE laufen ab MS-DOS 2.0 (getestet mit 3.30), MOUSETST ab MS-DOS 2.0 mit Maus-Adapter K806, ECODRIVE braucht MS-DOS 3.1–3.3. Tested in MAME (driver `dmv`, fork [rfka01/mame](https://github.com/rfka01/mame)).

## License · Lizenz

MIT License, see [LICENSE](LICENSE). **Exceptions:** Kermit-MS 2.32 for IBM PCs on `DMVKERM.IMG` is under the Revised 3-Clause BSD License of Columbia University, see [dmvkerm/KERMIT.LIC](dmvkerm/KERMIT.LIC); `KITT.PIC` is an edited photo by Poudou99 (Wikimedia Commons) under CC BY-SA 4.0, see [dmvdemo/KITT-LICENSE.txt](dmvdemo/KITT-LICENSE.txt).

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01

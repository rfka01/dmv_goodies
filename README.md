# DMV Goodies

Programs for the **NCR Decision Mate V** (DMV) under MS-DOS · Programme für die **NCR Decision Mate V** unter MS-DOS

| | |
|---|---|
| [`ecodrive/`](ecodrive/) | RAM disk in the unused red/blue video memory · RAM-Disk im ungenutzten Rot/Blau-Bildspeicher |
| [`dmvdemo/`](dmvdemo/) | Graphics demo for the µPD7220 · Grafikdemo für den µPD7220 |
| [`dmvrace/`](dmvrace/) | Processor race Z80 / 8088 / 8087 / 68008 · Prozessor-Rennen |
| [`mousetst/`](mousetst/) | Graphical test for the K806 mouse adapter · Grafischer Test für den Maus-Adapter K806 |
| [`kermit/`](kermit/) | MS-DOS Kermit 3.16 for the K211/K212/K801 serial modules · MS-DOS Kermit 3.16 für die seriellen Module K211/K212/K801 |
| [`Images/`](Images/) | `GOODIES.IMG`: 360 KB DMV disk with all programs, documentation and KITT.PIC · DMV-Diskette mit allen Programmen, Beschreibungen und KITT.PIC; `KERMIT.IMG`: Kermit disk · Kermit-Diskette |

The directories hold the sources, the disk image holds the ready-to-run files. · In den Verzeichnissen liegen die Quellen, auf dem Diskettenabbild die fertigen Programme.

## English

**ECODRIVE** – The colour graphics board has 96 KB video memory in three planes; text mode and green graphics only use the green one. ECODRIVE puts a 62 KB drive into the red and blue planes. It loads and unloads as a TSR, checks every sector against a checksum, asks before graphics programs are started and locks the drive if one of them damaged it.

**DMVDEMO** – Thirteen screens that show what the µPD7220 can do on the DMV: dithered colours, zoom, two display partitions, line and area drawing, a plasma, a Mandelbrot spiral, a KITT photo with scanner animation and the front LEDs, and an anaglyph at the end. The Mandelbrot screen is computed by the fastest available processor: 68008 (K234), 8087 or 8088/V20 (switches `/6`, `/7`, `/8`).

**DMVRACE** – A race between the processors a DMV can hold: Z80 on the mainboard, 8088/V20, 8088/V20 with 8087 and the 68008 on the K234. Every track section is a benchmark (square roots, Sierpinski, trial division, sieve, Mandelbrot); each processor really computes it and shows its own result picture. Three laps: the first is measured section by section, the second and third run in real time. With the keyboard firmware that has the race engine, a drag race follows: two snails (8741 keyboard controller, Z80) against a greyhound (8088/V20).

**MOUSETST** – A coordinate cross with the origin in the middle of the screen that traces the position of the K806 mouse adapter in all directions, with scales from 1 to 8 counts per pixel and limits at the screen edge or free. It finds the K806 on any IFSEL address and shows it. The NCR diagnostic program's expected counts for 5 inches are listed in the description.

**KERMIT** – MS-DOS Kermit 3.16 for the DMV: VT102/VT220/VT320 terminal and Kermit file transfer (client and server) over the K211/K212/K213 and K801 serial modules, under the DMV ports of MS-DOS 3.30, NCR MS-DOS 2.11 and DR-DOS 3.41 with 256 KB or more (so far tested under 3.30 in an emulator test bench). It finds the modules and their interrupt by itself and polls without an interrupt controller; CTRL-CLR is the escape key. Its own disk `KERMIT.IMG` also holds MS-Kermit for IBM-compatible PCs for the other end of the cable. Kermit keeps its Columbia BSD license, see [kermit/KERMIT.LIC](kermit/KERMIT.LIC).

## Deutsch

**ECODRIVE** – Die Farbgrafikkarte hat 96 KB Bildspeicher in drei Ebenen; Textbetrieb und grüne Grafik nutzen nur die grüne. ECODRIVE legt in Rot und Blau ein Laufwerk mit 62 KB ab. Es lädt und entfernt sich als TSR, prüft jeden Sektor gegen eine Prüfsumme, fragt vor dem Start von Grafikprogrammen nach und sperrt das Laufwerk, wenn eines davon es beschädigt hat.

**DMVDEMO** – Dreizehn Bilder zeigen, was der µPD7220 auf der DMV kann: gerasterte Mischfarben, Zoom, zwei Bildbereiche, Linien und Flächen, ein Plasma, eine Mandelbrot-Spirale, ein KITT-Foto mit Scanner-Animation und Front-LEDs und zum Schluss eine Anaglyphe. Das Mandelbrot-Bild rechnet der schnellste vorhandene Prozessor: 68008 (K234), 8087 oder 8088/V20 (Schalter `/6`, `/7`, `/8`).

**DMVRACE** – Ein Rennen der Prozessoren, die in einer DMV stecken können: Z80 der Hauptplatine, 8088/V20, 8088/V20 mit 8087 und der 68008 auf der K234. Jeder Streckenabschnitt ist ein Test (Wurzeln, Sierpinski, Probedivision, Sieb, Mandelbrot); jeder Prozessor rechnet ihn tatsächlich und zeigt sein eigenes Ergebnisbild. Drei Runden: Die erste wird abschnittsweise gemessen, die zweite und dritte laufen in Echtzeit. Mit der Tastatur-Firmware mit Rennwerk folgt ein Drag-Race: zwei Schnecken (8741 der Tastatur, Z80) gegen einen Windhund (8088/V20).

**MOUSETST** – Ein Koordinatenkreuz mit dem Nullpunkt in der Bildmitte, auf dem die Position des Maus-Adapters K806 in alle Richtungen als Spur erscheint, mit Maßstäben von 1 bis 8 Zählschritten pro Bildpunkt und Begrenzung am Bildrand oder frei. Es findet den K806 auf jeder IFSEL-Adresse und zeigt sie an. Die von der NCR-Diagnose erwarteten Werte für 5 Zoll stehen in der Beschreibung.

**KERMIT** – MS-DOS Kermit 3.16 für die DMV: Terminal (VT102/VT220/VT320) und Dateiübertragung nach dem Kermit-Protokoll (auch als Server) über die seriellen Module K211/K212/K213 und K801, unter den DMV-Ports von MS-DOS 3.30, NCR MS-DOS 2.11 und DR-DOS 3.41 ab 256 KB (bisher unter 3.30 in einem Emulator-Prüfstand getestet). Es findet die Module und ihren Interrupt selbst und fragt ohne Interruptcontroller ab; CTRL-CLR ist die Fluchttaste. Auf der eigenen Diskette `KERMIT.IMG` liegt zusätzlich MS-Kermit für IBM-kompatible PCs für die Gegenseite. Kermit behält seine BSD-Lizenz der Columbia University, siehe [kermit/KERMIT.LIC](kermit/KERMIT.LIC).

## Requirements · Voraussetzungen

NCR Decision Mate V with 8088/V20 card (K230/K231/K235). DMVDEMO and DMVRACE run under MS-DOS 2.0 or later (tested with 3.30), MOUSETST under MS-DOS 2.0 or later with a K806 mouse adapter, ECODRIVE needs MS-DOS 3.1–3.3. Colour graphics board for ECODRIVE and the colour screens. · DMVDEMO und DMVRACE laufen ab MS-DOS 2.0 (getestet mit 3.30), MOUSETST ab MS-DOS 2.0 mit Maus-Adapter K806, ECODRIVE braucht MS-DOS 3.1–3.3. Tested in MAME (driver `dmv`, fork [rfka01/mame](https://github.com/rfka01/mame)).

## License · Lizenz

MIT License, see [LICENSE](LICENSE). **Exceptions:** Kermit (`kermit/`) is under the Revised 3-Clause BSD License of Columbia University (DMV additions MIT), see [kermit/KERMIT.LIC](kermit/KERMIT.LIC); `KITT.PIC` is an edited photo by Poudou99 (Wikimedia Commons) under CC BY-SA 4.0, see [dmvdemo/KITT-LICENSE.txt](dmvdemo/KITT-LICENSE.txt).

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01

# DMV Goodies

Programs for the **NCR Decision Mate V** (DMV) under MS-DOS 3.3 · Programme für die **NCR Decision Mate V** unter MS-DOS 3.3

| | |
|---|---|
| [`ecodrive/`](ecodrive/) | RAM disk in the unused red/blue video memory · RAM-Disk im ungenutzten Rot/Blau-Bildspeicher |
| [`dmvdemo/`](dmvdemo/) | Graphics demo for the µPD7220 · Grafikdemo für den µPD7220 |
| [`dmvrace/`](dmvrace/) | Processor race Z80 / 8088 / 8087 / 68008 · Prozessor-Rennen |
| [`Images/`](Images/) | `GOODIES.IMG`: 360 KB DMV disk with all programs, documentation and KITT.PIC · DMV-Diskette mit allen Programmen, Beschreibungen und KITT.PIC |

The directories hold the sources, the disk image holds the ready-to-run files. · In den Verzeichnissen liegen die Quellen, auf dem Diskettenabbild die fertigen Programme.

## English

**ECODRIVE** – The colour graphics board has 96 KB video memory in three planes; text mode and green graphics only use the green one. ECODRIVE puts a 62 KB drive into the red and blue planes. It loads and unloads as a TSR, checks every sector against a checksum, asks before graphics programs are started and locks the drive if one of them damaged it.

**DMVDEMO** – Thirteen screens that show what the µPD7220 can do on the DMV: dithered colours, zoom, two display partitions, line and area drawing, a plasma, a Mandelbrot spiral, a KITT photo with scanner animation and the front LEDs, and an anaglyph at the end. The Mandelbrot screen is computed by the fastest available processor: 68008 (K234), 8087 or 8088/V20 (switches `/6`, `/7`, `/8`).

**DMVRACE** – A race between the processors a DMV can hold: Z80 on the mainboard, 8088/V20, 8088/V20 with 8087 and the 68008 on the K234. Every track section is a benchmark (square roots, Sierpinski, trial division, sieve, Mandelbrot); each processor really computes it and shows its own result picture. Three laps: the first is measured section by section, the second and third run in real time.

## Deutsch

**ECODRIVE** – Die Farbgrafikkarte hat 96 KB Bildspeicher in drei Ebenen; Textbetrieb und grüne Grafik nutzen nur die grüne. ECODRIVE legt in Rot und Blau ein Laufwerk mit 62 KB ab. Es lädt und entfernt sich als TSR, prüft jeden Sektor gegen eine Prüfsumme, fragt vor dem Start von Grafikprogrammen nach und sperrt das Laufwerk, wenn eines davon es beschädigt hat.

**DMVDEMO** – Dreizehn Bilder zeigen, was der µPD7220 auf der DMV kann: gerasterte Mischfarben, Zoom, zwei Bildbereiche, Linien und Flächen, ein Plasma, eine Mandelbrot-Spirale, ein KITT-Foto mit Scanner-Animation und Front-LEDs und zum Schluss eine Anaglyphe. Das Mandelbrot-Bild rechnet der schnellste vorhandene Prozessor: 68008 (K234), 8087 oder 8088/V20 (Schalter `/6`, `/7`, `/8`).

**DMVRACE** – Ein Rennen der Prozessoren, die in einer DMV stecken können: Z80 der Hauptplatine, 8088/V20, 8088/V20 mit 8087 und der 68008 auf der K234. Jeder Streckenabschnitt ist ein Test (Wurzeln, Sierpinski, Probedivision, Sieb, Mandelbrot); jeder Prozessor rechnet ihn tatsächlich und zeigt sein eigenes Ergebnisbild. Drei Runden: Die erste wird abschnittsweise gemessen, die zweite und dritte laufen in Echtzeit.

## Requirements · Voraussetzungen

NCR Decision Mate V with 8088/V20 card (K230/K231/K235), MS-DOS 3.3; colour graphics board for ECODRIVE and the colour screens. Tested in MAME (driver `dmv`, fork [rfka01/mame](https://github.com/rfka01/mame)).

## License · Lizenz

MIT License, see [LICENSE](LICENSE). **Exception:** `KITT.PIC` is an edited photo by Poudou99 (Wikimedia Commons) under CC BY-SA 4.0, see [dmvdemo/KITT-LICENSE.txt](dmvdemo/KITT-LICENSE.txt).

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01

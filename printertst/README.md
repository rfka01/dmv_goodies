# PRINTERTST

*Deutsch weiter unten.*

## English

Test files for the printers that the [rfka01/mame](https://github.com/rfka01/mame) fork emulates on the **NCR Decision Mate V** (K210 Centronics module, K801 serial module for the EX-800). The ready-made disk is [`Images/DMVPRN.IMG`](../Images/) (360 KB, MS-DOS). Send every file with `COPY /B` (the graphics files contain the byte 1Ah):

    COPY /B MX80.PRN LPT1:

| File | Printer | Contents |
|---|---|---|
| `BILD.PRN` | all | ASCII picture, plain text |
| `ZEILEN.PRN` | all | 100 numbered lines and a form feed: page breaks, single sheets, green bar paper |
| `FX80FONT.PRN` | Epson FX-80 (LX-810L) | fonts and attributes, ESC !, international sets, italic set A0h–FEh |
| `JX80FONT.PRN` | Epson JX-80 | as FX-80, plus seven colours |
| `LX800FNT.PRN` | Epson LX-800 | as FX-80, plus NLQ |
| `EX800FNT.PRN` | Epson EX-800 | as FX-80, plus NLQ Roman and Sans Serif |
| `NL10FONT.PRN` | Star NL-10 (IBM cartridge) | fonts, NLQ, IBM character set 2 |
| `MD16FONT.PRN` | Nixdorf MD 16 (Epson mode) | fonts, NLQ Roman/Script |
| `MX80.PRN` | Epson MX-80 (all ROM sets) | basic command set: SI, SO, ESC E/G, ESC 0/1/2/A, characters, ruler |
| `MX80GTRX.PRN` | Epson MX-80 Graftrax | bit image ESC L/K, bidirectional alignment, ESC A n, ESC R, ESC 4 |
| `MX80DP.PRN` | Epson MX-80 Dots Perfect | attributes within a line, underline, italics, ESC W, super/subscript, bit image |
| `GRAFIK.PRN` | 9-pin Epson compatibles | bit image ESC L (dithered Mandelbrot), grey wedge, ESC K line art |
| `JX80GRAF.PRN` | Epson JX-80 | colour bit image graphics, colour bars |
| `PRNTEST.BAT` | all | prints `BILD.PRN` and `GRAFIK.PRN` |
| `LIESMICH.TXT` | | description (German, DOS text) |

The MX-80 "Graftrax" ROMs in MAME know neither ESC 3 nor ESC 4 and apply the print mode to a whole line, so `MX80GTRX.PRN` uses ESC A 8 for the graphics. On the Dots Perfect, NLQ is selected with DIP switch SW2-2.

`mkprn.py` builds the `.PRN` files (`python3 mkprn.py <directory>`), `mkimg.py` the FAT12 disk image (`python3 mkimg.py DMVPRN.IMG DMVPRNTEST <files>`), `printer.txt` is the picture for `BILD.PRN`.

## Deutsch

Testdateien für die Drucker, die der Fork [rfka01/mame](https://github.com/rfka01/mame) an der **NCR Decision Mate V** emuliert (Centronics-Modul K210, für den EX-800 auch das serielle Modul K801). Die fertige Diskette ist [`Images/DMVPRN.IMG`](../Images/) (360 KB, MS-DOS). Alle Dateien mit `COPY /B` an den Drucker schicken (die Grafikdateien enthalten das Byte 1Ah):

    COPY /B MX80.PRN LPT1:

Die Tabelle oben gilt sinngemäß; `LIESMICH.TXT` auf der Diskette beschreibt alle Dateien und die MAME-Optionen.

Die MX-80-„Graftrax“-ROMs in MAME kennen weder ESC 3 noch ESC 4 und wenden die Druckart auf die ganze Zeile an; `MX80GTRX.PRN` nimmt für die Grafik deshalb ESC A 8. Beim Dots Perfect wird NLQ mit DIP-Schalter SW2-2 eingeschaltet.

`mkprn.py` erzeugt die `.PRN`-Dateien, `mkimg.py` das FAT12-Diskettenabbild, `printer.txt` ist das Bild für `BILD.PRN`.

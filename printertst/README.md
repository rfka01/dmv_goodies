# PRINTERTST

*Deutsch weiter unten.*

## English

Test files for the printers that the [rfka01/mame](https://github.com/rfka01/mame) fork emulates on the **NCR Decision Mate V** (K210 Centronics module, K801 serial module for the EX-800). The ready-made disk is [`Images/DMVPRN.IMG`](../Images/) (360 KB FAT12). It works under MS-DOS 2.11 on the DMV and under MS-DOS/PC-DOS on IBM compatibles. Commands, file names and texts on the disk are in English.

### Use

    PRINTERS          list of the printers
    <printer>         black-and-white test, e.g.  FX85
    <printer> COLOUR  colour test on colour printers, e.g.  JX80 COLOUR (COLOR or C work too)
    TEXT              PICTURE.PRN and LINES.PRN (all printers)
    PRNTEST           short test PICTURE.PRN and GRAPHICS.PRN (9-pin printers)

The batch files send the files with `COPY /B` to `LPT1:` and use only what MS-DOS 2.x already knows. By hand (the `/B` matters, the graphics files contain the byte 1Ah):

    COPY /B FX85FONT.PRN LPT1:

| Batch file | Printer | Files |
|---|---|---|
| `MX80` | Epson MX-80 | `MX80.PRN` |
| `MX80GT` | Epson MX-80 Graftrax | `MX80.PRN`, `MX80GTRX.PRN` |
| `MX80DP` | Epson MX-80 Dots Perfect | `MX80DP.PRN` |
| `RX80` | Epson RX-80 | `RX80FONT.PRN`, `GRAPHICS.PRN` |
| `FX80` | Epson FX-80 | `FX80FONT.PRN`, `GRAPHICS.PRN` |
| `JX80` | Epson JX-80 | `FX80FONT.PRN`, `GRAPHICS.PRN`; colour: `JX80FONT.PRN`, `COLOUR.PRN`, `COLGRAPH.PRN` |
| `FX85` | Epson FX-85 | `FX85FONT.PRN`, `GRAPHICS.PRN` |
| `LX86` | Epson LX-86 | `LX86FONT.PRN`, `GRAPHICS.PRN` |
| `LX800` | Epson LX-800 | `LX800FNT.PRN`, `GRAPHICS.PRN` |
| `LX810L` | Epson LX-810L / ActionPrinter 2000 | `FX80FONT.PRN`, `GRAPHICS.PRN` |
| `EX800` | Epson EX-800 | `EX800FNT.PRN`, `GRAPHICS.PRN`; colour (colour option): `COLOUR.PRN`, `COLGRAPH.PRN` |
| `FX880` | Epson FX-880 | `FX880FNT.PRN`, `FX880GRF.PRN` |
| `AP3250` | Epson ActionPrinter 3250 (LQ-100) | `AP32FNT.PRN`, `AP32GRF.PRN` |
| `C200GX` | Citizen 200GX | `C200FNT.PRN`, `GRAPHICS.PRN`; colour (colour kit, DIP SW2-8): `COLOUR.PRN`, `COLGRAPH.PRN` |
| `BX80` | BMC BX-80 | `BX80FONT.PRN`, `GRAPHICS.PRN` |
| `GEM10X` | Star Gemini-10X | `GEM10FNT.PRN`, `GRAPHICS.PRN` |
| `NL10` | Star NL-10 (IBM cartridge) | `NL10FONT.PRN` |
| `MD16` | Nixdorf MD 16 | `MD16FONT.PRN`, `GRAPHICS.PRN` |

The font pages only use commands the printer knows: the RX-80 has no `ESC !`, Elite or proportional spacing, the BX-80 and the Gemini-10X neither `ESC M`, `ESC p` nor `ESC R` (on the Gemini `ESC M` sets the left margin). The colour files select the colours with `ESC r n` (0 black, 1 magenta, 2 cyan, 3 violet, 4 yellow, 5 orange, 6 green).

### Building the disk

    ./mkdisk.sh

builds `../Images/DMVPRN.IMG` from `mkprn.py` (the `.PRN` files; `printer.txt` is the picture for `PICTURE.PRN`), `mkbat.py` (the batch files) and `README.TXT` (DOS text, CRLF); `mkimg.py` writes the FAT12 image.

## Deutsch

Testdateien für die Drucker, die der Fork [rfka01/mame](https://github.com/rfka01/mame) an der **NCR Decision Mate V** emuliert (Centronics-Modul K210, für den EX-800 auch das serielle Modul K801). Die fertige Diskette ist [`Images/DMVPRN.IMG`](../Images/) (360 KB, FAT12). Sie läuft unter MS-DOS 2.11 auf der DMV und unter MS-DOS/PC-DOS auf IBM-kompatiblen Rechnern.

    PRINTERS          Liste der Drucker
    <Drucker>         Test in Schwarzweiß, z. B.  FX85
    <Drucker> COLOUR  Farbtest bei Farbdruckern, z. B.  JX80 COLOUR (auch COLOR oder C)
    TEXT              PICTURE.PRN und LINES.PRN (alle Drucker)
    PRNTEST           Kurztest PICTURE.PRN und GRAPHICS.PRN (9-Nadel-Drucker)

Die Batchdateien schicken die Dateien mit `COPY /B` an `LPT1:` und kommen mit MS-DOS 2.x aus. Farbe gibt es beim JX-80, beim EX-800 mit Farboption und beim Citizen 200GX mit Farbkit (DIP-Schalter SW2-8). Die Tabelle oben gilt sinngemäß; `README.TXT` auf der Diskette (englisch) beschreibt alle Dateien und die MAME-Optionen. Befehle, Dateinamen und Texte der Diskette sind englisch.

`./mkdisk.sh` baut die Diskette `../Images/DMVPRN.IMG` neu aus `mkprn.py`, `mkbat.py` und `README.TXT`.

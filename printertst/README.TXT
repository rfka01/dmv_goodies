DMV printer test disk (2 November 2026)
=======================================

The disk runs under MS-DOS 2.11 on the NCR Decision Mate V and under
MS-DOS/PC-DOS on IBM compatibles (360 KB, FAT12).

Use
---
PRINTERS            shows the list of printers
<printer>           prints the black and white test, e.g.  FX85
<printer> COLOUR    prints the colour test on colour printers,
                    e.g.  JX80 COLOUR  (COLOR or C work as well)
TEXT                PICTURE.PRN and LINES.PRN (all printers)
PRNTEST             short test PICTURE.PRN and GRAPHICS.PRN (9-pin)

The batch files send the files to LPT1: with COPY /B. By hand it works
the same way (the /B matters: the graphics files contain the byte 1Ah,
which COPY otherwise takes as the end of the file):

    COPY /B FX85FONT.PRN LPT1:

For the EX-800 on the serial module K801 use the COM port instead of
LPT1:.

Colour printers
---------------
JX80     Epson JX-80 (the colour ribbon is always fitted)
EX800    Epson EX-800 with colour option (MAME: setting "Colour option")
C200GX   Citizen 200GX with colour kit (DIP switch SW2-8
         "Colour printing")

ESC r n selects the colour: 0 black, 1 magenta (red), 2 cyan (blue),
3 violet, 4 yellow, 5 orange, 6 green. The printer prints mixed colours
in two passes.

Files
-----
PICTURE.PRN    all                ASCII picture (plain text)
LINES.PRN      all                100 numbered lines and FF
GRAPHICS.PRN   9-pin, Epson       bit image ESC L 120 dpi, grey wedge,
                                  line graphics ESC K
COLOUR.PRN     colour printers    colour text, colour bars (ESC r)
COLGRAPH.PRN   colour printers    colour graphics: Mandelbrot set in
                                  the ribbon colours, colour bars
FX80FONT.PRN   FX-80, LX-810L     fonts, attributes, ESC !,
                                  international sets, italic set
JX80FONT.PRN   JX-80              like FX-80, plus 7 colours
FX85FONT.PRN   FX-85              like FX-80, plus NLQ
LX86FONT.PRN   LX-86              like FX-80, plus NLQ
LX800FNT.PRN   LX-800             like FX-80, plus NLQ
EX800FNT.PRN   EX-800             like FX-80, plus NLQ Roman/Sans Serif
RX80FONT.PRN   RX-80              fonts and attributes (no ESC !,
                                  Elite or proportional spacing)
BX80FONT.PRN   BMC BX-80          fonts and attributes (no ESC !,
                                  ESC M, ESC p or ESC R)
GEM10FNT.PRN   Star Gemini-10X    fonts and attributes (Star command
                                  set: no ESC !, ESC M or ESC R)
C200FNT.PRN    Citizen 200GX      like FX-80, plus NLQ Roman, Sans
                                  Serif, Courier
FX880FNT.PRN   FX-880             like FX-80, plus NLQ Roman/Sans Serif
FX880GRF.PRN   FX-880             all bit image modes ESC * and ESC ^
AP32FNT.PRN    ActionPrinter 3250 ESC/P 2: fonts, scalable fonts
AP32GRF.PRN    ActionPrinter 3250 24-pin bit image graphics
NL10FONT.PRN   Star NL-10 (IBM)   fonts, NLQ, IBM character set 2
MD16FONT.PRN   Nixdorf MD 16      fonts, NLQ Roman/Script
MX80.PRN       MX-80 (all ROMs)   basic command set, character set,
                                  ruler
MX80GTRX.PRN   MX-80 Graftrax     bit image ESC L/K, ESC A n, ESC R
MX80DP.PRN     MX-80 Dots Perfect attributes, italics, bit image
                                  (NLQ with DIP switch SW2-2 ON)

MAME options (example: K210 in slot 5)
--------------------------------------
    -slot5 k210 -slot5:k210:centronics fx85 -printout print.pdf

Options: mx80 (BIOS sets for Graftrax), mx80dp, rx80, fx80, jx80, fx85,
lx86, lx800, lx810l, ap2000, ex800, fx880, ap3250, c200gx, bx80,
gem10x, nl10, md16

FX-85: two ROM sets, bios=v340 (default) or bios=u10 (Cyrillic).

Paper: in the menu Machine Configuration, device bitmap_printer:
format (fanfold 12"/11", A4, Letter), green bar paper, sheet feeder.
The PDF file is complete when MAME exits or the "printout" medium is
unloaded.

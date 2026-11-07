# DMVDEMO

*Deutsch weiter unten.*

## English

A graphics demo with fifteen screens for the **NCR Decision Mate V** under MS-DOS (2.0 or later, tested with 3.30). It shows what the µPD7220 graphics controller and the DMV colour board can do; on a monochrome DMV the colours appear as dither patterns. Tested in MAME and on two real DMVs (colour with 8088, mono with V20 and 8087, both with K235 and K234); the Z8001 (K238) exists only in MAME.

1. Title: "NCR" with the GDC character zoom, 3D layers and a gloss stripe
2. Remake of NCR's planning chart from DEMO5/DEMO7 (1983), in colour
3. Line moiré with hundreds of XOR lines
4. Interference of concentric XOR circles
5. Mystify: bouncing polylines with a trail
6. Dithering: 36 mixed colours from 8, then a sunset
7. Hardware scrolling across two display partitions, mirror, earthquake
8. Hardware magnifier: display zoom 2/4/8× with panning
9. Mandelbrot set 160 × 100
10. Plasma in the style of FRACTINT
11. The "blue tentacle", a Mandelbrot spiral, computed by the 68008 (K234), Z8001 (K238), 8087 or 8088/V20
12. KITT: a photo as a low-key image with a red scanner light that also runs on the DMV's eight front LEDs
13. Interwoven ribbons: four coloured bands braid around a swaying axis and flow down off the screen (hardware scrolling over a 408-line ring, one new line per frame)
14. Tutankhamun: a dithered photo of the burial mask
15. Finale: anaglyph cube above a scrolling text line in real text mode

Screen 11 is computed by the strongest processor found: a coprocessor in slot 7 (68008 on the K234, or Z8001 on the K238, a home-made card that exists only in the MAME fork), else the 8087, else the 8088/V20. `/7` leaves the coprocessor out, `/8` uses the 8088/V20 only (also on screens 9 and 10). Hardware is detected by the rules measured on two real machines (`GHW.INC` from DMVGRAF): V20 only if `PUSH 0` and `AAD 16` agree, 8087 by its control word, K234/K238 only on a K235, started by the Z80 with `OUT DFh`; the coprocessor computes 20 rows per start while the 8088 waits, and its time is taken from PIT channel 0, calibrated against channel 2 (`GZEIT.INC`). `DMVDEMO n` shows only screen n; `/M` and `/C` force monochrome or colour. Computing times are shown on screens 9–11.

`LIESMICH.TXT` is the full description (German), `BUILD.BAT` builds `DMVDEMO.COM` with MASM 5.10. `KITT.PIC` and `TUT.PIC` have to be in the current directory; they are packed full-screen images in the DMVGRAF format (LZSS), loaded with `GPIC.INC` from DMVGRAF.

## Deutsch

Eine Grafikdemo mit fünfzehn Bildern für die **NCR Decision Mate V** unter MS-DOS (ab 2.0, getestet mit 3.30). Sie zeigt, was der Grafikcontroller µPD7220 und die Farbkarte der DMV können; auf einer Mono-DMV erscheinen die Farben als Raster. In MAME und auf zwei echten DMV getestet (Farbe mit 8088, Mono mit V20 und 8087, beide mit K235 und K234); den Z8001 (K238) gibt es nur in MAME.

1. Titel: „NCR“ mit dem Zeichenzoom des GDC, 3D-Schichten und Glanzstreifen
2. Remake des NCR-Planungscharts aus DEMO5/DEMO7 (1983), in Farbe
3. Linien-Moiré aus Hunderten XOR-Linien
4. Interferenz konzentrischer XOR-Kreise
5. Mystify: springende Linienzüge mit Schweif
6. Dithering: 36 Mischfarben aus 8, danach ein Sonnenuntergang
7. Hardware-Scrolling über zwei Bildbereiche, Spiegel, Erdbeben
8. Hardware-Lupe: Anzeige-Zoom 2-, 4- und 8-fach mit Schwenk
9. Mandelbrotmenge 160 × 100
10. Plasma wie bei FRACTINT
11. Der „blaue Tentakel“, eine Mandelbrot-Spirale, gerechnet vom 68008 (K234), Z8001 (K238), 8087 oder 8088/V20
12. KITT: ein Foto als Low-Key-Bild mit rotem Scanner-Lauflicht, das gleichzeitig auf den acht Front-LEDs der DMV läuft
13. Verwobene Bänder: vier Farbbänder winden sich als Zopf um eine schwenkende Achse und laufen nach unten aus dem Bild (Hardware-Scrolling über einen Ring aus 408 Zeilen, eine neue Zeile je Bildwechsel)
14. Tutanchamun: gerastertes Foto der Totenmaske
15. Finale: Anaglyphen-Würfel über einer Laufschrift im echten Textmodus

Bild 11 rechnet das stärkste gefundene Rechenwerk: ein Koprozessor auf Steckplatz 7 (68008 der K234 oder Z8001 der K238, ein Eigenbau, den es nur im MAME-Fork gibt), sonst der 8087, sonst der 8088/V20. `/7` lässt den Koprozessor weg, `/8` nimmt nur den 8088/V20 (auch in Bild 9 und 10). Erkannt wird nach den auf zwei echten Maschinen gemessenen Regeln (`GHW.INC` aus DMVGRAF): V20 nur, wenn `PUSH 0` und `AAD 16` übereinstimmen, 8087 am Steuerwort, K234/K238 nur auf der K235, gestartet vom Z80 mit `OUT DFh`; der Koprozessor rechnet je Start 20 Zeilen, während der 8088 wartet, seine Zeit misst PIT-Kanal 0, geeicht gegen Kanal 2 (`GZEIT.INC`). `DMVDEMO n` zeigt nur Bild n; `/M` und `/C` erzwingen Mono oder Farbe. Die Rechenzeiten stehen in den Bildern 9–11.

`LIESMICH.TXT` ist die ausführliche Beschreibung, `BUILD.BAT` baut `DMVDEMO.COM` mit MASM 5.10. `KITT.PIC` und `TUT.PIC` müssen im aktuellen Verzeichnis liegen; es sind gepackte Vollbilder im DMVGRAF-Format (LZSS), geladen mit `GPIC.INC` aus DMVGRAF.

## Credits

- 8×8 font: font8x8 by Daniel Hepper, public domain.
- `KITT.PIC`: edited photo by Poudou99, Wikimedia Commons, **CC BY-SA 4.0**, see [KITT-LICENSE.txt](KITT-LICENSE.txt).
- `TUT.PIC`: edited photo "King Tut Burial Mask" by Mark Fischer, Wikimedia Commons, **CC BY-SA 2.0**, see [TUT-LICENSE.txt](TUT-LICENSE.txt).

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01 · MIT License (except KITT.PIC and TUT.PIC)

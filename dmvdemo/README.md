# DMVDEMO

*Deutsch weiter unten.*

## English

A graphics demo with thirteen screens for the **NCR Decision Mate V** under MS-DOS (2.0 or later, tested with 3.30). It shows what the µPD7220 graphics controller and the DMV colour board can do; on a monochrome DMV the colours appear as dither patterns.

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
11. The "blue tentacle", a Mandelbrot spiral, computed by the 68008, 8087 or 8088/V20
12. KITT: a photo as a low-key image with a red scanner light that also runs on the DMV's eight front LEDs
13. Finale: anaglyph cube above a scrolling text line in real text mode

The switch decides which processor may compute screen 11 at most before the 8088/V20 takes over: `/6` 68008 on the K234, `/7` 8087 (default), `/8` 8088/V20 only. `DMVDEMO n` shows only screen n; `/M` and `/C` force monochrome or colour. Computing times are shown on screens 9–11.

`LIESMICH.TXT` is the full description (German), `BUILD.BAT` builds `DMVDEMO.COM` with MASM 5.10. `KITT.PIC` has to be in the current directory.

## Deutsch

Eine Grafikdemo mit dreizehn Bildern für die **NCR Decision Mate V** unter MS-DOS (ab 2.0, getestet mit 3.30). Sie zeigt, was der Grafikcontroller µPD7220 und die Farbkarte der DMV können; auf einer Mono-DMV erscheinen die Farben als Raster.

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
11. Der „blaue Tentakel“, eine Mandelbrot-Spirale, gerechnet vom 68008, 8087 oder 8088/V20
12. KITT: ein Foto als Low-Key-Bild mit rotem Scanner-Lauflicht, das gleichzeitig auf den acht Front-LEDs der DMV läuft
13. Finale: Anaglyphen-Würfel über einer Laufschrift im echten Textmodus

Der Schalter legt fest, welcher Prozessor Bild 11 höchstens rechnen darf, bevor der 8088/V20 einspringt: `/6` 68008 auf der K234, `/7` 8087 (Voreinstellung), `/8` nur 8088/V20. `DMVDEMO n` zeigt nur Bild n; `/M` und `/C` erzwingen Mono oder Farbe. Die Rechenzeiten stehen in den Bildern 9–11.

`LIESMICH.TXT` ist die ausführliche Beschreibung, `BUILD.BAT` baut `DMVDEMO.COM` mit MASM 5.10. `KITT.PIC` muss im aktuellen Verzeichnis liegen.

## Credits

- 8×8 font: font8x8 by Daniel Hepper, public domain.
- `KITT.PIC`: edited photo by Poudou99, Wikimedia Commons, **CC BY-SA 4.0**, see [KITT-LICENSE.txt](KITT-LICENSE.txt).

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01 · MIT License (except KITT.PIC)

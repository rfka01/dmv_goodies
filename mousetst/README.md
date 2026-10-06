# MOUSETST

*Deutsch weiter unten.*

## English

A graphical test for the **K806 mouse adapter** of the **NCR Decision Mate V** under MS-DOS (2.0 or later). It draws a coordinate cross with the origin in the middle of the screen and traces the absolute position of the K806. Moving right and up gives positive values, as in the NCR diagnostic program. The top line shows X, Y, the pressed buttons (L, M, R), the scale and the limit mode; the bottom line shows the keys and the address of the K806.

- `MOUSETST` searches the K806 on all IFSEL addresses (30h = IFSEL 2A first, then 38h, 60h, 68h, 70h, 78h, B0h, B8h, C0h, C8h); `MOUSETST 38` uses address 38h only. A card counts as a K806 if it accepts command 02 and answers command 00 with exactly five bytes. During the search every address gets one command byte. To avoid this with other cards, give the address.
- Keys: `ESC` quit, `0` set the origin (the trace stays), `C` clear the trace and set the origin, `1` `2` `4` `8` counts per pixel, `B` limits at the screen edge (like a mouse driver) or free (−32768…32767).
- Ticks every 100 counts, long ticks every 500. The trace (up to 7000 positions) is redrawn when the scale changes.
- The K806 keeps its counters between XMIN/XMAX and YMIN/YMAX itself, but only stops when a counter reaches a limit exactly; MOUSETST moves a position that is already outside to the edge first.
- Works with the colour and the monochrome graphics board (green plane only).

Counts expected by the NCR diagnostic program (test 9, phase 6) for 5 inches right and 5 inches up: Alps and Mouse Systems Quad 400–600, Hawley Mark II and Logitech LM-P5 900–1100, Depraz Souris P4 and Logitech P4 1700–2000.

Pixels are written with WDAT in "set" mode (23h): MAME's µPD7220 ignores the mask register in "replace" mode (20h) and clears the other pixels of the word.

`MOUSETST.TXT` is the full description (German), `BUILD.BAT` builds `MOUSETST.COM` with MASM 5.10. The font is font8x8 by D. Hepper (public domain), converted from `dmvrace/FONT.INC`.

## Deutsch

Ein grafischer Test für den **Maus-Adapter K806** der **NCR Decision Mate V** unter MS-DOS (ab 2.0). Er zeichnet ein Koordinatenkreuz mit dem Nullpunkt in der Bildmitte und verfolgt die absolute Position des K806 als Spur. Nach rechts und nach oben werden die Werte positiv, wie in der NCR-Diagnose. Oben stehen X, Y, die gedrückten Tasten (L, M, R), der Maßstab und die Begrenzung, unten die Tastenhilfe und die Adresse des K806.

- `MOUSETST` sucht den K806 auf allen IFSEL-Adressen (zuerst 30h = IFSEL 2A, dann 38h, 60h, 68h, 70h, 78h, B0h, B8h, C0h, C8h); `MOUSETST 38` benutzt nur die Adresse 38h. Als K806 gilt eine Karte, die Befehl 02 annimmt und auf Befehl 00 genau fünf Bytes liefert. Bei der Suche erhält jede geprüfte Adresse ein Befehlsbyte. Wer das bei anderen Karten vermeiden will, gibt die Adresse an.
- Tasten: `ESC` Ende, `0` Nullpunkt setzen (die Spur bleibt), `C` Spur löschen und Nullpunkt setzen, `1` `2` `4` `8` Zählschritte pro Bildpunkt, `B` Begrenzung am Bildrand (wie ein Maustreiber) oder frei (−32768…32767).
- Skalenstriche alle 100 Zählschritte, lange alle 500. Die Spur (bis 7000 Positionen) wird beim Wechsel des Maßstabs neu gezeichnet.
- Der K806 hält seine Zähler selbst zwischen XMIN/XMAX und YMIN/YMAX, bleibt aber nur stehen, wenn ein Zähler die Grenze genau erreicht; eine Position, die schon außerhalb liegt, setzt MOUSETST vorher an den Rand.
- Läuft mit Farb- und Monografikkarte (nur die grüne Ebene).

Von der NCR-Diagnose (Test 9, Phase 6) erwartete Werte für 5 Zoll nach rechts und 5 Zoll nach oben: Alps und Mouse Systems Quad 400–600, Hawley Mark II und Logitech LM-P5 900–1100, Depraz Souris P4 und Logitech P4 1700–2000.

Bildpunkte werden mit WDAT im Modus „setzen“ (23h) geschrieben: Der µPD7220 in MAME beachtet im Modus „ersetzen“ (20h) die Maske nicht und löscht die übrigen Punkte des Wortes.

`MOUSETST.TXT` ist die ausführliche Beschreibung, `BUILD.BAT` baut `MOUSETST.COM` mit MASM 5.10. Der Zeichensatz ist font8x8 von D. Hepper (Public Domain), umgerechnet aus `dmvrace/FONT.INC`.

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01 · MIT License

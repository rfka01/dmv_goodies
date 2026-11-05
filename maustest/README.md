# MAUSTEST

*Deutsch weiter unten.*

## English

A test for the **K806 mouse adapter** of the **NCR Decision Mate V** under MS-DOS (2.0 or later), with two modes selected on the command line. It replaces `MOUSETST` from this repository and `MAUSTEST` 1.1 from the German MS-DOS 3.30 disks for the DMV: both used the same K806 access layer, so two programs meant two copies of it.

```
MAUSTEST           graphical: coordinate cross and trace
MAUSTEST /T        text: interrupts, status byte, counters
MAUSTEST 38        use address 38h (hex) instead of searching
MAUSTEST /T 38     both
```

Without an address the K806 is searched on all IFSEL addresses (30h = IFSEL 2A first, then 38h, 60h, 68h, 70h, 78h, B0h, B8h, C0h, C8h). A card counts as a K806 if it accepts command 02 and answers command 00 with exactly five bytes. During the search every address gets one command byte; to avoid this with other cards, give the address.

### Graphical (default)

Draws a coordinate cross with the origin in the middle of the screen and traces the absolute position of the K806. Moving right and up gives positive values, as in the NCR diagnostic program. The top line shows X, Y, the pressed buttons (L, M, R), the scale and the limit mode; the bottom line shows the keys and the address of the K806.

- Keys: `ESC` quit, `0` set the origin (the trace stays), `C` clear the trace and set the origin, `1` `2` `4` `8` counts per pixel, `B` limits at the screen edge (like a mouse driver) or free (−32768…32767).
- Ticks every 100 counts, long ticks every 500. The trace (up to 7000 positions) is redrawn when the scale changes.
- The K806 keeps its counters between XMIN/XMAX and YMIN/YMAX itself, but only stops when a counter reaches a limit exactly; MAUSTEST moves a position that is already outside to the edge first.
- Works with the colour and the monochrome graphics board (green plane only).

Counts expected by the NCR diagnostic program (test 9, phase 6) for 5 inches right and 5 inches up: Alps and Mouse Systems Quad 400–600, Hawley Mark II and Logitech LM-P5 900–1100, Depraz Souris P4 and Logitech P4 1700–2000.

Pixels are written with WDAT in "set" mode (23h): MAME's µPD7220 ignores the mask register in "replace" mode (20h) and clears the other pixels of the word.

### Text (`/T`)

Answers a different question: does the K806 interrupt reach the CPU at all? It needs no graphics board. One line is rewritten in place:

```
[QIS] 0A=  209 0D=    0 St=04 INT=n X=+00250 Y=+00000 T=... s=  0 a=  0
```

`[QIS]` are the three switches, a dash meaning off: `Q` the handler acknowledges, `I` module interrupts are on, `S` the query is locked against the handler. Then the interrupt counts for INT 0Ah (IRQ2) and INT 0Dh (IRQ5), the status byte with its INT bit, position and buttons, `s` (silent: the loop saw the INT bit set but no interrupt arrived) and `a` (aborted position queries). The header prints the 8259 mask — FFh means nothing can arrive. ESC repeats the counters in words, with the leftover bytes drained after an abort.

`s` is the one that matters: if it rises, the module pulls its line but the CPU does not vector — the mask, the vector, or a missing interrupt controller.

Both switches exist to demonstrate a failure:

- `Q` off: the DMV's 8259 is edge triggered (ICW1 = 17h) and the K806 holds its line until command 03 acknowledges it, so without acknowledging exactly **one** interrupt arrives and no more.
- `S` off: the firmware checks after each of the five position bytes whether a command has arrived and then aborts (K806 ROM 33378, 055h and 05Ch), so acknowledging from inside the handler destroys a query in progress — `a` starts counting, and the summary shows the bytes drained afterwards.

Status bits: 0 OBF, 1 IBF, 2 F0, 3 F1, 4 ROM, 5 RAM, 6 INT (active high). Bits 2 and 3 are *not* unused as the manual says — on the UPI-41 they come from the hardware: F0 is set when the firmware has processed an event (ROM 180h), F1 shows that the last write was a command (ROM 022h).

The text mode uses nothing beyond DOS 2.0 (AH=09h, 02h, 06h, 25h, 35h, 30h), so the same program measures under MS-DOS 2.11 and under 3.30.

`MAUSTEST.TXT` is the full description (German), `BUILD.BAT` builds `MAUSTEST.COM` with MASM 5.10. The font is font8x8 by D. Hepper (public domain), converted from `dmvrace/FONT.INC`.

## Deutsch

Ein Test für den **Maus-Adapter K806** der **NCR Decision Mate V** unter MS-DOS (ab 2.0), mit zwei Betriebsarten, die beim Aufruf gewählt werden. Er ersetzt `MOUSETST` aus diesem Repository und `MAUSTEST` 1.1 von den deutschen MS-DOS-3.30-Disketten für die DMV: beide benutzten dieselbe Zugriffsschicht auf den K806, zwei Programme hießen also zwei Kopien davon.

```
MAUSTEST           grafisch: Koordinatenkreuz und Spur
MAUSTEST /T        Text: Interrupts, Statusbyte, Zähler
MAUSTEST 38        Adresse 38h (hexadezimal) statt suchen
MAUSTEST /T 38     beides zusammen
```

Ohne Adressangabe wird der K806 auf allen IFSEL-Adressen gesucht (zuerst 30h = IFSEL 2A, dann 38h, 60h, 68h, 70h, 78h, B0h, B8h, C0h, C8h). Als K806 gilt eine Karte, die Befehl 02 annimmt und auf Befehl 00 genau fünf Bytes liefert. Bei der Suche erhält jede geprüfte Adresse ein Befehlsbyte; wer das bei anderen Karten vermeiden will, gibt die Adresse an.

### Grafisch (ohne Schalter)

Zeichnet ein Koordinatenkreuz mit dem Nullpunkt in der Bildmitte und verfolgt die absolute Position des K806 als Spur. Nach rechts und nach oben werden die Werte positiv, wie in der NCR-Diagnose. Oben stehen X, Y, die gedrückten Tasten (L, M, R), der Maßstab und die Begrenzung, unten die Tastenhilfe und die Adresse des K806.

- Tasten: `ESC` Ende, `0` Nullpunkt setzen (die Spur bleibt), `C` Spur löschen und Nullpunkt setzen, `1` `2` `4` `8` Zählschritte pro Bildpunkt, `B` Begrenzung am Bildrand (wie ein Maustreiber) oder frei (−32768…32767).
- Skalenstriche alle 100 Zählschritte, lange alle 500. Die Spur (bis 7000 Positionen) wird beim Wechsel des Maßstabs neu gezeichnet.
- Der K806 hält seine Zähler selbst zwischen XMIN/XMAX und YMIN/YMAX, bleibt aber nur stehen, wenn ein Zähler die Grenze genau erreicht; eine Position, die schon außerhalb liegt, setzt MAUSTEST vorher an den Rand.
- Läuft mit Farb- und Monografikkarte (nur die grüne Ebene).

Von der NCR-Diagnose (Test 9, Phase 6) erwartete Werte für 5 Zoll nach rechts und 5 Zoll nach oben: Alps und Mouse Systems Quad 400–600, Hawley Mark II und Logitech LM-P5 900–1100, Depraz Souris P4 und Logitech P4 1700–2000.

Bildpunkte werden mit WDAT im Modus „setzen“ (23h) geschrieben: Der µPD7220 in MAME beachtet im Modus „ersetzen“ (20h) die Maske nicht und löscht die übrigen Punkte des Wortes.

### Text (`/T`)

Beantwortet eine andere Frage: kommt der Interrupt des K806 überhaupt bei der CPU an? Dafür braucht es keine Grafikkarte. Eine Zeile wird laufend überschrieben:

```
[QIS] 0A=  209 0D=    0 St=04 INT=n X=+00250 Y=+00000 T=... s=  0 a=  0
```

`[QIS]` sind die drei Schalter, ein Strich heißt aus: `Q` der Behandler quittiert, `I` die Modulinterrupts sind an, `S` die Abfrage ist gegen den Behandler gesperrt. Dann die Interruptzähler für INT 0Ah (IRQ2) und INT 0Dh (IRQ5), das Statusbyte mit seinem INT-Bit, Position und Tasten, `s` (stumm: die Schleife sah das INT-Bit gesetzt, es kam aber kein Interrupt) und `a` (abgebrochene Positionsabfragen). Die Kopfzeile nennt die Maske des 8259 — FFh heißt, daß nichts kommen kann. Nach ESC stehen die Zähler noch einmal ausgeschrieben da, dazu die weggeräumten Restbytes.

`s` ist der wichtigste Zähler: steigt er, zieht das Modul seine Leitung, aber die CPU springt nicht — Maske, Vektor oder gar kein Interruptcontroller.

Beide Schalter sind da, um einen Fehlerfall vorzuführen:

- `Q` aus: der 8259 der DMV ist flankengesteuert (ICW1 = 17h) und der K806 hält seine Leitung, bis Befehl 03 sie quittiert — ohne Quittieren kommt also genau **ein** Interrupt und danach keiner mehr.
- `S` aus: die Firmware prüft nach jedem der fünf Positionsbytes, ob ein Befehl eingetroffen ist, und bricht dann ab (K806-ROM 33378, 055h und 05Ch) — ein Quittieren aus dem Behandler zerstört also eine laufende Abfrage, `a` zählt hoch, und der Abschluß nennt die weggeräumten Restbytes.

Statusbits: 0 OBF, 1 IBF, 2 F0, 3 F1, 4 ROM, 5 RAM, 6 INT (active high). Bit 2 und Bit 3 sind *nicht* unbenutzt, wie die Anleitung sagt — beim UPI-41 kommen sie aus der Hardware: F0 wird gesetzt, wenn die Firmware ein Ereignis verarbeitet hat (ROM 180h), F1 zeigt, daß der letzte Schreibzugriff ein Befehl war (ROM 022h).

Die Textbetriebsart benutzt nichts jenseits von DOS 2.0 (AH=09h, 02h, 06h, 25h, 35h, 30h), damit dasselbe Programm unter MS-DOS 2.11 und unter 3.30 messen kann.

`MAUSTEST.TXT` ist die ausführliche Beschreibung, `BUILD.BAT` baut `MAUSTEST.COM` mit MASM 5.10. Der Zeichensatz ist font8x8 von D. Hepper (Public Domain), umgerechnet aus `dmvrace/FONT.INC`.

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01 · MIT License

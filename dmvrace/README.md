# DMVRACE

*Deutsch weiter unten.*

## English

A processor race for the **NCR Decision Mate V** under MS-DOS 3.3. Four cars stand for the four processors a DMV can hold:

| Car | Processor |
|---|---|
| red | Z80 on the mainboard (4 MHz) |
| green | 8088 or V20 (K230/K231/K235), detected and labelled |
| yellow | the same with 8087 |
| cyan | 68008 on the K234 |

A missing processor leaves its car in the pit. Every section of the kidney-shaped track is a benchmark that each processor really computes, showing its own result as a picture:

| Section | Test | Picture |
|---|---|---|
| Square-root curve | lattice points in a quarter circle, Σ isqrt(R² − x²), R = 6000 | quarter circle |
| Sierpinski straight | Pascal's triangle mod 2 by shift and XOR, 256 bits | Sierpinski triangle |
| Division hairpin | prime factors of 50000…50399 by trial division | factor bars |
| Eratosthenes curve | sieve of Eratosthenes as in the BYTE benchmark 1981 | Ulam spiral |
| Mandelbrot chicane | Mandelbrot 40 × 25, depth 50 | colour cells |

Lap one is measured section by section: first all processors compute the test, then the cars drive until the fastest reaches the end of the section. Laps two and three run without a break. Everything is in real time (one frame = 1/50 s of measured computing time). A wrong checksum disqualifies the car.

`DMVRACE /Z` without Z80, `/K` without 68008, `/8` without 8087, `/M` and `/C` force monochrome or colour.

`LIESMICH.TXT` is the full description (German), `BUILD.BAT` builds `DMVRACE.COM` with MASM 5.10. The Z80 and 68008 kernels are included as sources (`RACEZ80.ASM`, `RACE68.S`) and as assembled bytes (`Z80.INC`, `R68.INC`), so no Z80 or 68000 assembler is needed.

## Deutsch

Ein Prozessor-Rennen für die **NCR Decision Mate V** unter MS-DOS 3.3. Vier Wagen stehen für die vier Prozessoren, die in einer DMV stecken können:

| Wagen | Prozessor |
|---|---|
| rot | Z80 auf der Hauptplatine (4 MHz) |
| grün | 8088 bzw. V20 (K230/K231/K235), wird erkannt und beschriftet |
| gelb | derselbe mit 8087 |
| cyan | 68008 auf der K234 |

Fehlt ein Prozessor, bleibt sein Wagen in der Box. Jeder Abschnitt der nierenförmigen Strecke ist ein Test, den jeder Prozessor tatsächlich rechnet und dessen Ergebnis er als Bild zeigt:

| Abschnitt | Test | Bild |
|---|---|---|
| Wurzel-Kurve | Gitterpunkte im Viertelkreis, Σ isqrt(R² − x²), R = 6000 | Viertelkreis |
| Sierpinski-Gerade | Pascal-Dreieck modulo 2 durch Schieben und XOR, 256 Bit | Sierpinski-Dreieck |
| Divisions-Kehre | Primfaktoren von 50000…50399 durch Probedivision | Faktorbalken |
| Eratosthenes-Kurve | Sieb des Eratosthenes wie im BYTE-Benchmark 1981 | Ulam-Spirale |
| Mandelbrot-Schikane | Mandelbrot 40 × 25, Tiefe 50 | Farbzellen |

Die erste Runde wird abschnittsweise gemessen: Erst rechnen alle Prozessoren den Test, dann fahren die Wagen, bis der Schnellste das Abschnittsende erreicht. Runde zwei und drei folgen ohne Pause. Alles läuft in Echtzeit (ein Bildwechsel = 1/50 s gemessene Rechenzeit). Eine falsche Prüfsumme disqualifiziert den Wagen.

`DMVRACE /Z` ohne Z80, `/K` ohne 68008, `/8` ohne 8087, `/M` und `/C` erzwingen Mono oder Farbe.

`LIESMICH.TXT` ist die ausführliche Beschreibung, `BUILD.BAT` baut `DMVRACE.COM` mit MASM 5.10. Die Kerne für Z80 und 68008 liegen als Quelle (`RACEZ80.ASM`, `RACE68.S`) und fertig übersetzt (`Z80.INC`, `R68.INC`) bei; einen Z80- oder 68000-Assembler braucht man nicht.

## Credits

- 8×8 font: font8x8 by Daniel Hepper, public domain.

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01 · MIT License

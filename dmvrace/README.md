# DMVRACE

*Deutsch weiter unten.*

## English

A processor race for the **NCR Decision Mate V** under MS-DOS (2.0 or later, tested with 3.30). Four cars stand for the four processors a DMV can hold:

| Car | Processor |
|---|---|
| red | Z80 on the mainboard (4 MHz) |
| green | 8088 or V20 (K230/K231/K235), detected and labelled |
| yellow | the same with 8087 |
| cyan | coprocessor in slot 7: 68008 on the K234, or Z8001 on the K238 (home-made card, MAME fork only) |

A missing processor leaves its car in the pit. Every section of the kidney-shaped track is a benchmark that each processor really computes, showing its own result as a picture:

| Section | Test | Picture |
|---|---|---|
| Square-root curve | lattice points in a quarter circle, Σ isqrt(R² − x²), R = 6000 | quarter circle |
| Sierpinski straight | Pascal's triangle mod 2 by shift and XOR, 256 bits | Sierpinski triangle |
| Division hairpin | prime factors of 50000…50399 by trial division | factor bars |
| Eratosthenes curve | sieve of Eratosthenes as in the BYTE benchmark 1981 | Ulam spiral |
| Mandelbrot chicane | Mandelbrot 40 × 25, depth 50 | colour cells |

Lap one is measured section by section: first all processors compute the test, then the cars drive until the fastest reaches the end of the section. Laps two and three run without a break. Everything is in real time (one frame = 1/50 s of measured computing time). A wrong checksum disqualifies the car.

`DMVRACE /Z` without Z80, `/K` without K234/K238, `/8` without 8087, `/M` and `/C` force monochrome or colour.

### Drag race (second page)

Two snails and a greyhound on the division straight: the 8741 keyboard controller, the Z80 and the 8088/V20 factorise 50000…50399. The page is only offered if the keyboard controller runs the firmware with the race engine (`dmv_mb_8741_32678_synth.bin`, CRC32 `bcb29bec`: keyclick, synthesizer and race engine). DMVRACE probes for it at start-up (synthesizer command 05h, data pair 1Ah/01h, status 50h = race engine present) and aborts the probe at once; with any other firmware nothing is sent, or the synthesizer is only switched on and off.

After the Grand Prix, `D` starts the drag race. The 8741 really computes the test itself (a little over 23 s) and reports its progress, so its snail runs live; the Z80 snail and the greyhound run with their division times from lap one. While the 8741 computes, it does not scan the keyboard. At the end: the 8741's result (1388 = correct), the ranking and how many times faster the greyhound is than the keyboard.

`LIESMICH.TXT` is the full description (German), `BUILD.BAT` builds `DMVRACE.COM` with MASM 5.10. The Z80, 68008 and Z8001 kernels are included as sources (`RACEZ80.ASM`, `RACE68.S`, `RACEZ8K.8KN`) and as assembled bytes (`Z80.INC`, `R68.INC`, `RZ8.INC`), so no Z80, 68000 or Z8000 assembler is needed. Hardware detection follows the rules measured on two real machines (`GHW.INC` from DMVGRAF, shared with DMVDEMO and PITTEST): V20 only if `PUSH 0` and `AAD 16` agree, 8087 by its control word, K234/K238 only on a K235, started by the Z80 with `OUT DFh`. The timebase (PIT channel 0 = frame clock, about 56 Hz on real hardware) is calibrated against channel 2 at start-up (`GZEIT.INC`).

## Deutsch

Ein Prozessor-Rennen für die **NCR Decision Mate V** unter MS-DOS (ab 2.0, getestet mit 3.30). Vier Wagen stehen für die vier Prozessoren, die in einer DMV stecken können:

| Wagen | Prozessor |
|---|---|
| rot | Z80 auf der Hauptplatine (4 MHz) |
| grün | 8088 bzw. V20 (K230/K231/K235), wird erkannt und beschriftet |
| gelb | derselbe mit 8087 |
| cyan | Koprozessor auf Steckplatz 7: 68008 der K234 oder Z8001 der K238 (Eigenbau, nur im MAME-Fork) |

Fehlt ein Prozessor, bleibt sein Wagen in der Box. Jeder Abschnitt der nierenförmigen Strecke ist ein Test, den jeder Prozessor tatsächlich rechnet und dessen Ergebnis er als Bild zeigt:

| Abschnitt | Test | Bild |
|---|---|---|
| Wurzel-Kurve | Gitterpunkte im Viertelkreis, Σ isqrt(R² − x²), R = 6000 | Viertelkreis |
| Sierpinski-Gerade | Pascal-Dreieck modulo 2 durch Schieben und XOR, 256 Bit | Sierpinski-Dreieck |
| Divisions-Kehre | Primfaktoren von 50000…50399 durch Probedivision | Faktorbalken |
| Eratosthenes-Kurve | Sieb des Eratosthenes wie im BYTE-Benchmark 1981 | Ulam-Spirale |
| Mandelbrot-Schikane | Mandelbrot 40 × 25, Tiefe 50 | Farbzellen |

Die erste Runde wird abschnittsweise gemessen: Erst rechnen alle Prozessoren den Test, dann fahren die Wagen, bis der Schnellste das Abschnittsende erreicht. Runde zwei und drei folgen ohne Pause. Alles läuft in Echtzeit (ein Bildwechsel = 1/50 s gemessene Rechenzeit). Eine falsche Prüfsumme disqualifiziert den Wagen.

`DMVRACE /Z` ohne Z80, `/K` ohne K234/K238, `/8` ohne 8087, `/M` und `/C` erzwingen Mono oder Farbe.

### Drag-Race (zweite Seite)

Zwei Schnecken und ein Windhund auf der Divisions-Geraden: der 8741 der Tastatur, der Z80 und der 8088/V20 zerlegen 50000…50399 in Primfaktoren. Die Seite gibt es nur, wenn der Tastatur-Controller die Firmware mit Rennwerk hat (`dmv_mb_8741_32678_synth.bin`, CRC32 `bcb29bec`: Keyclick, Synthesizer und Rennwerk). DMVRACE fragt beim Start nach (Synthesizer-Befehl 05h, Datenpaar 1Ah/01h, Status 50h = Rennwerk da) und bricht den Probelauf sofort ab; mit jeder anderen Firmware wird nichts gesendet bzw. nur der Synthesizer kurz ein- und ausgeschaltet.

Nach dem Grand Prix startet `D` das Drag-Race. Der 8741 rechnet den Test wirklich selbst (gut 23 s) und meldet seinen Fortschritt, seine Schnecke fährt also live; die Z80-Schnecke und der Windhund fahren mit ihren Divisionszeiten aus Runde 1. Solange der 8741 rechnet, fragt er die Tastatur nicht ab. Am Ende: Ergebnis des 8741 (1388 = richtig), Rangfolge und wievielmal schneller der Windhund als die Tastatur ist.

`LIESMICH.TXT` ist die ausführliche Beschreibung, `BUILD.BAT` baut `DMVRACE.COM` mit MASM 5.10. Die Kerne für Z80, 68008 und Z8001 liegen als Quelle (`RACEZ80.ASM`, `RACE68.S`, `RACEZ8K.8KN`) und fertig übersetzt (`Z80.INC`, `R68.INC`, `RZ8.INC`) bei; einen Z80-, 68000- oder Z8000-Assembler braucht man nicht. Die Hardware wird nach den auf zwei echten Maschinen gemessenen Regeln erkannt (`GHW.INC` aus DMVGRAF, gemeinsam mit DMVDEMO und PITTEST): V20 nur, wenn `PUSH 0` und `AAD 16` übereinstimmen, 8087 am Steuerwort, K234/K238 nur auf der K235, gestartet vom Z80 mit `OUT DFh`. Die Zeitbasis (PIT-Kanal 0 = Bildtakt, auf echter Hardware rund 56 Hz) wird beim Start gegen Kanal 2 geeicht (`GZEIT.INC`).

## Credits

- 8×8 font: font8x8 by Daniel Hepper, public domain.

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01 · MIT License

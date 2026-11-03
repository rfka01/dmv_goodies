# PITTEST

*Deutsch weiter unten.*

## English

PITTEST measures the time base and the processor of an **NCR Decision Mate V** under MS-DOS (point 5 of the HWCONFIRM test disk). Everything is measured against 8253 channel 2, which DOS runs in mode 3 (one count = 1 µs); nothing is reprogrammed.

- Processor: 8088 or V20 (two independent probes, `PUSH 0` and `AAD 16`), 8087 (status and control word shown raw), 8259 (K235 or K230/K231). The detection is `HWDET.INC`, the same as in DMVRACE and DMVDEMO.
- Channel 2: reload value (5000 under NCR DOS 2.11, 27463 under the DOS 3.3 port)
- Channel 0 (floppy motor timeout, clocked by the frame rate) before and after kicking its gate via port 14h
- Frame rate (GDC status bit 5)
- With a K803: channel 2 against the real-time clock; otherwise a stopwatch mode (`S`)
- Where DMVRACE would put its Z80 kernel in bank 0

Results from two real machines: see `PITTEST-Ergebnisse.md` in the project notes (channel 0 = frame clock, 56.09 Hz colour / 56.19 Hz mono; channel 2 about 499.75 kHz against the K803).

Build: `jwasm -Zm -Zne -bin -Fo PITTEST.COM PITTEST.ASM` (needs `PITCAL.INC` and `HWDET.INC`, the same files as in `dmvrace/`). If DMVRACE grows, update `R_END` (its `ENDPROG`) and `R_Z80` (`Z80SIZE`).

## Deutsch

PITTEST misst Zeitbasis und Prozessor einer **NCR Decision Mate V** unter MS-DOS (Punkt 5 der Testdiskette HWCONFIRM). Bezug ist 8253 Kanal 2, den DOS in Modus 3 betreibt (ein Zählschritt = 1 µs); umprogrammiert wird nichts.

- Prozessor: 8088 oder V20 (zwei unabhängige Proben, `PUSH 0` und `AAD 16`), 8087 (Status- und Steuerwort roh angezeigt), 8259 (K235 oder K230/K231). Erkannt wird mit `HWDET.INC`, derselben Datei wie in DMVRACE und DMVDEMO.
- Kanal 2: Startwert (5000 unter NCR-DOS 2.11, 27463 unter dem DOS-3.3-Port)
- Kanal 0 (Motor-Auszeit, getaktet vom Bildtakt) vor und nach dem Anstoß seines Gates über Port 14h
- Bildfrequenz (GDC-Status Bit 5)
- Mit K803: Kanal 2 gegen die Echtzeituhr, sonst Stoppuhr (`S`)
- Wo DMVRACE seinen Z80-Kern in Bank 0 ablegen würde

Ergebnisse von zwei echten Maschinen: `PITTEST-Ergebnisse.md` in den Projektnotizen (Kanal 0 = Bildtakt, 56,09 Hz Farbe / 56,19 Hz Mono; Kanal 2 etwa 499,75 kHz gegen die K803).

Bauen: `jwasm -Zm -Zne -bin -Fo PITTEST.COM PITTEST.ASM` (braucht `PITCAL.INC` und `HWDET.INC`, dieselben Dateien wie in `dmvrace/`). Wächst DMVRACE, `R_END` (dessen `ENDPROG`) und `R_Z80` (`Z80SIZE`) nachziehen.

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01 · MIT License

# DMVDIAG, DMVFRAGE, DMVINFO

*Deutsch weiter unten.*

## English

Self-booting diagnostic disks for the **NCR Decision Mate V**. They start without DOS directly from the Z80 ROM, print through the firmware of the 8088 card (K230/K231/K235) and write their results to the boot disk. All three are built from one core (`dmvdiag.nasm`).

- **DMVINFO** – clear hardware report. Page 1: firmware, processors with clock and 8087, CPU card, coprocessor (K234 68008 / K238 Z8001, only with a K235), memory, shared RAM K233 (only probed when IFSEL 1B is free or holds a 2651/2661), keyboard, drive A:, hard disk controller K012/C3282, clock K803 and the plug-in cards with their IFSEL half; module numbers first, in red on colour machines. Page 2: slots and interrupts. `R` raw values, `D` writes `DMVINFO.TXT`, `W` measures again, `X` restarts the machine.
- **DMVDIAG** – ten pages of raw values and their interpretation (system, processor, memory, modules, interfaces, interrupts, drives, coprocessor), with special tests on keys.
- **DMVFRAGE** – small test with a fixed card layout and one key that answers the open questions about bridges S1/S2, slot lines and interrupt sources.

`DMV-Hardware-erkennen.md` (German) is the guide for operating systems and programs: how to detect the known DMV hardware and where each rule comes from. `BESCHREIBUNG.md` (German) describes every page and key of DMVDIAG, `TESTANLEITUNG.md` a measurement run. The slot configurator, which shows what software will detect for a planned layout, is in [rfka01/dmv_os](https://github.com/rfka01/dmv_os), `tools/konfig/index.html`.

**[`grafik/`](grafik/)**: GTEST and MTEST, test programs for the DMVGRAF graphics library, and a test disk for 16-bit CP/M (CP/M-68K, CP/M-8000) built by `grafik/mkcpm16.py`.

Build (needs `nasm` and Python 3): `python3 mkimg.py dmvinfo` (or `dmvdiag`, `dmvfrage`) → `build/DMVINFO-x.yy.IMG`. Ready-made images are in [`../Images/`](../Images/). This directory is the only place these programs are maintained. `dmvhw.nas` is generated in [rfka01/dmv_os](https://github.com/rfka01/dmv_os) by `hw/gen_inc.py` from `hw/dmvhw.def`; copy it here when the constants change. Tested in MAME (fork [rfka01/mame](https://github.com/rfka01/mame)); earlier versions of DMVDIAG and DMVFRAGE also on two real machines.

## Deutsch

Selbststartende Diagnosedisketten für die **NCR Decision Mate V**. Sie laufen ohne DOS direkt nach dem Z80-ROM, schreiben über die Firmware der 8088-Karte (K230/K231/K235) und legen ihre Ergebnisse auf der Startdiskette ab. Alle drei entstehen aus einem Kern (`dmvdiag.nasm`).

- **DMVINFO** – übersichtliche Hardwareinfo. Seite 1: Firmware, Prozessoren mit Takt und 8087, CPU-Karte, Koprozessor (K234 68008 / K238 Z8001, nur mit K235), Speicher, gemeinsames RAM K233 (nur geprüft, wenn IFSEL 1B frei ist oder einen 2651/2661 trägt), Tastatur, Laufwerk A:, Plattencontroller K012/C3282, Uhr K803 und die Steckkarten mit ihrer IFSEL-Hälfte; Modulnummer zuerst, auf Farbmaschinen rot. Seite 2: Steckplätze und Interrupts. `R` Rohwerte, `D` schreibt `DMVINFO.TXT`, `W` misst neu, `X` startet die Maschine neu.
- **DMVDIAG** – zehn Seiten Rohwerte und Deutung (System, Prozessor, Speicher, Module, Schnittstellen, Interrupts, Laufwerke, Koprozessor), Sondertests auf Tasten. Ablauf für Messläufe: `TESTANLEITUNG.md`.
- **DMVFRAGE** – kleine Probe mit fester Belegung und einer Taste für die offenen Fragen zu den Brücken S1/S2, den Steckplatzleitungen und den Interruptquellen.

`DMV-Hardware-erkennen.md` ist die Anleitung für Betriebssysteme und Programme: wie man die bekannte Hardware der DMV feststellt und woher jede Regel kommt. `BESCHREIBUNG.md` beschreibt jede Seite und Taste von DMVDIAG. Der Steckplatz-Konfigurator, der zu einer geplanten Belegung zeigt, was Software erkennt, liegt in [rfka01/dmv_os](https://github.com/rfka01/dmv_os), `tools/konfig/index.html`.

**[`grafik/`](grafik/)**: GTEST und MTEST, Prüfprogramme für die Grafikbibliothek DMVGRAF, und eine Testdiskette für 16-Bit-CP/M (CP/M-68K, CP/M-8000), gebaut von `grafik/mkcpm16.py`.

Bauen (braucht `nasm` und Python 3): `python3 mkimg.py dmvinfo` (oder `dmvdiag`, `dmvfrage`) → `build/DMVINFO-x.yy.IMG`. Fertige Images liegen in [`../Images/`](../Images/). Gepflegt werden die Programme nur hier. `dmvhw.nas` erzeugt [rfka01/dmv_os](https://github.com/rfka01/dmv_os) mit `hw/gen_inc.py` aus `hw/dmvhw.def`; ändern sich die Konstanten, hierher kopieren. In MAME getestet; frühere Fassungen von DMVDIAG und DMVFRAGE auch auf zwei echten Maschinen.

# KERMIT

*Deutsch weiter unten.*

## English

**MS-DOS Kermit 3.16** for the **NCR Decision Mate V**: VT102/VT220/VT320 terminal and Kermit file transfer over the serial modules **K211/K212/K213** (Signetics 2651) and **K801** (2661). Based on the Columbia University sources as collected in [hackerb9/mskermit](https://github.com/hackerb9/mskermit).

- Runs under the DMV ports of MS-DOS 3.30, NCR MS-DOS 2.11 and DR-DOS 3.41 with at least 256 KB. With 256 KB about 14 KB remain free under DOS 3.30 and about 1.5 KB under DR-DOS 3.41: no screen rollback, no PUSH/RUN.
- Finds the serial modules by itself (IFSEL halves 1A, 1B, 0A, 0B, 2A, 2B, 3A, 3B; first = PORT 1, second = PORT 2). With a K235 it finds the module's interrupt and receives by interrupt; without an interrupt controller (K230/K231) or when no interrupt arrives it polls. As built, the K211/K212 (schematic 017-0031865) and the K801 drive the slot's IRQ line (P1-a2) from RxRDY or TxRDY; the K235 makes it IRQ3, 4, 5 or 7 depending on the slot and its DIP switches. Kermit finds it by enabling the transmitter once. A K211/K212 changed after section 3.2 of NCR's RS-232 handbook (TxRDY cut off, INT/ on c31) interrupts on RxRDY only, the c31 line is IRQ2; Kermit finds that by sending one character to itself in the 2651's local loopback, which leaves the line alone. the 2651 holds only two characters, so without an interrupt receive at 2400 Baud at most (sending works at 9600).
- The DMV's 8259 is edge triggered and the 2651 drives one line for RxRDY and TxRDY, so the transmitter is enabled only while a character is handed over, and every receive path re-checks the status before it lets go.
- Screen output goes through the DOS console using only what all three DOS versions understand (ESC = row col, ESC T, 1Ah, ESC G 0/2/4, ESC ) / ESC (, LF on the bottom line). Kermit's screen is a buffer behind a private video BIOS (Int E5h); only cells that differ are sent.
- The keyboard is read from the 8741 directly (Int E6h stand-in for the IBM keyboard BIOS). **CTRL-CLR** is the escape key in CONNECT mode (then `C` to return, `?` for help).
- Files are stored as they arrive: the DMV has no code pages.
- Removed to fit into 256 KB: networks, Tektronix graphics, the Heath-19, PT200, DG D217/D463/D470, Wyse-50 and Honeywell emulations, file character-set translation, rare terminal character sets (Hebrew, Cyrillic, Japanese, HP-Roman8, Latin-2; shown as ASCII), 132 columns, the `\f...()` script functions, the DECRQSS/DECRQTSR/DECRQPSR reports, APC macros, EMS/XMS rollback, the long help pages (they are appended to `KERMIT.TXT`). SERVER mode is included.

`KERMIBM.EXE` on the disk is MS-Kermit 3.16 for IBM-compatible PCs built from the same sources without networks (packed with UPX), for the other side of the cable.

Files: `src/` the Kermit sources (`MSXDMV.ASM`, `MSXDMVS.INC`, `MSXDMVT.INC` are new, the other changes are marked `[DMV]` / `ifdef dmv`), `tools/build.sh` builds both programs with JWasm/JWlink under Linux, `tools/mkdisk.py` writes `../Images/KERMIT.IMG`, `tools/mktab.py` generates `MSXDMVT.INC` (character and keyboard tables), `tools/hilfetext.py` extracts the long help pages. `KERMIT.TXT` is the description (German, with the help pages in English), `MSKERMIT.INI` the start-up settings.

Tested in an emulator test bench of the DMV under MS-DOS 3.30 (K211 with K235 at 9600 Baud and without interrupt at 2400 Baud, 256 and 512 KB) and under DR-DOS 3.41 (256 KB), each with file transfer both ways against C-Kermit. Not yet tried under NCR MS-DOS 2.11, with the K801, in MAME or on real hardware.

## Deutsch

**MS-DOS Kermit 3.16** für die **NCR Decision Mate V**: Terminal (VT102/VT220/VT320) und Dateiübertragung nach dem Kermit-Protokoll über die seriellen Module **K211/K212/K213** (Signetics 2651) und **K801** (2661). Grundlage sind die Quellen der Columbia University in der Sammlung [hackerb9/mskermit](https://github.com/hackerb9/mskermit).

- Läuft unter den DMV-Ports von MS-DOS 3.30, NCR MS-DOS 2.11 und DR-DOS 3.41 ab 256 KB. Mit 256 KB bleiben unter DOS 3.30 rund 14 KB frei, unter DR-DOS 3.41 rund 1,5 KB: kein Rückblickspeicher, kein PUSH/RUN.
- Findet die seriellen Module selbst (IFSEL-Hälften 1A, 1B, 0A, 0B, 2A, 2B, 3A, 3B; das erste ist PORT 1, das zweite PORT 2). Mit K235 ermittelt es den Interrupt des Moduls und empfängt per Interrupt; ohne Interruptcontroller (K230/K231) oder wenn kein Interrupt ankommt, fragt es ab. Ab Werk legen K211/K212 (Schaltbild 017-0031865) und K801 RxRDY oder TxRDY auf die IRQ-Leitung des Steckplatzes (P1-a2); die K235 macht daraus je nach Steckplatz und DIP-Schaltern IRQ3, 4, 5 oder 7. Kermit findet ihn, indem es den Sender einmal einschaltet. Eine K211/K212, die nach Abschnitt 3.2 des NCR-RS-232-Handbuchs geändert ist (TxRDY aufgetrennt, INT/ auf c31), meldet nur noch RxRDY, die Leitung c31 ist IRQ2; Kermit findet das, indem es im lokalen Loopback des 2651 ein Zeichen an sich selbst schickt, ohne die Leitung zu berühren. der 2651 hält nur zwei Zeichen, ohne Interrupt daher beim Empfangen höchstens 2400 Baud (Senden geht mit 9600).
- Der 8259 der DMV ist flankengesteuert, und der 2651 legt RxRDY und TxRDY auf dieselbe Leitung. Darum ist der Sender nur eingeschaltet, solange ein Zeichen übergeben wird, und jeder Empfangsweg prüft den Status noch einmal, bevor er aufhört.
- Die Bildschirmausgabe läuft über die DOS-Konsole und benutzt nur, was alle drei DOS-Fassungen verstehen (ESC = Zeile Spalte, ESC T, 1Ah, ESC G 0/2/4, ESC ) / ESC (, LF in der untersten Zeile). Kermits Bildschirm ist ein Puffer hinter einem eigenen Video-BIOS (Int E5h); gesendet werden nur geänderte Zellen.
- Die Tastatur wird direkt am 8741 gelesen (Int E6h als Ersatz für das IBM-Tastatur-BIOS). **CTRL-CLR** ist die Fluchttaste im Terminalbetrieb (danach `C` zurück, `?` Hilfe).
- Dateien werden gespeichert, wie sie ankommen: Die DMV kennt keine Codepages.
- Damit alles in 256 KB passt, fehlen: Netzwerke, Tektronix-Grafik, die Emulationen Heath-19, PT200, DG D217/D463/D470, Wyse-50 und Honeywell, die Zeichensatzumsetzung für Dateien, seltene Terminal-Zeichensätze (Hebräisch, Kyrillisch, Japanisch, HP-Roman8, Latin-2; sie erscheinen als ASCII), 132 Spalten, die Skriptfunktionen `\f...()`, die Berichte DECRQSS/DECRQTSR/DECRQPSR, APC-Makros, Rückblick in EMS/XMS und die langen Hilfeseiten (sie stehen am Ende von `KERMIT.TXT`). Der Servermodus ist enthalten.

`KERMIBM.EXE` auf der Diskette ist MS-Kermit 3.16 für IBM-kompatible PCs aus denselben Quellen, ohne Netzwerk (mit UPX gepackt), für die Gegenseite.

Dateien: `src/` die Kermit-Quellen (`MSXDMV.ASM`, `MSXDMVS.INC` und `MSXDMVT.INC` sind neu, die übrigen Änderungen sind mit `[DMV]` bzw. `ifdef dmv` gekennzeichnet), `tools/build.sh` baut beide Programme mit JWasm/JWlink unter Linux, `tools/mkdisk.py` schreibt `../Images/KERMIT.IMG`, `tools/mktab.py` erzeugt `MSXDMVT.INC` (Zeichen- und Tastaturtabellen), `tools/hilfetext.py` holt die langen Hilfeseiten heraus. `KERMIT.TXT` ist die Beschreibung, `MSKERMIT.INI` die Starteinstellungen.

Getestet in einem Emulator-Prüfstand der DMV unter MS-DOS 3.30 (K211 mit K235 bei 9600 Baud und ohne Interrupt bei 2400 Baud, 256 und 512 KB) und unter DR-DOS 3.41 (256 KB), jeweils mit Dateiübertragung in beide Richtungen gegen C-Kermit. Unter NCR MS-DOS 2.11, mit der K801, in MAME und auf echter Hardware noch nicht erprobt.

## License · Lizenz

Kermit: Revised 3-Clause BSD License of Columbia University; source collection: BSD 2-Clause (hackerb9); DMV additions (`MSXDMV*`, the `[DMV]` changes, `tools/`): MIT like the rest of this repository. Full texts in [KERMIT.LIC](KERMIT.LIC).

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01

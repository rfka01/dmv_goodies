# DMVKERM

*Deutsch weiter unten.*

## English

**DMVKERM** is a small Kermit for the **NCR Decision Mate V**: a terminal with a VT102 subset, and file transfer with Kermit (client and server) and XMODEM. It works with the serial modules **K211/K212/K213** (Signetics 2651) and **K801** (2661). It is written from scratch in assembler: one `.COM` file of about 22 KB that needs about 38 KB of memory when it runs. Commands are English, as in every Kermit. Messages are German.

- Runs under NCR MS-DOS 2.11 and under the DMV ports of MS-DOS 3.30 and DR-DOS 3.41, with an 8088/V20 card (K230, K231, K235).
- **Terminal**: `CONNECT`. Supports cursor movement, erase, insert/delete line and character, scrolling region, save/restore cursor, attributes (inverse, half intensity for bold/underline, blink), DEC line graphics (shown as `+ - |`), application cursor keys, and the DA/DSR reports. **CTRL-CLR** is the escape key: then `C` returns, `B` sends a break, `H` hangs up. To dial a Hayes modem, type `ATDT…` in CONNECT.
- **Kermit**: `SEND` (wildcards), `RECEIVE`, `GET`, `SERVER`, `FINISH`, `BYE`, and `REMOTE DIR/CD/DELETE/TYPE/SPACE/HELP/HOST` (also served by DMVKERM's own server, except HOST). A full-screen transfer display like MS-Kermit's; ^X/^Z cancel a file or the batch. Long packets up to 1000 bytes, CRC-16, repeat compression, the attribute packet with the file size, and 8th-bit prefixing when parity is set. Files are always transferred binary.
- **XMODEM**: `XSEND`, `XRECEIVE`. Checksum or CRC, 128- and 1024-byte blocks (`SET XMODEM 1K|128`).
- `SET PORT/SPEED/PARITY/FLOW`, `SHOW`, `HANGUP`, `STATISTICS`, `LOG SESSION`/`CLOSE`, `TRANSMIT`, `DIR`, `CD`, `DEL`, `TYPE`, `PUSH`, `RUN`, `TAKE`, and `DMVKERM.INI` at start-up.
- **Scripts** for dialling a mailbox: `OUTPUT`, `INPUT`, `PAUSE`, `CLEAR`, `ECHO`, `IF SUCCESS|FAILURE`, `GOTO`, `STOP`.
- Finds the serial modules and their interrupt by itself, like the large Kermit port did: the module's slot IRQ on the K235, or IRQ2 on a K211/K212 modified after NCR's handbook. Without an interrupt it polls.
- Screen output goes through the DOS console with only what all three DOS versions understand. The keyboard is read from the 8741 directly.

`DMVKERM.COM` and `DMVKERM.TXT` are on `../Images/GOODIES.IMG`, which is rebuilt with every change. `../Images/DMVKERM.IMG` is the last separate DMVKERM disk (16 Oct 2026, no longer updated); it also holds the original **Kermit-MS 2.32/A** from Columbia University for IBM-compatible PCs (`KERMIBM.EXE`), for the other end of the cable.

Files: `src/` the sources (`dmvkerm.asm` and its includes), `tools/build.sh` builds `DMVKERM.COM` with JWasm under Linux, `tools/mktab.py` generates `src/tabellen.inc` (character and keyboard tables), `tools/mkdisk.py` puts DMVKERM on `GOODIES.IMG`. `DMVKERM.TXT` is the description (German), `DMVKERM.INI` a sample start-up file.

Tested in MAME: DMV with K211 and K235 under MS-DOS 3.30 at 9600 baud against an IBM XT running MS-Kermit 2.32 (SEND and GET, linked with `-bitb socket.127.0.0.1:2323` and `ncat --broker`). Also tested in an emulator test bench against C-Kermit (also as a server, with long packets, even parity and the REMOTE commands both ways), XMODEM against lrzsz, and the VT102 subset against the pyte reference. Not yet tested under NCR MS-DOS 2.11 or DR-DOS 3.41, with the K801, or on real hardware.

## Deutsch

**DMVKERM** ist ein kleines Kermit für die **NCR Decision Mate V**: ein Terminal mit einer VT102-Teilmenge und Dateiübertragung mit Kermit (Client und Server) und XMODEM. Es arbeitet mit den seriellen Modulen **K211/K212/K213** (Signetics 2651) und **K801** (2661). Es ist von Grund auf neu in Assembler geschrieben: eine `.COM`-Datei mit rund 22 KB, die beim Lauf rund 38 KB Speicher braucht. Die Befehle sind englisch, wie bei jedem Kermit. Die Meldungen sind deutsch.

- Läuft unter NCR MS-DOS 2.11 und unter den DMV-Ports von MS-DOS 3.30 und DR-DOS 3.41, mit 8088/V20-Karte (K230, K231, K235).
- **Terminal**: `CONNECT`. Versteht Cursorbewegung, Löschen, Zeilen und Zeichen einfügen/löschen, Rollbereich, Cursor sichern und wiederherstellen, Attribute (invers, halbhell für fett/unterstrichen, blinkend), DEC-Liniengrafik (als `+ - |`), Anwendungsmodus der Cursortasten sowie die Abfragen DA/DSR. **CTRL-CLR** ist die Fluchttaste: danach `C` zurück, `B` Break, `H` auflegen. Ein Hayes-Modem wählt man in CONNECT mit `ATDT…`.
- **Kermit**: `SEND` (mit Platzhaltern), `RECEIVE`, `GET`, `SERVER`, `FINISH`, `BYE` sowie `REMOTE DIR/CD/DELETE/TYPE/SPACE/HELP/HOST` (DMVKERMs eigener Server bedient sie auch, außer HOST). Übertragungsmaske wie bei MS-Kermit; ^X/^Z bricht eine Datei bzw. alle ab. Lange Pakete bis 1000 Byte, CRC-16, Wiederholungskompression, Attributpaket mit der Dateigröße und Präfix für das 8. Bit bei eingestellter Parität. Dateien werden immer binär übertragen.
- **XMODEM**: `XSEND`, `XRECEIVE`. Prüfsumme oder CRC, Blöcke zu 128 und 1024 Byte (`SET XMODEM 1K|128`).
- `SET PORT/SPEED/PARITY/FLOW`, `SHOW`, `HANGUP`, `STATISTICS`, `LOG SESSION`/`CLOSE`, `TRANSMIT`, `DIR`, `CD`, `DEL`, `TYPE`, `PUSH`, `RUN`, `TAKE` und `DMVKERM.INI` beim Start.
- **Skripte** für die Einwahl in eine Mailbox: `OUTPUT`, `INPUT`, `PAUSE`, `CLEAR`, `ECHO`, `IF SUCCESS|FAILURE`, `GOTO`, `STOP`.
- Findet die seriellen Module und ihren Interrupt selbst, wie schon der große Kermit-Port: den IRQ des Steckplatzes an der K235 oder IRQ2 bei einer K211/K212, die nach dem NCR-Handbuch geändert ist. Ohne Interrupt fragt es ab.
- Die Bildschirmausgabe läuft über die DOS-Konsole und benutzt nur, was alle drei DOS-Fassungen verstehen. Die Tastatur wird direkt am 8741 gelesen.

`DMVKERM.COM` und `DMVKERM.TXT` stehen auf `../Images/GOODIES.IMG`, die bei jeder Änderung neu gebaut wird. `../Images/DMVKERM.IMG` ist der letzte Stand der eigenen DMVKERM-Diskette (16.10.2026, wird nicht mehr gepflegt); darauf liegt zusätzlich das originale **Kermit-MS 2.32/A** der Columbia University für IBM-kompatible PCs (`KERMIBM.EXE`), für die Gegenseite.

Dateien: `src/` die Quellen (`dmvkerm.asm` mit seinen Include-Dateien), `tools/build.sh` baut `DMVKERM.COM` mit JWasm unter Linux, `tools/mktab.py` erzeugt `src/tabellen.inc` (Zeichen- und Tastaturtabellen), `tools/mkdisk.py` trägt DMVKERM in `GOODIES.IMG` ein. `DMVKERM.TXT` ist die Beschreibung, `DMVKERM.INI` eine Beispiel-Startdatei.

In MAME getestet: DMV mit K211 und K235 unter MS-DOS 3.30 bei 9600 Baud gegen einen IBM XT mit MS-Kermit 2.32 (SEND und GET, verbunden mit `-bitb socket.127.0.0.1:2323` und `ncat --broker`). Dazu im Emulator-Prüfstand gegen C-Kermit (auch als Server, mit langen Paketen, gerader Parität und den REMOTE-Befehlen in beide Richtungen), XMODEM gegen lrzsz und die VT102-Teilmenge gegen die Referenz pyte. Unter NCR MS-DOS 2.11 und DR-DOS 3.41, mit der K801 und auf echter Hardware noch nicht erprobt.

## License · Lizenz

DMVKERM: MIT, like the rest of this repository · wie der Rest des Repositorys. Kermit-MS 2.32/A (`KERMIBM.EXE` on `DMVKERM.IMG` · auf `DMVKERM.IMG`): Revised 3-Clause BSD License of Columbia University, see · siehe [KERMIT.LIC](KERMIT.LIC).

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01

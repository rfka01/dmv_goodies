# ECODRIVE

*Deutsch weiter unten.*

## English

ECODRIVE is a 62 KB RAM disk for the **NCR Decision Mate V with colour graphics board**. The colour board has 96 KB of video memory in three 32 KB planes (green, red, blue) behind the µPD7220 graphics controller. Text mode and green graphics only use the green plane, so ECODRIVE stores a FAT12 drive with 128 sectors in the red and blue planes. "Eco" because the screen stays green.

- Loads as a TSR without CONFIG.SYS and takes the next free drive letter; `ECODRIVE /U` removes it again (after a J/N prompt, the files are lost).
- `ECODRIVE /F` sets the drive up again, empty.
- Refuses to load on a monochrome board (only 32 KB video memory).
- Protection, since the 8088 cannot block access to the graphics ports:
  - a checksum per sector in main memory: damaged sectors give a DOS data error instead of wrong data;
  - a J/N prompt before graphics programs (DMVDEMO, DMVRACE, GEM and own names with `/X:NAME`);
  - after such a program ends, all sectors are checked; if any is damaged, the drive is locked ("not ready") until `ECODRIVE /F`;
  - an INT 2Fh installation check (AX=EC00h) for cooperating programs.
- Requires MS-DOS 3.1–3.3 on the DMV (8088/V20 card). Messages are in German.

Programs from the CP/M era (e.g. WordStar 3.3) do not understand paths: use `F:NAME.TXT`, not `F:\NAME.TXT`.

### Build

```
MASM ECODRIVE;
LINK ECODRIVE;
EXE2BIN ECODRIVE ECODRIVE.COM
```

`test/ecotest.py` is the test harness used during development: an 8086 emulator (Unicorn), a model of the µPD7220 and the DOS 3.3 tables ECODRIVE touches (`pip install unicorn`, then `python3 ecotest.py ECODRIVE.COM`).

## Deutsch

ECODRIVE ist eine RAM-Disk mit 62 KB für die **NCR Decision Mate V mit Farbgrafikkarte**. Die Farbkarte hat 96 KB Bildspeicher in drei Ebenen zu je 32 KB (Grün, Rot, Blau) hinter dem Grafikcontroller µPD7220. Textbetrieb und grüne Grafik nutzen nur die Grün-Ebene, deshalb legt ECODRIVE in Rot und Blau ein FAT12-Laufwerk mit 128 Sektoren ab. „Eco“, weil der Bildschirm grün bleibt.

- Lädt als TSR ohne CONFIG.SYS auf den nächsten freien Laufwerksbuchstaben; `ECODRIVE /U` entfernt es wieder (nach J/N-Rückfrage, die Dateien gehen verloren).
- `ECODRIVE /F` richtet das Laufwerk leer neu ein.
- Verweigert den Start auf einer Monokarte (nur 32 KB Bildspeicher).
- Schutz, weil der 8088 Zugriffe auf die Grafikports nicht sperren kann:
  - eine Prüfsumme je Sektor im Hauptspeicher: beschädigte Sektoren ergeben einen DOS-Datenfehler statt falscher Daten;
  - eine J/N-Rückfrage vor Grafikprogrammen (DMVDEMO, DMVRACE, GEM und eigene Namen mit `/X:NAME`);
  - nach dem Ende eines solchen Programms werden alle Sektoren geprüft; ist einer beschädigt, ist das Laufwerk bis `ECODRIVE /F` gesperrt („nicht bereit“);
  - eine Installationsprüfung über INT 2Fh (AX=EC00h) für Programme, die Rücksicht nehmen wollen.
- Braucht MS-DOS 3.1–3.3 auf der DMV (8088/V20-Karte). Die Meldungen sind deutsch.

Programme aus der CP/M-Zeit (z. B. WordStar 3.3) kennen keine Pfade: `F:NAME.TXT` angeben, nicht `F:\NAME.TXT`.

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01 · MIT License

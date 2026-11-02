# Maschinensprache fuer die Pascal-Fassung

Turbo Pascal 3 hat keinen Binder; Maschinensprache steht als `inline(...)`
in den Hardwareschichten `pascal/DMVHWC.INC` (Z80) und `pascal/DMVHWD.INC`
(8086). Hier liegen die Quellen, aus denen diese Bloecke entstehen.

| Datei | Erzeugt in | Inhalt |
|---|---|---|
| `z80inl.py` | – | kleiner Z80-Assembler fuer inline-Bloecke |
| `z80list.py` | `DMVHWC.INC` `ghlist` | GDC-Liste an den 7220 |
| `z80zeile.py` | `DMVHWC.INC` `hwEin` | Listeneintrag fuer eine Bitzeile (drei Bloecke) |
| `z80zoom.py` | `DMVHWC.INC` `ghzbits` | Bits vervielfachen und ausschneiden |
| `z80blit.py` | `DMVHWC.INC` `hwBlitZ` | schneller Weg der Bilder: eine Zeile eines Tabelleneintrags direkt an den 7220 |
| `i86inl.py` | – | NASM-Quelle -> inline-Block (Variablen per zweimaligem Uebersetzen) |
| `I86LIST.ASM` | `DMVHWD.INC` `ghlist` | GDC-Liste |
| `I86ZEILE.ASM` | `DMVHWD.INC` `hwEin` | Listeneintrag fuer eine Bitzeile |
| `I86ZOOM.ASM` | `DMVHWD.INC` `ghzbits` | Bits vervielfachen und ausschneiden |
| `I86ROM.ASM` | `DMVHWD.INC` `hwRom` | Kennbuchstabe der Firmware lesen |
| `I86BLIT.ASM` | `DMVHWD.INC` `ghblit` | schneller Weg der Bilder wie HOPPLERs `BlitAsm`; dieselbe Schleife steht als `gblit` in `src/GCRTDOS.S` |

Aufruf: `python3 z80list.py`, `python3 i86inl.py I86ZEILE.ASM hwL,hwQn,hwQe,hwAd,hwSx,hwNb,hwW,hwAa,hwSrc,hwLz,hwX,hwSh,hwRe,hwK2,hwMsk,hwFFo,hwOr,hwWst,hwHl`
(die Variablenliste steht jeweils am Kopf der Quelle), Ausgabe von Hand
in die Include-Datei uebernehmen. Die 8086-Bloecke beginnen mit `cld`:
Turbo Pascal laesst das Richtungsbit nach `Move` mitunter gesetzt.

Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT.

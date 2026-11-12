# DMVMEM

*Deutsch weiter unten.*

## English

**DMVMEM 1.0** shows how the DMV's memory is used: how much DOS takes, which programs stayed resident (TSRs), which interrupts they hook, and how much is free for the next program. MS-DOS 3.30 has no `MEM` command (it came with DOS 4.0); DMVMEM fills the gap.

| | |
|---|---|
| `DMVMEM` | summary |
| `DMVMEM /B` | every memory block as well |
| `DMVMEM ?` | short help |

```
Programm                         Byte   Hinweis
------------------------------------------------------------------
DOS (Kern, Puffer, Treiber)    63.344
COMMAND.COM                     3.840   Befehlsprozessor
DMVKEY                          4.720   INT 21 2F
------------------------------------------------------------------
Frei                          452.384   groesster Block 452.192
Gesamt                        524.288   Arbeitsspeicher
```

- DMVMEM walks the chain of DOS memory control blocks and adds the blocks up by owner. "DOS" is everything below the first block (interrupt table, BIOS and DOS data, IO.SYS, MSDOS.SYS, buffers, device drivers) plus the blocks owned by DOS.
- A program's name comes from its environment. Resident programs often give the environment back; then DMVMEM looks for a name tag in the first bytes of the program (DMVKEY has one). The command processor is the program that is its own parent.
- The interrupts whose vectors point into a program are listed behind it.
- DMVMEM's own blocks count as free. "groesster Block" (largest block) is what the next program gets in one piece. Every block is counted with its 16-byte header, so the lines add up exactly to the memory size.
- Started from the GEM desktop ("DOS-Kommandos eingeben") it also shows GEM's parts (GEMVDI, GEM.EXE with AES, screen driver and buffers).
- Plain MS-DOS calls (2.0 and later, names from 3.0), no ports; output can be redirected (`DMVMEM /B >PRN`). Output is German.

Files: `DMVMEM.ASM` the source, `DMVMEM.TXT` the description (German), `BUILD.BAT` builds `DMVMEM.COM` with MASM 5.10, `tools/build.sh` with JWasm under Linux, `tools/mkdisk.py` puts DMVMEM on `../Images/GOODIES.IMG`.

## Deutsch

**DMVMEM 1.0** zeigt, wie der Arbeitsspeicher der DMV belegt ist: wie viel DOS braucht, welche Programme resident geblieben sind (TSRs), auf welche Interrupts sie sich gesetzt haben und wie viel für das nächste Programm frei ist. MS-DOS 3.30 hat noch keinen `MEM`-Befehl (der kam mit DOS 4.0); DMVMEM ist der Ersatz.

| | |
|---|---|
| `DMVMEM` | Übersicht |
| `DMVMEM /B` | dazu jeden Speicherblock einzeln |
| `DMVMEM ?` | kurze Hilfe |

- DMVMEM geht die Kette der Speicherblöcke von DOS durch und zählt die Blöcke je Besitzer zusammen. „DOS“ ist alles unterhalb des ersten Blocks (Interrupttabelle, Daten von BIOS und DOS, IO.SYS, MSDOS.SYS, Puffer, Treiber) und die Blöcke, die DOS gehören.
- Der Name eines Programms kommt aus seiner Umgebung. Residente Programme geben die Umgebung oft zurück; dann sucht DMVMEM in den ersten Bytes des Programms nach einer Kennung (DMVKEY hat eine). Der Befehlsprozessor ist das Programm, das sein eigener Vorgänger ist.
- Hinter jedem Programm stehen die Interrupts, deren Vektoren in das Programm zeigen.
- DMVMEMs eigene Blöcke zählen als frei. „groesster Block“ ist das, was das nächste Programm am Stück bekommt. Jeder Block wird mit seinem 16-Byte-Kopf gezählt, so ergeben die Zeilen zusammen genau den Arbeitsspeicher.
- Aus dem GEM-Desktop gestartet („DOS-Kommandos eingeben“) zeigt es auch die Teile von GEM (GEMVDI, GEM.EXE mit AES, Bildschirmtreiber und Puffern).
- Nur DOS-Aufrufe (ab 2.0, Namen ab 3.0), keine Ports; die Ausgabe lässt sich umlenken (`DMVMEM /B >PRN`).

Dateien: `DMVMEM.ASM` die Quelle, `DMVMEM.TXT` die Beschreibung, `BUILD.BAT` baut `DMVMEM.COM` mit MASM 5.10, `tools/build.sh` mit JWasm unter Linux, `tools/mkdisk.py` trägt DMVMEM in `../Images/GOODIES.IMG` ein.

## License · Lizenz

MIT, like the rest of this repository · wie der Rest des Repositorys.

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01

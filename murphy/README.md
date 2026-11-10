# MURPHY

*Deutsch weiter unten.*

## English

**MURPHY 3.0** shows one of Murphy's Laws, chosen at random, every time it runs — put it in `AUTOEXEC.BAT` and the DMV greets you with a new piece of wisdom at every start. It is a new version of **MURPHY 2.0** (June 1986) by Charles J. C. Lyall. In Lyall's program the laws were compiled in; now they are in the text file `MURPHY.LAW`, which anyone can extend with an editor. The command line and the messages are the same as Lyall's:

| | |
|---|---|
| `MURPHY` | one law chosen at random |
| `MURPHY n` | law number n (counting from 0) |
| `MURPHY n m` | laws n to m, e.g. `MURPHY 0 8` for the first nine |
| `MURPHY ?` | short help and the number of laws |
| `/A` | show the source line (e.g. `Murphy's Corollary:`) in inverse video with ANSI codes `ESC [7m` / `ESC [0m` – the default |
| `/T` | plain text without codes, for PCs without ANSI.SYS |

- Inverse is the one attribute that looks the same on every console: NCR MS-DOS 2.11, the DMV ports of MS-DOS 3.30 and DR-DOS 3.41, and ANSI.SYS on IBM compatibles (bold, `ESC [1m`, is normal intensity on the DMV; half intensity, `ESC [8m`, is concealed on a PC; `ESC [0m` keeps the colour under NCR 2.11). The switches may stand anywhere on the command line; redirected output is always plain text. The source line is the first line of a law if it ends with `:`. Each law is printed with two empty lines before and one after.
- `MURPHY.LAW` is plain text: the laws are separated by lines holding only a `%` (the format of the Unix fortune files). CR/LF and LF line ends both work.
- The file is looked for in the directory of `MURPHY.COM` (MS-DOS 3.0 and later), in the current directory, and in the directories of the `PATH`.
- The file is read only once: law *k* replaces the one chosen so far with probability 1/*k* (reservoir sampling), so each law has the same chance and a 30 KB file need not be read twice from the floppy. The random generator is seeded from the DOS date and time.
- Plain MS-DOS 2.0 calls only, no ports: it runs on the DMV and on any other MS-DOS machine. Output can be redirected (`MURPHY 0 999 >PRN`).
- The collection has 296 laws: 0–200 Lyall's collection from MURPHY 2.0, unchanged except for one duplicate and six added colons after sources; 201–277 further classics from Arthur Bloch's *Murphy's Law* books and computer folklore (Murphy's corollaries, programming, the electronics bench, the lab and the office); 278–295 **"Lucas, Prince of Darkness"**, the jokes about Joseph Lucas's electrics in British cars and motorcycles.

Files: `MURPHY.ASM` the source, `MURPHY.LAW` the laws, `MURPHY.TXT` the description (German), `BUILD.BAT` builds `MURPHY.COM` with MASM 5.10, `tools/build.sh` with JWasm under Linux, `tools/mkdisk.py` puts MURPHY on `../Images/GOODIES.IMG`.

## Deutsch

**MURPHY 3.0** zeigt bei jedem Aufruf ein zufällig gewähltes Gesetz von Murphy — in die `AUTOEXEC.BAT` geschrieben, begrüßt einen die DMV bei jedem Start mit einer neuen Weisheit. Es ist eine Neufassung von **MURPHY 2.0** (Juni 1986) von Charles J. C. Lyall. Bei Lyall waren die Gesetze ins Programm hineinübersetzt; jetzt stehen sie in der Textdatei `MURPHY.LAW`, die jeder mit einem Editor ergänzen kann. Aufruf und Meldungen sind dieselben wie bei Lyall:

| | |
|---|---|
| `MURPHY` | ein zufällig gewähltes Gesetz |
| `MURPHY n` | Gesetz Nummer n (gezählt ab 0) |
| `MURPHY n m` | die Gesetze n bis m, z. B. `MURPHY 0 8` für die ersten neun |
| `MURPHY ?` | kurze Hilfe und die Zahl der Gesetze |
| `/A` | die Quellzeile (z. B. `Murphy's Corollary:`) mit den ANSI-Codes `ESC [7m` / `ESC [0m` invers zeigen – die Vorgabe |
| `/T` | reiner Text ohne Steuerzeichen, für PCs ohne ANSI.SYS |

- Invers ist das einzige Attribut, das auf jeder Konsole gleich wirkt: NCR MS-DOS 2.11, die DMV-Ports von MS-DOS 3.30 und DR-DOS 3.41 und ANSI.SYS auf IBM-Kompatiblen (hell, `ESC [1m`, ist auf der DMV die normale Helligkeit; halbhell, `ESC [8m`, ist auf dem PC unsichtbar; `ESC [0m` lässt unter NCR 2.11 die Farbe stehen). Die Schalter dürfen an beliebiger Stelle stehen; umgelenkte Ausgabe ist immer reiner Text. Als Quellzeile gilt die erste Zeile eines Spruchs, wenn sie mit `:` endet. Vor jedem Spruch stehen zwei Leerzeilen, danach eine.
- `MURPHY.LAW` ist reiner Text: Die Gesetze sind durch Zeilen getrennt, auf denen nur ein `%` steht (das Format der fortune-Dateien von Unix). Zeilenenden CR/LF und LF gehen beide.
- Die Datei wird im Verzeichnis von `MURPHY.COM` (ab MS-DOS 3.0), im aktuellen Verzeichnis und in den Verzeichnissen des `PATH` gesucht.
- Die Datei wird nur einmal gelesen: Das *k*-te Gesetz ersetzt das bisher gewählte mit der Wahrscheinlichkeit 1/*k*, so hat jedes Gesetz dieselbe Chance, und 30 KB müssen nicht zweimal von der Diskette gelesen werden. Der Zufallsgenerator wird aus Datum und Uhrzeit von DOS gesetzt.
- Nur Aufrufe von MS-DOS 2.0, keine Ports: läuft auf der DMV und auf jedem anderen MS-DOS-Rechner. Die Ausgabe lässt sich umlenken (`MURPHY 0 999 >PRN`).
- Die Sammlung umfasst 296 Gesetze: 0–200 Lyalls Sammlung aus MURPHY 2.0, unverändert bis auf ein doppeltes und sechs ergänzte Doppelpunkte hinter Quellen; 201–277 weitere Klassiker aus Arthur Blochs *Murphy's Law*-Büchern und der Computer-Folklore (Murphys Folgerungen, Programmieren, Elektronik-Werkstatt, Labor und Büro); 278–295 **„Lucas, Prince of Darkness“**, die Witze über die Elektrik von Joseph Lucas in englischen Autos und Motorrädern.

Dateien: `MURPHY.ASM` die Quelle, `MURPHY.LAW` die Gesetze, `MURPHY.TXT` die Beschreibung, `BUILD.BAT` baut `MURPHY.COM` mit MASM 5.10, `tools/build.sh` mit JWasm unter Linux, `tools/mkdisk.py` trägt MURPHY in `../Images/GOODIES.IMG` ein.

## License · Lizenz

MURPHY 3.0: MIT, like the rest of this repository · wie der Rest des Repositorys. The laws from MURPHY 2.0 were released into the public domain by Charles J. C. Lyall in 1986, "not to be used for profit without the author's permission" · Die Gesetze aus MURPHY 2.0 hat Charles J. C. Lyall 1986 als Public Domain freigegeben, nicht für gewerbliche Zwecke ohne seine Erlaubnis.

---

Code: Claude (Anthropic) · Idee und Prompts / idea and prompts: rfka01

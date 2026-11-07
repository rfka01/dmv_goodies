# DMVDIAG, DMVFRAGE, DMVINFO – Beschreibung


Disketten, die ohne DOS direkt nach dem Z80-ROM starten. Kein eigener
Treiber verfälscht die Messung, und sie laufen auch auf einer Maschine,
auf der kein Betriebssystem hochkommt.

| Datei | |
|---|---|
| `boot.nasm` | Bootsektor: lädt das Programm aus den reservierten Sektoren nach `0000:4000` |
| `dmvdiag.nasm` | DMVDIAG – Erkennung der Hardware, Rohwerte und Deutung |
| `ports.inc` | Seite „Schnittstellen“: Bausteinproben für die seriellen und parallelen Karten |
| `results.inc` | jede Seite und jeder Sondertest automatisch als Textdatei mit festem Namen auf die Startdiskette |
| `TESTANLEITUNG.md` | drei Starts an zwei Maschinen: Bestückung, Tasten je Seite, was zurückkommt |
| `dmvfrage.nasm`, `frage.inc`, `VERSION.dmvfrage` | DMVFRAGE: kleine Testdiskette aus demselben Kern (mit FRAGE gebaut), feste Belegung, eine Taste, eine Seite – Brücken S1/S2, 13h-Bit von Platz 2, Leitungen von K211/K803, Interruptquelle der K211. Bauen: `python3 diag/mkimg.py dmvfrage` |
| `dmvinfo.nasm`, `info.inc`, `VERSION.dmvinfo` | DMVINFO: übersichtliche Hardwareinfo aus demselben Kern (mit INFO gebaut). Seite 1 Übersicht (Firmware, Prozessoren, CPU-Karte, Koprozessor K234/K238 nur mit K235, Speicher, gemeinsames RAM K233 (ab 1.03, nur wenn IFSEL 1B frei ist oder einen 2651/2661 trägt), Tastatur, Laufwerk, Platte, Uhr, Steckkarten mit Modulnummer zuerst, in Farbe rot), Seite 2 Steckplätze und Interrupts; `R` Rohwertseiten, `D` schreibt `DMVINFO.TXT`, `W` misst neu, `X` startet neu. Bauen: `python3 mkimg.py dmvinfo` |
| `irqtest.inc` | Seite „Interrupts“: Leitungen der Steckplätze (Taste I) |
| `motor.inc` | Taste M: Motoranlauf und Schrittzeit an A: |
| `mkimg.py` | baut das 360-KB-Image |

```
python3 mkimg.py               # -> build/DMVDIAG-x.yy.IMG
```

**Version:** steht einzig in `VERSION`. Daraus entstehen die
Anzeige im Programm, der Name des Images (`DMVDIAG-0.02.IMG`) und der
Datenträgername (`DMVDIAG0.02`). Jede Änderung an Programm oder Image
bekommt eine neue Version, damit auf jedem Foto zu sehen ist, welcher
Stand lief. Ohne `mkimg.py` lässt sich das Programm nicht bauen.

Die Diskette ist ein gewöhnliches 360-KB-Format (40 Zylinder, zwei
Köpfe, neun Sektoren zu 512 Byte). Das Programm liegt in den
reservierten Sektoren hinter dem Bootsektor; DOS sieht eine leere,
gültige Diskette mit dem Namen `DMVDIAG`.

## Grundsätze

1. **Im Zweifel nichts behaupten.** „unbekannt" ist ein Ergebnis.
2. **Erst die Rohwerte, darunter die Deutung.** Regeln für Karten werden
   aus Fotos mit bekannter Bestückung abgeleitet, nicht vorher gesetzt.
3. **Eine Seite je Schirm**, fotolesbar. Jede Seite trägt in der
   Kopfzeile Firmware, CPU, 8087 und CPU-Karte – so ist auf jedem Foto
   zu sehen, welche Maschine es war.
4. **Ohne DOS.** Ausgabe nur über `DISPLAY` der Firmware (`FF00:0000`).
   Als Steuerzeichen nur `88h` (neue Zeile): das ROM der K230/K231 gibt
   `00h`–`0Fh` als Zeichen aus. Der Cursor wird über `0:F800/F801`
   gesetzt. Gelöscht wird über die Löschroutine des ROMs (bei bekannter
   Prüfsumme) und ein eigenes WDAT über die 2000 Textworte.
5. **Gliederung über Attribute** (Werte aus rfka01/dmv_os, `docs/DMV-Hardware-Boot-und-Bildschirm.md`, Attributbyte):

   | Rolle | Mono | Farbe |
   |---|---|---|
   | Kopf- und Fußzeile | E9h invers | 7Ch weiß auf blau |
   | Seitentitel | E9h invers | ECh gelb |
   | Rohwerte | E8h normal | E8h grün |
   | Deutung („->") | E8h normal | FCh weiß |
   | Beschriftung, Erläuterung | ECh halbhell | F8h türkis |

   Mono kennt kein „intensiv"; normal ist dort die höchste Stufe. Farbe
   wird nur benutzt, wenn Firmware und Ebenenprobe auf Farbe stehen
   (oder die Probe unklar ist und die Firmware „C" sagt).
   In Texten stehen die Rollen als Steuerbytes 01h–05h.
6. **Aufbauzeit** jeder Seite steht in der Fußzeile (8253 Kanal 2).

## DMVDIAG – Seiten

| Seite | Inhalt | greift ein? |
|---|---|---|
| 1 System | Kennung `0FF7h–0FFFh` des Hauptplatinen-ROMs, CRC-32 beider ROMs, Postfach `FE00–FE3F`, Antwort des 8741 auf `01h`, **Ebenenprobe des GDC** (Mono oder Farbe, unabhängig von der Firmware) | schreibt Wort 3FF0h/7FF0h und stellt es wieder her |
| 2 Prozessor | `PUSH imm8` und `AAD 16` (V20 oder 8088), 8087 Status- und Steuerwort, 8259-Rücklesung, Takt über eine `AAM`-Schleife gegen 8253 Kanal 2 | setzt die 8259-Maske auf FFh |
| 3 Speicher | Bänke 1–7 je ein eigenes Muster, erst alle schreiben, dann alle lesen (zeigt auch Spiegelung). **Taste `K`**: K233-Probe bei `0:C000` (kippt viermal über `OUT 7Fh`), Rohwerte „vorher/aus/ein/durch" und 7Bh | schreibt `n:FFF0–FFF3`; mit K: `0:C000` und stellt es wieder her |
| 4 Module | IFSEL `x0–x3` je dreimal gelesen, mit 00h/FFh/5Ah zuletzt auf dem Bus | nein |
| 5 Schnittstellen | **Taste `S`**, je IFSEL-Hälfte 0A–3B der Reihe nach: belegt? (x0–x3 lesen), K215-Echo (Befehl 0Bh, 55h/AAh; nur 70/78/30/38/B0/B8 und nur bei Status 00h), 2651/2661 (Befehlsregister x3/x7 mit 23h und 03h, Spiegelung über xB → K211/K212/K213, sonst K801), 8255 nur auf 0A (Steuerwort AAh, INTE A über PC6 → K210), Z8530 je Viertel (WR12/RR12 → K216). Was erkannt ist, sperrt die Hälfte für die weiteren Proben. 2A nur K215 (dort sitzt die K806), IFSEL 4 gar nicht | schreibt Register und stellt sie zurück; Drucker an der K210 bekommt evtl. INIT |
| 6 Interrupts | **Taste `I`**: Bausteinproben wie Taste S, dann Ruhestand von 13h, IRR und Maske des 8259 (pegelgesteuert wie nach dem ROM 33473, daher zeigt das IRR die Eingänge: leere Plätze stehen wegen der Pull-ups auf 1). Danach je gefundener K210 (PC3 an/aus, Steuerwort 8Ah, danach wieder AAh) und 2651/2661 (Befehlsregister 01h/00h, TxRDY) 13h und IRR in „aus/an/aus“ → welches Bit in 13h und welcher IR zu diesem Platz gehört | schaltet die Leitung der Karte kurz; die K210 gibt dabei evtl. einen Strobe an den Drucker |
| 7 Laufwerke, Platte, Uhr | FDC-Hauptstatus, offene Interrupts, ST3 je Laufwerk (ohne Kopfbewegung), Spurzahl von A: aus `FE08`/`FE09` des Z80-ROMs, Plattencontroller und Einheiten 0–2 mit und ohne Warten, K803-Sekunden im Abstand von 1,2 s | schreibt `C2h`/`C3h`/`C6h`, `C8h` = 0 |
| 8 Laufwerke aktiv | Anzeige in der Reihenfolge M, T, B. **Taste `M`** (Diskette in A:): warten, bis die Hardware den Motor abschaltet (13h Bit 0, bis 20 s), dann `OUT 14h` und die Zeit bis „Motor an“ und bis READY (13h Bit 2); danach mit SPECIFY `D6 02` und `E6 02` je SEEK 0→39 und 39→0 mit Zeit → ms je Schritt und daraus der Takt des 8272 (6 ms = 4 MHz, 3 ms = 8 MHz). **Taste `T`**: A: und B: je RECALIBRATE, SEEK 2, RECALIBRATE, ST0/PCN/ST3 roh (ohne Diskette erwartet: nicht bereit, kein Schritt). **Taste `B`**: DMVDIAG-Diskette in B:, READ ID auf physischem Zylinder 2 → 40 oder 80 Spuren | höchstens zwei Schritte je Laufwerk |
| 9 Koprozessor | K234 (68008) oder K238 (Z8001): **nur auf Taste `P`**, nur mit K235. Der Z80 startet die Karte über seinen Warteplatz `F111h` mit `OUT DFh` wie NCRs CP/M-68K (bis 0.15 `OUT D8h`: in einem ersten Test auf Maschine A lief die K234 damit nicht an, Sonde 00, Z80-Marke 5A); eine zweisprachige Sonde an `0:0000` schreibt `6` oder `Z` und läutet über `FFFDFh` bzw. `OUTB DFh` zurück | rettet und schreibt zurück: Vektoren 0–9, Warteplatz |
| 10 Deutung | nur was die Rohwerte sicher tragen; Hinweis „FOTO“ (ebenso Seite 6 nach `I`) | – |

Tasten: beliebige Taste weiter, `Z` zurück, `W` neu messen, `X` Ende
(zurück zum Z80), `D` Sichern wieder einschalten und sofort schreiben.

### Ergebnisse auf der Diskette (automatisch)

Beim Aufbau einer Seite wird ihr Text (Kopfzeile und Seite, ohne
Fußzeile) mitgeschrieben. Wer die Seite verlässt (weiter, zurück, `X`),
schreibt sie unter ihrem festen Namen auf die Startdiskette in A:
(CR LF, ASCII; unter DOS oder mit mtools lesbar). Die Erweiterung ist
die **Laufnummer** dieses Starts: Beim ersten Schreiben nach dem Booten
nimmt DMVDIAG die höchste Nummer auf der Diskette + 1. Nach Umstecken
und Neustart bleiben die Dateien des vorigen Laufs also stehen
(`05PORTS.L1`, `06IRQTST.L1`, dann `05PORTS.L2`, …).

| Seite | Datei | Sondertest | Datei |
|---|---|---|---|
| 1 | `01SYSTEM.Ln` | | |
| 2 | `02CPU.Ln` | | |
| 3 | `03SPEICH.Ln` | `K` | `03K233.Ln` |
| 4 | `04MODULE.Ln` | | |
| 5 | `05PORTS.Ln` | `S` | `05PROBEN.Ln` |
| 6 | `06IRQ.Ln` | `I` | `06IRQTST.Ln` |
| 7 | `07DISK.Ln` | | |
| 8 | `08LAUFW.Ln` | `M`, `T`, `B` | `08MOTOR.Ln`, `08SCHRIT.Ln`, `08BSPUR.Ln` |
| 9 | `09COPRO.Ln` | `P` | `09COPROT.Ln` |
| 10 | `10DEUTNG.Ln` | | |

Nach einem Sondertest wird die Seite mit dem Ergebnis neu aufgebaut;
dieser Stand kommt unter dem Namen des Tests auf die Diskette, bevor
die Seite verlassen, `W` gedrückt oder der nächste Test gestartet wird.
Die Seitendatei enthält dann ebenfalls die Ergebnisse der gelaufenen
Tests. Innerhalb eines Laufs ersetzt ein gleicher Name die vorige
Fassung. Was sich seit dem letzten Schreiben nicht geändert hat
(Prüfsumme), wird nicht noch einmal geschrieben. Während des Schreibens
steht in der Fußzeile „sichere … auf A:“.

Cluster werden aus der FAT vergeben (je Datei meist einer): erst die
Daten in freie Cluster, dann beide FATs, dann das Verzeichnis; die alte
Fassung wird erst danach freigegeben. Platz: 112 Verzeichniseinträge,
also etwa 7–9 Läufe. Dateien `DMVDIAG*.TXT` (0.12–0.14) und `nn….TXT`
(Vorab-0.15) werden gelöscht.

Schlägt das Schreiben fehl (fremde Datei, schreibgeschützt, Verzeichnis
oder Diskette voll, Lauf 99 erreicht), zeigt ein Bild einmal Datei,
Schritt und Ergebnisbytes des 8272; danach ist das Sichern aus
(Fußzeile „D: Sichern an“), bis `D` gedrückt wird. Hinweis zu `M`: Weil
das Sichern beim Seitenwechsel den Motor startet, misst „Warten auf
Motor aus“ die Zeit ab diesem Schreiben.

**Brücken S1/S2 ohne Ausbau.** Seite 6 deutet die geschaltete Leitung
nach dem K235-Schaltplan: IR3 = Platz 3, IR4 = Platz 4, IR5 = Platz 2
(S2 1–3) oder 2a (S2 1–2), IR7 = Platz 5 (S1 1–2) oder 6 (S1 1–3).
Zusammen mit dem Platz, in dem die Karte steckt, folgt die Stellung der
Brücke: schaltbare Karten in Platz 5 und 6 gleichzeitig → genau eine
schaltet IR7, das ist S1. Eine Karte in Platz 2, die IR5 nicht schaltet
(wohl aber ihr Bit in 13h), heißt S2 auf 2a. In MAME (Vorgabe S1 = 5,
S2 = 2a): K210 in Platz 5 → IR7, K211 in Platz 6 → keine IR; K210 in
Platz 2 → 13h Bit 6, keine IR.

- Geschrieben wird nur auf eine DMVDIAG-Diskette: Der erste
  Verzeichnissektor muss den Datenträgernamen `DMVDIAG…` tragen, und
  im Verzeichnis dürfen nur die eigenen Ergebnisdateien stehen. Dann gehört
  die FAT allein diesen Dateien und wird neu geschrieben (beide Kopien).
- Spurverdoppelung für das 80-Spur-Werk aus `FE08`, bei „Sektor nicht
  gefunden“ umgeschaltet (wie der Bootsektor).
- Speicher: Programm 4000h–9FFFh, Textpuffer A000h–AFFFh (eine Seite,
  höchstens 4 KB), FAT D800h, Verzeichnis (7 Sektoren) DC00h–E9FFh
  (alles unter der Firmware ab F000h). `mkimg.py` baut das Programm zweimal,
  weil es seine eigene Sektorzahl braucht (Lage von FAT und Verzeichnis).

In MAME getestet (0.15): alle Seiten durchgeblättert, Tests K, S, I, M,
T, B mit eigenen Dateien, drei Starts mit verschiedener Bestückung
(`L1`–`L3`, FAT beider Kopien gleich, keine verlorenen Cluster),
zweimal `D` ersetzt dieselbe Datei, alte `….TXT` gelöscht, fremde
Datei im Verzeichnis (Fehlerbild Schritt 3, danach „D: Sichern an“). Nicht getestet: schreibgeschützte
Diskette (MAME schützt ein schreibgeschütztes Abbild nicht).

### Wie die Proben gemeint sind

- **Offener Bus.** Ein unbestückter Anschluss gibt zurück, was zuletzt
  auf dem Bus lag. Jede Lesung folgt darum einer Speicherschreibung mit
  einem anderen Wert; ein Register antwortet dreimal gleich, ein offener
  Bus folgt dem Muster. Auch die 8259-Probe schreibt zwischen `OUT` und
  `IN` das Gegenmuster in den Speicher.
- **Takt.** `AAM` ist reine Registerarbeit mit fester Taktzahl (8088: 83,
  V20: 15) und holt nur zwei Befehlsbytes; Wartezyklen am Bus fallen
  kaum ins Gewicht. 1600 Stück gegen 8253 Kanal 2 (2 µs je Schritt),
  die leere Schleife abgezogen. Ohne sicher erkannte CPU kein Ergebnis.
- **Mono oder Farbe.** Die Farbkarte hat drei Ebenen zu 16 K Worten
  (0000h, 4000h, 8000h), die Monokarte eine. Zwei Muster nach 3FF0h und
  7FF0h (außerhalb der 2000 Textworte), zurücklesen: beide da = drei
  Ebenen; das zweite überschreibt das erste = Spiegel, eine Ebene.
  Angezeigt wird „Farbe" oder „Mono" nur, wenn Firmwarekennung und Probe
  übereinstimmen, sonst „Schirm?". Dabei wird auch gezählt, ob RDAT
  mehr Bytes liefert als verlangt (Frage aus dem GEM-Thread).
- **Spurzahl von A:.** Das Z80-ROM liest vor dem Laden eine Kennung auf
  dem physischen Zylinder 2 und legt das Ergebnis in `FE08`/`FE09` ab.
  Von unserer 40-Spur-Diskette gestartet heißt `FE08 = 0` ein
  40-Spur-Werk, `1` ein 80-Spur-Werk – ohne eigene Kopfbewegung.
- **8253.** Nur Kanal 2 wird gesetzt (Modus 2, voller Umfang). Kanal 0
  und 1 hängen am Bildtakt und an der Motor-Auszeit und bleiben, wie sie
  sind.

## Stand

Im eigenen Prüfstand (Unicorn mit nachgebildeter DMV, Firmware-DISPLAY
als Haken; dort muss die CPU-Zeile „unbekannt" sagen) und in MAME
getestet. 0.11/0.12 in MAME: Seite 5 erkennt K210, K211, K212, K215 (HLE
des Forks) und K801, Seite 3 die K233. Die Bausteinproben sind noch
nicht auf echter Hardware gelaufen.
0.14 in MAME, Taste I: leere Maschine IRR F9 (IR3/4/5/7 über die
Pull-ups, dazu Zeitgeber und Diskette); K801 auf Platz 3 → 13h Bit 5,
IR3; K211 auf Platz 4 → 13h Bit 4, IR4; K210 auf Platz 5 (PC3) → IR7
(Platz 5 erscheint in MAMEs 13h nicht). Drei Sicherungen
(`DMVDIAG3`, `DMVDIAG5`, `DMVDIAG`) mit getrennten FAT-Ketten.
0.13 in MAME, Taste M: 234,33 ms / 156,24 ms für 39 Schritte mit D6 / E6,
also 6,00 / 4,00 ms je Schritt; READY 258 ms nach Motor ein.

## Geplant

- Aktive Proben auf Tastendruck: K806 wie MOUSETST, K804 (µPD7210),
  K803 über x4–x7.
- Die Klärungsproben von HWCONFIRM auf demselben Kern.
- Die Erkennung als gemeinsames Include für die Startbildschirme.

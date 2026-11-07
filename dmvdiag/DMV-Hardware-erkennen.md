# NCR Decision Mate V – Hardware erkennen

Anleitung für Betriebssysteme, Treiber und Programme (Stand 2. 11. 2026)

Diese Anleitung sagt, wie Software die bekannte Hardware der DMV
selbst feststellt, und woher jede Regel kommt. Sie fasst zusammen, was
mit den Diagnosedisketten DMVDIAG (0.02–0.17) und DMVFRAGE (1.00/1.01)
auf zwei echten Maschinen gemessen und in MAME gegengeprüft wurde.
DMVINFO (ab 1.00, K233 ab 1.03) setzt die Regeln als übersichtliche Hardwareinfo um;
der Konfigurator (`tools/konfig/index.html`) zeigt zu jeder geplanten
Belegung, was davon erkannt wird.

**Belegstufen** (hinter jeder Regel in eckigen Klammern):

- **[H]** auf echter Hardware gemessen. Maschine A: Farbe, ROM C.07.00,
  K235 mit 8088, ohne 8087. Maschine B: Mono, ROM M.07.00, K235 mit V20
  und 8087. Beide haben K208 und K234.
- **[D]** aus NCR-Unterlagen (Schaltpläne, Kit-Anleitungen, NCR-Software).
- **[M]** nur in MAME (Fork rfka01/mame) geprüft.

Quelltexte: `rfka01/dmv_goodies`, Verzeichnis `dmvdiag/` (diese
Anleitung liegt dort mit). Konstanten stehen in `rfka01/dmv_os`,
`hw/dmvhw.def`, die Kartendaten in `hw/dmvmod.json` (dort je Karte auch
die Probe, Feld `erkennung`); der Konfigurator in `tools/konfig/`.

---

## 0. Grundregeln, bevor man misst

1. **Der leere Steckplatzbus liefert die eigene Portadresse zurück.**
   Ein `IN` von einem unbelegten IFSEL-Anschluss gibt das **untere Byte
   der Portadresse** zurück, also B0h von B0h und C3h von C3h. Ursache
   ist der gemultiplexte Adress- und Datenbus des 8088: Die Adresse aus
   dem ersten Buszyklus bleibt auf den Leitungen stehen. Ein vorher in
   den Speicher geschriebenes Füllmuster ändert daran nichts.
   **Regel: Ein gelesener Wert, der dem unteren Adressbyte gleicht,
   heißt „nichts da“.** [H]
   MAME liefert an dieser Stelle FFh, das Echo gibt es dort nicht. Eine
   Probe, die nur gegen FFh prüft, läuft in MAME und scheitert auf
   echter Hardware.
2. **Jede Lesung mehrfach machen und jedes Ergebnis begründen.** Ein
   echtes Register antwortet mehrmals gleich und reagiert auf ein
   Schreiben so, wie der Baustein es vorschreibt. Ein bloßer Wert
   beweist nichts.
3. **Nie von x4–x7 einer 2651-Karte lesen (K211, K212, K213).** Dort
   liegt BA2 am R/W-Eingang des 2651. x0–x3 sind die Leseregister,
   x4–x7 die Schreibregister. Ein `IN` auf x4–x7 schreibt in den
   Baustein. [D, durch ASCOM-Treiber und Messung bestätigt]
4. **Von x3 einer K210 (8255) nicht lesen.** Das Steuerwort ist nicht
   lesbar, der Wert schwebt. [D]
5. **IFSEL 4 ist Platte (4A) und Uhr (4B).** Allgemeine Bausteinproben
   lassen IFSEL 4 aus. Für diese beiden Geräte gibt es eigene Proben
   (Abschnitt 4).
6. Proben, die etwas schalten (Register schreiben, Leitungen kippen),
   **nur auf ausdrückliche Anforderung** laufen lassen und den alten
   Zustand wiederherstellen. Beim Start nur lesen.

---

## 1. Hauptplatine und Firmware

| Was | Wie | Beleg |
|---|---|---|
| **Firmwarekennung** | `OUT 11h` legt das 8-KB-ROM über 0000h–1FFFh, `OUT 10h` schaltet zurück. Der Code muss dabei **über 2000h** liegen. Bei 0FF7h stehen ein Zählbyte (08h) und 8 Zeichen, z. B. `" C.07.00"`. Das Zeichen bei 0FF9h ist `M` (Mono) oder `C` (Farbe). | [H] |
| **ROM sicher zuordnen** | CRC-32 über alle 8 KB: 120951B6 = M.07.00 (33609), BF25F3F0 = C.07.00 (33610). Weitere: M.05.00 (32664), M.06.00 (32676), C.06.00 (32838). | [H] für 07.00 |
| **Postfach der Firmware** | 0:FE00–FE3F, vom Z80-ROM beim Start gefüllt. FE04 = 32h heißt „16-Bit-Start“. FE06 ist die Zahl der Erweiterungsbänke, FE07 = 1 heißt „von Platte gestartet“. FE08/FE09 siehe Abschnitt 4. | [H] |
| **Tastatur und Landescode** | Befehl 01h an 41h, dann auf Status Bit 0 (an 41h) warten, Datenbyte an 40h. Landescode = Daten − E8h, z. B. EBh = 3 = Deutsch. Antwort nach 2–8 ms. | [H] |

---

## 2. Prozessoren

### 2.1 Welche 16-Bit-Karte

| Ergebnis | Merkmal | Beleg |
|---|---|---|
| **16-Bit-Karte vorhanden** | Anschluss 13h Bit 1 = 0 (das Bit ist negiert) | [D] |
| **K235** (mit 8259) | ROM der 8088-Karte bei FF00:0000, 4 KB, CRC-32 104195DC = ROM 33473. Dazu antwortet der 8259: ICW-Folge an 90h/91h, AAh und 55h ins Maskenregister schreiben und zurücklesen. | [H] |
| **K230 / K231** (ohne 8259) | ROM 32167. Die Karten sind elektrisch gleich, K230 steckt intern und K231 extern; Software kann sie nicht unterscheiden. | [D, M]; K230 auf Foto: ROM 32167, Quarz 15 MHz |

Wichtig: Die K230/K231 wertet BA3 nicht aus und nimmt D8h–DFh als D0h,
also als Prozessorumschaltung. Eine K234-Probe ist dort verboten. [D]

### 2.2 8088 oder V20

| Test | 8088 | V20 | Beleg |
|---|---|---|---|
| `6A 00` (beim V20 `PUSH 0`, beim 8088 ein bedingter Sprung um 0); SP vorher − nachher | 0 | 2 | [H] |
| `MOV AX,0102h` und `D5 10` (AAD mit Basis 16) | AL = 12h (die Basis zählt) | AL = 0Ch (der V20 nimmt immer 10) | [H] |

Beide Tests müssen übereinstimmen, sonst gilt „CPU?“.

### 2.3 8087

Das Status- und das Steuerwort vorher mit 5A5Ah belegen, dann FNINIT,
FNSTSW und FNSTCW, jeweils mit kurzer Warteschleife dazwischen.

- **Ohne 8087** bleiben beide Wörter auf 5A5Ah. [H]
- **Mit 8087** ergibt sich CW = 03FFh. Das SW war 0000h oder 4100h;
  entscheidend ist das CW. [H]
- MAME liefert CW = 037Fh, also gilt „CW ≠ 5A5Ah“ als 8087 vorhanden.

### 2.4 Takt

AAM ist reine Registerarbeit mit fester Taktzahl. Dafür 1600 × AAM
gegen 8253 Kanal 2 messen (Modus 2, voller Zählumfang, 2 µs je
Schritt) und die leere Schleife abziehen.

- **V20:** 15 Takte je AAM ergeben auf Maschine B genau **5,00 MHz**. [H]
- **8088:** Mit den 83 Takten aus dem Datenblatt ergeben sich auf
  Maschine A 5,38 MHz. Das spricht dafür, dass der echte 8088 etwa
  **77 Takte** braucht und auch mit 5,00 MHz läuft. [H, Deutung offen]
- Die K230 hat einen 15-MHz-Quarz (8284 teilt durch 3), also 5,00 MHz.
  [H, Foto der Platine]
- Es gibt eine umgebaute K235 mit V20 und Quarz 22,1984 MHz, also
  7,40 MHz. Der Takt ist deshalb zu messen, nicht anzunehmen. [D]

### 2.5 Koprozessorkarte K234 (68008) und K238 (Z8001, nur MAME)

Nur mit K235. Der 8088 legt ein kleines Z80-Programm an den Warteplatz
0:F111h. Dorthin springt der Z80 nach jeder Rückgabe des Busses. Das
Programm macht **`OUT DFh`**, setzt eine Marke und gibt mit `OUT D0h`
zurück; der 8088 gibt den Bus mit `OUT D0h` an den Z80 ab.

Der 68008 holt SSP und PC aus 0:0000–0007, schreibt ein Kennbyte nach
0:0000 und gibt mit einem Schreibzugriff auf **FFFDFh** zurück
(`move.b …,$FFDF.w`). Danach Vektoren und Warteplatz wiederherstellen.

- Kennbyte 36h und Z80-Marke 5Ah: K234 vorhanden. [H, beide Maschinen]
- Mit `OUT D8h` lief die echte K234 **nicht** an. Nur DFh nehmen, wie
  NCRs CP/M-68K. MAME nimmt D8h–DFh gleich. [H]
- **K238** (Z8001, Eigenbau, kein NCR-Produkt, bisher nur in MAME): Sie
  startet über denselben Weg (`OUT DFh` vom Z80). Die Probe an 0:0000
  ist deshalb **zweisprachig**: Für den 68008 stehen dort SSP und PC,
  für den Z8001 im selben Bereich sein Startwort (FCW, Segment, PC), und
  der Code ist so gelegt, dass jeder Prozessor nur seinen Teil
  ausführt. Der Z8001
  schreibt **5Ah („Z“)** nach 0:0000 und meldet mit `OUTB DFh` zurück.
  Kennbyte 5Ah und Z80-Marke 5Ah: K238 vorhanden. [M]
- Kommt der Z80 nicht zurück (Marke fehlt), ist der Ablauf gestört; kommt
  er zurück, aber das Kennbyte bleibt unverändert, steckt kein
  Koprozessor oder er antwortet nicht.

---

## 3. Speicher und Bildschirm

| Was | Wie | Beleg |
|---|---|---|
| **Erweiterungsbänke** | Für jede Bank n = 1–7 ein eigenes Muster nach n:FFF0 schreiben (A50nh und das Komplement), erst alle schreiben, dann alle lesen. So fällt auch eine gespiegelte Bank auf. Nur zusammenhängende Bänke ab 1 zählen: 1 Bank = K200 (64 KB), 3 = K202 (192 KB), 7 = K208 (448 KB). Gegenprobe ist FE06 im Postfach. | [H] |
| **Mono oder Farbe** | GDC-Ebenenprobe: Die Farbkarte hat drei Ebenen zu 16 K Worten, die Monokarte eine. A5C3h nach Wort 3FF0h und 3C5Ah nach 7FF0h schreiben (außerhalb der 2000 Textworte), in der Reihenfolge 7FF0h, 3FF0h zurücklesen und beide Wörter wiederherstellen. Bleiben beide stehen: drei Ebenen, Farbe. Überschreibt das zweite das erste: eine Ebene, Mono. „Farbe“ oder „Mono“ nur melden, wenn die ROM-Kennung (M/C) übereinstimmt. | [H] |

---

## 4. Laufwerke, Platte, Uhr

| Was | Wie | Beleg |
|---|---|---|
| **Spurzahl von A:** | Das Z80-ROM liest beim Start eine Kennung auf dem physischen Zylinder 2. FE08 = 0 heißt 40-Spur-Werk, FE08 = 1 heißt 80-Spur-Werk (Doppelschritt), wenn von einer 40-Spur-Diskette gestartet wurde. Es braucht keine eigene Kopfbewegung. | [H] |
| **Spurzahl von B:** | Mit einer Diskette bekannten Formats in B: RECALIBRATE, SEEK 2, READ ID. C = 2 heißt 40 Spuren; C = 1 oder N = 3 heißt 80 Spuren. | [H] |
| **Laufwerk vorhanden** | Ohne Diskette meldet das Laufwerk nicht „bereit“, und der 8272 macht keinen Schritt. „Nicht bereit“ beweist also kein fehlendes Laufwerk. Mit Diskette: RECALIBRATE, SEEK 2, RECALIBRATE; Spur 0 (ST3 Bit 4) geht aus und wieder an. | [H] |
| **8272-Takt** | SPECIFY D6h 02h, dann je SEEK 0→39 und 39→0 mit Zeitmessung. Gemessen 6,18 ms je Schritt, also 8272 mit **4 MHz**. Mit E6h gemessen 4,1 ms. | [H] |
| **Motor** | `OUT 14h` schaltet ihn ein. 13h **Bit 0 = 1 heißt „Motor aus“** (in `hw/dmvhw.def` von dmv_os steht es noch falsch herum). READY (13h Bit 2) kommt 0,52–0,63 s nach dem Einschalten. Nicht mit fester Zeit warten, sondern READY mit Zeitgrenze abwarten. | [H] |
| **Festplattencontroller K012/C3282** (K012 intern auf Platz 2a, C3282 extern auf Platz 2–6; derselbe WD1001 auf 4A, Software kann sie nicht unterscheiden) | 55h nach C2h und AAh nach C3h schreiben, zurücklesen. Nur **55h/AAh** zählt; C2h/C3h ist das Echo des leeren Busses. Danach ein Status von 00h auf C7h für jede Einheit heißt „Controller da, keine Platte bereit“. Nach jedem Zugriff 18h nach C6h (abwählen). | [H] |
| **Uhr K803** (MM58167, Vorgabe 4B) | 00h nach C8h (Latch), dann CEh (Sekunden) zweimal im Abstand von 1,2 s lesen. Beide Werte gültiges BCD 00–59 und verschieden: Uhr läuft. Ein Wert von CEh ist das Echo, also keine Uhr. Register nur über x4–x7 ansprechen. Die Karte ist auf jede Hälfte schaltbar; DMVINFO prüft nur 4B. | [H] |

---

## 5. Steckkarten an IFSEL 0–3

Die Lage der Karten:

| IFSEL | A-Hälfte | B-Hälfte |
|---|---|---|
| 0 | 60h | 68h |
| 1 | 70h | 78h |
| 2 | 30h | 38h |
| 3 | B0h | B8h |
| 4 | C0h (Platte) | C8h (Uhr) |

Wie viel eine Karte belegt:
- **Ganze Gruppe**, weil sie BA3 nicht auswertet: K210, K211, K212,
  K213. Sie antworten in A und B gleich.
- **Eine Hälfte:** K801, K215, K806, K803.
- **Ein Viertel:** K216.

**Reihenfolge der Proben:** Jede Hälfte der Reihe nach prüfen (0A …
3B). Was erkannt ist, sperrt die Hälfte (bei Gruppenkarten beide) für
alle weiteren Proben.

| Karte | Baustein | Probe | Beleg |
|---|---|---|---|
| **K210** Centronics | 8255, nur IFSEL 0 | Steuerwort AAh nach x3; PC0–PC2 wie vorher setzen; INTE A über die Bitsteuerung PC6 setzen und löschen, Port C (x2) folgt in Bit 6. Port A nie beschreiben, sonst geht ein Zeichen an den Drucker. | [H] |
| **K211 / K212 / K213** | 2651, IFSEL 0 oder 1 | Befehlsregister x3 lesen (alter Wert), 23h über x7 schreiben und über x3 zurücklesen, dann 03h ebenso, alten Wert zurück. Antwortet die B-Hälfte (xB) gleich, ist es eine Gruppenkarte. K211, K212 und K213 unterscheiden sich nur durch Brücke und Kabel. | [H] |
| **K801** | 2661 | Wie oben, aber nur eine Hälfte antwortet (die Karte vergleicht BA3). | [H] |
| **K215** gepuffert | 8741 und 2651 | Vorprüfung: Status x1 = 00h. Dann Befehl 0Bh (Echo) an x1, ein Byte an x0, die Antwort an x0 abholen, dabei auf Status Bit 0 warten. 55h und AAh kommen zurück: K215. Befehle: 00h Befehlsregister und 01h Modusregister des 2651, 03h dessen Status, 08h Zeichen lesen, 09h Zeichen schreiben, 10h Selbsttest. Vorgabe 1A; neben einer K211 auf 3A (B0h). | [H] |
| **K806** Maus | 8741 | Statusbit 3 (F1) zeigt Adressbit 0 des letzten Schreibens. Danach liefert Befehl 00h genau fünf Datenbytes. Vorgabe 2A, erste zwei Anschlüsse. | [H] |
| **K216** | Z8530 SCC, je Viertel | WR12 schreiben, RR12 lesen (55h/AAh). | [D] (keine Hardware) |
| **K233** gemeinsames RAM | – | Schaltet bei jedem Schreiben auf 7Eh/7Fh um, das Fenster liegt bei C000h; kein Statusregister, beim Lesen von 1B antwortet sie nicht (sieht aus wie „frei“). Probe: `0:C000` retten, kippen, 5Ah schreiben, kippen, A5h schreiben und lesen („aus“), kippen und lesen („ein“), 3Ch schreiben, kippen und lesen, `0:C000` zurück. Karte da, wenn „aus“ = A5h und „ein“ = 5Ah; der letzte Wert 3Ch heißt „Hauptplatine schreibt mit“. Während die Karte an ist: CLI, kein Firmwareaufruf, Stapel unter C000h. **Nur, wenn auf 7Fh nichts Fremdes hört:** 1B frei, oder ein 2651/2661 dort (K211-Gruppe auf IFSEL 1, K801 auf 1B) – dann vor jedem Kippen dessen Befehlsregister über 7Bh lesen und genau diesen Wert auf 7Fh schreiben [M, auf echter Hardware unbestätigt]. Eine 8741-Karte (K215, K806) oder Unbekanntes auf 1B: nicht prüfen. DMVINFO ab 1.03 hält sich daran. | [D, M] |

---

## 6. Interrupts (nur K235)

### 6.1 Zustand des 8259

- **Nach dem ROM 33473:** pegelgesteuert, ICW1 1Bh, ICW4 01h (8088,
  normales EOI), Maske FFh. In diesem Zustand zeigt das IRR (OCW3 0Ah an
  90h, dann von 90h lesen) den Stand der Eingänge, auch wenn sie
  gesperrt sind. Damit misst DMVFRAGE. [H]
- **NCR DOS 2.11:** ICW1 17h (flankengesteuert), ICW4 03h (AEOI). [D]
- Die Maske nicht blind überschreiben, sondern lesen, ändern,
  zurückschreiben. [D]

### 6.2 Feste Quellen

| IR | INT | Quelle | Beleg |
|---|---|---|---|
| 0 | 08h | 8253, TIMINT | [D] |
| 1 | 09h | Tastatur (8741) | [D] |
| 2 | 0Ah | INT/ aller Steckplätze (gemeinsam): K803, K804, K806, umgebaute K211/K212 | [D] |
| 6 | 0Eh | 8272; dasselbe Signal steht in 13h Bit 3 | [H] |

### 6.3 Die eigene Leitung der Steckplätze (Stift a2)

| Platz | IR | 13h-Bit | Beleg |
|---|---|---|---|
| 2 | IR5 nur mit S2 auf 1–3, sonst keine | Bit 6 | [H] Bit, [D] Brücke |
| 2a (intern, Platte) | IR5 mit S2 auf 1–2 | – | [H] |
| 3 | IR3 | Bit 5 | [H] |
| 4 | IR4 | Bit 4 | [H] |
| 5 | IR7 mit S1 auf 1–2 | – | [H] |
| 6 | IR7 nur mit S1 auf 1–3, sonst keine | – | [D] |

- **Brücken auf beiden Maschinen gemessen:** S1 = 1–2 (Platz 5),
  S2 = 1–2 (Platz 2a). Software kann die Stellung nicht lesen.
  Messen geht so: Eine schaltbare Karte in Platz 5 schaltet IR7, dann
  ist S1 = 1–2. Ein leerer Platz 6 hält IR7 dann nicht hoch. [H]
- Alle Leitungen sind **aktiv high** und haben auf der K235
  **Pull-ups**. Ein leerer Platz 3 oder 4 und der leere Platz 2a melden
  dauernd: IR gesetzt, 13h-Bit gesetzt. [H]
- **Karten, die ihre Leitung inaktiv halten** (kein Pull-up-Pegel,
  wenn sie stecken): K210, K211, K801, K803, K806. [H]
- **Karte ohne Interruptausgang:** K215. Ihr Platz meldet dauernd wie
  ein leerer. [H]
- **Regel für ein Betriebssystem:** Einen IR-Eingang erst freigeben,
  wenn hinter ihm eine Karte steckt, die die Leitung treibt und deren
  Treiber geladen ist. Leere Plätze, Platz 2a ohne Platte und Plätze mit
  einer K215 gesperrt lassen. Ob ein Platz leer ist, zeigt die Messung
  im Ruhezustand (pegelgesteuert, IRR oder 13h-Bit gesetzt).

### 6.4 Wodurch die Karten melden

| Karte | Quelle der eigenen Leitung | Beleg |
|---|---|---|
| K210 | PC3 des 8255 | [H] |
| K211 (2651) | **nur RxRDY**. Das Einschalten des Senders (TxRDY, TxEMT) löst nichts aus. Die Handbuch-Lesart „RxRDY und TxRDY“ ist damit widerlegt. | [H] |
| K212, K213 | dieselbe Platine wie K211, vermutlich ebenfalls RxRDY | [D, nicht gemessen] |
| K801 (2661) | TxRDY (und laut Schaltbild RxRDY) | [H] TxRDY |
| C3282, K012 | WD1001 INTRQ | [D, M] |

---

## 7. Neustart aus der Software

Bisher galt: neu starten nur mit Ctrl-F20 (der Tastatur-8741 zieht
RESET). Ein 8088-Programm kann aber einen **Kaltstart des Z80-ROMs**
auslösen. [M, mit K230 und K235; auf echter Hardware noch nicht geprüft]

1. Prüfen, dass bei `FF00:0018` die Befehlsschleife des 8088-ROMs steht:
   `E6 11 E6 D0` (`OUT 11h`, `OUT D0h`). So in ROM 32167 (K230/K231)
   und 33473 (K235). Sonst abbrechen.
2. Nach `0:F111` (Warteplatz des Z80) den Z80-Code `DI`,
   `OUT (11h),A`, `JP 0000h` legen.
3. DS = ES = SS = 0, SP = 0 (wie nach `FF00:0003` im ROM), dann
   `JMP FF00:0018`.

Die ROM-Schleife gibt den Bus mit `OUT D0h` an den Z80. Der springt in
sein ROM, testet den Speicher und bootet. Den 8088 findet er in seiner
Schleife, genau wie nach dem Einschalten (ROM 05F0h: `FE04 = 33h`,
`OUT D0h`).

**Warum nicht einfach `JMP FFFF:0000` hinter dem `OUT D0h`:** Der
Speichertest des Z80 überschreibt auch den Code, den der 8088 nach der
Rückgabe noch aus dem RAM holen würde. Der 8088 läuft dann in
überschriebenes RAM, und der Z80 wartet bei ROM 05FCh ewig auf FE05.

**Grenze:** Das ist kein Hardware-Reset. 8259, Steckkarten und
K234/K238 bleiben im alten Zustand, soweit das Z80-ROM sie nicht selbst
einstellt. Ein Betriebssystem muss nach so einem Neustart seine Bausteine
selbst initialisieren.

---

## 8. Woher diese Regeln kommen

- **DMVDIAG** (selbststartend, ohne DOS, Ausgabe über die Firmware):
  Rohwerte zuerst, Deutung danach. Ergebnisse als Bildschirmfotos und
  als Textdateien auf der Startdiskette (ab 0.15 je Seite, mit
  Laufnummer).
- **DMVINFO** (ab 1.00): Seite 1 Übersicht, Seite 2 Steckplätze und
  Interrupts, Modulnummer immer zuerst (in Farbe rot), `D` schreibt
  `DMVINFO.TXT`, `X` startet neu (Abschnitt 7). Misst nach genau diesen
  Regeln.
- **DMVFRAGE 1.00:** feste Belegung, eine Taste, eine Seite. Damit
  wurden die Brücken, die 13h-Bits, die Leitungen und die
  Interruptquelle der K211 gemessen, jeweils zweimal und auf beiden
  Maschinen. Die Leitungen werden nur gewertet, wenn sie mit „an“ kommen
  und mit „aus“ wieder gehen. Bei der K211 schickt die interne
  Rückschleife des 2651 ein Byte, das zurückkommt.
- **Gegenprüfung in MAME:** Der Fork wurde nach den Messungen
  nachgeführt (K211 über RxRDY, K803/K806 halten ihre Leitung,
  K215-Vorgabe 3A). DMVFRAGE zeigt dort heute dasselbe Bild wie auf den
  Maschinen. Was MAME nicht nachbildet, ist das Echo des leeren Busses.
- **Unterlagen:** Schaltplan der K235 (017-0033502), Schaltbilder der
  Karten, NCRs Testprogramme (V1K215, V1K216) und, als zweite Quelle,
  die ASCOM-Treiber für K211 und K215 (Register und Befehle bestätigt).
  Keine dieser Quellen wird kopiert, nur ausgewertet.

## 9. Noch offen

- 13h Bit 7: „Indexloch“ nach MAME, am Gerät nicht bestätigt; ein
  einmaliger Treffer beim Schalten der K210.
- Taktzahl von AAM auf dem echten 8088 (Abschnitt 2.4).
- Neustart aus der Software (Abschnitt 7) auf echter Hardware.
- Interruptquelle von K212/K213, Verhalten von K804, K216 und K600
  (keine Hardware gemessen).
- Die DMV V2 hat laut einem ASCOM-Treiber von 1987 einen 2651 auf
  600h/601h (lesen) und 604h/606h/607h (schreiben), also dieselbe
  Anordnung in einem 16-Bit-E/A-Raum.

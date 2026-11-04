# DMVDIAG 0.15 – Testanleitung (drei Starts)

Ziel mit möglichst wenigen Starts: eigene Interruptleitungen aller
Plätze 2–6, Brücken S1/S2 beider K235, jede schaltbare Karte zweimal,
Erkennung aller Karten, Laufwerke, Takt, 8087 und K234 beider Maschinen.

Maschine A: Farbe C.07.00, K235 mit 8088, ohne 8087, K234.
Maschine B: Mono M.07.00, K235 mit V20 (7,40 MHz), C8087-1, K234.

## Vorbereitung

- `DMVDIAG-0.15.IMG` auf eine Diskette schreiben, **ohne
  Schreibschutz**. Sie bleibt in A: und sammelt alle drei Starts
  (`*.L1`, `*.L2`, `*.L3`).
- Eine zweite Kopie desselben Images für B: (nur für Taste B, wird nur
  gelesen).
- IFSEL der Karten (so kollidiert nichts):

| Karte | IFSEL | Port | Bemerkung |
|---|---|---|---|
| K210 | 0 | 60h | ganze Gruppe |
| K211 | 1 | 70h | ganze Gruppe |
| K801 | 2B | 38h | |
| K806 | 2A | 30h | |
| K215 | **3A** | B0h | NCR-Vorgabe wäre 1A, dort sitzt die K211 |
| C3282 | 4A | C0h | |
| K803 | 4B | C8h | |
| K234 | – | – | steckt in Platz 7 (Prozessorplatz), belegt keinen der Plätze 2–6 |

- Kein Drucker- oder Seriellkabel nötig.
- Für jeden Start die Belegung notieren – die Software sieht nicht,
  welche Karte in welchem Platz steckt.

## Bestückung

| Start | Maschine | Platz 2 | Platz 3 | Platz 4 | Platz 5 | Platz 6 | Platz 7 |
|---|---|---|---|---|---|---|---|
| 1 → L1 | A | K801 | K806 | K803 | K210 | K211 | K234 |
| 2 → L2 | A | K215 | K210 | K211 | C3282 | K801 | K234 |
| 3 → L3 | B | K801 | K806 | K803 | K210 | K211 | K234 |

Was jeder Start klärt:

- **Start 1 und 3:** S1 – von K210 (Platz 5) und K211 (Platz 6)
  schaltet genau eine IR7. S2 – K801 in Platz 2 schaltet IR5 (S2 1–3)
  oder nur ihr Bit in 13h (S2 auf 2a). K806/K803 treiben die eigene
  Leitung nicht: in Ruhe IR3 und IR4 über die Pull-ups gesetzt. Dazu
  Laufwerke (M, T, B) und K234 (P) der Maschine.
- **Start 2:** Plätze 3 und 4 (K210 über PC3? K211), Gegenprobe S1 mit
  K801 in Platz 6, Erkennung K215 (3A) und C3282 (Seite 7).

## Tasten

Nach dem Booten steht Seite 1. „weiter“ = Leertaste.

| Seite | Start 1 und 3 | Start 2 | Hinweis |
|---|---|---|---|
| 1 System | weiter | weiter | |
| 2 Prozessor | weiter | weiter | Takt, V20/8088, 8087 |
| 3 Speicher | weiter | weiter | **nicht K**: die K233-Probe schreibt auf 7Fh, das ist mit der K211 auf IFSEL 1 deren Befehlsregister |
| 4 Module | weiter | weiter | |
| 5 Schnittstellen | **S**, weiter | **S**, weiter | |
| 6 Interrupts | **I**, Foto, weiter | **I**, Foto, weiter | die Seite zeigt nach I „FOTO“ |
| 7 Laufwerke, Platte, Uhr | weiter | weiter | Uhr (K803) und Platte (C3282) passiv |
| 8 Laufwerke aktiv | **M**, **T**, **B**, weiter | weiter | M wartet bis 25 s; vor B die zweite Diskette in B: |
| 9 Koprozessor | **P**, weiter | weiter | steht die Maschine nach P: Foto, das ist ein Befund |
| 10 Deutung | Foto, **X** | Foto, **X** | die Seite zeigt „FOTO“ |

## Wichtig

- Gesichert wird beim Verlassen einer Seite – immer mit **X** enden,
  erst dann ausschalten.
- Fehlerbild „nicht gespeichert“: fotografieren, mit **D** neu
  versuchen.
- Schaltet eine serielle Karte in keinem Platz eine IR, kann das am
  Sendefreigabe-Verhalten ohne Kabel liegen und nicht an der Leitung
  (in MAME geht es ohne Kabel, auf echter Hardware ungeprüft).

## Zurück

Dateien `*.L1`–`*.L3` (oder das Image), die Belegungszettel und die
Fotos von Seite 6 und 10. Daraus: Steckplatz → IR, 13h-Bits und S1/S2
nach `docs/`, `hw/dmvmod.json` (Beleg H) und MAME.

## Gegenprobe in MAME (Fork rfka01/mame)

Dieselben drei Starts mit `-slot7a k235 -slot7 k234` und den Plätzen wie
oben, Diskette in `-flop1`, Kopie in `-flop2`. MAME-Vorgaben: S1 auf
Platz 5, S2 auf 2a, K215 auf 3A, CPU immer V20 mit 5 MHz. Ergebnis
0.15: Start 1/3 K210 (5) → IR7, K211 (6) → keine IR, K801 (2) → 13h
Bit 6, keine IR; Start 2 K210 (3) → 13h Bit 5, IR3; K211 (4) → 13h
Bit 4, IR4; K801 (6) → nichts; K215 auf 3A erkannt, C3282 55/AA, K234
antwortet.

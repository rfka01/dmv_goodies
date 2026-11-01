# DMVGRAF für Turbo Pascal 3

*English below.*

Dieselbe Bibliothek wie die C-Fassung, nach Pascal übersetzt, als Include-Dateien für **Turbo Pascal 3.0x unter MS-DOS** (8088/V20) und **unter CP/M-80** (Z80 der Hauptplatine) auf der DMV. Namen, Parameter und Verhalten wie in [`../README.md`](../README.md) beschrieben. Unter CP/M-80 passt die ganze Bibliothek nicht in 64 KB; dort gibt es eine kleine Fassung der Bildfunktionen.

| Datei | Inhalt |
|---|---|
| `DMVHWD.INC` | Hardwareschicht MS-DOS: 7220, K806 und 8741 über `Port[]`, Tastatur, Text und Dateien über `MsDos`, Bildspeicher in 64-KB-Blöcken von DOS |
| `DMVHWC.INC` | Hardwareschicht CP/M-80: dieselbe Hardware über `Port[]`, Tastatur und Text über BDOS 6 und 2, Dateien über FCB, Bildspeicher als Block auf dem Heap (bis 16 KB) |
| `DMVFONT.INC` | Schrift (8 × 16, mit Umlauten), erzeugt von `tools/mkfont.py` |
| `DMVGRAF.INC` | die Bibliothek: Figuren, Text, Muster, Maus, Tastatur, Zeit, Ton |
| `DMVBILD.INC` | Bilder vollständig: alle Formate, `gbmap`, `gbget`, Zusammensetzen (`glayer`, `glput`, `gldraw`), Mauszeiger (`gzeig`) |
| `DMVBKL.INC` | Bilder, kleine Fassung für CP/M-80: Formate 1, 2, 4 und 5 mit `gbdef`, `gbload`, `gbput`, `gbfree`, `gbinfo`; die übrigen Bildfunktionen setzen `gerr` (`GE_FORM`, `GE_LAGE`) |
| `GTEST.PAS`, `GTESTZ.PAS`, `GTESTK.INC` | Prüf- und Vorführprogramm (wie `test/GTEST.C`) für MS-DOS bzw. CP/M-80, gemeinsamer Hauptteil |

## Einbinden

    program Mein;
    {$I DMVHWD.INC}           { CP/M-80: DMVHWC.INC }
    {$I DMVFONT.INC}
    {$I DMVGRAF.INC}
    {$I DMVBILD.INC}          { CP/M-80: DMVBKL.INC }
    begin
      gstart;                 { zuerst: Anfangswerte setzen }
      ginit;
      if gerr <> 0 then ...
      gpoint(100, 50); gline(500, 300);
      while gkey = 0 do ;
      gexit;
    end.

Unterschiede zur C-Fassung, weil Turbo Pascal 3 keine Funktion als Anweisung aufrufen kann und keine vorbelegten Variablen kennt:

- **`gstart`** muss als Erstes aufgerufen werden.
- Was in C einen Fehlercode liefert, ist eine **Prozedur**; der Code steht danach in **`gerr`** (0 = gut). Funktionen sind nur `gkey`, `grnd`, `gdirekt` und `mpos` (`boolean`: gelesen).
- `ginfo(f, m, s, c)` ist eine Prozedur; `gbinfo(nr, r)` füllt ein `GBInfoArr` (`array[0..5] of integer`).
- Zeichenketten sind `GStr` (`string[255]`), Bilddaten für `gbdef` beliebige Variablen (untypisierter `var`-Parameter). Format 5 erwartet `integer`-Worte, wie Turbo Pascal sie ablegt.

**Wichtig beim Übersetzen:** Unter *Options* den Wert **„mAximum free dynamic memory“** herabsetzen, etwa auf `0400` (16 KB für Heap und Stapel). Sonst nimmt das Programm allen Speicher und DMVGRAF bekommt von DOS keinen Bildspeicher (`gbdef`, `gbload` und `gzeig` melden dann `GE_PLATZ`). Das Programm als `.COM` übersetzen (*Options* → *Com-file*).

GTEST belegt unter MS-DOS 41 KB Code und 12 KB Daten. Unter CP/M-80 (mit `DMVBKL.INC`) reicht der Code bis 9F75h, die Daten belegen 10 KB; bei einem 56-KB-System (BDOS ab E406h) bleiben knapp 7 KB frei.

**Unterschiede zwischen den Turbo-Pascal-Fassungen:** TP3 für den Z80 rechnet `mod` mit negativen Zahlen anders als die 8086-Fassung; die Bibliothek benutzt `mod` darum nur mit nicht negativen Zahlen. `Addr` liefert unter CP/M eine Zahl, unter DOS einen Zeiger; die Hardwareschicht stellt dafür `hwAdr` bereit.

## Stand

- **MS-DOS:** Im Prüfstand (`tools/pruefdos.py`, 8086 in Unicorn) zeigt das mit Turbo Pascal 3.01A übersetzte `GTEST.COM` in Farbe und mono dieselben Bilder wie die C-Fassung; ebenso stimmen ein Test des Mauszeigers (`gzeig`) und das Laden eines TIL-Steinsatzes mit `gbload` samt Zusammensetzen mit der C-Fassung überein.
- **CP/M-80:** Im Prüfstand (`tools/pruefcpm.py`, Z80 mit nachgebildetem CP/M 2.2) zeigt `GTESTZ.COM` (TP3 3.01A für CP/M) Figuren und Text in Farbe und mono wie die C-Fassung; im Bild der Bilder fehlt genau, was `DMVBKL.INC` nicht enthält.
- In MAME und auf echter Hardware noch nicht gelaufen.

Am PC übersetzen: `python3 tools/tp3.py VERZ GTEST.PAS` (MS-DOS) – `VERZ` enthält `TURBO.COM` (Borland, nicht im Repository) und die Pascal-Dateien mit CR/LF. Für CP/M-80: `python3 tools/pruefcpm.py VERZ TURBO.COM --tasten "YWGTESTZ\rOCQC"` mit dem `TURBO.COM` der CP/M-Fassung.

## English

DMVGRAF translated to Pascal as include files for **Turbo Pascal 3.0x under MS-DOS and CP/M-80** on the DMV. Include the hardware layer (`DMVHWD.INC` for DOS, `DMVHWC.INC` for CP/M-80), `DMVFONT.INC`, `DMVGRAF.INC` and the image part (`DMVBILD.INC`, or the small `DMVBKL.INC` on CP/M-80, where the whole library does not fit into 64 KB) in that order and call `gstart` first. Functions that return an error code in C are procedures here and leave the code in `gerr`; `gkey`, `grnd`, `gdirekt` and `mpos` are functions. When compiling, lower **"mAximum free dynamic memory"** in the options (e.g. to `0400`), otherwise the program takes all memory and DMVGRAF gets no image memory from DOS. Compiled with Turbo Pascal 3.01A, `GTEST.COM` gives the same screens as the C version in the test bench; under CP/M-80 (`tools/pruefcpm.py`) figures and text match as well. Not yet run in MAME.

Code: Claude (Anthropic), idea and prompts: rfka01. Licence: MIT.

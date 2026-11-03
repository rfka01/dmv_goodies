# DMVGRAF für Turbo Pascal 3

*English below.*

Dieselbe Bibliothek wie die C-Fassung, nach Pascal übersetzt, als Include-Dateien für **Turbo Pascal 3.0x unter MS-DOS** (8088/V20) und **unter CP/M-80** (Z80 der Hauptplatine) auf der DMV. Namen, Parameter und Verhalten wie in [`../README.md`](../README.md) beschrieben. Weil TP3 keinen Binder hat und alles einbindet, ist die Bibliothek in Teile zerlegt; ein Programm nimmt nur, was es braucht.

| Datei | Inhalt |
|---|---|
| `DMVHWD.INC` | Hardwareschicht MS-DOS: 7220, K806 und 8741 über `Port[]`, Tastatur, Text und Dateien über `MsDos`, Bildspeicher in 64-KB-Blöcken von DOS; innere Schleifen in 8086-Maschinensprache |
| `DMVHWC.INC` | Hardwareschicht CP/M-80: dieselbe Hardware über `Port[]`, Tastatur und Text über BDOS 6 und 2, Dateien über FCB, Bildspeicher als Block auf dem Heap (bis 16 KB); innere Schleifen in Z80-Maschinensprache |
| `DMVFONT.INC` | Schrift (8 × 16, mit Umlauten), erzeugt von `tools/mkfont.py` |
| `DMVGRAF.INC` | Kern: Ein/Aus, Zeichenpunkt, Farbe, Betriebsart, Muster und Flächen (`gbox`), Text, Tastatur, Zeit, Ton mit Warteschlange, `ginfo`, `gdmv` |
| `DMVFIG.INC` | Linien, Rechtecke, Kreise, Bögen (`gline`, `grect`, `gcircl`, `garc`) |
| `DMVMAUS.INC` | Maus (`mpos`, `mset`) |
| `DMVSPR.INC` | Figuren über GCHRD (`gsprdef`, `gsprset`, `gspr`); vorher `const SPMAX = …;` |
| `DMVBILD.INC` | Bilder vollständig: alle Formate, `gbmap`, `gbget`, Zusammensetzen (`glayer`, `glput`, `gldraw`), Mauszeiger (`gzeig`) |
| `DMVBKL.INC` | Bilder, kleine Fassung: Formate 1, 2, 4 und 5 mit `gbdef`, `gbload`, `gbput`, `gbfree`, `gbinfo`; die übrigen Bildfunktionen setzen `gerr` (`GE_FORM`, `GE_LAGE`). Die Zahl der Bilder legt das Programm fest (`const BKMAX`). |
| `GTEST.PAS`, `GTESTZ.PAS`, `GTESTK.INC` | Prüf- und Vorführprogramm (wie `test/GTEST.C`) für MS-DOS bzw. CP/M-80, gemeinsamer Hauptteil |
| `STEST.PAS`, `STESTZ.PAS`, `STESTK.INC` | Figuren über GCHRD (wie `test/STEST.C`) |
| `PTEST.PAS`, `PTESTZ.PAS`, `PTESTK.INC` | Muster (wie `test/PTEST.C`) |
| `BTEST.PAS`, `BTESTZ.PAS`, `BTESTK.INC` | schneller Weg der Bilder: links mit, rechts ohne (wie `test/BTEST.C`) |

Die Quellen der Maschinensprache liegen in [`../tools/inline/`](../tools/inline/).

## Einbinden

    program Mein;
    {$I DMVHWD.INC}           { CP/M-80: DMVHWC.INC }
    {$I DMVFONT.INC}
    {$I DMVGRAF.INC}
    {$I DMVFIG.INC}           { wenn Linien, Kreise, Boegen gebraucht }
    {$I DMVMAUS.INC}          { wenn die Maus gebraucht wird }
    const SPMAX = 20;         { Figuren 0..20 }
    {$I DMVSPR.INC}           { wenn Figuren ueber GCHRD gebraucht }
    {$I DMVBILD.INC}          { oder: const BKMAX = 16; DMVBKL.INC }
    begin
      gstart;                 { zuerst: Anfangswerte setzen }
      ginit;
      if gerr <> 0 then ...
      gpoint(100, 50); gline(500, 300);
      while gkey = 0 do ;
      gexit;
    end.

Reihenfolge: Hardwareschicht, `DMVFONT.INC`, `DMVGRAF.INC`, danach die übrigen in beliebiger Reihenfolge; genau eine Bildfassung (`DMVBILD.INC` oder `DMVBKL.INC`) muss dabei sein. Für `DMVBKL.INC` steht vorher `const BKMAX = n;` (Bilder 0 bis n − 1).

Unterschiede zur C-Fassung, weil Turbo Pascal 3 keine Funktion als Anweisung aufrufen kann und keine vorbelegten Variablen kennt:

- **`gstart`** muss als Erstes aufgerufen werden.
- Was in C einen Fehlercode liefert, ist eine **Prozedur**; der Code steht danach in **`gerr`** (0 = gut). Funktionen sind nur `gkey`, `grnd`, `gdirekt`, `gdmv`, `tonidle` und `mpos` (`boolean`: gelesen).
- `ginfo(f, m, s, c)` ist eine Prozedur; `gbinfo(nr, r)` füllt ein `GBInfoArr` (`array[0..5] of integer`).
- Zeichenketten sind `GStr` (`string[255]`, unter CP/M-80 `string[80]`), Bilddaten für `gbdef` und Muster für `gsprdef` und `gpdef` beliebige Variablen (untypisierter `var`-Parameter). Format 5 erwartet `integer`-Worte, wie Turbo Pascal sie ablegt.
- `gbload(nr0, name, b, h, anz, fmt, kopf)`: `kopf` ist `real`, Dateien mit Bildern hinter 32 KB gehen also auch (ganze Zahlen gehen wie bisher).
- `ghinfo`, `ghcpu`, die Zeitbasis und `gcorun` gibt es nur in der C-Fassung unter MS-DOS.

## Neu gegenüber der C-Fassung 0.2

- **`gdmv`** (`boolean`): Ist das eine DMV? Liest nur die Statusbits des 7220 (Bildwechsel und Zeilenrücklauf wechseln, nie FFh); auch vor `ginit` und ohne fremde Hardware anzufassen. `ginit` prüft dasselbe und meldet sonst `GE_HW`.
- **Tonwarteschlange:** `tonq(h, l)` stellt einen Ton in eine Warteschlange (32 Töne, voll: `GE_PLATZ`), `tonclr` verwirft Wartendes, `tonidle` sagt, ob alles an den 8741 geschickt ist (ob der letzte Ton noch klingt, sieht man ihm nicht an). `gwait` und `gkey` schicken die Töne nach und nach, ohne zu warten. `ton` geht jetzt ebenfalls über die Warteschlange und überschreibt nie einen wartenden Befehl.
- **Muster:** `gpdef(p, m)` ersetzt Muster p durch acht eigene Zeilen, `gbcol(c)` macht Flächen zweifarbig (Musterpunkte in der Farbe, die übrigen in c; −1 aus). Muster liegen am Schirm fest, aneinanderstoßende Flächen passen ohne Naht (siehe `../README.md`).
- **Figuren über GCHRD:** `gsprdef(nr, b, h, muster)`, `gsprset(nr, b, h, worte)` (Format 5 mit Maske, wie HOPPLERs `Spr16`) und `gspr(nr, x, y, zoom, art)`, siehe `../README.md`. Für bewegte Figuren deutlich schneller als `gbput`, besonders mit Zoom: Drachenlibelle 16 × 16 mit Zoom 1 unter DOS 30 000 Befehle (`gbput` Format 5 mit Zoom 108 000, HOPPLERs `Sprite16` 37 000). `DMVSPR.INC` belegt 1,9 KB Code.
- **`gbload` Format 5** (nur `DMVBILD.INC`; `DMVBKL.INC` bleibt klein und meldet `GE_FORM`): liest je Bild alle Worte in einen Puffer vom Heap (Bilder bis 64 × 85) und gibt ihn danach frei; sonst `GE_PLATZ`. Für Figuren über `gsprset` braucht es das nicht: Die Daten müssen ohnehin im Programm bleiben, das Programm liest sie mit `BlockRead` in ein eigenes Feld.
- **Schneller Weg für Bilder** wie HOPPLERs `BlitAsm`: Liegt ein Bild auf einer Wortgrenze, ohne Zoom und ganz auf dem Schirm, schreibt `gbput` es ohne GDC-Liste direkt aus dem Bildspeicher an den 7220 (`ghblit`, Maschinensprache). Bedingungen und Wirkung wie in `../README.md`; mit `qbsch := false` lässt er sich zum Vergleichen abschalten.

## Speicher und Übersetzen

**MS-DOS:** Unter *Options* den Wert **„mAximum free dynamic memory“** herabsetzen, etwa auf `0400` (16 KB für Heap und Stapel). Sonst nimmt das Programm allen Speicher und DMVGRAF bekommt von DOS keinen Bildspeicher (`gbdef`, `gbload` und `gzeig` melden dann `GE_PLATZ`). Das Programm als `.COM` übersetzen (*Options* → *Com-file*). GTEST belegt 43 KB Code und 12 KB Daten.

**CP/M-80:** TP3 legt die Daten unter das obere Ende des Systems, auf dem übersetzt wird; ein auf einem größeren System übersetztes Programm meldet auf einem kleineren „Not enough memory“. Darum auf dem Zielsystem übersetzen oder unter *Options* die *End address* passend setzen. Damit es passt, ist dort `GStr` nur `string[80]` (eine Bildzeile), die GDC-Liste 256 Byte und der Zeilenpuffer der Bilder 384 Byte lang. GTESTZ (mit allen Teilen außer `DMVSPR.INC`, `BKMAX = 5`, ohne das Streifenbild im Format 3) reicht beim CP/M 2.2 von NCR (BDOS ab D006h) bis AE96h, die Daten belegen 6,8 KB, frei bleiben 1,7 KB; daraus nimmt sich DMVGRAF den Bildspeicher (alles bis auf 256 Byte, mindestens 256 Byte; TP3 hält selbst noch 1 KB zurück). Ohne Bilder, Linien oder Maus bleibt entsprechend mehr; die Bibliothek allein mit kleiner Bildfassung belegt rund 21 KB Code und 5,7 KB Daten neben den 8 KB Laufzeit von TP3.

**Geschwindigkeit:** Die GDC-Liste, die Einträge für Bitzeilen und das Vergrößern sind in beiden Hardwareschichten Maschinensprache (`inline`); bei aufeinanderfolgenden Bitzeilen schickt DMVGRAF nur noch CURS und WDAT (MASK und FIGS gelten noch). Im Prüfstand (Z80 mit 4 MHz, ohne die Zeit des 7220) braucht GTESTZ für die Figuren 4,1 s, den Text 5,3 s und die Bilder 7,6 s (vor dem Umbau 9,5, 21 und 34 s), die Titelzeile 0,6 statt 4,4 s. Unter MS-DOS (Befehle im Prüfstand): eine Figur 16 × 16 mit Zoom 1 über `gspr` rund 31 000, ein Text mit 24 Zeichen 55 000, ein Bild 64 × 47 mit Maske und drei Ebenen über `gbput` durchsichtig 32 000 auf dem schnellen Weg (`BlitAsm` in HOPPLER 23 000), sonst 136 000. Unter CP/M-80 braucht BTESTZ für seine 14 Bilder auf dem schnellen Weg 1,1 s, sonst 3,1 s. Die Maus kostet beim Zeichnen nichts; abgefragt wird sie nur in `mpos`.

**Unterschiede zwischen den Turbo-Pascal-Fassungen:** TP3 für den Z80 rechnet `mod` mit negativen Zahlen anders als die 8086-Fassung; die Bibliothek benutzt `mod` darum nur mit nicht negativen Zahlen. `Addr` liefert unter CP/M eine Zahl, unter DOS einen Zeiger; die Hardwareschicht stellt dafür `hwAdr` bereit. TP3 für DOS lässt nach `Move` mitunter das Richtungsbit gesetzt; die 8086-Maschinensprache beginnt darum mit `cld`.

## Stand

- **MS-DOS:** Im Prüfstand (`tools/pruefdos.py`, 8086 in Unicorn) zeigen die mit Turbo Pascal 3.01A übersetzten `GTEST.COM`, `STEST.COM` und `PTEST.COM` in Farbe und mono dieselben Bilder wie die C-Fassung, `BTEST.COM` links (schneller Weg) wie rechts und wie die C-Simulation; ebenso ein Test des Mauszeigers (`gzeig`), das Laden eines TIL-Steinsatzes mit `gbload` samt Zusammensetzen und `gbload` im Format 5 (`DMVBILD.INC`).
- **CP/M-80:** Im Prüfstand (`tools/pruefcpm.py`, Z80 mit nachgebildetem CP/M 2.2, BDOS D006h) zeigen `GTESTZ.COM`, `STESTZ.COM`, `PTESTZ.COM` und `BTESTZ.COM` (TP3 3.01A für CP/M) dieselben Bilder wie die C-Fassung; im Bild der Bilder fehlt genau, was `DMVBKL.INC` nicht enthält. `GTESTZ` in der Fassung vom 1.11.2026 (vor dem Umbau) in MAME getestet.
- Auf echter Hardware noch nicht gelaufen.

Am PC übersetzen: `python3 tools/tp3.py VERZ GTEST.PAS` (MS-DOS) – `VERZ` enthält `TURBO.COM` (Borland, nicht im Repository) und die Pascal-Dateien mit CR/LF. Für CP/M-80: `python3 tools/pruefcpm.py VERZ TURBO.COM --bdos D006 --tasten "YWGTESTZ\rOCQC"` mit dem `TURBO.COM` der CP/M-Fassung.

## English

DMVGRAF translated to Pascal as include files for **Turbo Pascal 3.0x under MS-DOS and CP/M-80** on the DMV, split into parts because TP3 includes everything. Include the hardware layer (`DMVHWD.INC` for DOS, `DMVHWC.INC` for CP/M-80), `DMVFONT.INC`, `DMVGRAF.INC`, then what the program needs (`DMVFIG.INC` lines and circles, `DMVMAUS.INC` mouse, `DMVSPR.INC` GCHRD sprites) and exactly one image part (`DMVBILD.INC`, or the small `DMVBKL.INC` after `const BKMAX = n;`), and call `gstart` first. Functions that return an error code in C are procedures here and leave the code in `gerr`. New: `gdmv` (DMV check that only reads), a tone queue pumped by `gwait`/`gkey` (`tonq`, `tonclr`, `tonidle`), own and two-colour patterns anchored to the screen (`gpdef`, `gbcol`), sprites (`gsprdef`, `gspr`), `gbload` with a `real` header offset, and a fast image path like HOPPLER's `BlitAsm` for word-aligned, unzoomed images. The inner loops are machine code (`inline`) in both hardware layers. On DOS lower **"mAximum free dynamic memory"** (e.g. to `0400`); on CP/M-80 compile on the target system or set the end address. In the test benches all test programs give the same screens as the C version; not yet run on real hardware.

Code: Claude (Anthropic), idea and prompts: rfka01. Licence: MIT.

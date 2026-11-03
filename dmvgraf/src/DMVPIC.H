/* DMVPIC.H - gepackte Vollbilder (.PIC, Kennung DMVP, LZSS) mit DMVGRAF.
 * Eigene Datei, weil der Binder von CP/M-8000 zu jeder vereinbarten
 * Funktion den Code dazunimmt; wer gpic nicht braucht, bindet sie nicht.
 *
 *   gpic(datei, xw, y, art)            Datei in den Bildspeicher
 *   gpicm(daten, laenge, xw, y, art)   dasselbe aus dem Speicher
 *
 * xw Wortspalte 0-39, y Zeile 0-399; art Bit 0 Anzeige dunkel, Bit 1 Mono,
 * Bit 2 Farbe (ohne beides: wie die Karte). Rueck 0 oder GE_DATEI,
 * GE_KURZ, GE_FORM, GE_LAGE, GE_VERW, GE_HW.
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
extern int gpic();
extern int gpicm();

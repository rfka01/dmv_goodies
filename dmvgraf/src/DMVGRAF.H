/* DMVGRAF.H - Grafik, Bilder, Maus und Ton fuer die NCR Decision Mate V.
 *
 * Bibliothek zum Einbinden ins Programm, Namen und Verhalten nach NCRGRAF
 * (CP/M-80, MS-BASIC) und TurboGraf (Turbo Pascal 3). Beschreibung in
 * README.md. K&R-C: alle Funktionen liefern int (0 = gut, sonst ein
 * Fehlercode wie NCRGRAF); externe Namen in 6 Zeichen eindeutig.
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */

/* Grundfunktionen (NCRGRAF, TurboGraf) */
extern int ginit();     /* ()                 Grafik ein, Schirm leer, Vorgaben */
extern int gexit();     /* ()                 zurueck in den Textbetrieb */
extern int gpoint();    /* (x, y)             Zeichenpunkt setzen */
extern int gline();     /* (x, y)             Linie vom Zeichenpunkt nach x,y */
extern int gcircl();    /* (r)                Kreis um den Zeichenpunkt */
extern int garc();      /* (r, anf, ende)     Bogen, Grad gegen den Uhrzeiger */
extern int grect();     /* (m, n, d)          Rechteck m x n, Richtung d 0-7 */
extern int gbox();      /* (m, n, d)          gefuelltes Rechteck mit Muster */
extern int gtext();     /* (s, d)             Text in Richtung d */
extern int gmode();     /* (m)                0 ersetzen 1 umkehren 2 schwarz 3 Farbe */
extern int gpatt();     /* (p)                Muster 0-15 */
extern int gzoom();     /* (z)                Zoom 0-15 fuer Text, Flaechen, Bilder */
extern int gcolor();    /* (c)                Farbe 0-7 (1 blau 2 gruen 4 rot) */
extern int gclear();    /* ()                 Schirm leer */
extern int where();     /* (int *x, int *y)   Zeichenpunkt lesen */
extern int gorig();     /* (o)                0 y nach oben, 1 y nach unten */

/* Bilder */
extern int gbdef();     /* (nr, b, h, format, daten) */
extern int gbload();    /* (nr0, datei, b, h, anz, format, kopf) */
extern int gbmap();     /* (farbe, a, b, muster)  Farbnummer 0-15 umsetzen */
extern int gbput();     /* (nr, art)          am Zeichenpunkt zeichnen */
extern int gbget();     /* (nr, b, h)         Schirmausschnitt als Bild */
extern int gbfree();    /* (nr) */
extern int gbinfo();    /* (nr, int r[6])     B, H, sichtbarer Bereich x0 y0 x1 y1 */

/* Zusammensetzen ueberlappender Bilder */
extern int glayer();    /* (x0, y0, x1, y1)   Ausschnitt beginnen */
extern int glput();     /* (nr, x, y, art)    Bild eintragen, hinten zuerst */
extern int gldraw();    /* ()                 zusammensetzen und zeichnen */

/* Maus, Tastatur, Zeit, Auskunft, Ton */
extern int ginfo();     /* (int *farbe, *maus, *syn, *covox) */
extern int mpos();      /* (int *x, int *y, int *knopf) -> 1 gelesen */
extern int mset();      /* (x, y) */
extern int gzeig();     /* (nr, x, y)  Mauszeiger: Bild nr, nr < 0 weg */
extern int gdirekt();   /* (an)  7220 direkt (1) oder ueber BIOS 66/67 (0)
                                  -> vorher, -1 kein direkter Weg */
extern int gkey();      /* () -> Taste oder 0 (wartet nicht) */
extern int grnd();      /* () Zufallszahl 0-32767 aus Bildlage und Maus */
extern int gwait();     /* (n)                n Bildwechsel abwarten */
extern int ton();       /* (h, l)             Ton h 1-42 (0 Pause), l x 20 ms */
extern int gputs();     /* (s)                Text auf die Konsole (Textbetrieb) */

/* Formate fuer gbdef/gbload */
#define GB_BIT7  1      /* 1 Bit je Punkt, Bit 7 links */
#define GB_BIT0  2      /* 1 Bit je Punkt, Bit 0 links */
#define GB_NIB   3      /* 4 Bit je Punkt (Farbnummer), hohes Halbbyte links */
#define GB_EBN   4      /* je Zeile Maske, Gruen, Rot, Blau, Bit 0 links */
#define GB_SETR  5      /* je Ebene SET- und RESET-Worte (int), Bit 0 links */

/* Arten fuer gbput/glput */
#define GB_DECK  0      /* deckend */
#define GB_DURCH 1      /* durchsichtig */
#define GB_SCHAT 2      /* durchsichtig, Weisses gerastert (Auswahl) */
#define GB_XOR   3      /* umkehren */

/* Fehlercodes (1-83 wie NCRGRAF) */
#define GE_ZOOM   10
#define GE_RAD    20
#define GE_RICHT  30
#define GE_MUST   40
#define GE_MODUS  50
#define GE_TRICHT 60
#define GE_SEITE  80
#define GE_NR     90    /* Bildnummer ungueltig */
#define GE_PLATZ  91    /* Bildspeicher voll */
#define GE_DATEI  92    /* Datei nicht gefunden */
#define GE_KURZ   93    /* Datei zu kurz */
#define GE_FORM   94    /* Format oder Groesse ungueltig */
#define GE_LEER   95    /* Bild nicht angelegt */
#define GE_LAGE   96    /* glput ohne glayer oder Liste voll */
#define GE_HW     99    /* keine DMV-Grafik (BIOS) */

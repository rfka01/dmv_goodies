/* GINTERN.H - interne Vereinbarungen von DMVGRAF (nicht fuer Programme).
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "DMVGRAF.H"

#ifdef HOST
#define UW unsigned short
#else
#ifdef __GNUC__                 /* gcc fuer CP/M-68K: int hat 32 Bit */
#define UW unsigned short
#else
#define UW unsigned
#endif
#endif
#define M16(x) ((x) & 0xFFFF)

/* Grenzen */
#define QLMAX   1024    /* GDC-Liste */
#define GBMAX   64      /* Bilder */
#define GLMAX   200     /* Eintraege eines Ausschnitts (glput) */
#define BH      8       /* Bandhoehe beim Zusammensetzen */
#define QBUF    1280    /* Puffer fuer Bildzeilen */

/* ---- Hardwareschicht (GHWCPM.C, GHWSIM.C) ---- */
#define GH_FARBE 1      /* Farbbildschirm (3 Ebenen) */
#define GH_MAUS  2      /* K806 */
#define GH_SYN   4      /* Synthesizer-Firmware (BIOS 73) */
#define GH_COVOX 8      /* Covox */

extern int ghinit();    /* () -> GH_..., -1 keine DMV-Grafik */
extern int ghgraf();    /* (an) Grafikbetrieb ein (1) oder aus (0) */
extern int ghlist();    /* (char *liste) GDC-Liste wie BIOS 66 */
extern int ghrdat();    /* (char *block) Liste ausfuehren und lesen, BIOS 67/2 */
extern int ghdset();    /* (an) 7220 direkt -> vorher, -1 nicht moeglich */
extern int ghstat();    /* () GDC-Statusbyte */
extern int ghvsyn();    /* () auf den naechsten Bildwechsel warten */
extern int ghvzahl();   /* () Statusabfragen bis zum Bildwechsel (grnd) */
extern int ghmaus();    /* (befehl, char *block) K806 wie BIOS 68 */
extern int ghton();     /* (h, l) -> 0 gesendet, 1 besetzt */
extern int ghtast();    /* () -> Taste oder 0 */
extern int ghtext();    /* (char *s) */
extern int ghopen();    /* (char *name) -> 0 gut */
extern int ghread();    /* (char *puffer) 128 Byte -> 1 gelesen, 0 Ende */
extern int ghclos();    /* () */
/* Bildspeicher: Bloecke von hoechstens 64 KB */
extern int ghsnum();    /* () -> Zahl der Bloecke */
extern long ghsgro();   /* (blk) -> Groesse in Byte */
extern int ghsput();    /* (blk, UW ziel, char *quelle, n) */
extern int ghsget();    /* (char *ziel, blk, UW quelle, n) */
extern int ghsmov();    /* (blk, UW ziel, UW quelle, UW n), ziel < quelle */

/* ---- GRAF.C ---- */
extern int qneb;        /* Ebenen 1 oder 3 */
extern int qhw;         /* GH_... */
extern int qx, qy;      /* Zeichenpunkt (Programmkoordinaten) */
extern int qorig;       /* 0 y nach oben, 1 nach unten */
extern int qcol, qmode, qpatt, qzoom;
extern UW qebad[3];     /* Wortadresse der Ebenen: gruen, rot, blau */
extern int qebit[3];    /* Farbbit der Ebenen */
extern int qan;         /* Grafik eingeschaltet */

extern int qcmd();      /* (c) Eintrag beginnen */
extern int qpar();      /* (b) Parameterbyte */
extern int qwrd();      /* (w) Parameterwort, niederwertig zuerst */
extern int qplatz();    /* (n) Platz fuer n Byte schaffen */
extern int qflush();    /* () Liste ausgeben */
extern int qcurs();     /* (UW adr, punkt) CURS, MASK = dieser Punkt */
extern int qsy();       /* (y) Programm-y -> Bildzeile von oben */
extern int qrow();      /* (sy, sx, bits, nbit, ebene, wdat, art) eine Zeile */
extern int qebon();     /* (ebene, farbe) Farbbit der Ebene gesetzt? */
extern int qstart();    /* () Hardware abfragen (einmal) -> 0 oder GE_HW */
extern int qbits();
extern int qblk();      /* (ne, sy, c, nc, n, char *s, k, es) Wortblock */
/* ---- Mauszeiger (GBILD.C) ---- */
extern int qzeig;       /* Zeiger steht */
extern int qzhid();     /* () vor anderer Ausgabe wegnehmen */
extern int qzwied();    /* () weggenommenen Zeiger wieder zeigen */     /* (sy, sx, char *bits, nbit) Zeile in Farbe und Modus */

/* ---- Kern (GKERN.C, unter CP/M-8000 GZ8K.8KN) ---- */
extern int gkopi();     /* (d, s, n) */
extern int gnull();     /* (d, n) */
extern int gnib();      /* (z, q, tb, n) Format 3 -> sichtbar, Ebenen */
extern int gzeile();    /* (d, z, n, sh, cnt, ns, c01, c23, eb) */

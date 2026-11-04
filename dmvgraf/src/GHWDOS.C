/* GHWDOS.C - DMVGRAF-Hardwareschicht fuer MS-DOS auf der DMV (8088/V20
 * der K230/K231/K235), mit gcc-ia16 im Speichermodell small uebersetzt.
 *
 * Unter DOS darf das Programm alle Anschluesse selbst ansprechen: 7220
 * (A0h-A2h), Maus K806 (Steckplatz 2A, 30h-33h) und Tastatur-8741
 * (40h/41h). Die Ablaeufe sind dieselben wie in GHD68K.C (dort fuer
 * NCRs CP/M-68K ohne BIOS-Block); Tastatur, Bildschirmtext und Dateien
 * gehen ueber INT 21h. Den Bildspeicher holt DMVGRAF bei DOS (INT 21h,
 * 48h) in Bloecken zu 64 KB.
 *
 * Zu GCRTDOS.S (Start, inb/outb, INT 21h, Kopieren in fremde Segmente)
 * und GDOS.S (GDC-Liste ausgeben).
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

struct dosr {
        UW ax, bx, cx, dx;
};
extern int gdos();              /* (struct dosr *) INT 21h -> Uebertrag */
extern int inb();               /* (port) */
extern int outb();              /* (port, wert) */
extern int gromz();             /* () Kennbuchstabe der Firmware: C Farbe, M Mono */
extern int gdlst();             /* (liste) wie BIOS 66 an den 7220 */
extern int gfput();             /* (seg, off, char *quelle, n) */
extern int gfget();             /* (char *ziel, seg, off, n) */
extern int gfmov();             /* (seg, ziel, quelle, n), ziel < quelle */

#define GDCST   0xA0            /* lesen Status, schreiben Parameter */
#define GDCCM   0xA1            /* lesen Daten, schreiben Befehl */
#define GDCZM   0xA2            /* Parameter und Zoom-Klinke */
#define KBDDAT  0x40
#define KBDCTL  0x41
#define ZEIT    30000           /* Wartegrenze in Abfragen (je 2 Runden) */

static int k806;                /* Anschluss der K806, 0 = keine */
static int farbe;

static int dos1(ah, dx)         /* einfacher Aufruf, -> AL */
int ah, dx;
{
        struct dosr r;

        r.ax = (UW)ah << 8;
        r.bx = r.cx = 0;
        r.dx = (UW)dx;
        gdos(&r);
        return r.ax & 0xFF;
}

/* ---- 7220 ---- */

static int gw()                 /* bis das FIFO nicht voll ist */
{
        int n, k;

        for (k = 0; k < 2; k++)
                for (n = 0; n < ZEIT; n++)
                        if (!(inb(GDCST) & 2))
                                return 0;
        return 1;
}

static int gcmd(c)
int c;
{
        gw();
        outb(GDCCM, c);
        return 0;
}

static int gpar(c)
int c;
{
        gw();
        outb(GDCST, c);
        return 0;
}

static int gparz(c)             /* ZOOM-Parameter ueber A2h */
int c;
{
        gw();
        outb(GDCZM, c);
        return 0;
}

static int glies(m, d)          /* m Bytes aus dem FIFO -> 0, 1 Zeit */
int m;
char *d;
{
        int n, k;

        while (m-- > 0) {
                for (k = 0; k < 2; k++) {
                        for (n = 0; n < ZEIT; n++)
                                if (inb(GDCST) & 1)
                                        break;
                        if (n < ZEIT)
                                break;
                }
                if (k == 2)
                        return 1;
                *d++ = inb(GDCCM);
        }
        return 0;
}

/* Erkennung nach "NCR Decision Mate V - Hardware erkennen": DMV-Pruefung
   (nur lesen), Bildschirmprobe mit Wiederherstellen samt Schreibstelle,
   K806 ueber F1 - im Kern aus src/GHW.INC (GHW86.S), wie in DMVDEMO. */
extern int ghk(), ghkcf;
#define K_DMV    0
#define K_SCHIRM 7
#define K_MAUS   8

int ghinit()
{
        int a;

        ghk(K_DMV, 0, 0, 0, 0);
        if (ghkcf)
                return -1;
        farbe = ghk(K_SCHIRM, 0, 0, 0, 0) == 2;
        k806 = ghk(K_MAUS, 0, 0, 0, 0);
        a = 0;
        if (farbe)
                a |= GH_FARBE;
        if (k806)
                a |= GH_MAUS;
        return a;
}

/* Grafik ein und aus wie GINIT/GEXIT von NCRs GRAPH-Bibliothek unter
   DOS (DMVDEMO): CCHAR, PRAM, je Ebene loeschen; zurueck mit CCHAR 8Fh,
   PRAM fuer 25 Textzeilen und Strg-Z an die Konsole */
static int grafan()
{
        int i, e;

        gcmd(0x0C);                             /* Anzeige aus */
        gcmd(0x4B); gpar(0);                    /* CCHAR: kein Cursor */
        gcmd(0x70);                             /* PRAM: zwei Bildbereiche */
        for (i = 0; i < 2; i++) {
                gpar(0); gpar(0); gpar(0); gpar(0x59);
        }
        gcmd(0x47); gpar(80);                   /* PITCH */
        gcmd(0x49); gpar(0); gpar(0); gpar(0);  /* CURS 0 */
        gcmd(0x46); gparz(0);                   /* ZOOM 0 */
        gcmd(0x4A); gpar(0xFF); gpar(0xFF);     /* MASK */
        for (e = 0; e < (farbe ? 3 : 1); e++) {
                gcmd(0x4C); gpar(0x02); gpar(0xFF); gpar(0x7F);
                gcmd(0x22); gpar(0xFF); gpar(0xFF);
        }
        gcmd(0x6B);                           /* START */
        gcmd(0x0D);                             /* Anzeige an */
        return 0;
}

static int textan()
{
        int i;

        gcmd(0x0C);
        gcmd(0x46); gparz(0);
        gcmd(0x4B); gpar(0x8F);                 /* CCHAR wie NCRs GEXIT */
        gcmd(0x70);
        gpar(0); gpar(0); gpar(0); gpar(0x19);
        gpar(0); gpar(0); gpar(0); gpar(0x19);
        for (i = 0; i < 8; i++)
                gpar(0xFF);
        gcmd(0x47); gpar(80);
        gcmd(0x49); gpar(0); gpar(0); gpar(0);
        gcmd(0x4A); gpar(0xFF); gpar(0xFF);
        gcmd(0x6B);
        gcmd(0x0D);
        return 0;
}

int ghgraf(an)
int an;
{
        if (an)
                grafan();
        else {
                textan();
                dos1(2, 0x1A);          /* die Konsole loescht selbst */
        }
        return 0;
}

int ghdset(an)                  /* unter DOS gibt es nur den direkten Weg */
int an;
{
        return -1;
}

int ghlist(l)
char *l;
{
        gdlst(l);
        return 0;
}

int ghrdat(b)                   /* Block wie BIOS 67/2 */
char *b;
{
        int m;
        char *p;

        m = ((b[0] & 0xFF) << 8) | (b[1] & 0xFF);
        p = b + 2;
        while (*p)
                p += (*p & 0xFF) + 1;
        gdlst(b + 2);
        return glies(m, p + 1);
}

int ghstat()
{
        return inb(GDCST);
}

int ghvsyn()                    /* Status Bit 5 von 0 auf 1 */
{
        int n, k;

        for (k = 0; k < 2; k++)
                for (n = 0; n < ZEIT && (inb(GDCST) & 0x20); n++)
                        ;
        for (k = 0; k < 2; k++)
                for (n = 0; n < ZEIT; n++)
                        if (inb(GDCST) & 0x20)
                                return 0;
        return 1;
}

int ghvzahl()                   /* Statusabfragen bis zum Bildwechsel */
{
        int n;

        n = 0;
        while ((inb(GDCST) & 0x20) && n < 20000)
                n++;
        while (!(inb(GDCST) & 0x20) && n < 30000)
                n++;
        return n;
}

/* ---- Maus K806 (wie BIOS 68) ---- */

static int mw(bit, soll)        /* Status-Bit abwarten -> 0 gut */
int bit, soll;
{
        int n, k;

        for (k = 0; k < 2; k++)
                for (n = 0; n < ZEIT; n++)
                        if (((inb(k806 + 1) & bit) != 0) == soll)
                                return 0;
        return 1;
}

int ghmaus(c, b)
int c;
char *b;
{
        int i, p, a;

        if (!k806)
                return 1;
        for (i = 0; i < 64 && (inb(k806 + 1) & 1); i++)
                (void)inb(k806);
        p = *b++ & 0xFF;
        if (mw(2, 0))
                return 2;
        outb(k806 + 1, c);
        for (; p > 0; p--) {
                if (mw(2, 0))
                        return 2;
                outb(k806, *b++);
        }
        a = *b++ & 0xFF;
        for (; a > 0; a--) {
                if (mw(1, 1))
                        return 2;
                *b++ = inb(k806);
        }
        return 0;
}

/* ---- Ton ueber den 8741 (wie BIOS 69, DMVTRAK) ---- */

static int kw()
{
        int n;

        for (n = 0; n < ZEIT; n++)
                if (!(inb(KBDCTL) & 2))
                        return 0;
        return 1;
}

/* Tonwarteschlange (GEIN.C): 8741 nimmt etwas an? Byte als Befehl (an
   41h) oder Daten (40h), ohne zu warten - der 8741 liest waehrend eines
   Tons nicht, darum Schritt fuer Schritt */
int ghkfrei()
{
        return !(inb(KBDCTL) & 2);
}

int ghkaus(bef, v)
int bef, v;
{
        outb(bef ? KBDCTL : KBDDAT, v);
        return 0;
}

int ghton(h, l)
int h, l;
{
        if (inb(KBDCTL) & 2)
                return 1;
        outb(KBDCTL, 0x06);
        kw();
        if (h > 42)
                h = 42;
        outb(KBDDAT, h + 0x20);
        kw();
        if (l < 1)
                l = 1;
        if (l > 224)
                l = 224;
        outb(KBDDAT, l + 0x1F);
        return 0;
}

/* ---- Tastatur, Text, Dateien ueber INT 21h ---- */

int ghtast()                    /* 06h mit DL = FFh: holen ohne Warten */
{
        struct dosr r;

        r.ax = 0x0600;
        r.bx = r.cx = 0;
        r.dx = 0xFF;
        if (gdos(&r) & 0x40)    /* ZF: keine Taste */
                return 0;
        return r.ax & 0xFF;
}

int ghtext(s)
char *s;
{
        while (*s) {
                if (*s == '\n')
                        dos1(2, 13);
                dos1(2, *s & 0xFF);
                s++;
        }
        return 0;
}

static int fh = -1;

int ghopen(name)
char *name;
{
        struct dosr r;

        r.ax = 0x3D00;          /* nur lesen */
        r.bx = r.cx = 0;
        r.dx = (UW)name;
        if (gdos(&r) & 1)
                return 1;
        fh = r.ax;
        return 0;
}

int ghread(b)                   /* 128 Byte; ein kurzer Rest wird mit ^Z */
char *b;                        /* aufgefuellt wie unter CP/M */
{
        struct dosr r;
        int i;

        r.ax = 0x3F00;
        r.bx = fh;
        r.cx = 128;
        r.dx = (UW)b;
        if ((gdos(&r) & 1) || r.ax == 0)
                return 0;
        for (i = r.ax; i < 128; i++)
                b[i] = 0x1A;
        return 1;
}

int ghclos()
{
        struct dosr r;

        if (fh < 0)
                return 0;
        r.ax = 0x3E00;
        r.bx = fh;
        r.cx = r.dx = 0;
        gdos(&r);
        fh = -1;
        return 0;
}

/* ---- Bildspeicher: Bloecke zu 64 KB von DOS ---- */

#ifndef GBBLK
#define GBBLK 2                 /* hoechstens so viele Bloecke */
#endif

static int bn = -1;
static UW bseg[GBBLK];

int ghsnum()
{
        struct dosr r;

        if (bn >= 0)
                return bn;
        for (bn = 0; bn < GBBLK; bn++) {
                r.ax = 0x4800;
                r.bx = 0x1000;          /* 4096 Absaetze = 64 KB */
                r.cx = r.dx = 0;
                if (gdos(&r) & 1)
                        break;
                bseg[bn] = r.ax;
        }
        return bn;
}

long ghsgro(b)
int b;
{
        return 65536L;
}

int ghsput(b, o, s, n)
int b, n;
UW o;
char *s;
{
        return gfput(bseg[b], o, s, n);
}

int ghsget(d, b, o, n)
char *d;
int b, n;
UW o;
{
        return gfget(d, bseg[b], o, n);
}

int ghsmov(b, z, q, n)
int b;
UW z, q, n;
{
        return gfmov(bseg[b], z, q, n);
}

/* Bild direkt an den 7220 wie HOPPLERs BlitAsm (gblit in GCRTDOS.S):
   einmal MASK FFFF und FIGS, dann je Zeile und Eintrag der Tabelle tu
   CURS ohne Punkt, WDAT und die Worte, aus dem Bildspeicherblock b ab o */
extern int gblit();

int ghblit(b, o, n, rs, nw, lm, ad, tu, nt)
int b, n, rs, nw, lm, nt, *tu;
UW o, ad;
{
        static UW p[44];
        int i;

        p[0] = bseg[b]; p[1] = o; p[2] = n; p[3] = rs; p[4] = nw;
        p[5] = lm; p[6] = ad; p[7] = nt;
        for (i = 0; i < nt * 6; i++)
                p[8 + i] = tu[i];
        gblit(p);
        return 0;
}

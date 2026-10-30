/* GRAF.C - DMVGRAF: Zustand, GDC-Liste, Figuren und Text.
 *
 * Alles wird als GDC-Befehlsliste gesammelt (Eintrag: Byte n, Befehl,
 * n - 1 Parameter; 0 = Ende, wie BIOS 66) und am Ende jedes Aufrufs
 * ausgegeben. Linien, Rechtecke, Kreise und Flaechen zeichnet der
 * uPD7220 selbst (FIGD, GCHRD), je Ebene mit passendem Zeichenmodus;
 * Text in Richtung 0 und Bilder gehen wortweise (WDAT).
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"
#include "GFONT.H"

int qneb = 3;
int qhw = -2;                   /* -2: noch nicht abgefragt */
int qx, qy, qorig, qan;
int qcol = 7, qmode = 1, qpatt, qzoom;
UW qebad[3] = { 0x0000, 0x4000, 0x8000 };
int qebit[3] = { 2, 4, 1 };

static int qzs;                 /* ZOOM, wie zuletzt gesendet */

/* Schritt je 7220-Richtung, Bildzeilen nach unten */
static int qdx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static int qdy[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };

/* 16 Muster, je 8 Zeilen von oben, Bit 0 = links. Die ersten zwei
   Zeilen sind zugleich das Linienmuster. */
static char qmust[16][8] = {
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },  /* voll */
        { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA },  /* 50 % */
        { 0x11, 0x44, 0x11, 0x44, 0x11, 0x44, 0x11, 0x44 },  /* 25 % */
        { 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB, 0xEE, 0xBB },  /* 75 % */
        { 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00 },  /* waagrecht */
        { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 },  /* senkrecht */
        { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 },  /* / */
        { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 },  /* \ */
        { 0xFF, 0x11, 0x11, 0x11, 0xFF, 0x11, 0x11, 0x11 },  /* Gitter */
        { 0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81 },  /* Kreuz */
        { 0xFF, 0x01, 0x01, 0x01, 0xFF, 0x10, 0x10, 0x10 },  /* Ziegel */
        { 0x11, 0x00, 0x44, 0x00, 0x11, 0x00, 0x44, 0x00 },  /* 12,5 % */
        { 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00 },  /* breit waagr. */
        { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 },  /* breit senkr. */
        { 0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0 },  /* grosses Brett */
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };/* leer */

/* sin(t) * 256 fuer t = 0..45 Grad */
static int qsin[46] = {
          0,   4,   9,  13,  18,  22,  27,  31,  36,  40,  44,  49,
         53,  58,  62,  66,  71,  75,  79,  83,  88,  92,  96, 100,
        104, 108, 112, 116, 120, 124, 128, 132, 136, 139, 143, 147,
        150, 154, 158, 161, 165, 168, 171, 175, 178, 181 };

/* ------------------------------------------------------------------ */
/* GDC-Liste                                                          */

static char ql[QLMAX];
static int qn;                  /* belegte Bytes */
static int qe = -1;             /* Zaehlbyte des offenen Eintrags */

int qflush()
{
        if (qn) {
                ql[qn] = 0;
                if (qan)
                        ghlist(ql);
                qn = 0;
        }
        qe = -1;
        return 0;
}

int qplatz(n)
int n;
{
        if (qzeig)              /* Mauszeiger weg, bevor anderes kommt */
                qzhid();
        if (qn + n + 2 >= QLMAX)
                qflush();
        return 0;
}

int qcmd(c)
int c;
{
        qplatz(24);
        qe = qn;
        ql[qn++] = 1;
        ql[qn++] = c;
        return 0;
}

int qpar(b)
int b;
{
        ql[qn++] = b;
        ql[qe]++;
        return 0;
}

int qwrd(w)
UW w;
{
        qpar((int)(w & 0xFF));
        qpar((int)((w >> 8) & 0xFF));
        return 0;
}

/* Block von n Zeilen zu nc Worten ab Bildzeile sy, Wortspalte c auf
   den Ebenen 0..ne-1 ersetzen; Quelle s, Zeilenabstand k, Ebenenabstand
   es Byte. Zeile fuer Zeile alle Ebenen, damit nie eine Ebene lange
   vorausliegt (sonst kurz Mischfarben, etwa ein gelber Zeiger) */
int qblk(ne, sy, c, nc, n, s, k, es)
int ne, sy, c, nc, n, k, es;
char *s;
{
        register char *p;
        UW adr;
        int m, e;

        m = 2 * nc;
        for (; n > 0; n--, s += k, sy++)
                for (e = 0; e < ne; e++) {
                        if (qn + m + 20 >= QLMAX)
                                qflush();
                        adr = M16(qebad[e] + (UW)sy * 40 + c);
                        p = &ql[qn];
                        *p++ = 4; *p++ = 0x49;
                        *p++ = adr & 0xFF; *p++ = (adr >> 8) & 0xFF;
                        *p++ = 0;
                        *p++ = 3; *p++ = 0x4A; *p++ = 0xFF; *p++ = 0xFF;
                        *p++ = 4; *p++ = 0x4C; *p++ = 0x02; *p++ = 0;
                        *p++ = 0;
                        *p++ = 1 + m; *p++ = 0x20;
                        gkopi(p, s + e * es, m);
                        qn += 16 + m;
                }
        qe = -1;
        return 0;
}

/* CURS: Wortadresse, Punkt 0-15 im Wort (setzt MASK auf diesen Punkt) */
int qcurs(adr, p)
UW adr;
int p;
{
        qcmd(0x49);
        qwrd(adr);
        qpar((p & 15) << 4);
        return 0;
}

static int qzm(z)               /* GCHRD-Zoom */
int z;
{
        if (z != qzs) {
                qcmd(0x46);
                qpar(z & 15);
                qzs = z;
        }
        return 0;
}

/* ------------------------------------------------------------------ */
/* Hilfen                                                             */

int qsy(y)
int y;
{
        return qorig ? y : 399 - y;
}

static int quy(sy)              /* Bildzeile -> Programm-y */
int sy;
{
        return qorig ? sy : 399 - sy;
}

int qebon(e, c)
int e, c;
{
        return (c & qebit[e]) != 0;
}

/* Zeichenmodus einer Ebene fuer Figuren: 0 auslassen, 1 mit Muster,
   2 mit vollem Muster; *w = WDAT-Befehl */
static int qfmod(e, w)
int e, *w;
{
        int an;

        an = qebon(e, qcol);
        switch (qmode) {
        case 0:
                *w = an ? 0x20 : 0x22;
                return an ? 1 : 2;
        case 1:
                *w = 0x21;
                return an ? 1 : 0;
        case 2:
                *w = 0x22;
                return 1;
        }
        *w = an ? 0x23 : 0x22;
        return 1;
}

static int qdir(d)              /* Richtung 0-7 des Programms -> 7220 */
int d;
{
        return qorig ? (10 - d) & 7 : (2 + d) & 7;
}

/* Ecke, an der der 7220 beginnt: bei y nach unten liegt der Anker oben,
   der 7220 beginnt aber an der unteren Ecke (um l Schritte versetzt) */
static int qank(sx, sy, d, l, ox, oy)
int sx, sy, d, l, *ox, *oy;
{
        int k;

        *ox = sx;
        *oy = sy;
        if (qorig) {
                k = (8 - d) & 7;
                *ox += l * qdx[k];
                *oy += l * qdy[k];
        }
        return 0;
}

static UW qadr(e, sx, sy)
int e, sx, sy;
{
        return M16(qebad[e] + (UW)sy * 40 + (sx >> 4));
}

/* Figur auf allen Ebenen: Startpunkt, Muster (n Byte ab PRAM 8),
   FIGS-Parameter, Abschlussbefehl (FIGD 6Ch oder GCHRD 68h) */
static int qfig(sx, sy, mu, nm, fp, nf, ende)
int sx, sy, nm, nf, ende;
char *mu, *fp;
{
        int e, k, i, w;

        for (e = 0; e < qneb; e++) {
                k = qfmod(e, &w);
                if (!k)
                        continue;
                qplatz(48);
                qcmd(w);
                qcurs(qadr(e, sx, sy), sx & 15);
                qcmd(0x78);
                for (i = 0; i < nm; i++)
                        qpar(k == 2 ? 0xFF : mu[i]);
                qcmd(0x4C);
                for (i = 0; i < nf; i++)
                        qpar(fp[i]);
                qcmd(ende);
        }
        return 0;
}

static int qput(p, w)           /* Wort in FIGS-Parameter, 14 Bit */
char *p;
int w;
{
        p[0] = w & 0xFF;
        p[1] = (w >> 8) & 0x3F;
        return 0;
}

/* ------------------------------------------------------------------ */
/* Ein- und Ausschalten                                               */

static int qhwin()
{
        if (qhw == -2) {
                qhw = ghinit();
                if (qhw >= 0 && !(qhw & GH_FARBE)) {
                        qneb = 1;
                        qebit[0] = 7;
                }
        }
        return qhw < 0 ? GE_HW : 0;
}

int qstart()
{
        return qhwin();
}

int ginit()
{
        if (qhwin())
                return GE_HW;
        qflush();
        ghgraf(1);
        qan = 1;
        qzs = 0;
        qpatt = 0;
        qzoom = 0;
        qmode = 1;
        qcol = 7;
        qx = qy = 0;
        return 0;
}

int gexit()
{
        gzeig(-1, 0, 0);
        qflush();
        if (qan)
                ghgraf(0);
        qan = 0;
        return 0;
}

int gclear()
{
        static char fp[5] = { 0x12, 0x8F, 0x41, 0x80, 0x02 };   /* 400 x 640 */
        int e, i;

        qzm(0);
        for (e = 0; e < qneb; e++) {
                qplatz(40);
                qcmd(0x22);
                qcurs(qadr(e, 0, 399), 0);
                qcmd(0x78);
                for (i = 0; i < 8; i++)
                        qpar(0xFF);
                qcmd(0x4C);
                for (i = 0; i < 5; i++)
                        qpar(fp[i]);
                qcmd(0x68);
        }
        qflush();
        return 0;
}

int gpoint(x, y)
int x, y;
{
        if (x < 0) return 1;
        if (x > 639) return 2;
        if (y < 0) return 3;
        if (y > 399) return 4;
        qx = x;
        qy = y;
        return 0;
}

int where(x, y)
int *x, *y;
{
        *x = qx;
        *y = qy;
        return 0;
}

int gorig(o)
int o;
{
        qorig = o ? 1 : 0;
        return 0;
}

int gcolor(c)
int c;
{
        qcol = c & 7;
        return 0;
}

int gmode(m)
int m;
{
        if (m < 0 || m > 3)
                return GE_MODUS;
        qmode = m;
        return 0;
}

int gpatt(p)
int p;
{
        if (p < 0 || p > 15)
                return GE_MUST;
        qpatt = p;
        return 0;
}

int gzoom(z)
int z;
{
        if (z < 0 || z > 15)
                return GE_ZOOM;
        qzoom = z;
        return 0;
}

/* ------------------------------------------------------------------ */
/* Linien, Rechtecke, Kreise                                          */

int gline(x, y)
int x, y;
{
        char fp[9];
        int x0, y0, x1, y1, dx, dy, ax, ay, dir, maj, min;

        if (x < 0) return 5;
        if (x > 639) return 6;
        if (y < 0) return 7;
        if (y > 399) return 8;
        x0 = qx; y0 = qsy(qy);
        x1 = x;  y1 = qsy(y);
        dx = x1 - x0; dy = y1 - y0;
        ax = dx < 0 ? -dx : dx;
        ay = dy < 0 ? -dy : dy;
        /* Oktant wie GDCLIB.INC (DMVDEMO), y nach unten */
        if (dy >= 0) {
                if (dx >= 0)
                        dir = ay >= ax ? 0 : 1;
                else
                        dir = ay >= ax ? 7 : 6;
        } else {
                if (dx > 0)
                        dir = ax >= ay ? 2 : 3;
                else
                        dir = ay >= ax ? 4 : 5;
        }
        if (dir == 0 || dir == 3 || dir == 4 || dir == 7) {
                maj = ay; min = ax;
        } else {
                maj = ax; min = ay;
        }
        fp[0] = dir | 0x08;
        fp[1] = maj & 0xFF;
        fp[2] = ((maj >> 8) & 0x3F) | 0x40;
        qput(&fp[3], 2 * min - maj);
        qput(&fp[5], 2 * (min - maj));
        qput(&fp[7], 2 * min);
        qfig(x0, y0, qmust[qpatt], 2, fp, 9, 0x6C);
        qflush();
        qx = x;
        qy = y;
        return 0;
}

int grect(m, n, d)
int m, n, d;
{
        char fp[11];
        int sx, sy;

        if (d < 0 || d > 7) return GE_RICHT;
        if (m < 2) return 80;
        if (m > 640) return 81;
        if (n < 2) return 82;
        if (n > 640) return 83;
        qank(qx, qsy(qy), d, n - 1, &sx, &sy);
        fp[0] = 0x40 | qdir(d);
        fp[1] = 3;
        fp[2] = 0x40;
        qput(&fp[3], m - 1);
        qput(&fp[5], n - 1);
        qput(&fp[7], 0x3FFF);
        qput(&fp[9], m - 1);
        qfig(sx, sy, qmust[qpatt], 2, fp, 11, 0x6C);
        qflush();
        return 0;
}

int gbox(m, n, d)
int m, n, d;
{
        char fp[5];
        int sx, sy, z;

        z = qzoom + 1;
        if (d < 0 || d > 7) return GE_RICHT;
        if (m < 1) return 80;
        if ((long)m * z > 640) return 81;
        if (n < 1) return 82;
        if ((long)n * z > 640) return 83;
        qank(qx, qsy(qy), d, n * z - 1, &sx, &sy);
        qzm(qzoom);
        fp[0] = 0x10 | qdir(d);
        qput(&fp[1], n - 1);
        fp[2] |= 0x40;
        qput(&fp[3], m);
        qfig(sx, sy, qmust[qpatt], 8, fp, 5, 0x68);
        qflush();
        return 0;
}

/* Kreisachtel: 7220-Richtung, Startpunkt in Radien (y nach unten),
   DM, Grundwinkel, Drehsinn (Winkel mathematisch, gegen den Uhrzeiger) */
static int qokt[8][6] = {
        { 4,  1,  0, 0,   0,  1 },
        { 7,  1,  0, 1,   0, -1 },
        { 1,  0, -1, 0,  90, -1 },
        { 6,  0, -1, 1,  90,  1 },
        { 0, -1,  0, 0, 180,  1 },
        { 3, -1,  0, 1, 180, -1 },
        { 2,  0,  1, 0, 270,  1 },
        { 5,  0,  1, 1, 270, -1 } };

static int qbog(cx, cy, r, k, t0, t1)   /* Achtel k von t0 bis t1 Grad */
int cx, cy, r, k, t0, t1;
{
        char fp[11];
        int dm, dc;

        dm = (int)(((long)qsin[t0] * r + 255) / 256);
        dc = (int)(((long)qsin[t1] * r) / 256);
        if (t0 == 0)
                dm = qokt[k][3];
        if (dm > dc)
                return 0;
        fp[0] = 0x20 | qokt[k][0];
        fp[1] = dc & 0xFF;
        fp[2] = ((dc >> 8) & 0x3F) | 0x40;
        qput(&fp[3], r - 1);
        qput(&fp[5], 2 * (r - 1));
        qput(&fp[7], 0x3FFF);
        qput(&fp[9], dm);
        qfig(cx + r * qokt[k][1], cy + r * qokt[k][2], qmust[qpatt], 2,
             fp, 11, 0x6C);
        return 0;
}

static int qwink(a)             /* auf 0..359 */
int a;
{
        a %= 360;
        return a < 0 ? a + 360 : a;
}

int garc(r, a, e)
int r, a, e;
{
        int cx, cy, k, t, t0, w, l, voll, drin;

        if (r < 1 || r > 1000)
                return GE_RAD;
        cx = qx;
        cy = qsy(qy);
        voll = e - a >= 360 || a - e >= 360;
        if (qorig) {            /* gespiegelt: Winkel umdrehen */
                t = a;
                a = -e;
                e = -t;
        }
        a = qwink(a);
        l = qwink(e - a);
        if (l == 0 && e != a)
                voll = 1;
        for (k = 0; k < 8; k++) {
                if (voll) {
                        qbog(cx, cy, r, k, 0, 45);
                        continue;
                }
                t0 = -1;
                for (t = 0; t <= 46; t++) {
                        drin = 0;
                        if (t <= 45) {
                                w = qwink(qokt[k][4] + qokt[k][5] * t - a);
                                drin = w <= l;
                        }
                        if (drin && t0 < 0)
                                t0 = t;
                        if (!drin && t0 >= 0) {
                                qbog(cx, cy, r, k, t0, t - 1);
                                t0 = -1;
                        }
                }
        }
        qflush();
        return 0;
}

int gcircl(r)
int r;
{
        return garc(r, 0, 360);
}

/* ------------------------------------------------------------------ */
/* Zeilen wortweise                                                   */

/* Eine Bitzeile (Bit 0 = Punkt sx) in Ebene e mit WDAT-Befehl w.
   a: 0 Bits, 1 Bits umgekehrt, 2 lauter Einsen, 3 lauter Nullen.
   Setzen, Loeschen und Umkehren lassen Nullbits unberuehrt; so stoeren
   angeschnittene Worte am Rand nicht. */
int qrow(sy, sx, b, nb, e, w, a)
int sy, sx, nb, e, w, a;
char *b;
{
        register int j, v, alt;
        int nw, sh, n8, k, letzt, rest, i, leer;
        UW adr;
        char *p;

        if (nb <= 0 || sy < 0 || sy > 399)
                return 0;
        sh = sx & 15;
        nw = (sh + nb + 15) >> 4;
        n8 = (nb + 7) >> 3;
        rest = nb & 7;
        letzt = rest ? (1 << rest) - 1 : 0xFF;
        qplatz(40 + 2 * nw);    /* kein Ausgeben mittendrin (Ruecknahme) */
        adr = qadr(e, sx, sy);
        p = &ql[qn];
        *p++ = 4; *p++ = 0x49;                          /* CURS */
        *p++ = adr & 0xFF; *p++ = (adr >> 8) & 0xFF; *p++ = 0;
        *p++ = 3; *p++ = 0x4A; *p++ = 0xFF; *p++ = 0xFF;  /* MASK */
        *p++ = 4; *p++ = 0x4C; *p++ = 0x02; *p++ = 0; *p++ = 0;  /* FIGS */
        k = 2 * nw;
        *p++ = 1 + k; *p++ = w;                         /* WDAT */
        qe = qn + 14;
        qn += 16;
        if (sh == 0 && a == 0 && (nb & 15) == 0) {      /* ganze Worte */
                gkopi(p, b, k);
                qn += k;
                return 0;
        }
        ql[qe] = 1;
        /* Quelle um sh Bit nach rechts versetzt (ganze Bytes + Rest) */
        i = -(sh >> 3);
        sh &= 7;
        alt = 0;
        leer = 1;
        for (j = 0; j < k; j++, i++) {
                v = 0;
                if (i >= 0 && i < n8) {
                        switch (a) {
                        case 0: v = b[i] & 0xFF; break;
                        case 1: v = ~b[i] & 0xFF; break;
                        case 2: v = 0xFF; break;
                        }
                        if (i == n8 - 1)
                                v &= letzt;
                }
                *p++ = ((v << sh) | alt) & 0xFF;
                if (((v << sh) | alt) & 0xFF)
                        leer = 0;
                alt = sh ? v >> (8 - sh) : 0;
        }
        if (leer && w != 0x20) {        /* nichts zu tun: Eintrag zurueck */
                qn = qe - 14;   /* CURS 5, MASK 4, FIGS 5 Byte */
                qe = -1;
                return 0;
        }
        qn += k;
        ql[qe] += k;
        return 0;
}

/* Bitzeile in der aktuellen Farbe und Betriebsart auf alle Ebenen */
int qbits(sy, sx, b, nb)
int sy, sx, nb;
char *b;
{
        int e, an, ganz;

        ganz = (sx & 15) == 0 && (nb & 15) == 0;
        for (e = 0; e < qneb; e++) {
                an = qebon(e, qcol);
                switch (qmode) {
                case 0:
                        if (ganz)
                                qrow(sy, sx, b, nb, e, 0x20, an ? 0 : 3);
                        else if (an) {
                                qrow(sy, sx, b, nb, e, 0x22, 1);
                                qrow(sy, sx, b, nb, e, 0x23, 0);
                        } else
                                qrow(sy, sx, b, nb, e, 0x22, 2);
                        break;
                case 1:
                        if (an)
                                qrow(sy, sx, b, nb, e, 0x21, 0);
                        break;
                case 2:
                        qrow(sy, sx, b, nb, e, 0x22, 0);
                        break;
                default:
                        qrow(sy, sx, b, nb, e, an ? 0x23 : 0x22, 0);
                }
        }
        return 0;
}

/* ------------------------------------------------------------------ */
/* Text                                                               */

static int qglyph(c)            /* Zeichen -> Index in fntbit */
int c;
{
        int k;

        c &= 0xFF;
        if (c >= 32 && c < 127)
                return c - 32;
        for (k = 0; k < 7; k++)
                if ((fntuml[k] & 0xFF) == c)
                        return 95 + k;
        return 0;
}

/* Text waagrecht, wortweise: Zeile fuer Zeile aus der Schrift */
static int qtxt0(s, n)
char *s;
int n;
{
        static char z[80];
        int z1, gr, zr, i, j, k, v, sy, bit, nb;

        z1 = qzoom + 1;
        nb = n * 8 * z1;
        sy = qsy(qy);
        if (!qorig)
                sy -= 16 * z1 - 1;
        for (gr = 0; gr < 16; gr++) {
                for (i = 0; i < 80; i++)
                        z[i] = 0;
                for (i = 0; i < n; i++) {
                        v = fntbit[qglyph(s[i]) * 16 + gr] & 0xFF;
                        if (z1 == 1) {
                                z[i] = v;
                                continue;
                        }
                        for (j = 0; j < 8; j++) {
                                if (!(v & (1 << j)))
                                        continue;
                                bit = (i * 8 + j) * z1;
                                for (k = 0; k < z1; k++, bit++)
                                        z[bit >> 3] |= 1 << (bit & 7);
                        }
                }
                for (zr = 0; zr < z1; zr++)
                        qbits(sy + gr * z1 + zr, qx, z, nb);
        }
        return 0;
}

/* Text in Richtung d: je Zeichen zwei 8 x 8-Muster ueber GCHRD */
static int qtxtd(s, n, d)
char *s;
int n, d;
{
        char fp[5], mu[8];
        int z1, td, ud, sx, sy, i, h, r, g, x, y;

        z1 = qzoom + 1;
        td = qdir(d);
        ud = (td + 2) & 7;
        qank(qx, qsy(qy), d, 16 * z1 - 1, &sx, &sy);
        qzm(qzoom);
        fp[0] = 0x10 | td;
        fp[1] = 7;
        fp[2] = 0x40;
        fp[3] = 8;
        fp[4] = 0;
        for (i = 0; i < n; i++) {
                g = qglyph(s[i]) * 16;
                for (h = 0; h < 2; h++) {
                        /* PRAM 8 = oberste Zeile, 15 = unterste (gezeichnet
                           wird von unten): untere Haelfte zuerst */
                        for (r = 0; r < 8; r++)
                                mu[r] = fntbit[g + (h ? 0 : 8) + r];
                        x = sx + i * 8 * z1 * qdx[td] + h * 8 * z1 * qdx[ud];
                        y = sy + i * 8 * z1 * qdy[td] + h * 8 * z1 * qdy[ud];
                        qfig(x, y, mu, 8, fp, 5, 0x68);
                }
        }
        return 0;
}

static int qlen(s)
char *s;
{
        int n;

        for (n = 0; s[n]; n++)
                ;
        return n;
}

int gtext(s, d)
char *s;
int d;
{
        int n, l, sx, sy;

        if (d < 0 || d > 7)
                return GE_TRICHT;
        n = qlen(s);
        if (n * 8 * (qzoom + 1) > 640)
                n = 640 / (8 * (qzoom + 1));
        if (n == 0)
                return 0;
        if (d == 0)
                qtxt0(s, n);
        else
                qtxtd(s, n, d);
        qflush();
        /* Zeichenpunkt hinter das letzte Zeichen */
        l = n * 8 * (qzoom + 1);
        sx = qx + l * qdx[qdir(d)];
        sy = qsy(qy) + l * qdy[qdir(d)];
        if (sx >= 0 && sx < 640 && sy >= 0 && sy < 400) {
                qx = sx;
                qy = quy(sy);
        }
        return 0;
}

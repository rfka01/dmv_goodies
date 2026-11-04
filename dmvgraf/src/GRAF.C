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
int qbcol = -1;                 /* Hintergrundfarbe der Muster, -1 keine */
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
        qbcol = -1;
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

/* Eigenes Muster p (0-15) aus 8 Zeilen von oben, Bit 0 = links */
int gpdef(p, m)
int p;
char *m;
{
        int i;

        if (p < 0 || p > 15)
                return GE_MUST;
        for (i = 0; i < 8; i++)
                qmust[p][i] = m[i];
        return 0;
}

/* Hintergrundfarbe fuer Muster in Flaechen (gbox): 0-7, dann zweifarbig
   und deckend; -1 (Vorgabe) wie bisher nur die Musterpunkte */
int gbcol(c)
int c;
{
        if (c < -1 || c > 7)
                return GE_MODUS;
        qbcol = c;
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

/* Muster am Schirm verankern: der 7220 beginnt GCHRD an der unteren
   linken Ecke (sx, sb) mit PRAM 15 und zeichnet die Zeilen abwechselnd
   von links und von rechts. Fuer Richtung 0 ohne Zoom die Musterzeilen
   so drehen, dass Punkt (x, y) Bit x & 7 von Zeile y & 7 bekommt. */
static int qmanker(mu, p, sx, sb, w)
char *mu, *p;
int sx, sb, w;
{
        int r, c, v;

        for (r = 0; r < 8; r++) {
                v = p[(sb - r) & 7] & 0xFF;
                c = ((r & 1) ? sx + w : sx) & 7;
                mu[7 - r] = ((v >> c) | (v << (8 - c))) & 0xFF;
        }
        return 0;
}

int gbox(m, n, d)
int m, n, d;
{
        char fp[5], mu[8], mz[8];
        int sx, sy, z, e, i, f, h;

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
        if (d == 0 && qzoom == 0)
                qmanker(mu, qmust[qpatt], sx, sy, m);
        else
                for (i = 0; i < 8; i++)
                        mu[i] = qmust[qpatt][i];
        if (qbcol < 0) {
                qfig(sx, sy, mu, 8, fp, 5, 0x68);
                qflush();
                return 0;
        }
        for (e = 0; e < qneb; e++) {    /* zweifarbig: je Ebene ersetzen */
                f = qebon(e, qcol);
                h = qebon(e, qbcol);
                for (i = 0; i < 8; i++)
                        mz[i] = (f ? mu[i] : 0) | (h ? ~mu[i] : 0);
                qplatz(48);
                qcmd(0x20);
                qcurs(qadr(e, sx, sy), sx & 15);
                qcmd(0x78);
                for (i = 0; i < 8; i++)
                        qpar(mz[i] & 0xFF);
                qcmd(0x4C);
                for (i = 0; i < 5; i++)
                        qpar(fp[i] & 0xFF);
                qcmd(0x68);
        }
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

/* Verschobene Bitzeile: einmal rechnen, dann fuer jede Ebene und jeden
   Durchgang nur kopieren. qzd Daten, qzb Maske (nb Einsen), qzi Maske
   ohne Daten; je k Byte ab Bit sx & 15. qzl: leer (Daten, umgekehrt). */
static char qzd[84], qzb[84], qzi[84];
#ifdef __ia16__
extern int qshz();
#endif
static int qzk, qzld, qzli, qzhi;

static int qshift(sx, b, nb)
int sx, nb;
char *b;
{
        register int j, v, m, altv, altm;
        int i, sh, n8, rest, letzt;

        sh = sx & 15;
        qzk = 2 * ((sh + nb + 15) >> 4);
        n8 = (nb + 7) >> 3;
        rest = nb & 7;
        letzt = rest ? (1 << rest) - 1 : 0xFF;
        i = -(sh >> 3);
        sh &= 7;
        qzhi = 0;               /* qzi erst bei Bedarf */
        if (sh == 0) {          /* auf Bytegrenze: nur kopieren */
                j = -i;         /* 0 oder 1 Byte davor */
                for (m = 0; m < qzk; m++) {
                        qzd[m] = 0;
                        qzb[m] = m >= j && m < j + n8 ? 0xFF : 0;
                }
                gkopi(&qzd[j], b, n8);
                qzd[j + n8 - 1] &= letzt;
                qzb[j + n8 - 1] = letzt;
                qzld = 1;
                for (m = j; m < j + n8; m++)
                        if (qzd[m]) {
                                qzld = 0;
                                break;
                        }
                return 0;
        }
#ifdef __ia16__
        qzld = qshz(qzd, qzb, b, qzk, i, n8, sh, letzt);    /* GCRTDOS.S */
        return 0;
#endif
        altv = altm = 0;
        qzld = 1;
        for (j = 0; j < qzk; j++, i++) {
                v = m = 0;
                if (i >= 0 && i < n8) {
                        v = b[i] & 0xFF;
                        m = 0xFF;
                        if (i == n8 - 1) {
                                v &= letzt;
                                m = letzt;
                        }
                }
                qzd[j] = (v << sh) | altv;
                qzb[j] = (m << sh) | altm;
                if (qzd[j])
                        qzld = 0;
                altv = sh ? v >> (8 - sh) : 0;
                altm = sh ? m >> (8 - sh) : 0;
        }
        return 0;
}

static int qinv()               /* Maske ohne Daten (a = 1) */
{
        register int j;

        if (!qzhi) {
                qzli = 1;
                for (j = 0; j < qzk; j++)
                        if ((qzi[j] = qzb[j] & ~qzd[j]) != 0)
                                qzli = 0;
                qzhi = 1;
        }
        return 0;
}

/* Eintrag CURS, MASK, FIGS, WDAT w mit k Byte aus q (0: Nullen) */
static int qrowk(sy, sx, e, w, q)
int sy, sx, e, w;
char *q;
{
        UW adr;
        char *p;
        int k;

        k = qzk;
        qplatz(40 + k);         /* kein Ausgeben mittendrin (Ruecknahme) */
        adr = qadr(e, sx, sy);
        p = &ql[qn];
        *p++ = 4; *p++ = 0x49;                          /* CURS */
        *p++ = adr & 0xFF; *p++ = (adr >> 8) & 0xFF; *p++ = 0;
        *p++ = 3; *p++ = 0x4A; *p++ = 0xFF; *p++ = 0xFF;  /* MASK */
        *p++ = 4; *p++ = 0x4C; *p++ = 0x02; *p++ = 0; *p++ = 0;  /* FIGS */
        *p++ = 1 + k; *p++ = w;                         /* WDAT */
        qe = qn + 14;
        qn += 16;
        if (q)
                gkopi(p, q, k);
        else
                while (k-- > 0)
                        *p++ = 0;
        qn += qzk;
        return 0;
}

/* Bitzeile aus qshift in Ebene e, Art a wie qrow */
static int qrowa(sy, sx, e, w, a)
int sy, sx, e, w, a;
{
        char *q;
        int leer;

        if (sy < 0 || sy > 399 || qzk <= 0)
                return 0;
        switch (a) {
        case 0:  q = qzd; leer = qzld; break;
        case 1:  qinv(); q = qzi; leer = qzli; break;
        case 2:  q = qzb; leer = 0; break;
        default: q = (char *)0; leer = 1;
        }
        if (leer && w != 0x20)          /* nichts zu tun */
                return 0;
        return qrowk(sy, sx, e, w, q);
}

/* Eine Bitzeile (Bit 0 = Punkt sx) in Ebene e mit WDAT-Befehl w.
   a: 0 Bits, 1 Bits umgekehrt, 2 lauter Einsen, 3 lauter Nullen.
   Setzen, Loeschen und Umkehren lassen Nullbits unberuehrt; so stoeren
   angeschnittene Worte am Rand nicht. */
int qrow(sy, sx, b, nb, e, w, a)
int sy, sx, nb, e, w, a;
char *b;
{
        UW adr;
        char *p;
        int k;

        if (nb <= 0 || sy < 0 || sy > 399)
                return 0;
        if ((sx & 15) == 0 && a == 0 && (nb & 15) == 0) {   /* ganze Worte */
                k = nb >> 3;
                qplatz(40 + k);
                adr = qadr(e, sx, sy);
                p = &ql[qn];
                *p++ = 4; *p++ = 0x49;
                *p++ = adr & 0xFF; *p++ = (adr >> 8) & 0xFF; *p++ = 0;
                *p++ = 3; *p++ = 0x4A; *p++ = 0xFF; *p++ = 0xFF;
                *p++ = 4; *p++ = 0x4C; *p++ = 0x02; *p++ = 0; *p++ = 0;
                *p++ = 1 + k; *p++ = w;
                qe = qn + 14;
                qn += 16;
                gkopi(p, b, k);
                qn += k;
                return 0;
        }
        qshift(sx, b, nb);
        return qrowa(sy, sx, e, w, a);
}

/* Bitzeile in der aktuellen Farbe und Betriebsart auf alle Ebenen */
int qbits(sy, sx, b, nb)
int sy, sx, nb;
char *b;
{
        int e, an, ganz;

        if (nb <= 0 || sy < 0 || sy > 399)
                return 0;
        ganz = (sx & 15) == 0 && (nb & 15) == 0;
        qshift(sx, b, nb);              /* einmal fuer alle Ebenen */
        for (e = 0; e < qneb; e++) {
                an = qebon(e, qcol);
                switch (qmode) {
                case 0:
                        if (ganz) {
                                if (an)
                                        qrow(sy, sx, b, nb, e, 0x20, 0);
                                else
                                        qrowa(sy, sx, e, 0x20, 3);
                        } else if (an) {
                                qrowa(sy, sx, e, 0x22, 1);
                                qrowa(sy, sx, e, 0x23, 0);
                        } else
                                qrowa(sy, sx, e, 0x22, 2);
                        break;
                case 1:
                        if (an)
                                qrowa(sy, sx, e, 0x21, 0);
                        break;
                case 2:
                        qrowa(sy, sx, e, 0x22, 0);
                        break;
                default:
                        qrowa(sy, sx, e, an ? 0x23 : 0x22, 0);
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
        static int gx[80];
        int z1, gr, zr, i, j, k, v, sy, bit, nb, nz;

        z1 = qzoom + 1;
        nb = n * 8 * z1;
        nz = (nb + 7) >> 3;
        if (nz > 80)
                nz = 80;
        sy = qsy(qy);
        if (!qorig)
                sy -= 16 * z1 - 1;
        for (i = 0; i < n; i++)
                gx[i] = qglyph(s[i]) * 16;
        for (gr = 0; gr < 16; gr++) {
                if (z1 > 1)
                        for (i = 0; i < nz; i++)
                                z[i] = 0;
                for (i = 0; i < n; i++) {
                        v = fntbit[gx[i] + gr] & 0xFF;
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

#ifndef GKLEIN
/* ------------------------------------------------------------------ */
/* Figuren ueber GCHRD ("Sprites"): Muster 8 x 8 im Parameter-RAM, der  */
/* 7220 zeichnet und vergroessert sie selbst. Schneller als Bilder,     */
/* gut fuer kleine bewegte Figuren.                                     */

#ifndef GSMAX
#define GSMAX 32
#endif
static char *qspd[GSMAX];       /* Muster des Programms (bleibt dort) */
static char qspb[GSMAX], qsph[GSMAX], qspf[GSMAX];

static int qspan(nr, b, h, mu, f)
int nr, b, h, f;
char *mu;
{
        if (nr < 0 || nr >= GSMAX)
                return GE_NR;
        if ((b != 8 && b != 16) || (h != 8 && h != 16))
                return GE_FORM;
        qspd[nr] = mu;
        qspb[nr] = b;
        qsph[nr] = h;
        qspf[nr] = f;
        return 0;
}

/* Figur nr anlegen: b und h je 8 oder 16 Punkte; mu: je Ebene (gruen,
   rot, blau) h Zeilen zu b/8 Byte, Bit 0 links. Maske ist das ODER der
   Ebenen: schwarze Punkte sind durchsichtig. Die Daten bleiben im
   Programm und muessen erhalten bleiben. */
int gsprdef(nr, b, h, mu)
int nr, b, h;
char *mu;
{
        return qspan(nr, b, h, mu, 0);
}

/* Figur nr im Format 5 (wie gbdef GB_SETR): int-Worte [Ebene gruen, rot,
   blau][SET, RESET][Zeile], ein Wort je Zeile, Bit 0 links. Punkte, die
   in keiner Ebene gesetzt oder geloescht werden, sind durchsichtig;
   Schwarz ist RESET in allen Ebenen und deckt. */
int gsprset(nr, b, h, wd)
int nr, b, h;
int *wd;
{
        return qspan(nr, b, h, (char *)wd, 1);
}

/* ein Byte der Figur: Format 4/Ebenen (s = 0) oder Format 5 SET/RESET */
static int qspby(nr, e, s, z, c)
int nr, e, s, z, c;
{
        if (qspf[nr])
                return (((int *)qspd[nr])[(2 * e + s) * qsph[nr] + z]
                        >> (8 * c)) & 0xFF;
        return qspd[nr][(e * qsph[nr] + z) * (qspb[nr] >> 3) + c] & 0xFF;
}

/* Ein 8 x 8-Feld auf Ebene e mit WDAT-Befehl w; x, y = untere linke Ecke
   (Bildzeile), dort beginnt der 7220 */
static int qkach(e, w, x, y, mu)
int e, w, x, y;
char *mu;
{
        int i;

        qplatz(48);
        qcmd(w);
        qcurs(qadr(e, x, y), x & 15);
        qcmd(0x78);
        for (i = 0; i < 8; i++)
                qpar(mu[i] & 0xFF);
        qcmd(0x4C);
        qpar(0x12); qpar(7); qpar(0x40); qpar(8); qpar(0);
        qcmd(0x68);
        return 0;
}

static int qleer(mu)
char *mu;
{
        int i;

        for (i = 0; i < 8; i++)
                if (mu[i])
                        return 0;
        return 1;
}

/* Figur nr bei x, y (Ecke wie gbput) mit Zoom z (0-15) zeichnen; art:
   GB_XOR umkehren (zweimal = weg), GB_DURCH nur die gesetzten Punkte in
   ihrer Farbe, GB_DECK das ganze Rechteck. Die Figur muss ganz auf den
   Schirm passen (sonst GE_LAGE). */
int gspr(nr, x, y, z, art)
int nr, x, y, z, art;
{
        char m[8], o[8], p[8], q[8];
        int nc, z1, sx, sy, c, r, e, i, k, f, j, xt, yt;

        if (nr < 0 || nr >= GSMAX)
                return GE_NR;
        if (!qspd[nr])
                return GE_LEER;
        if (z < 0 || z > 15)
                return GE_ZOOM;
        if (art < 0 || art > 3 || art == GB_SCHAT)
                return GE_FORM;
        if (qstart())
                return GE_HW;
        f = qspf[nr];
        nc = qspb[nr] >> 3;
        z1 = z + 1;
        sx = x;
        sy = qsy(y);
        if (!qorig)
                sy -= qsph[nr] * z1 - 1;
        if (sx < 0 || sy < 0 || sx + qspb[nr] * z1 > 640 ||
            sy + qsph[nr] * z1 > 400)
                return GE_LAGE;
        qzm(z);
        for (r = 0; r < qsph[nr] >> 3; r++)
                for (c = 0; c < nc; c++) {
                        xt = sx + c * 8 * z1;
                        yt = sy + (r + 1) * 8 * z1 - 1;
                        for (i = 0; i < 8; i++) {   /* Maske, ODER der Farben */
                                k = r * 8 + i;
                                m[i] = o[i] = 0;
                                for (e = 0; e < 3; e++) {
                                        j = qspby(nr, e, 0, k, c);
                                        o[i] |= j;
                                        m[i] |= j;
                                        if (f)
                                                m[i] |= qspby(nr, e, 1, k, c);
                                }
                        }
                        for (e = 0; e < qneb; e++) {
                                for (i = 0; i < 8; i++) {
                                        k = r * 8 + i;
                                        if (qneb == 1)
                                                p[i] = o[i];
                                        else
                                                p[i] = qspby(nr, e, 0, k, c);
                                        if (f && qneb > 1)
                                                q[i] = qspby(nr, e, 1, k, c);
                                        else
                                                q[i] = m[i] & ~p[i];
                                }
                                if (art == GB_DECK)
                                        qkach(e, 0x20, xt, yt, p);
                                else if (art == GB_XOR) {
                                        if (!qleer(p))
                                                qkach(e, 0x21, xt, yt, p);
                                } else {
                                        if (!qleer(q))
                                                qkach(e, 0x22, xt, yt, q);
                                        if (!qleer(p))
                                                qkach(e, 0x23, xt, yt, p);
                                }
                        }
                }
        qflush();
        return 0;
}

#endif

/* ------------------------------------------------------------------ */
/* Tabelle fuer den schnellen Weg der Bilder (ghblit): je Ebene, was aus
   einer gespeicherten Bildzeile (np = 1: Punkte in der Farbe, sonst Maske
   und Ebenen zu je bb Byte) zu schreiben ist. Eintrag: Ebenenadresse,
   WDAT-Befehl, Maske (0 aus der Zeile, 1 keine), XOR, Versatz, Nullen. */
int qbsch = 1;
#ifndef GKLEIN

static int *qbt;

static int qbe(e, w, m, x, v, z)
int e, w, m, x, v, z;
{
        *qbt++ = qebad[e]; *qbt++ = w; *qbt++ = m;
        *qbt++ = x; *qbt++ = v; *qbt++ = z;
        return 0;
}

int qbtupel(np, bb, art, tu)
int np, bb, art, *tu;
{
        int e, an, v;

        qbt = tu;
        for (e = 0; e < qneb; e++) {
                an = qebon(e, qcol);
                v = (e + 1) * bb;
                if (np == 1) {
                        if (art == GB_DECK)
                                an ? qbe(e, 0x20, 1, 0, 0, 0) : qbe(e, 0x20, 0, 0, 0, 1);
                        else if (art == GB_DURCH)
                                qbe(e, an ? 0x23 : 0x22, 0, 0, 0, 0);
                        else if (an)
                                qbe(e, 0x21, 0, 0, 0, 0);
                } else if (art == GB_DECK)
                        qbe(e, 0x20, 1, 0, v, 0);
                else if (art == GB_DURCH) {
                        qbe(e, 0x22, 0, -1, v, 0);
                        qbe(e, 0x23, 0, 0, v, 0);
                } else
                        qbe(e, 0x21, 0, 0, v, 0);
        }
        return (int)(qbt - tu) / 6;
}
#endif

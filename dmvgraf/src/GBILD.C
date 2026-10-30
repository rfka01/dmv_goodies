/* GBILD.C - DMVGRAF: Bilder und Zusammensetzen.
 *
 * Bilder liegen im Bildspeicher der Hardwareschicht (unter CP/M-8000 in
 * freien Segmenten, BIOS 72), je Zeile Maske und Ebenen (Bit 0 = links)
 * oder bei 1-Bit-Bildern nur die Bits. Gezeichnet wird wortweise ueber
 * Setzen/Loeschen je Ebene (GBPUT) oder bandweise zusammengesetzt und
 * ersetzend (GLAYER, GLPUT, GLDRAW).
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

/* Bildtafel; bnp: 0 frei, 1 = 1-Bit-Bild, sonst Maske + Ebenen */
static int bw[GBMAX], bh[GBMAX], bbpr[GBMAX], bnp[GBMAX], bblk[GBMAX];
static UW boff[GBMAX];
static int bx0[GBMAX], by0[GBMAX], bx1[GBMAX], by1[GBMAX];
static long btop[8];            /* belegt je Block */
static int znr = -1;            /* Bild des Mauszeigers (gzeig) */
static int bnblk = -1;

/* Farbumsetzung fuer Format 3: Farbe a (-1 durchsichtig), b, Muster */
static int mpa[16], mpb[16], mpm[16];
static int mpok;

static char bpuf[QBUF];         /* Zeilen holen und ablegen */
static char bm[80], bp[3][80], bt[80], bt2[80];
static char bein[80], bnul[80];

static int bnrok(nr)
int nr;
{
        return nr >= 0 && nr < GBMAX;
}

/* ------------------------------------------------------------------ */
/* Farbtafel                                                          */

static int bmini()
{
        static int fa[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7 };
        static int fm[16] = { 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7 };
        int i;

        if (mpok)
                return 0;
        qstart();
        for (i = 0; i < 16; i++) {
                mpa[i] = qneb == 3 ? fa[i] : fm[i];
                mpb[i] = mpa[i];
                mpm[i] = 0;
        }
        /* Farbe: braun -> Raster gelb/rot, dunkelgrau -> Raster
           schwarz/blau; mono: dunkelgrau im Raster */
        if (qneb == 3) {
                mpa[6] = 6; mpb[6] = 4; mpm[6] = 1;
        }
        mpa[8] = 0; mpb[8] = qneb == 3 ? 1 : 7; mpm[8] = 1;
        mpa[0] = -1; mpb[0] = 0;        /* 0 durchsichtig */
        mpok = 1;
        return 0;
}

int gbmap(f, a, b, m)
int f, a, b, m;
{
        bmini();
        if (f < 0 || f > 15 || a < -1 || a > 7 || b < 0 || b > 7
            || m < 0 || m > 3)
                return GE_FORM;
        mpa[f] = a;
        mpb[f] = b;
        mpm[f] = m;
        return 0;
}

/* Tafel je Byte (zwei Punkte) fuer gerade und ungerade Zeilen:
   Bit 0/1 Ebene 0 (links/rechts), 2/3 Ebene 1, 4/5 Ebene 2, 6/7 sichtbar */
static char ptab[2][256];

static int bfarb(c, x, y)       /* Farbnummer an der Stelle -> DMV-Farbe */
int c, x, y;
{
        int r;

        if (mpa[c] < 0)
                return -1;
        switch (mpm[c]) {
        case 1: r = (x + y) & 1; break;
        case 2: r = x & y & 1; break;
        case 3: r = y & 1; break;
        default: r = 0;
        }
        return r ? mpb[c] : mpa[c];
}

static int ptbau()
{
        int y, v, e, w, c0, c1;

        for (y = 0; y < 2; y++)
                for (v = 0; v < 256; v++) {
                        c0 = bfarb(v >> 4, 0, y);
                        c1 = bfarb(v & 15, 1, y);
                        w = 0;
                        for (e = 0; e < qneb; e++) {
                                if (c0 >= 0 && (c0 & qebit[e]))
                                        w |= 1 << (2 * e);
                                if (c1 >= 0 && (c1 & qebit[e]))
                                        w |= 2 << (2 * e);
                        }
                        if (c0 >= 0) w |= 0x40;
                        if (c1 >= 0) w |= 0x80;
                        ptab[y][v] = w;
                }
        return 0;
}

/* ------------------------------------------------------------------ */
/* Bildspeicher                                                       */

static int balloc(nr, n)
int nr;
long n;
{
        int k;

        if (bnblk < 0) {
                bnblk = ghsnum();
                if (bnblk > 8)
                        bnblk = 8;
                for (k = 0; k < bnblk; k++)
                        btop[k] = 0;
        }
        for (k = 0; k < bnblk; k++)
                if (btop[k] + n <= ghsgro(k)) {
                        bblk[nr] = k;
                        boff[nr] = (UW)btop[k];
                        btop[k] += n;
                        return 0;
                }
        return GE_PLATZ;
}

static long bgr(nr)             /* Groesse eines Bildes */
int nr;
{
        return (long)bh[nr] * bnp[nr] * bbpr[nr];
}

int gbfree(nr)
int nr;
{
        int k, b;
        long n, o, rest;

        if (!bnrok(nr))
                return GE_NR;
        if (!bnp[nr])
                return 0;
        if (nr == znr)
                gzeig(-1, 0, 0);
        b = bblk[nr];
        n = bgr(nr);
        o = (long)boff[nr] & 0xFFFFL;
        rest = btop[b] - o - n;
        if (rest > 0)                   /* dahinter liegendes nachruecken */
                ghsmov(b, (UW)o, (UW)(o + n), (UW)rest);
        for (k = 0; k < GBMAX; k++)
                if (bnp[k] && k != nr && bblk[k] == b
                    && ((long)boff[k] & 0xFFFFL) > o)
                        boff[k] = (UW)(((long)boff[k] & 0xFFFFL) - n);
        btop[b] -= n;
        bnp[nr] = 0;
        return 0;
}

static int hnr, hn;             /* Ablegen: Bild, Bytes im Puffer */
static long hpos;

static int hflush()
{
        if (hn) {
                ghsput(bblk[hnr], (UW)(((long)boff[hnr] & 0xFFFFL) + hpos),
                       bpuf, hn);
                hpos += hn;
                hn = 0;
        }
        return 0;
}

static int hput(s, n)
char *s;
int n;
{
        if (hn + n > QBUF)
                hflush();
        gkopi(&bpuf[hn], s, n);
        hn += n;
        return 0;
}

static int bhol(nr, y, n, d)    /* n Zeilen ab y holen */
int nr, y, n;
char *d;
{
        int rs;

        rs = bnp[nr] * bbpr[nr];
        ghsget(d, bblk[nr], (UW)(((long)boff[nr] & 0xFFFFL) + (long)y * rs),
               n * rs);
        return 0;
}

/* ------------------------------------------------------------------ */
/* Quelle: Speicher oder Datei                                        */

static char *rmem;
static char rbuf[128];
static int rpos;

static int brd(d, n)            /* n Byte lesen -> 1 bei Dateiende */
register char *d;
int n;
{
        register int i;

        if (rmem) {
                gkopi(d, rmem, n);
                rmem += n;
                return 0;
        }
        while (n > 0) {
                if (rpos >= 128) {
                        if (!ghread(rbuf))
                                return 1;
                        rpos = 0;
                }
                i = 128 - rpos;
                if (i > n)
                        i = n;
                gkopi(d, &rbuf[rpos], i);
                d += i;
                rpos += i;
                n -= i;
        }
        return 0;
}

/* ------------------------------------------------------------------ */
/* Bild anlegen                                                       */

static char rraw[320];
static char ra[4 * 80], rb[4 * 80];     /* je 80: sichtbar, Ebenen */
static int brest;               /* Bits im letzten Byte einer Zeile */

/* niedrigstes und hoechstes gesetztes Bit eines Halbbytes */
static char nlo[16] = { 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };
static char nhi[16] = { 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3 };

/* erster und letzter gesetzter Punkt von n Byte, -1 = keiner */
static int bspan(z, n, lo, hi)
register char *z;
int n, *lo, *hi;
{
        register int k, v;

        *lo = *hi = -1;
        for (k = 0; k < n; k++)
                if ((v = z[k] & 0xFF) != 0) {
                        *lo = k * 8 + ((v & 15) ? nlo[v & 15]
                                                : 4 + nlo[v >> 4]);
                        break;
                }
        if (*lo < 0)
                return 0;
        for (k = n - 1; k >= 0; k--)
                if ((v = z[k] & 0xFF) != 0) {
                        *hi = k * 8 + ((v >> 4) ? 4 + nhi[v >> 4]
                                                : nhi[v & 15]);
                        break;
                }
        return 0;
}

static int bbox(nr, z, y)       /* sichtbare Punkte einer Zeile in bx0.. */
int nr, y;
char *z;
{
        int lo, hi;

        bspan(z, bbpr[nr], &lo, &hi);
        if (hi < 0)
                return 0;
        if (lo < bx0[nr]) bx0[nr] = lo;
        if (hi > bx1[nr]) bx1[nr] = hi;
        if (y < by0[nr]) by0[nr] = y;
        if (y > by1[nr]) by1[nr] = y;
        return 0;
}

/* Format 3: Zeile y nach z (je 80 Byte: sichtbar, Ebene 0, 1, 2) */
static int bnib(nr, y, z)
int nr, y;
char *z;
{
        int n, k, e;

        n = bbpr[nr];
        for (k = (bw[nr] + 1) >> 1; k < 4 * n; k++)
                rraw[k] = 0;            /* letzte Gruppe auffuellen */
        gnib(z, rraw, ptab[y & 1], n);
        z[n - 1] &= brest;
        for (e = 1; e <= 3; e++)
                z[80 * e + n - 1] &= z[n - 1];
        return 0;
}

/* Format 3: Zeile fertig machen (Maske nach Fuellart) und ablegen */
static int bfin(nr, y, z, lo, hi, plo, phi, nlo2, nhi2, art, fb)
int nr, y, lo, hi, plo, phi, nlo2, nhi2, art, fb;
char *z;
{
        register char *q, *d;
        register int k;
        int n, e;

        n = bbpr[nr];
        if (hi >= 0) {
                if (lo < bx0[nr]) bx0[nr] = lo;
                if (hi > bx1[nr]) bx1[nr] = hi;
                if (y < by0[nr]) by0[nr] = y;
                if (y > by1[nr]) by1[nr] = y;
        }
        for (k = 0; k < n; k++)
                bm[k] = art ? 0 : z[k];
        if (art) {
                if (art == 2) {
                        /* Umriss um einen Punkt erweitern, auch senkrecht:
                           die Spanne umfasst die Nachbarzeilen (so bleibt
                           keine Zeile offen, deren Rand nur aus der
                           durchsichtigen Farbe besteht) */
                        if (phi >= 0) {
                                if (hi < 0 || plo < lo) lo = plo;
                                if (phi > hi) hi = phi;
                        }
                        if (nhi2 >= 0) {
                                if (hi < 0 || nlo2 < lo) lo = nlo2;
                                if (nhi2 > hi) hi = nhi2;
                        }
                }
                if (hi >= 0) {
                        if (art == 2) {
                                lo--;
                                hi++;
                        }
                        if (lo < 0) lo = 0;
                        if (hi > bw[nr] - 1) hi = bw[nr] - 1;
                        for (k = lo >> 3; k <= hi >> 3; k++)
                                bm[k] = 0xFF;
                        bm[lo >> 3] &= 0xFF << (lo & 7);
                        bm[hi >> 3] &= 0xFF >> (7 - (hi & 7));
                }
        }
        hput(bm, n);
        q = z + 80;
        for (e = 0; e < qneb; e++, q += 80) {
                if (fb & qebit[e]) {
                        d = bt;
                        for (k = 0; k < n; k++)
                                *d++ = q[k] | (bm[k] & ~z[k]);
                        hput(bt, n);
                } else
                        hput(q, n);
        }
        return 0;
}

static int bfmt3(nr)
int nr;
{
        int y, lo, hi, plo, phi, clo, chi, art, fb, i, da;
        char *cur, *nxt, *t;

        art = 0;
        fb = 0;
        for (i = 0; i < 16; i++)
                if (mpa[i] < 0 && mpm[i] > art) {
                        art = mpm[i] > 2 ? 2 : mpm[i];
                        fb = mpb[i];
                }
        cur = ra;
        nxt = rb;
        plo = phi = clo = chi = -1;
        da = 0;
        for (y = 0; y < bh[nr]; y++) {
                if (brd(rraw, (bw[nr] + 1) >> 1))
                        return GE_KURZ;
                bnib(nr, y, nxt);
                bspan(nxt, bbpr[nr], &lo, &hi);
                if (da) {
                        bfin(nr, y - 1, cur, clo, chi, plo, phi, lo, hi,
                             art, fb);
                        plo = clo;
                        phi = chi;
                }
                t = cur; cur = nxt; nxt = t;
                clo = lo;
                chi = hi;
                da = 1;
        }
        bfin(nr, bh[nr] - 1, cur, clo, chi, plo, phi, -1, -1, art, fb);
        return 0;
}

static int brev(v)              /* Bitfolge eines Bytes umdrehen */
int v;
{
        int r, j;

        r = 0;
        for (j = 0; j < 8; j++)
                if (v & (1 << j))
                        r |= 0x80 >> j;
        return r;
}

static int bfmt12(nr, fmt)
int nr, fmt;
{
        int y, k, n;

        n = bbpr[nr];
        for (y = 0; y < bh[nr]; y++) {
                if (brd(rraw, n))
                        return GE_KURZ;
                for (k = 0; k < n; k++) {
                        if (fmt == GB_BIT7)
                                rraw[k] = brev(rraw[k] & 0xFF);
                        if (k == n - 1)
                                rraw[k] &= brest;
                }
                bbox(nr, rraw, y);
                hput(rraw, n);
        }
        return 0;
}

static int bfmt4(nr)
int nr;
{
        int y, k, n, e;

        n = bbpr[nr];
        for (y = 0; y < bh[nr]; y++) {
                if (brd(rraw, 4 * n))
                        return GE_KURZ;
                rraw[n - 1] &= brest;
                bbox(nr, rraw, y);
                hput(rraw, n);
                if (qneb == 3)
                        for (e = 0; e < 3; e++) {
                                for (k = 0; k < n; k++)
                                        bt[k] = rraw[(e + 1) * n + k]
                                                & rraw[k];
                                hput(bt, n);
                        }
                else {
                        for (k = 0; k < n; k++)
                                bt[k] = (rraw[n + k] | rraw[2 * n + k]
                                         | rraw[3 * n + k]) & rraw[k];
                        hput(bt, n);
                }
        }
        return 0;
}

/* Format 5: int-Worte, [Ebene gruen/rot/blau][SET, RESET][Zeile][Wort] */
static int bfmt5(nr, wd)
int nr;
int *wd;
{
        int y, k, n, e, wpr, s, r, i;
        UW ws, wr;

        n = bbpr[nr];
        wpr = (bw[nr] + 15) >> 4;
        for (y = 0; y < bh[nr]; y++) {
                for (k = 0; k < n; k++) {
                        bm[k] = 0;
                        bp[0][k] = bp[1][k] = bp[2][k] = 0;
                }
                for (e = 0; e < 3; e++)
                        for (k = 0; k < wpr; k++) {
                                i = ((e * 2) * bh[nr] + y) * wpr + k;
                                ws = (UW)wd[i];
                                wr = (UW)wd[i + bh[nr] * wpr];
                                for (s = 0; s < 2 && 2 * k + s < n; s++) {
                                        r = s ? (ws >> 8) & 0xFF : ws & 0xFF;
                                        bm[2 * k + s] |= r | (s ? (wr >> 8)
                                                        & 0xFF : wr & 0xFF);
                                        bp[qneb == 3 ? e : 0][2 * k + s] |= r;
                                }
                        }
                bm[n - 1] &= brest;
                bbox(nr, bm, y);
                hput(bm, n);
                for (e = 0; e < qneb; e++)
                        hput(bp[e], n);
        }
        return 0;
}

/* Bild nr anlegen; die Daten kommen ueber brd() oder bei Format 5
   aus wd */
static int bdef(nr, b, h, fmt, wd)
int nr, b, h, fmt;
int *wd;
{
        int r;

        if (!bnrok(nr))
                return GE_NR;
        if (b < 1 || b > 640 || h < 1 || h > 400 || fmt < 1 || fmt > 5)
                return GE_FORM;
        gbfree(nr);
        bw[nr] = b;
        bh[nr] = h;
        bbpr[nr] = (b + 7) >> 3;
        bnp[nr] = (fmt == GB_BIT7 || fmt == GB_BIT0) ? 1 : qneb + 1;
        brest = (b & 7) ? (1 << (b & 7)) - 1 : 0xFF;
        if (balloc(nr, bgr(nr))) {
                bnp[nr] = 0;
                return GE_PLATZ;
        }
        bx0[nr] = by0[nr] = 9999;
        bx1[nr] = by1[nr] = -1;
        hnr = nr;
        hn = 0;
        hpos = 0;
        switch (fmt) {
        case GB_NIB:  r = bfmt3(nr); break;
        case GB_EBN:  r = bfmt4(nr); break;
        case GB_SETR: r = bfmt5(nr, wd); break;
        default:      r = bfmt12(nr, fmt);
        }
        hflush();
        if (r)
                gbfree(nr);
        return r;
}

int gbdef(nr, b, h, fmt, daten)
int nr, b, h, fmt;
char *daten;
{
        int r;

        if (qstart())
                return GE_HW;
        bmini();
        if (fmt == GB_NIB)
                ptbau();
        rmem = daten;
        r = bdef(nr, b, h, fmt, (int *)daten);
        rmem = 0;
        return r;
}

int gbload(nr0, name, b, h, anz, fmt, kopf)
int nr0, b, h, anz, fmt, kopf;
char *name;
{
        int i, r;
        char c;

        if (qstart())
                return GE_HW;
        bmini();
        if (fmt == GB_SETR)
                return GE_FORM;
        if (fmt == GB_NIB)
                ptbau();
        if (ghopen(name))
                return GE_DATEI;
        rmem = 0;
        rpos = 128;
        r = 0;
        for (i = 0; i < kopf && !r; i++)
                if (brd(&c, 1))
                        r = GE_KURZ;
        for (i = 0; i < anz && !r; i++)
                r = bdef(nr0 + i, b, h, fmt, (int *)0);
        ghclos();
        return r;
}

int gbinfo(nr, r)
int nr;
int *r;
{
        if (!bnrok(nr))
                return GE_NR;
        if (!bnp[nr])
                return GE_LEER;
        r[0] = bw[nr];
        r[1] = bh[nr];
        r[2] = bx0[nr];
        r[3] = by0[nr];
        r[4] = bx1[nr];
        r[5] = by1[nr];
        return 0;
}

/* ------------------------------------------------------------------ */
/* Zeichnen                                                           */

/* Geholte Zeile z fuer Art art als zusammenhaengende Zeile: Maske, im
   Abstand n die Ebenen. Ohne Kopie, wo es geht (Ebenenbild, Art 1, 3),
   sonst in bzr. Schatten im Raster passend zur Bildlage sx + sy. */
static char bzr[4 * 80];

static char *bzei(nr, z, art, sx, sy)
int nr, art, sx, sy;
char *z;
{
        register int k, n;
        int e, pat, s;
        char *q, *g, *r;

        n = bbpr[nr];
        if (bnp[nr] != 1 && (art == GB_DURCH || art == GB_XOR))
                return z;
        if (bnp[nr] == 1) {
                gkopi(bzr, z, n);
                for (e = 0; e < qneb; e++)
                        gkopi(&bzr[(e + 1) * n], qebon(e, qcol) ? z : bnul,
                              n);
        } else
                gkopi(bzr, z, (qneb + 1) * n);
        if (art == GB_DECK) {
                for (k = 0; k < n; k++)
                        bzr[k] = 0xFF;
                bzr[n - 1] = (bw[nr] & 7) ? (1 << (bw[nr] & 7)) - 1 : 0xFF;
        }
        if (art == GB_SCHAT) {
                pat = ((sx + sy) & 1) ? 0x55 : 0xAA;
                g = &bzr[n];
                r = g + n;
                q = r + n;
                for (k = 0; k < n; k++) {
                        if (qneb == 3) {
                                s = g[k] & r[k] & q[k] & pat;
                                g[k] &= ~s;
                                r[k] &= ~s;
                        } else
                                g[k] &= ~pat;
                }
        }
        return bzr;
}

static int bkopz(z, n)          /* Zeile nach bm, bp (fuer GBPUT) */
char *z;
int n;
{
        int e;

        gkopi(bm, z, n);
        for (e = 0; e < qneb; e++)
                gkopi(bp[e], z + (e + 1) * n, n);
        return 0;
}

/* d = Bits off .. off + n - 1 von s */
static int bcut(d, s, off, n)
char *d, *s;
int off, n;
{
        int i, j;

        for (i = 0; i < (n + 7) >> 3; i++)
                d[i] = 0;
        for (i = 0; i < n; i++) {
                j = off + i;
                if (s[j >> 3] & (1 << (j & 7)))
                        d[i >> 3] |= 1 << (i & 7);
        }
        return 0;
}

static int bkopi(d, s)          /* 80 Byte */
register char *d, *s;
{
        register int i;

        for (i = 80; i > 0; i--)
                *d++ = *s++;
        return 0;
}

static int bzoom(d, s, n, z1)   /* jedes Bit z1-mal, Ergebnis n * z1 Bit */
char *d, *s;
int n, z1;
{
        int i, k, j, m;

        m = n * z1;
        if (m > 640)
                m = 640;
        for (i = 0; i < (m + 7) >> 3; i++)
                d[i] = 0;
        for (i = 0; i < n; i++)
                if (s[i >> 3] & (1 << (i & 7)))
                        for (k = 0; k < z1; k++) {
                                j = i * z1 + k;
                                if (j < m)
                                        d[j >> 3] |= 1 << (j & 7);
                        }
        return 0;
}

/* Zeile (bm, bp; nb Bit ab Punkt sx) auf den Schirm */
static int bzeig(sy, sx, nb, art)
int sy, sx, nb, art;
{
        int e, k, n, ganz;

        n = (nb + 7) >> 3;
        ganz = art == GB_DECK && (sx & 15) == 0 && (nb & 15) == 0;
        for (e = 0; e < qneb; e++) {
                if (ganz) {
                        qrow(sy, sx, bp[e], nb, e, 0x20, 0);
                        continue;
                }
                if (art == GB_XOR) {
                        for (k = 0; k < n; k++)
                                bt[k] = bm[k] & bp[e][k];
                        qrow(sy, sx, bt, nb, e, 0x21, 0);
                        continue;
                }
                for (k = 0; k < n; k++) {
                        bt[k] = bm[k] & ~bp[e][k];
                        bt2[k] = bm[k] & bp[e][k];
                }
                qrow(sy, sx, bt, nb, e, 0x22, 0);
                qrow(sy, sx, bt2, nb, e, 0x23, 0);
        }
        return 0;
}

static int bvor(nr, art)        /* Pruefen, Hilfspuffer */
int nr, art;
{
        int k;

        if (!bnrok(nr))
                return GE_NR;
        if (!bnp[nr])
                return GE_LEER;
        if (art < 0 || art > 3)
                return GE_FORM;
        for (k = 0; k < 80; k++) {
                bein[k] = 0xFF;
                bnul[k] = 0;
        }
        return 0;
}

int gbput(nr, art)
int nr, art;
{
        int z1, W, H, sx, sy, y, n, zr, rs, k, e, c0, c1, ys, nb;
        char *z;

        if ((k = bvor(nr, art)) != 0)
                return k;
        z1 = qzoom + 1;
        W = bw[nr] * z1;
        H = bh[nr] * z1;
        sx = qx;
        sy = qsy(qy);
        if (!qorig)
                sy -= H - 1;
        c0 = sx < 0 ? -sx : 0;          /* sichtbare Spalten */
        c1 = sx + W > 640 ? 640 - sx : W;
        if (c0 >= c1)
                return 0;
        rs = bnp[nr] * bbpr[nr];
        for (y = 0; y < bh[nr]; y += n) {
                n = QBUF / rs;
                if (n > bh[nr] - y)
                        n = bh[nr] - y;
                bhol(nr, y, n, bpuf);
                for (k = 0; k < n; k++) {
                        z = &bpuf[k * rs];
                        for (zr = 0; zr < z1; zr++) {
                                ys = sy + (y + k) * z1 + zr;
                                if (ys < 0 || ys > 399)
                                        continue;
                                bkopz(bzei(nr, z, art, sx, ys), bbpr[nr]);
                                nb = bw[nr];
                                if (z1 > 1) {
                                        bzoom(bt, bm, nb, z1);
                                        bkopi(bm, bt);
                                        for (e = 0; e < qneb; e++) {
                                                bzoom(bt, bp[e], nb, z1);
                                                bkopi(bp[e], bt);
                                        }
                                        nb = W > 640 ? 640 : W;
                                }
                                if (c0 > 0 || c1 < nb) {
                                        bcut(bt, bm, c0, c1 - c0);
                                        bkopi(bm, bt);
                                        for (e = 0; e < qneb; e++) {
                                                bcut(bt, bp[e], c0, c1 - c0);
                                                bkopi(bp[e], bt);
                                        }
                                }
                                bzeig(ys, sx + c0, c1 - c0, art);
                        }
                }
        }
        qflush();
        return 0;
}

/* Schirmausschnitt am Zeichenpunkt als Bild nr */
int gbget(nr, b, h)
int nr, b, h;
{
        static char blk[24 + 84];
        int sx, sy, y, e, nw, i, k, n, sh;
        UW a;
        char *p, *q;

        if (!bnrok(nr))
                return GE_NR;
        if (qzeig)
                qzhid();
        sx = qx;
        sy = qsy(qy);
        if (!qorig)
                sy -= h - 1;
        if (b < 1 || h < 1 || sx < 0 || sx + b > 640 || sy < 0
            || sy + h > 400)
                return GE_FORM;
        gbfree(nr);
        bw[nr] = b;
        bh[nr] = h;
        n = bbpr[nr] = (b + 7) >> 3;
        bnp[nr] = qneb + 1;
        if (balloc(nr, bgr(nr))) {
                bnp[nr] = 0;
                return GE_PLATZ;
        }
        bx0[nr] = by0[nr] = 0;
        bx1[nr] = b - 1;
        by1[nr] = h - 1;
        hnr = nr;
        hn = 0;
        hpos = 0;
        sh = sx & 15;
        nw = (sh + b + 15) >> 4;
        qflush();
        for (k = 0; k < n; k++)
                bm[k] = 0xFF;
        bm[n - 1] = (b & 7) ? (1 << (b & 7)) - 1 : 0xFF;
        for (y = 0; y < h; y++) {
                hput(bm, n);
                for (e = 0; e < qneb; e++) {
                        a = M16(qebad[e] + (UW)(sy + y) * 40 + (sx >> 4));
                        /* Wort m, Liste CURS, MASK, FIGS, RDAT, Ende;
                           DC = Zahl der Worte (so liest MAMEs 7220 genau
                           nw Worte; der echte liest eines mehr, das der
                           naechste Befehl verwirft) */
                        p = blk;
                        *p++ = 0; *p++ = 2 * nw;
                        *p++ = 4; *p++ = 0x49; *p++ = a & 0xFF;
                        *p++ = (a >> 8) & 0xFF; *p++ = 0;
                        *p++ = 3; *p++ = 0x4A; *p++ = 0xFF; *p++ = 0xFF;
                        *p++ = 4; *p++ = 0x4C; *p++ = 0x02; *p++ = nw;
                        *p++ = 0;
                        *p++ = 1; *p++ = 0xA0;
                        *p++ = 0;
                        for (i = 0; i < 2 * nw; i++)
                                p[i] = 0;
                        ghrdat(blk);
                        bcut(bt, p, sh, b);
                        q = bt;
                        hput(q, n);
                }
        }
        hflush();
        return 0;
}

/* ------------------------------------------------------------------ */
/* Zusammensetzen                                                     */

static int lan, ln;
static int lx0, ly0, lx1, ly1;  /* Bildzeilen, x auf Worte gerundet */
static char lnr[GLMAX], lart[GLMAX];
static int lsx[GLMAX], lsy[GLMAX];
static char band[3][BH][80];

int glayer(x0, y0, x1, y1)
int x0, y0, x1, y1;
{
        int t;

        y0 = qsy(y0);
        y1 = qsy(y1);
        if (x0 > x1) { t = x0; x0 = x1; x1 = t; }
        if (y0 > y1) { t = y0; y0 = y1; y1 = t; }
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 > 639) x1 = 639;
        if (y1 > 399) y1 = 399;
        lan = x0 <= x1 && y0 <= y1;
        lx0 = x0 & ~15;
        lx1 = (x1 | 15) + 1;
        ly0 = y0;
        ly1 = y1 + 1;
        ln = 0;
        return 0;
}

int glput(nr, x, y, art)
int nr, x, y, art;
{
        int sy;

        if (!lan)
                return GE_LAGE;
        if (!bnrok(nr))
                return GE_NR;
        if (!bnp[nr])
                return GE_LEER;
        sy = qsy(y);
        if (!qorig)
                sy -= bh[nr] - 1;
        if (x >= lx1 || x + bw[nr] <= lx0 || sy >= ly1 || sy + bh[nr] <= ly0)
                return 0;
        if (ln >= GLMAX)
                return GE_LAGE;
        lnr[ln] = nr;
        lsx[ln] = x;
        lsy[ln] = sy;
        lart[ln] = art;
        ln++;
        return 0;
}

#define EB (BH * 80)            /* Abstand der Ebenen im Band */

/* Zeile z (Maske, Ebenen im Abstand n) um sh Punkte versetzt in das
   Band ab d: cnt Bytes ab Quellbyte k0, allgemein (mono, XOR) */
static int lgen(d, z, n, sh, k0, cnt, art)
char *d, *z;
int n, sh, k0, cnt, art;
{
        register int k, mm, v;
        int j, e, w;
        char *q;

        for (j = 0; j < cnt; j++, d++) {
                k = k0 + j;
                w = (k < n ? z[k] & 0xFF : 0) << sh;
                if (k > 0)
                        w |= (z[k - 1] & 0xFF) >> (8 - sh);
                mm = w & 0xFF;
                if (!mm)
                        continue;
                for (e = 0; e < qneb; e++) {
                        q = z + (e + 1) * n;
                        w = (k < n ? q[k] & 0xFF : 0) << sh;
                        if (k > 0)
                                w |= (q[k - 1] & 0xFF) >> (8 - sh);
                        v = w & mm;
                        if (art == GB_XOR)
                                d[e * EB] ^= v;
                        else
                                d[e * EB] = (d[e * EB] & ~mm) | v;
                }
        }
        return 0;
}

/* Uebertrag aus dem Byte vor k0 */
static int lueb(z, k0, sh)
char *z;
int k0, sh;
{
        return sh && k0 > 0 ? (z[k0 - 1] & 0xFF) >> (8 - sh) : 0;
}

int gldraw()
{
        int nb, yb, yz, e, r, k, nr, r0, r1, rs, y, n, i, art;
        int bx, b0, sh, k0, cnt, ns, nn, schnell, c01, c23, direkt;
        char *z, *zz, *dz;

        if (!lan)
                return GE_LAGE;
        for (k = 0; k < 80; k++) {
                bein[k] = 0xFF;
                bnul[k] = 0;
        }
        nb = (lx1 - lx0) >> 3;
        for (yb = ly0; yb < ly1; yb += BH) {
                yz = ly1 - yb;
                if (yz > BH)
                        yz = BH;
                if (nb == 80)
                        gnull(band[0][0], 3 * EB);
                else
                        for (e = 0; e < qneb; e++)
                                for (r = 0; r < yz; r++)
                                        gnull(band[e][r], nb);
                for (k = 0; k < ln; k++) {
                        nr = lnr[k] & 0xFF;
                        if (lsy[k] >= yb + yz || lsy[k] + bh[nr] <= yb)
                                continue;
                        /* Lage im Band, fuer alle Zeilen gleich */
                        nn = bbpr[nr];
                        bx = lsx[k] - lx0;
                        b0 = ((bx + 8192) >> 3) - 1024;
                        sh = bx - 8 * b0;
                        k0 = b0 < 0 ? -b0 : 0;
                        cnt = sh ? nn + 1 : nn;
                        if (cnt > nb - b0)
                                cnt = nb - b0;
                        cnt -= k0;
                        if (cnt <= 0)
                                continue;
                        ns = nn - k0;
                        art = lart[k];
                        schnell = qneb == 3 && art != GB_XOR;
                        r0 = lsy[k] > yb ? lsy[k] : yb;
                        r1 = lsy[k] + bh[nr];
                        if (r1 > yb + yz)
                                r1 = yb + yz;
                        rs = bnp[nr] * nn;
                        direkt = bnp[nr] != 1
                                 && (art == GB_DURCH || art == GB_XOR);
                        dz = &band[0][r0 - yb][b0 + k0];
                        for (y = r0; y < r1; y += n) {
                                n = QBUF / rs;
                                if (n > r1 - y)
                                        n = r1 - y;
                                bhol(nr, y - lsy[k], n, bpuf);
                                z = bpuf;
                                for (i = 0; i < n; i++, z += rs, dz += 80) {
                                        zz = direkt ? z : bzei(nr, z, art,
                                                               lsx[k], y + i);
                                        if (!schnell) {
                                                lgen(dz, zz, nn, sh, k0, cnt,
                                                     art);
                                                continue;
                                        }
                                        c01 = c23 = 0;
                                        if (sh && k0) {
                                                c01 = (lueb(zz, k0, sh) << 8)
                                                      | lueb(zz + nn, k0, sh);
                                                c23 = (lueb(zz + 2 * nn, k0,
                                                            sh) << 8)
                                                      | lueb(zz + 3 * nn, k0,
                                                             sh);
                                        }
                                        gzeile(dz, zz + k0, nn, sh, cnt, ns,
                                               c01, c23, EB);
                                }
                        }
                }
                for (r = 0; r < yz; r++)
                        for (e = 0; e < qneb; e++)
                                qrow(yb + r, lx0, band[e][r], nb * 8, e,
                                     0x20, 0);
        }
        qflush();
        lan = 0;
        return 0;
}

/* ------------------------------------------------------------------ */
/* Mauszeiger mit gesichertem Hintergrund                             */
/*                                                                    */
/* gzeig legt das Bild nr durchsichtig an x, y und merkt sich, was    */
/* darunter war. Beim Versetzen wird der Kasten um alten und neuen    */
/* Platz einmal gelesen (je Ebene und Wortspalte ein RDAT nach        */
/* unten), der alte Hintergrund eingesetzt, der Zeiger neu darueber   */
/* gelegt und der Kasten einmal geschrieben: kein Flackern, und ohne  */
/* das Zusammensetzen der Steine. Jede andere Ausgabe nimmt den       */
/* Zeiger vorher weg (qplatz -> qzhid); mpos, gkey und gwait zeigen   */
/* ihn wieder.                                                        */

#define ZH      32              /* Zeiger hoechstens 32 x 32 Punkte */
#define ZKH     48              /* Kasten hoechstens 6 Worte x 48 Zeilen */
#define ZKB     12
#define ZBUF(e) (band[0][0] + (e) * (ZKH * ZKB))  /* Kasten im Band */

int qzeig;                      /* Zeiger steht, Ausgabe muss ihn wegnehmen */
static int zaus;                /* weggenommen, wieder zeigen */
static int zx, zsy, zh, znn, zbnp;
static int zr0, zr1, zc0, znc;  /* sichtbare Zeilen, erste Wortspalte, Zahl */
static int zsh = -1;            /* Versatz, fuer den zpm/zpe gelten */
static char zbild[4 * ZH * 4];  /* Maske, Ebenen (maskiert), Zeile 4 Byte */
static char zsich[3][ZH * 6];   /* Hintergrund, Zeile zu 3 Worten */
static char zpm[ZH * 6];        /* Maske, versetzt */
static char zpe[3][ZH * 6];     /* Ebenen, versetzt und maskiert */

static int zvers(sh)            /* Bild um sh Punkte versetzt */
int sh;
{
        register char *q, *d;
        register int i, v;
        int r, e, b0, s, t, n;

        zsh = sh;
        b0 = sh >> 3;
        s = sh & 7;
        t = 8 - s;
        n = znn;
        for (e = 0; e <= qneb; e++) {
                d = e ? zpe[e - 1] : zpm;
                q = &zbild[e * ZH * 4];
                gnull(d, zh * 6);
                for (r = 0; r < zh; r++, d += 6, q += 4) {
                        d += b0;
                        for (i = 0; i < n; i++) {
                                v = q[i] & 0xFF;
                                d[i] |= v << s;
                                d[i + 1] |= v >> t;
                        }
                        d -= b0;
                }
        }
        return 0;
}

static int zlad(nr)             /* Bild holen: Maske, Ebenen maskiert */
int nr;
{
        int r, e, i, rs, v;
        char *z;

        znr = nr;
        zh = bh[nr];
        znn = bbpr[nr];
        zbnp = bnp[nr];
        rs = zbnp * znn;
        gnull(zbild, sizeof(zbild));
        for (r = 0; r < zh; r++) {
                bhol(nr, r, 1, bpuf);
                z = bpuf;
                for (i = 0; i < znn; i++) {
                        zbild[r * 4 + i] = z[i];
                        for (e = 0; e < qneb; e++) {
                                if (zbnp == 1)
                                        v = qebon(e, qcol) ? z[i] : 0;
                                else
                                        v = z[(e + 1) * znn + i] & z[i];
                                zbild[(e + 1) * ZH * 4 + r * 4 + i] = v;
                        }
                }
        }
        zsh = -1;
        return 0;
}

static int zlage(x, sy, l)      /* Lage: l[0] erste Zeile, l[1] Ende,
                                   l[2] erste Spalte, l[3] Spalten */
int x, sy, *l;
{
        l[0] = sy < 0 ? 0 : sy;
        l[1] = sy + zh > 400 ? 400 : sy + zh;
        l[2] = x >> 4;
        l[3] = ((x & 15) + bw[znr] + 15) >> 4;
        if (l[3] > 40 - l[2])
                l[3] = 40 - l[2];
        return 0;
}

static int zles(y0, n, c0, nc)  /* Kasten vom Schirm nach ZBUF */
int y0, n, c0, nc;
{
        static char blk[20 + 2 * ZKH];
        register int r;
        int e, c;
        UW a;
        char *p, *d;

        qflush();
        for (e = 0; e < qneb; e++)
                for (c = 0; c < nc; c++) {
                        a = M16(qebad[e] + (UW)y0 * 40 + c0 + c);
                        p = blk;
                        *p++ = 0; *p++ = 2 * n;
                        *p++ = 4; *p++ = 0x49; *p++ = a & 0xFF;
                        *p++ = (a >> 8) & 0xFF; *p++ = 0;
                        *p++ = 3; *p++ = 0x4A; *p++ = 0xFF; *p++ = 0xFF;
                        /* nach unten, DC wie gbget; GD: Grafikzeilen */
                        *p++ = 4; *p++ = 0x4C; *p++ = 0x00; *p++ = n;
                        *p++ = 0x40;
                        *p++ = 1; *p++ = 0xA0;
                        *p++ = 0;
                        gnull(p, 2 * n);
                        ghrdat(blk);
                        d = ZBUF(e) + 2 * c;
                        for (r = 0; r < n; r++, d += ZKB, p += 2) {
                                d[0] = p[0];
                                d[1] = p[1];
                        }
                }
        return 0;
}

/* Rechteck tauschen: k Kasten (Zeile, Spalte, Abstand ZKB), s Sicherung
   (Abstand 6); Zeilen ab y, Spalten ab c, je n und nc; nach = 1:
   Kasten -> Sicherung, 0: Sicherung -> Kasten */
static int zkopi(ky, kc, sy, n, c, nc, nach)
int ky, kc, sy, n, c, nc, nach;
{
        register char *k, *s;
        register int j;
        int e, r, m;

        m = 2 * nc;
        if (n <= 0 || m <= 0)
                return 0;
        for (e = 0; e < qneb; e++) {
                k = ZBUF(e) + (sy - ky) * ZKB + 2 * (c - kc);
                s = &zsich[e][(sy - zsy) * 6 + 2 * (c - zc0)];
                if (nach)
                        for (r = n; r > 0; r--, k += ZKB - m, s += 6 - m)
                                for (j = m; j > 0; j--)
                                        *s++ = *k++;
                else
                        for (r = n; r > 0; r--, k += ZKB - m, s += 6 - m)
                                for (j = m; j > 0; j--)
                                        *k++ = *s++;
        }
        return 0;
}

static int zweg()               /* Hintergrund zurueck (in die Liste) */
{
        qzeig = 0;
        if (zr1 > zr0 && znc > 0)
                qblk(qneb, zr0, zc0, znc, zr1 - zr0,
                     &zsich[0][(zr0 - zsy) * 6], 6, ZH * 6);
        return 0;
}

/* Zeiger an x, sy setzen; alt = 1: steht noch am alten Platz */
static int zhin(x, sy, alt)
int x, sy, alt;
{
        register int j, v;
        int l[4], y0, y1, c0, c1, e, r, n, nc, sh, k;
        char *d, *m, *p;

        qzeig = 0;
        zlage(x, sy, l);
        if (l[1] <= l[0] || l[3] <= 0) {        /* nicht zu sehen */
                if (alt)
                        zweg();
                zx = x; zsy = sy; zr0 = l[0]; zr1 = l[0]; znc = 0;
                qflush();
                qzeig = 1;
                return 0;
        }
        y0 = l[0]; y1 = l[1]; c0 = l[2]; c1 = l[2] + l[3];
        if (alt && znc > 0 && zr1 > zr0) {
                if (zr0 < y0) y0 = zr0;
                if (zr1 > y1) y1 = zr1;
                if (zc0 < c0) c0 = zc0;
                if (zc0 + znc > c1) c1 = zc0 + znc;
                if (c1 - c0 > 6 || y1 - y0 > ZKH) {     /* zu weit: getrennt */
                        zweg();
                        alt = 0;
                        y0 = l[0]; y1 = l[1]; c0 = l[2]; c1 = l[2] + l[3];
                }
        } else
                alt = 0;
        n = y1 - y0;
        nc = c1 - c0;
        zles(y0, n, c0, nc);
        if (alt)                        /* dort liegt noch der alte Zeiger */
                zkopi(y0, c0, zr0, zr1 - zr0, zc0, znc, 0);
        zx = x; zsy = sy; zr0 = l[0]; zr1 = l[1]; zc0 = l[2]; znc = l[3];
        zkopi(y0, c0, zr0, zr1 - zr0, zc0, znc, 1);
        sh = x & 15;
        if (sh != zsh)
                zvers(sh);
        /* Zeiger darueber */
        k = 2 * znc;
        for (e = 0; e < qneb; e++) {
                d = ZBUF(e) + (zr0 - y0) * ZKB + 2 * (zc0 - c0);
                m = &zpm[(zr0 - sy) * 6];
                p = &zpe[e][(zr0 - sy) * 6];
                for (r = zr1 - zr0; r > 0; r--) {
                        for (j = k; j > 0; j--) {
                                v = *d & ~*m++;
                                *d++ = v | *p++;
                        }
                        d += ZKB - k;
                        m += 6 - k;
                        p += 6 - k;
                }
        }
        qblk(qneb, y0, c0, nc, n, ZBUF(0), ZKB, ZKH * ZKB);
        qflush();
        qzeig = 1;
        return 0;
}

/* Von qplatz gerufen, wenn der Zeiger steht und anderes gezeichnet wird */
int qzhid()
{
        zweg();
        zaus = 1;
        return 0;
}

/* Von mpos, gkey, gwait: weggenommenen Zeiger wieder zeigen */
int qzwied()
{
        if (zaus && znr >= 0 && qan) {
                zaus = 0;
                zhin(zx, zsy, 0);
        }
        return 0;
}

/* Zeiger: Bild nr (hoechstens 32 x 32 Punkte) an x, y; nr < 0 nimmt ihn
   weg. Lage wie bei glput, durchsichtig wie GB_DURCH. */
int gzeig(nr, x, y)
int nr, x, y;
{
        int sy;

        if (nr < 0) {
                if (qzeig)
                        zweg();
                qflush();
                zaus = 0;
                znr = -1;
                return 0;
        }
        if (!bnrok(nr))
                return GE_NR;
        if (!bnp[nr])
                return GE_LEER;
        if (bw[nr] > 32 || bh[nr] > ZH)
                return GE_FORM;
        if (qstart() || !qan)
                return GE_HW;
        if (x < 0) x = 0;
        if (x > 639) x = 639;
        sy = qsy(y);
        if (!qorig)
                sy -= bh[nr] - 1;
        if (nr != znr) {
                if (qzeig)
                        zweg();
                zaus = 0;
                zlad(nr);
        }
        if (x == zx && sy == zsy && (qzeig || zaus)) {
                qzwied();
                return 0;
        }
        zaus = 0;
        zhin(x, sy, qzeig);
        return 0;
}

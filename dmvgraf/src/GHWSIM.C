/* GHWSIM.C - DMVGRAF-Hardwareschicht fuer die Simulation auf dem PC.
 *
 *   gcc -DHOST -std=gnu89 ... programm.c GRAF.C GBILD.C GEIN.C GHWSIM.C
 *
 * Fuehrt die GDC-Listen so aus, wie der uPD7220 der DMV im Mischbetrieb
 * es tut (nach MAMEs upd7220.cpp: CURS, MASK, PRAM, ZOOM, FIGS, FIGD fuer
 * Punkt, Linie, Bogen und Rechteck, GCHRD, WDAT, RDAT), und schreibt auf
 * Befehl Bildschirmabzuege als PPM. Eingaben aus einer Skriptdatei:
 *
 *   maus x y knopf     Mausstand (Bildzeilen von oben, wie der Schirm)
 *   taste c            Taste (ein Zeichen oder 0xNN)
 *   bild datei.ppm     Bildschirmabzug
 *
 * Umgebung: GSKRIPT = Skriptdatei, GMONO = 1 monochrom, GMAUS = 0 ohne
 * Maus, GPROT = 1 meldet jeden GDC-Befehl.
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GINTERN.H"

static unsigned short vram[0x10000];  /* mono: nur A0-A13 (wie MAME) */
static unsigned long ead;
static unsigned mask = 1, pattern = 0xFFFF;
static int modus, gchr, prot;
static int fdir, ftyp, fdc, fgd, fd = 8, fd1 = 0xFFFF, fd2 = 8, fdm = 0xFFFF;
static unsigned char ra[16];
static int farbe = 1, maus = 1, graf;
static FILE *skript, *datei;
static int sx = 320, sy = 200, sb, taste;
static long nbytes, nlisten, nbefehl;
static unsigned long summe;     /* Pruefsumme aller Listenbytes (GSUMME=1) */

static const int xdir[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

int ghinit()
{
        char *s;

        s = getenv("GMONO");
        farbe = !(s && *s == '1');
        s = getenv("GMAUS");
        maus = !(s && *s == '0');
        s = getenv("GPROT");
        prot = s && *s == '1';
        s = getenv("GSKRIPT");
        skript = s ? fopen(s, "r") : 0;
        return (farbe ? GH_FARBE : 0) | (maus ? GH_MAUS : 0);
}

int ghgraf(an)
int an;
{
        memset(vram, 0, sizeof(vram));
        mask = 0xFFFF;
        gchr = 0;
        graf = an;
        if (!an) {
                fprintf(stderr, "GDC: %ld Listen, %ld Befehle, %ld Bytes\n",
                        nlisten, nbefehl, nbytes);
                if (getenv("GSUMME"))
                        fprintf(stderr, "Summe %08lX\n", summe & 0xFFFFFFFFUL);
        }
        return 0;
}

/* ---- uPD7220 ---------------------------------------------------- */

static int sext14(int v)
{
        v &= 0x3FFF;
        return (v & 0x2000) ? v - 0x4000 : v;
}

static int pitch(void)
{
        return 80 >> (fgd ? 1 : 0);
}

static void figreset(void)
{
        fdc = 0; fd = 8; fd1 = 0xFFFF; fd2 = 8; fdm = 0xFFFF;
        fgd = 0; ftyp = 0;
        pattern = ra[8] | (ra[9] << 8);
}

static unsigned musterp(int c)
{
        if (fgd)
                return ((pattern >> (c & 15)) & 1) ? 0xFFFF : 0;
        return pattern;
}

static void schreib(unsigned data, unsigned m)
{
        unsigned short *p = &vram[ead & (farbe ? 0xFFFF : 0x3FFF)];
        unsigned cur = *p;

        switch (modus & 3) {
        case 0: cur = (cur & ~m) | (data & m); break;
        case 1: cur ^= data & m; break;
        case 2: cur &= ~(data & m); break;
        case 3: cur |= data & m; break;
        }
        *p = cur;
}

static unsigned rotl(unsigned v) { return ((v << 1) | (v >> 15)) & 0xFFFF; }
static unsigned rotr(unsigned v) { return ((v >> 1) | (v << 15)) & 0xFFFF; }

static void weiter(int d)
{
        switch (d & 7) {
        case 0: ead += pitch(); break;
        case 1: ead += pitch(); if (mask & 0x8000) ead++; mask = rotl(mask); break;
        case 2: if (mask & 0x8000) ead++; mask = rotl(mask); break;
        case 3: ead -= pitch(); if (mask & 0x8000) ead++; mask = rotl(mask); break;
        case 4: ead -= pitch(); break;
        case 5: ead -= pitch(); if (mask & 1) ead--; mask = rotr(mask); break;
        case 6: if (mask & 1) ead--; mask = rotr(mask); break;
        case 7: ead += pitch(); if (mask & 1) ead--; mask = rotr(mask); break;
        }
        ead &= 0x3FFFF;
}

static void punkt(void)
{
        int i;
        for (i = 0; i <= fdc; i++) {
                schreib(musterp(i), mask);
                weiter(fdir);
        }
}

static void linie(void)
{
        int d = sext14(fd), d1 = sext14(fd1), d2 = sext14(fd2);
        int okt = fdir, i;

        for (i = 0; i <= fdc; i++) {
                schreib(musterp(i & 15), mask);
                if (okt & 1)
                        fdir = d < 0 ? (okt + 1) & 7 : okt;
                else
                        fdir = d < 0 ? okt : (okt + 1) & 7;
                d += d < 0 ? d1 : d2;
                weiter(fdir);
        }
}

static void bogen(void)
{
        int err = -fd, d = fd + 1, okt = fdir, i;

        for (i = 0; i <= fdc; i++) {
                if (i >= fdm)
                        schreib(musterp(i % 15), mask);
                if (err < 0)
                        fdir = (okt & 1) ? (okt + 1) & 7 : okt;
                else
                        fdir = (okt & 1) ? okt : (okt + 1) & 7;
                err += err < 0 ? ((i + 1) << 1) : ((i - --d + 1) << 1);
                weiter(fdir);
        }
}

static void rechteck(void)
{
        int i, j, l;

        for (i = 0; i <= 3; i++) {
                l = (i & 1) ? fd2 : fd;
                for (j = 0; j < l; j++) {
                        schreib(musterp(j & 15), mask);
                        if (i > 0 && j == 0)
                                fdir = (fdir + 2) & 7;
                        weiter(fdir);
                }
        }
}

static void zeichen(void)
{
        static const int wechsel[2][4] = { { 2, 2, -2, -2 }, { 1, 3, -3, -1 } };
        int typ = (ftyp & 0x10) >> 4, i, di, zdc, j, zd;
        unsigned p;

        for (i = 0, di = 0; i < fdc + 1; i++) {
                pattern = (ra[15 - (i & 7)] << 8) | ra[15 - (i & 7)];
                for (zdc = 0; zdc <= gchr; zdc++, di++) {
                        for (j = 0; j < fd; j++) {
                                p = (di % 2) ? musterp(15 - (j & 15))
                                             : musterp(j & 15);
                                for (zd = 0; zd <= gchr; zd++) {
                                        schreib(p, mask);
                                        if (j != fd - 1 || zd != gchr)
                                                weiter(fdir);
                                }
                        }
                        fdir = (fdir + wechsel[typ][(di % 2) << 1]) & 7;
                        weiter(fdir);
                        fdir = (fdir + wechsel[typ][((di % 2) << 1) + 1]) & 7;
                }
        }
}

/* Ein Eintrag der Liste; lies != 0: RDAT-Worte dort ablegen */
static int befehl(int c, unsigned char *p, int n, unsigned char *lies,
                  int *nl)
{
        int i, k;

        nbefehl++;
        if (prot) {
                fprintf(stderr, "%02X:", c);
                for (i = 0; i < n && i < 12; i++)
                        fprintf(stderr, " %02X", p[i]);
                fprintf(stderr, n > 12 ? " ...\n" : "\n");
        }
        if (c == 0x49) {
                if (n >= 2)
                        ead = p[0] | (p[1] << 8) | (n >= 3 ? (p[2] & 3) << 16 : 0);
                if (n >= 3)
                        mask = 1u << ((p[2] >> 4) & 15);
        } else if (c == 0x4A) {
                if (n >= 2)
                        mask = p[0] | (p[1] << 8);
        } else if (c == 0x46) {
                if (n >= 1)
                        gchr = p[0] & 15;
        } else if ((c & 0xF0) == 0x70) {
                k = c & 15;
                for (i = 0; i < n && k < 16; i++, k++) {
                        ra[k] = p[i];
                        if (k == 8) pattern = (pattern & 0xFF00) | p[i];
                        if (k == 9) pattern = (pattern & 0x00FF) | (p[i] << 8);
                }
        } else if (c == 0x4C) {
                if (n >= 1) { fdir = p[0] & 7; ftyp = (p[0] & 0xF8) >> 3; }
                if (n >= 2) fdc = p[1] | (fdc & 0x3F00);
                if (n >= 3) { fdc = p[1] | ((p[2] & 0x3F) << 8); fgd = (p[2] & 0x40) != 0; }
                if (n >= 5) fd = p[3] | ((p[4] & 0x3F) << 8);
                if (n >= 7) fd2 = p[5] | ((p[6] & 0x3F) << 8);
                if (n >= 9) fd1 = p[7] | ((p[8] & 0x3F) << 8);
                if (n >= 11) fdm = p[9] | ((p[10] & 0x3F) << 8);
        } else if (c == 0x6C) {
                switch (ftyp) {
                case 0: punkt(); break;
                case 1: linie(); break;
                case 4: bogen(); break;
                case 8: rechteck(); break;
                default: fprintf(stderr, "FIGD Art %d nicht simuliert\n", ftyp);
                }
                figreset();
        } else if (c == 0x68) {
                if ((ftyp & 15) == 2)
                        zeichen();
                else
                        fprintf(stderr, "GCHRD Art %d nicht simuliert\n", ftyp);
                figreset();
        } else if ((c & 0xE4) == 0x20) {
                modus = c & 3;
                if (((c >> 3) & 3) != 0 && n)
                        fprintf(stderr, "WDAT Byteart nicht simuliert\n");
                for (i = 0; i + 1 < n; i += 2) {
                        pattern = p[i] | (p[i + 1] << 8);
                        if (ftyp)
                                continue;
                        /* das Wort DC+1-mal, danach DC = 0 (MAME wdat) */
                        for (k = 0; k <= fdc; k++) {
                                schreib(pattern & mask, mask);
                                ead += xdir[fdir] + (fdir == 0 || fdir == 1
                                        || fdir == 7 ? pitch() : 0)
                                        - (fdir >= 3 && fdir <= 5 ? pitch() : 0);
                                ead &= 0x3FFFF;
                        }
                        figreset();
                }
        } else if (c == 0xE0) {
                /* CURD: EAD in drei Bytes (wie MAME; der echte 7220
                   schickt danach noch MASK) */
                if (lies) {
                        lies[(*nl)++] = ead & 0xFF;
                        lies[(*nl)++] = (ead >> 8) & 0xFF;
                        lies[(*nl)++] = (ead >> 16) & 0x03;
                }
        } else if ((c & 0xE4) == 0xA0) {
                /* RDAT: wie MAME genau DC Worte (der echte 7220: DC+1) */
                for (k = 0; k < fdc && lies; k++) {
                        unsigned w = vram[ead & (farbe ? 0xFFFF : 0x3FFF)];
                        lies[(*nl)++] = w & 0xFF;
                        lies[(*nl)++] = (w >> 8) & 0xFF;
                        ead += xdir[fdir] + (fdir == 0 || fdir == 1
                                || fdir == 7 ? pitch() : 0)
                                - (fdir >= 3 && fdir <= 5 ? pitch() : 0);
                        ead &= 0x3FFFF;
                }
                figreset();
        } else
                fprintf(stderr, "GDC-Befehl %02X nicht simuliert\n", c);
        return 0;
}

static unsigned char *liste(unsigned char *p, unsigned char *lies, int *nl)
{
        int n;

        nlisten++;
        {
                unsigned char *q = p;
                while (*q) {
                        int i, m = *q + 1;
                        for (i = 0; i < m; i++)
                                summe = (summe * 31 + q[i]) & 0xFFFFFFFFUL;
                        q += m;
                }
        }
        while ((n = *p) != 0) {
                befehl(p[1], p + 2, n - 1, lies, nl);
                nbytes += n + 1;
                p += n + 1;
        }
        nbytes++;
        return p + 1;
}

int ghlist(l)
char *l;
{
        if (graf)
                liste((unsigned char *)l, 0, 0);
        return 0;
}

int ghdset(an) int an; { return -1; }

int ghvzahl()
{
        int n = 0;

        while ((ghstat() & 0x20) && n < 20000)
                n++;
        while (!(ghstat() & 0x20) && n < 30000)
                n++;
        return n;
}

int ghrdat(b)
char *b;
{
        static unsigned char puf[512];
        unsigned char *p = (unsigned char *)b;
        int m = (p[0] << 8) | p[1], nl = 0;

        if (!graf)
                return 0xFFFF;
        p = liste(p + 2, puf, &nl);
        memcpy(p, puf, m);
        return nl < m;
}

int ghvsyn() { return 0; }

/* Status: VSYNC (Bit 5) jeweils 7 Abfragen lang, fest wiederholbar */
int ghstat() { static int n; n++; return (n / 7) & 1 ? 0x20 : 0; }

/* ---- Bildschirmabzug, Eingaben -------------------------------------- */

static void abzug(char *name)
{
        FILE *f = fopen(name, "wb");
        int x, y;

        if (!f)
                return;
        fprintf(f, "P6 640 400 255\n");
        for (y = 0; y < 400; y++)
                for (x = 0; x < 640; x++) {
                        unsigned a = y * 40 + x / 16, b = x & 15;
                        int g = (vram[a] >> b) & 1;
                        int r = (vram[a + 0x4000] >> b) & 1;
                        int bl = (vram[a + 0x8000] >> b) & 1;
                        unsigned char px[3];
                        if (farbe) {
                                px[0] = r ? 255 : 0;
                                px[1] = g ? 255 : 0;
                                px[2] = bl ? 255 : 0;
                        } else {
                                px[0] = g ? 40 : 0;
                                px[1] = g ? 255 : 0;
                                px[2] = g ? 90 : 0;
                        }
                        fwrite(px, 1, 3, f);
                }
        fclose(f);
}

static void zeile(void)
{
        char b[200], w[100];
        int x, y, k;

        taste = 0;
        if (!skript || !fgets(b, sizeof(b), skript)) {
                taste = 0x1B;
                return;
        }
        if (sscanf(b, "maus %d %d %d", &x, &y, &k) == 3) {
                sx = x; sy = y; sb = k;
        } else if (sscanf(b, "taste %99s", w) == 1) {
                taste = (w[0] == '0' && w[1] == 'x') ? (int)strtol(w, 0, 16)
                                                      : w[0];
        } else if (sscanf(b, "bild %99s", w) == 1)
                abzug(w);
}

int ghtast()
{
        zeile();
        return taste;
}

int ghmaus(c, b)
int c;
char *b;
{
        if (c == 0) {
                int y = 399 - sy;
                b[2] = sx & 0xFF; b[3] = sx >> 8;
                b[4] = y & 0xFF;  b[5] = y >> 8;
                b[6] = sb;
        } else if (c == 4) {
                sx = (b[1] & 0xFF) | (b[2] << 8);
                sy = 399 - ((b[3] & 0xFF) | (b[4] << 8));
        }
        return 0;
}

int ghton(h, l) int h, l; { return 0; }

int ghopen(name)
char *name;
{
        datei = fopen(name, "rb");
        return datei == 0;
}

int ghread(b)
char *b;
{
        memset(b, 0x1A, 128);
        return fread(b, 1, 128, datei) > 0;
}

int ghclos() { fclose(datei); return 0; }

int ghtext(s) char *s; { fputs(s, stderr); return 0; }

/* ---- Bildspeicher: drei Bloecke zu 64 KB wie Segment 4-6 der K208 ---- */

static char spei[3][65536];

int ghsnum() { return 3; }
long ghsgro(b) int b; { return 65536L; }

int ghsput(b, o, s, n)
int b, n;
UW o;
char *s;
{
        if ((long)o + n > 65536L) { fprintf(stderr, "ghsput ueber Block\n"); exit(1); }
        memcpy(&spei[b][o], s, n);
        return 0;
}

int ghsget(d, b, o, n)
char *d;
int b, n;
UW o;
{
        if ((long)o + n > 65536L) { fprintf(stderr, "ghsget ueber Block\n"); exit(1); }
        memcpy(d, &spei[b][o], n);
        return 0;
}

int ghsmov(b, z, q, n)
int b;
UW z, q, n;
{
        memmove(&spei[b][z], &spei[b][q], n);
        return 0;
}

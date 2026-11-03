/* GPIC.C - gepackte Vollbilder (.PIC mit Kennung DMVP, LZSS) direkt in
   den Bildspeicher: gpic(datei, xw, y, art), gpicm(daten, laenge, xw, y,
   art). Das Dateiformat steht in README.md ("Gepackte Vollbilder").

   MS-DOS: der Kern aus src/GPIC.INC (8086, schreibt selbst an den 7220,
   liest mit INT 21h), hier nur der Aufruf. Sonst (CP/M-68K, CP/M-8000,
   PC-Simulation) dieselbe Arbeit in C ueber die GDC-Liste; das Ergebnis
   im Bildspeicher ist dasselbe.

   art: Bit 0 Anzeige waehrend des Schreibens dunkel, Bit 1 Mono
   erzwingen, Bit 2 Farbe erzwingen (ohne beides: wie die Karte).
   Rueck: 0, GE_DATEI, GE_KURZ (zu kurz, Ebenenlaenge stimmt nicht),
   GE_FORM (Kennung, Version, Groesse), GE_LAGE (ragt ueber den Schirm),
   GE_VERW (Verweis vor den Anfang der Ebene).

   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "GINTERN.H"
#include "DMVPIC.H"

extern int qzeig, qzhid();

static int gpart(art)
int art;
{
        if (!(art & 6))
                art |= qneb == 1 ? 2 : 4;
        return art & 7;
}

#ifdef __ia16__
/* ------------------------------------------------------------------ */
/* MS-DOS: Kern in Maschinensprache (GPIC86.S, aus GPIC.INC)            */

extern int gpicw();     /* (quelle, xw, y, art, laenge, arbeit) */
static char gpw[5312];  /* GP_GROESSE: Kopf, Lesepuffer 1 KB, Fenster 4 KB,
                           Zeilenpuffer */

/* Dunkelschalten hier ueber die GDC-Liste, nicht im Kern: so geht es in
   MAME auch aus einem DMVGRAF-Programm (im Kern blieb die Anzeige dort
   dunkel; GPIC.INC allein aus MASM ist nicht betroffen) */
static int gpaus(q, n, xw, y, art)
char *q;
unsigned n;
int xw, y, art;
{
        int r;

        if (qstart())
                return GE_HW;
        if (qzeig)
                qzhid();
        if (art & 1)
                qcmd(0x0C);
        qflush();
        r = gpicw(q, xw, y, art & ~1, n, gpw);
        if (art & 1) {
                qcmd(0x6B);
                qcmd(0x0D);
                qflush();
        }
        return r;
}

int gpic(name, xw, y, art)
char *name;
int xw, y, art;
{
        return gpaus(name, 0, xw, y, gpart(art));
}

int gpicm(p, n, xw, y, art)
char *p;
unsigned n;
int xw, y, art;
{
        return gpaus(p, n, xw, y, gpart(art) | 0x80);
}

#else
/* ------------------------------------------------------------------ */
/* C-Fassung: Zeile fuer Zeile in einen Puffer, dann ueber die Liste    */

static char *pm;                /* Quelle im Speicher, sonst 0 */
static unsigned pn;             /* Speicher: noch da */
static char prb[128];           /* Datei: Satz */
static int prp, pre;            /* Stelle im Satz, Satz gueltig bis */
static unsigned ppk;            /* Byte der Ebene, die noch kommen */
static char pwin[4096];
static char pzl[80];

static int pbyte()              /* naechstes Byte der Ebene, -1 Ende */
{
        if (!ppk)
                return -1;
        ppk--;
        if (pm) {
                if (!pn)
                        return -1;
                pn--;
                return *pm++ & 0xFF;
        }
        if (prp >= pre) {
                if (!ghread(prb))
                        return -1;
                prp = 0;
                pre = 128;
        }
        return prb[prp++] & 0xFF;
}

/* Kopf: 12 Byte, Tabelle 4 * n */
static char pk[12 + 16];

static int phol(n)              /* n Byte Kopf nach pk */
int n;
{
        int i, c;

        ppk = n;
        for (i = 0; i < n; i++) {
                if ((c = pbyte()) < 0)
                        return GE_KURZ;
                pk[i] = c;
        }
        return 0;
}

static int pauf(name)           /* Quelle (wieder) an den Anfang */
char *name;
{
        if (pm)
                return 0;
        ghclos();
        prp = pre = 0;
        return ghopen(name) ? GE_DATEI : 0;
}

/* eine Zeile (2 * nw Byte aus pzl) auf die Ebenen in ziel */
static int pzeile(ziel, adr, nw)
int ziel, nw;
UW adr;
{
        int e, i;

        for (e = 0; e < 3; e++)
                if (ziel & (1 << e)) {
                        qplatz(2 * nw + 30);
                        qcmd(0x49);             /* CURS, MASK, FIGS, WDAT */
                        qwrd(M16(qebad[e] + adr));
                        qcmd(0x4A);
                        qpar(0xFF);
                        qpar(0xFF);
                        qcmd(0x4C);
                        qpar(2);
                        qpar(0);
                        qpar(0);
                        qcmd(0x20);
                        for (i = 0; i < 2 * nw; i++)
                                qpar(pzl[i] & 0xFF);
                }
        return 0;
}

/* eine Ebene: Verfahren v (0 roh, 1 LZSS, 2 Nullen), gepackte Laenge l */
static int pebene(v, l, ziel, xw, y, nw, h)
int v, ziel, xw, y, nw, h;
unsigned l;
{
        unsigned tot, aus, d, k;
        int c, f, fn, z, n, w;
        UW adr;

        tot = 2 * nw * h;
        aus = 0;
        ppk = v == 2 ? 0 : l;
        adr = (UW)(y * 40 + xw);
        z = 0;
        f = fn = 0;
        while (aus < tot) {
                if (v == 2)
                        c = 0, n = 1, d = 0;
                else if (v == 0) {
                        if ((c = pbyte()) < 0)
                                return GE_KURZ;
                        n = 1, d = 0;
                } else {
                        if (!fn) {
                                if ((f = pbyte()) < 0)
                                        return GE_KURZ;
                                fn = 8;
                        }
                        fn--;
                        if (!(f & 1)) {
                                if ((c = pbyte()) < 0)
                                        return GE_KURZ;
                                n = 1, d = 0;
                        } else {
                                if ((c = pbyte()) < 0 || (w = pbyte()) < 0)
                                        return GE_KURZ;
                                d = c | (w & 0xF0) << 4;
                                n = (w & 15) + 3;
                                if (d == 0 || d > aus)
                                        return GE_VERW;
                        }
                        f >>= 1;
                }
                for (; n > 0 && aus < tot; n--) {
                        if (d)
                                c = pwin[(aus - d) & 0xFFF];
                        pwin[aus & 0xFFF] = c;
                        aus++;
                        pzl[z++] = c;
                        if (z == 2 * nw) {
                                pzeile(ziel, adr, nw);
                                adr += 40;
                                z = 0;
                        }
                }
        }
        if (ppk)
                return GE_KURZ;
        return 0;
}

static unsigned plen(e)         /* gepackte Laenge eines Eintrags */
char *e;
{
        return (e[2] & 0xFF) | (e[3] & 0xFF) << 8;
}

static int pbild(name, xw, y, art)
char *name;
int xw, y, art;
{
        int i, n, nw, h, hat, ges, r, z, mono;
        char *t;
        long gr;

        mono = (art & 2) != 0;
        if ((r = pauf(name)))
                return r;
        if (phol(12))
                return GE_KURZ;
        if (pk[0] == 'D' && pk[1] == 'M' && pk[2] == 'V' && pk[3] == 'P') {
                n = pk[5] & 0xFF;
                nw = (pk[6] & 0xFF) | (pk[7] & 0xFF) << 8;
                h = (pk[8] & 0xFF) | (pk[9] & 0xFF) << 8;
                if (pk[4] != 1 || n < 1 || n > 4 || nw < 1 || nw > 40 ||
                    h < 1 || h > 400)
                        return GE_FORM;
                if (phol(4 * n))
                        return GE_KURZ;
                for (i = 4 * n - 1; i >= 0; i--)
                        pk[12 + i] = pk[i];
        } else {                        /* alte Rohdatei: Laenge zaehlen */
                if (pm)
                        gr = pn + 12L;
                else {
                        ghclos();
                        if (ghopen(name))
                                return GE_DATEI;
                        for (gr = 0; ghread(prb); gr += 128)
                                ;
                }
                if (gr != 128000L && gr != 32000L)
                        return GE_FORM;
                if (pm) {
                        pm -= 12;
                        pn += 12;
                } else if ((r = pauf(name)))
                        return r;
                n = gr == 32000L ? 1 : 4;
                nw = 40;
                h = 400;
                for (i = 0; i < n; i++) {
                        pk[12 + 4 * i] = n == 1 ? 'M' : "GRBM"[i];
                        pk[13 + 4 * i] = 0;
                        pk[14 + 4 * i] = 32000 & 0xFF;
                        pk[15 + 4 * i] = 32000 >> 8;
                }
        }
        if (xw < 0 || y < 0 || xw + nw > 40 || y + h > 400)
                return GE_LAGE;
        t = &pk[12];
        hat = 0;
        for (i = 0; i < n; i++) {
                switch (t[4 * i]) {
                case 'G': hat |= 1; break;
                case 'R': hat |= 2; break;
                case 'B': hat |= 4; break;
                case 'M': hat |= 8; break;
                default: return GE_FORM;
                }
                if ((t[4 * i + 1] & 0xFF) > 1)
                        return GE_FORM;
        }
        ges = 0;
        for (i = 0; i < n; i++) {
                switch (t[4 * i]) {
                case 'G': z = mono ? !(hat & 8) : 1; break;
                case 'R': z = mono ? 0 : 2; break;
                case 'B': z = mono ? 0 : 4; break;
                default:  z = mono ? 1 : (hat & 7) ? 0 : 7;
                }
                if (z) {
                        r = pebene(t[4 * i + 1] & 0xFF, plen(t + 4 * i), z, xw,
                                   y, nw, h);
                        if (r)
                                return r;
                } else {                /* ueberspringen */
                        ppk = plen(t + 4 * i);
                        while (ppk)
                                if (pbyte() < 0)
                                        return GE_KURZ;
                }
                ges |= z;
        }
        if (!mono && (hat & 7))
                for (i = 0; i < 3; i++)
                        if (!(ges & 1 << i))
                                pebene(2, 0, 1 << i, xw, y, nw, h);
        return 0;
}

static int ppic(name, xw, y, art)
char *name;
int xw, y, art;
{
        int r;

        if (qstart())
                return GE_HW;
        art = gpart(art);
        if (art & 1) {
                qcmd(0x0C);
                qflush();
        }
        r = pbild(name, xw, y, art);
        if (!pm)
                ghclos();
        if (art & 1) {
                qcmd(0x6B);             /* START und Anzeige an */
                qcmd(0x0D);
        }
        qflush();
        return r;
}

int gpic(name, xw, y, art)
char *name;
int xw, y, art;
{
        pm = 0;
        prp = pre = 0;
        if (qstart())
                return GE_HW;
        if (ghopen(name))
                return GE_DATEI;
        return ppic(name, xw, y, art);
}

int gpicm(p, n, xw, y, art)
char *p;
unsigned n;
int xw, y, art;
{
        pm = p;
        pn = n;
        return ppic((char *)0, xw, y, art);
}
#endif

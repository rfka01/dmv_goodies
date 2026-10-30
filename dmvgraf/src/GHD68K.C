/* GHD68K.C - DMVGRAF fuer CP/M-68K (K234): 7220 direkt, und ohne
 * DMV-BIOS-Block (NCRs CP/M-68K 1.2) auch Grafikbetrieb, Farberkennung,
 * Maus K806 und Ton ueber den 8741.
 *
 * Die Anschluesse liegen beim 68008 auf FFF00h + Port; gesprochen wird
 * mit ihnen im Supervisorzustand (GSUP68.S, BDOS 62). Was hier an die
 * Hardware geht, macht unter CP/M-8000 der BIOS (docs/DMV-
 * Schnittstellen.md im Repo rfka01/dmv_os, Abschnitt 9: 65-69); die
 * Ablaeufe sind dieselben wie in dessen biosio.8kn (GFXON, TXTON,
 * MSPROBE, F68, F69).
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

extern long __BDOS();
extern long gsup();
extern int gusr();

#define IO(p)   (*(volatile unsigned char *)(0xFFF00L + (p)))
#define GDCST   0xA0            /* lesen Status, schreiben Parameter */
#define GDCCM   0xA1            /* lesen Daten, schreiben Befehl */
#define GDCZM   0xA2            /* Parameter und Zoom-Klinke */
#define KBDDAT  0x40
#define KBDCTL  0x41
#define ZEIT    200000L         /* Wartegrenze in Abfragen */

static int k806;                /* Anschluss der K806, 0 = keine */
static int farbe;

int ghdini()
{
        return 1;
}

/* Darf BIOS 64 gerufen werden? NCRs CP/M-68K 1.2 springt bei einer
   unbekannten BIOS-Funktion irgendwohin (im Versuch: es erscheint "TNG"
   oder das Programm haengt). Ein BIOS mit DMV-Block traegt darum die
   Kennung "DMVB" in den vier Bytes unmittelbar vor seinem TRAP-3-
   Einsprung (Vektor 35, 8Ch); nur dann fragt DMVGRAF Fn 64. */
int ghdbok()
{
        long s;
        unsigned char *p;
        int ok;

        s = gsup();
        p = (unsigned char *)(*(volatile long *)0x8CL & 0xFFFFFL);
        ok = (*(long *)0x8CL & 1) == 0 && p >= (unsigned char *)0x404L
             && p[-4] == 'D' && p[-3] == 'M' && p[-2] == 'V' && p[-1] == 'B';
        gusr(s);
        return ok;
}

static int gw()                 /* bis das FIFO nicht voll ist */
{
        long n;

        for (n = 0; n < ZEIT; n++)
                if (!(IO(GDCST) & 2))
                        return 0;
        return 1;
}

static int gcmd(c)
int c;
{
        gw();
        IO(GDCCM) = c;
        return 0;
}

static int gpar(c)
int c;
{
        gw();
        IO(GDCST) = c;
        return 0;
}

extern int gdlst();             /* GK68.S: Liste wie BIOS 66 ausgeben */

int ghdlst(l)
char *l;
{
        long s;

        s = gsup();
        gdlst(l);
        gusr(s);
        return 0;
}

static int glies(m, d)          /* m Bytes aus dem FIFO -> 0, 1 Zeit */
int m;
char *d;
{
        long n;

        while (m-- > 0) {
                for (n = 0; n < ZEIT; n++)
                        if (IO(GDCST) & 1)
                                break;
                if (n == ZEIT)
                        return 1;
                *d++ = IO(GDCCM);
        }
        return 0;
}

int ghdrd(b)                    /* Block wie BIOS 67/2 */
char *b;
{
        int m, r;
        char *p;
        long s;

        m = ((b[0] & 0xFF) << 8) | (b[1] & 0xFF);
        p = b + 2;
        while (*p)
                p += (*p & 0xFF) + 1;
        s = gsup();
        gdlst(b + 2);
        r = glies(m, p + 1);
        gusr(s);
        return r;
}

int ghdst()
{
        long s;
        int v;

        s = gsup();
        v = IO(GDCST);
        gusr(s);
        return v;
}

static int vsync()              /* Status Bit 5 von 0 auf 1 */
{
        long n;

        for (n = 0; n < ZEIT && (IO(GDCST) & 0x20); n++)
                ;
        for (n = 0; n < ZEIT; n++)
                if (IO(GDCST) & 0x20)
                        return 0;
        return 1;
}

int ghdvs()
{
        long s;

        s = gsup();
        vsync();
        gusr(s);
        return 0;
}

int ghdvz()                     /* Abfragen bis zum naechsten Bildwechsel */
{
        long s;
        int n;

        s = gsup();
        n = 0;
        while ((IO(GDCST) & 0x20) && n < 20000)
                n++;
        while (!(IO(GDCST) & 0x20) && n < 30000)
                n++;
        gusr(s);
        return n;
}

/* ---- Erkennung ohne BIOS-Block ---- */

static UW wlies(a)              /* ein Wort aus dem Bildspeicher */
UW a;
{
        static char b[2];

        gcmd(0x49); gpar(a & 0xFF); gpar((a >> 8) & 0xFF); gpar(0);
        gcmd(0x4A); gpar(0xFF); gpar(0xFF);
        gcmd(0x4C); gpar(0x02); gpar(1); gpar(0);
        gcmd(0xA0);
        b[0] = b[1] = 0;
        glies(2, b);
        return (UW)((b[0] & 0xFF) | ((b[1] & 0xFF) << 8));
}

static int wschr(a, w)          /* ein Wort schreiben */
UW a, w;
{
        gcmd(0x49); gpar(a & 0xFF); gpar((a >> 8) & 0xFF); gpar(0);
        gcmd(0x4A); gpar(0xFF); gpar(0xFF);
        gcmd(0x4C); gpar(0x02); gpar(0); gpar(0);
        gcmd(0x20); gpar(w & 0xFF); gpar((w >> 8) & 0xFF);
        return 0;
}

/* Farbe: die Monochromkarte dekodiert nur A0-A13 und findet ihre gruene
   Ebene bei 4000h und 8000h gespiegelt (MAME dmv.cpp, an echter
   Hardware bestaetigt). Geprueft wird hinter dem Textbereich. */
static int farbtest()
{
        UW v;

        wschr((UW)0x3FF0, (UW)0x1234);
        wschr((UW)0x7FF0, (UW)0xA55A);
        v = wlies((UW)0x3FF0);
        wschr((UW)0x3FF0, (UW)0);
        wschr((UW)0x7FF0, (UW)0);
        return v == 0x1234;
}

/* K806 auf 2A (30h-33h), wie MSPROBE im BIOS von CP/M-8000 */
static int maustest()
{
        int v, i;

        v = 0xFF;
        for (i = 0; i < 4; i++)
                v &= IO(0x30 + i);
        if (v == 0xFF)
                return 0;
        if (!(IO(0x33) & 0x80)) {
                IO(0x36) = 0x4E;
                IO(0x36) = 0x7E;
                v = IO(0x32);
                v = IO(0x32);
                if (v == 0x7E || v == 0x3E)     /* 2661 (K801), 2651 */
                        return 0;
        }
        for (i = 0; i < 64 && (IO(0x31) & 1); i++)
                v = IO(0x30);
        return 0x30;
}

int ghdhw()
{
        static char ead[4];
        long s;
        int a, n;

        s = gsup();
        /* Der Farbtest verstellt die Adresse des 7220 (EAD), auf die NCRs
           Konsole weiterschreibt (danach blieb jede Ausgabe unsichtbar):
           vorher mit CURD lesen, hinterher mit CURS zurueck, MASK FFFFh */
        for (n = 0; n < 16 && (IO(GDCST) & 1); n++)
                (void)IO(GDCCM);
        gcmd(0xE0);
        ead[0] = ead[1] = ead[2] = 0;
        glies(3, ead);
        for (n = 0; n < 16 && (IO(GDCST) & 1); n++)
                (void)IO(GDCCM);        /* der echte 7220 schickt noch MASK */
        farbe = farbtest();
        gcmd(0x49);
        gpar(ead[0] & 0xFF);
        gpar(ead[1] & 0xFF);
        gpar(ead[2] & 0x03);
        gcmd(0x4A); gpar(0xFF); gpar(0xFF);
        k806 = maustest();
        gusr(s);
        a = 0;
        if (farbe)
                a |= GH_FARBE;
        if (k806)
                a |= GH_MAUS;
        return a;
}

/* ---- Grafikbetrieb ohne BIOS-Block (GFXON und TXTON des BIOS) ---- */

static int gparz(c)             /* ZOOM-Parameter ueber A2h */
int c;
{
        gw();
        IO(GDCZM) = c;
        return 0;
}

static int grafan()
{
        int i, e;

        gcmd(0x0C);                             /* Anzeige aus */
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
        gcmd(0x6B);                             /* START */
        gcmd(0x0D);                             /* Anzeige an */
        return 0;
}

static int textan()
{
        int i;

        gcmd(0x0C);
        gcmd(0x46); gparz(0);
        gcmd(0x78);                             /* PRAM 8-15 */
        for (i = 0; i < 8; i++)
                gpar(0xFF);
        gcmd(0x47); gpar(80);
        gcmd(0x70);                             /* Text ab Zelle 0 */
        gpar(0); gpar(0); gpar(0); gpar(25);
        gpar(0); gpar(0); gpar(0); gpar(0);
        gcmd(0x49); gpar(0); gpar(0); gpar(0);
        gcmd(0x4A); gpar(0xFF); gpar(0xFF);     /* CURS setzt MASK zurueck */
        gcmd(0x4C); gpar(0x02); gpar(1999 & 0xFF); gpar(1999 >> 8);
        gcmd(0x20); gpar(0x20); gpar(0xE8);     /* Leerzeichen, normal */
        gcmd(0x4C); gpar(0x02); gpar(0); gpar(0);
        gcmd(0x49); gpar(0); gpar(0); gpar(0);
        gcmd(0x6B);
        gcmd(0x0D);
        return 0;
}

int ghdgrf(an)
int an;
{
        long s;

        s = gsup();
        if (an)
                grafan();
        else
                textan();
        gusr(s);
        if (!an)
                __BDOS(2, 0x1AL);       /* die Konsole loescht selbst */
        return 0;
}

/* ---- Maus K806 (wie BIOS 68) ---- */

static int mw(bit, soll)        /* Status-Bit abwarten -> 0 gut */
int bit, soll;
{
        long n;

        for (n = 0; n < ZEIT; n++)
                if (((IO(k806 + 1) & bit) != 0) == soll)
                        return 0;
        return 1;
}

int ghdms(c, b)
int c;
char *b;
{
        long s;
        int i, p, a, r;

        if (!k806)
                return 1;
        s = gsup();
        for (i = 0; i < 64 && (IO(k806 + 1) & 1); i++)
                (void)IO(k806);
        r = 2;
        p = *b++ & 0xFF;
        if (mw(2, 0))
                goto aus;
        IO(k806 + 1) = c;
        for (; p > 0; p--) {
                if (mw(2, 0))
                        goto aus;
                IO(k806) = *b++;
        }
        a = *b++ & 0xFF;
        for (; a > 0; a--) {
                if (mw(1, 1))
                        goto aus;
                *b++ = IO(k806);
        }
        r = 0;
aus:
        gusr(s);
        return r;
}

/* ---- Ton ueber den 8741 (wie BIOS 69) ---- */

static int kw()
{
        long n;

        for (n = 0; n < ZEIT; n++)
                if (!(IO(KBDCTL) & 2))
                        return 0;
        return 1;
}

int ghdton(h, l)
int h, l;
{
        long s;

        s = gsup();
        if (IO(KBDCTL) & 2) {
                gusr(s);
                return 1;
        }
        IO(KBDCTL) = 0x06;
        kw();
        if (h > 42)
                h = 42;
        IO(KBDDAT) = h + 0x20;
        kw();
        if (l < 1)
                l = 1;
        if (l > 224)
                l = 224;
        IO(KBDDAT) = l + 0x1F;
        gusr(s);
        return 0;
}

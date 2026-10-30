/* GHWCPM.C - DMVGRAF-Hardwareschicht fuer CP/M-8000 (K238) und CP/M-68K.
 *
 * Alles ueber BDOS: Grafik, Maus und Ton ueber den DMV-BIOS-Block
 * (BDOS 50, Funktionen 64-69, docs/DMV-Schnittstellen.md im Repo
 * rfka01/dmv_os, Abschnitt 9), Tastatur ueber BDOS 6, Dateien ueber FCB.
 * GDC-Listen gehen direkt an den 7220 (CP/M-8000: GHDIR.C, GDIR.8KN;
 * CP/M-68K: GHD68K.C, GSUP68.S; Systemmodus jeweils ueber BDOS 62),
 * sonst ueber Fn 66/67 (GHDNUL.C); gdirekt(0) erzwingt den BIOS-Weg.
 * Ohne BIOS-Block (NCRs CP/M-68K 1.2) uebernimmt GHD68K.C auch
 * Grafikbetrieb, Maus und Ton.
 * Der Bildspeicher steht in GHSEG.C (CP/M-8000, freie Segmente ueber
 * BIOS 72) oder GHPOOL.C (Feld im Programm).
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

extern long __BDOS();

static struct {
        short fn;               /* Wort, auch wo int 32 Bit hat */
        long p1;
        long p2;
} bpb;

long ghbios(fn, p1, p2)         /* BIOS-Funktion ueber BDOS 50 */
int fn;
long p1, p2;
{
        bpb.fn = fn;
        bpb.p1 = p1;
        bpb.p2 = p2;
        return __BDOS(50, (long)&bpb);
}

long ghausk;                    /* Ergebnis von Fn 64 */

/* Direkter Weg (GHDIR.C fuer CP/M-8000, GHD68K.C fuer CP/M-68K,
   GHDNUL.C ohne): dirok = 7220 direkt moeglich, diran = eingeschaltet;
   ohne BIOS-Block (NCRs CP/M-68K) macht er auch Grafikbetrieb, Status,
   Maus und Ton (blk = 0) */
static int dirok, diran, blk;
extern int ghdini(), ghdlst(), ghdrd(), ghdhw(), ghdgrf(), ghdst(),
           ghdvs(), ghdvz(), ghdms(), ghdton(), ghdbok();

int ghinit()
{
        long r;
        int a;

        /* Fn 64 nur, wo ein unbekannter BIOS-Aufruf ungefaehrlich ist
           (ghdbok): NCRs CP/M-68K 1.2 prueft die Funktionsnummer nicht */
        r = ghdbok() ? ghbios(64, 0L, 0L) : 0L;
        ghausk = r;
        blk = ((r >> 24) & 0xFF) == 'D' && (r & 8);
        dirok = ghdini();
        diran = dirok;
        if (!blk) {
                ghausk = 0;
                return dirok ? ghdhw() : -1;
        }
        a = 0;
        if (r & 1)
                a |= GH_FARBE;
        if (r & 2)
                a |= GH_MAUS;
        return a;
}

int ghgraf(an)
int an;
{
        if (!blk)
                return ghdgrf(an);
        ghbios(65, (long)(an ? 1 : 0), 0L);
        return 0;
}

int ghdset(an)                  /* -> vorher, -1 kein direkter Weg */
int an;
{
        int v;

        if (!dirok || !blk)     /* ohne Block gibt es nur den direkten */
                return -1;
        v = diran;
        diran = an ? 1 : 0;
        return v;
}

int ghlist(l)
char *l;
{
        if (diran)
                return ghdlst(l);
        ghbios(66, 0L, (long)l);
        return 0;
}

int ghrdat(b)
char *b;
{
        if (diran)
                return ghdrd(b);
        return (int)(ghbios(67, 2L, (long)b) & 0xFFFF);
}

int ghstat()                    /* GDC-Status (BIOS 67/0) */
{
        if (!blk)
                return ghdst();
        return (int)(ghbios(67, 0L, 0L) & 0xFF);
}

int ghvsyn()
{
        if (!blk)
                return ghdvs();
        ghbios(67, 1L, 0L);
        return 0;
}

int ghvzahl()                   /* Statusabfragen bis zum Bildwechsel */
{
        int n;

        if (!blk)
                return ghdvz();
        n = 0;
        while ((ghstat() & 0x20) && n < 20000)
                n++;
        while (!(ghstat() & 0x20) && n < 30000)
                n++;
        return n;
}

int ghmaus(c, b)
int c;
char *b;
{
        if (!blk)
                return ghdms(c, b);
        return (int)(ghbios(68, (long)c, (long)b) & 0xFFFF);
}

int ghton(h, l)
int h, l;
{
        if (!blk)
                return ghdton(h, l);
        return (int)(ghbios(69, (long)h, (long)l) & 0xFFFF);
}

int ghtast()
{
        /* BDOS 6: FEh fragt nur (0 oder FFh), FFh wartet auf eine Taste -
           unter CP/M-8000 im Simulator beobachtet, unter CP/M-68K in MAME
           (mit FFh allein stand Mahjongg, bis eine Taste kam) */
        if ((__BDOS(6, 0xFEL) & 0xFF) == 0)
                return 0;
        return (int)(__BDOS(6, 0xFFL) & 0xFF);
}

int ghtext(s)
char *s;
{
        while (*s) {
                if (*s == '\n')
                        __BDOS(2, 13L);
                __BDOS(2, (long)(*s & 0xFF));
                s++;
        }
        return 0;
}

/* ---- Dateien ueber FCB ---- */

static char fcb[36];

int ghopen(name)
char *name;
{
        int i, j, c;

        for (i = 0; i < 36; i++)
                fcb[i] = 0;
        for (i = 1; i < 12; i++)
                fcb[i] = ' ';
        if (name[0] && name[1] == ':') {
                c = name[0];
                if (c >= 'a' && c <= 'z')
                        c -= 32;
                fcb[0] = c - 'A' + 1;
                name += 2;
        }
        for (i = 1, j = 0; name[j] && name[j] != '.' && i < 9; j++)
                fcb[i++] = name[j];
        while (name[j] && name[j] != '.')
                j++;
        if (name[j] == '.')
                for (j++, i = 9; name[j] && i < 12; j++)
                        fcb[i++] = name[j];
        for (i = 1; i < 12; i++)
                if (fcb[i] >= 'a' && fcb[i] <= 'z')
                        fcb[i] -= 32;
        return (__BDOS(15, (long)fcb) & 0xFF) == 0xFF;
}

int ghread(b)
char *b;
{
        __BDOS(26, (long)b);
        return (__BDOS(20, (long)fcb) & 0xFF) == 0;
}

int ghclos()
{
        __BDOS(16, (long)fcb);
        return 0;
}

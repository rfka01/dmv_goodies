/* GHSEG.C - DMVGRAF-Bildspeicher fuer CP/M-8000: freie Segmente.
 *
 * BIOS 72 meldet die Segmente, die ein Programm benutzen darf, solange es
 * laeuft (bei der K208 4, 5, 6). Kopiert wird mit DRs Speicherkopie
 * (System Call 1, GSC1.8KN); die eigene Adresse rechnet derselbe Aufruf
 * ("Map Address", Bereich 0) einmal in eine segmentierte um.
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

extern long ghbios(), ghausk;
extern long gsc1m();            /* (int adr) -> segmentierte Adresse */
extern int gsc1c();             /* (long quelle, long ziel, int n) */

static int sn = -1;
static int sseg[8];
static long sbas;

int ghsnum()
{
        long m;
        int k;

        if (sn >= 0)
                return sn;
        sn = 0;
        if (!(ghausk & 0x40))           /* Fn 72 fehlt (vor v015) */
                return 0;
        m = ghbios(72, 0L, 0L);
        for (k = 0; k < 8; k++)
                if (m & (1 << k))
                        sseg[sn++] = k;
        sbas = gsc1m(0) & 0xFFFF0000L;
        return sn;
}

long ghsgro(b)
int b;
{
        return 65536L;
}

static long sadr(b, o)
int b;
UW o;
{
        return ((long)sseg[b] << 24) | ((long)o & 0xFFFFL);
}

static long eig(p)
char *p;
{
        return sbas | ((long)(UW)p & 0xFFFFL);
}

int ghsput(b, o, s, n)
int b, n;
UW o;
char *s;
{
        if (n > 0)
                gsc1c(eig(s), sadr(b, o), n);
        return 0;
}

int ghsget(d, b, o, n)
char *d;
int b, n;
UW o;
{
        if (n > 0)
                gsc1c(sadr(b, o), eig(d), n);
        return 0;
}

int ghsmov(b, z, q, n)
int b;
UW z, q, n;
{
        if (n)
                gsc1c(sadr(b, q), sadr(b, z), (int)n);
        return 0;
}

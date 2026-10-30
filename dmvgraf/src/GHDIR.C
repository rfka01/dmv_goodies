/* GHDIR.C - DMVGRAF fuer CP/M-8000 (K238): Anschluss der direkten
 * 7220-Ausgabe (GDIR.8KN) an die Hardwareschicht GHWCPM.C.
 * Andere Systeme binden GHDNUL.C (kein direkter Weg).
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

extern long gsc1m();            /* GSC1.8KN */
extern int gdirl(), gdirr();    /* GDIR.8KN */

static int dseg;                /* Segmentwort des Datenbereichs */

int ghdini()                    /* -> 1 direkter Weg moeglich */
{
        dseg = (int)((gsc1m(0) >> 16) & 0xFFFF);
        return 1;
}

int ghdlst(l)
char *l;
{
        gdirl(dseg, l);
        return 0;
}

int ghdrd(b)                    /* Block wie BIOS 67/2 */
char *b;
{
        int m;
        char *p;

        m = ((b[0] & 0xFF) << 8) | (b[1] & 0xFF);
        p = b + 2;
        while (*p)
                p += (*p & 0xFF) + 1;
        return gdirr(dseg, b + 2, m, p + 1);
}

/* ohne BIOS-Block gibt es unter CP/M-8000 keinen Betrieb */
int ghdhw() { return -1; }
int ghdgrf(an) int an; { return 0; }
int ghdst() { return 0; }
int ghdvs() { return 0; }
int ghdms(c, b) int c; char *b; { return 1; }
int ghdton(h, l) int h, l; { return 1; }
int ghdvz() { return 0; }
int ghdbok() { return 1; }   /* CP/M-8000: BIOS prueft die Nummer */

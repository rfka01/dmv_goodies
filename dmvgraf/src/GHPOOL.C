/* GHPOOL.C - DMVGRAF-Bildspeicher als Feld im Programm (CP/M-68K, oder
 * CP/M-8000 ohne freie Segmente). Groesse GBPOOL anpassen.
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

#define GBPOOL 16384

static char pool[GBPOOL];

int ghsnum()
{
        return 1;
}

long ghsgro(b)
int b;
{
        return (long)GBPOOL;
}

static int pkopi(d, s, n)
register char *d, *s;
register int n;
{
        while (n-- > 0)
                *d++ = *s++;
        return 0;
}

int ghsput(b, o, s, n)
int b, n;
UW o;
char *s;
{
        return pkopi(&pool[o], s, n);
}

int ghsget(d, b, o, n)
char *d;
int b, n;
UW o;
{
        return pkopi(d, &pool[o], n);
}

int ghsmov(b, z, q, n)
int b;
UW z, q, n;
{
        return pkopi(&pool[z], &pool[q], (int)n);
}

/* GHPOOL.C - DMVGRAF-Bildspeicher als Feld im Programm (CP/M-68K, oder
 * CP/M-8000 ohne freie Segmente). Groesse GBPOOL anpassen.
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

#ifndef GBPOOL
#define GBPOOL 16384   /* CP/M-68K: mit -DGBPOOL=61440 uebersetzen */
#endif

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

int ghsput(b, o, s, n)
int b, n;
UW o;
char *s;
{
        return gkopi(&pool[o], s, n);
}

int ghsget(d, b, o, n)
char *d;
int b, n;
UW o;
{
        return gkopi(d, &pool[o], n);
}

int ghsmov(b, z, q, n)
int b;
UW z, q, n;
{
        return gkopi(&pool[z], &pool[q], (int)n);
}

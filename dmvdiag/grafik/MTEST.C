/* MTEST.C - DMVGRAF: Maus und Erkennung im Textbetrieb pruefen.
 * Zeigt, was ginfo meldet, und danach jede Mausbewegung (x, y, Knoepfe)
 * als Textzeile; Ende mit einer Taste.
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "DMVGRAF.H"

static char z[64];

static int zahl(p, v)
char *p;
int v;
{
        char t[8];
        int n;

        if (v < 0) {
                *p++ = '-';
                v = -v;
        }
        n = 0;
        do {
                t[n++] = '0' + v % 10;
                v /= 10;
        } while (v && n < 7);
        while (n > 0)
                *p++ = t[--n];
        *p = 0;
        return 0;
}

static int ausg(text, v)
char *text;
int v;
{
        gputs(text);
        zahl(z, v);
        gputs(z);
        return 0;
}

int main()
{
        int f, m, s, c, r, x, y, k, ax, ay, ak, n;

        gputs("MTEST: frage die Hardware ab ...\n");
        r = ginfo(&f, &m, &s, &c);
        ausg("MTEST: ginfo ", r);
        ausg("  Farbe ", f);
        ausg("  Maus ", m);
        ausg("  Synth ", s);
        ausg("  Covox ", c);
        gputs("\n");
        if (!m) {
                gputs("Keine Maus erkannt.\n");
                return 1;
        }
        gorig(1);
        ax = ay = ak = -1;
        n = 0;
        while (!gkey()) {
                if (!mpos(&x, &y, &k)) {
                        if (n++ < 3)
                                gputs("mpos: keine Antwort\n");
                        continue;
                }
                if (x != ax || y != ay || k != ak) {
                        ausg("x ", x);
                        ausg("  y ", y);
                        ausg("  Knopf ", k);
                        gputs("\n");
                        ax = x;
                        ay = y;
                        ak = k;
                }
        }
        return 0;
}

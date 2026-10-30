/* GEIN.C - DMVGRAF: Maus, Tastatur, Zeit, Auskunft, Ton.
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

int ginfo(f, m, s, c)
int *f, *m, *s, *c;
{
        int r;

        r = qstart();
        *f = !r && (qhw & GH_FARBE) ? 1 : 0;
        *m = !r && (qhw & GH_MAUS) ? 1 : 0;
        *s = !r && (qhw & GH_SYN) ? 1 : 0;
        *c = !r && (qhw & GH_COVOX) ? 1 : 0;
        return r;
}

/* K806 Befehl 00h: X low, X high, Y low, Y high, Schalter; Y zaehlt
   wie NCRGRAF nach oben (0 = unten) */
int mpos(x, y, k)
int *x, *y, *k;
{
        static char blk[8];
        int v;

        if (qstart() || !(qhw & GH_MAUS))
                return 0;
        qzwied();
        blk[0] = 0;
        blk[1] = 5;
        if (ghmaus(0, blk))
                return 0;
        *x = (blk[2] & 0xFF) | ((blk[3] & 0xFF) << 8);
        v = (blk[4] & 0xFF) | ((blk[5] & 0xFF) << 8);
        if (v < 0) v = 0;
        if (v > 399) v = 399;
        *y = qorig ? 399 - v : v;
        *k = blk[6] & 7;
        return 1;
}

int mset(x, y)
int x, y;
{
        static char blk[8];

        if (qstart() || !(qhw & GH_MAUS))
                return GE_HW;
        if (qorig)
                y = 399 - y;
        blk[0] = 4;
        blk[1] = x & 0xFF;
        blk[2] = (x >> 8) & 0xFF;
        blk[3] = y & 0xFF;
        blk[4] = (y >> 8) & 0xFF;
        blk[5] = 0;
        return ghmaus(4, blk) ? GE_HW : 0;
}

int gkey()
{
        qzwied();
        return ghtast();
}

int gwait(n)
int n;
{
        qzwied();
        qflush();
        while (n-- > 0)
                ghvsyn();
        return 0;
}

int ton(h, l)
int h, l;
{
        if (h < 0 || h > 42 || l < 1 || l > 224)
                return GE_FORM;
        return ghton(h, l);
}

/* GDC-Listen direkt an den 7220 oder ueber den BIOS (Vorgabe: direkt,
   wo das System es erlaubt) */
int gdirekt(an)
int an;
{
        if (qstart())
                return -1;
        qflush();
        return ghdset(an);
}

int gputs(s)
char *s;
{
        return ghtext(s);
}

/* Zufallszahl fuer den Start: wie weit der Bildaufbau gerade ist (Zahl
   der Statusabfragen bis zum naechsten Bildwechsel), dazu die Maus */
int grnd()
{
        static UW z = 1;
        static char blk[8];
        int n;

        if (qstart())
                return z & 0x7FFF;
        n = 0;
        while ((ghstat() & 0x20) && n < 20000)
                n++;
        while (!(ghstat() & 0x20) && n < 30000)
                n++;
        z = M16(z * 31421 + n + 6927);
        if (qhw & GH_MAUS) {
                blk[0] = 0;
                blk[1] = 5;
                if (!ghmaus(0, blk))
                        z = M16(z * 75 + ((blk[2] & 0xFF) << 7)
                                + (blk[4] & 0xFF) * 13 + (blk[6] & 7));
        }
        return z & 0x7FFF;
}

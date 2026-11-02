/* STEST.C - Pruefprogramm fuer die Figuren ueber GCHRD (gsprdef, gspr).
   Ein Bild: Hintergrund mit Streifen, Figuren 8 x 8 und 16 x 16 in den
   drei Arten und mit Zoom 0, 1 und 3; eine umgekehrte Figur wird mit
   einem zweiten gspr wieder entfernt. Taste beendet.

   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "DMVGRAF.H"

/* 8 x 8 Raute, nur gruen und rot (gelb), Mitte blau */
static char raute[24] = {
        0x18, 0x3C, 0x7E, 0xE7, 0xE7, 0x7E, 0x3C, 0x18,    /* gruen */
        0x18, 0x3C, 0x7E, 0xE7, 0xE7, 0x7E, 0x3C, 0x18,    /* rot */
        0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00     /* blau */
};
static char figur[96];          /* 16 x 16, wird berechnet */

static int bauen()
{
        int y, x, k, g, r, b;

        for (y = 0; y < 16; y++)
                for (x = 0; x < 16; x++) {
                        k = y * 2 + (x >> 3);
                        g = r = b = 0;
                        if ((x - 8) * (x - 8) + (y - 8) * (y - 8) < 50) {
                                g = y < 8;              /* oben gruen */
                                r = x >= 8;             /* rechts rot */
                                b = (x + y) & 1;        /* Raster blau */
                        }
                        if (y == 15 || x == 0)
                                g = r = b = 1;          /* weisser Rand links, unten */
                        if (g) figur[k] |= 1 << (x & 7);
                        if (r) figur[32 + k] |= 1 << (x & 7);
                        if (b) figur[64 + k] |= 1 << (x & 7);
                }
        return 0;
}

int main()
{
        int i, k;

        if (ginit()) return 1;
        gorig(1);
        gmode(3);
        for (i = 0; i < 8; i++) {               /* Streifen */
                gcolor(i);
                gpoint(0, 200 + i * 20);
                gbox(640, 20, 0);
        }
        gcolor(7);
        gpoint(8, 4);
        gtext("DMVGRAF - Figuren ueber GCHRD: XOR, durchsichtig, deckend, Zoom", 0);
        bauen();
        gsprdef(0, 8, 8, raute);
        gsprdef(1, 16, 16, figur);
        for (k = 0; k < 3; k++) {               /* Arten in Zeilen */
                i = k == 0 ? GB_XOR : k == 1 ? GB_DURCH : GB_DECK;
                gspr(0, 20, 60 + k * 110, 0, i);
                gspr(0, 40, 60 + k * 110, 1, i);
                gspr(1, 80, 60 + k * 110, 0, i);
                gspr(1, 110, 60 + k * 110, 1, i);
                gspr(1, 160, 60 + k * 110, 3, i);
                gspr(1, 240, 150 + k * 80, 0, i);   /* ueber den Streifen */
                gspr(0, 280, 150 + k * 80, 2, i);
        }
        gspr(1, 400, 60, 2, GB_XOR);            /* zweimal: wieder weg */
        gwait(50);
        gspr(1, 400, 60, 2, GB_XOR);
        gpoint(460, 76);
        gtext("XOR 2x: weg = gut", 0);
        k = gspr(1, 630, 60, 0, GB_DURCH);      /* ragt hinaus: GE_LAGE */
        gpoint(400, 120);
        gtext(k == GE_LAGE ? "Rand: GE_LAGE gut" : "Rand: FALSCH", 0);
        while (!gkey())
                gwait(1);
        gexit();
        return 0;
}

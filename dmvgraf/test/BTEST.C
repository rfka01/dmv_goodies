/* BTEST.C - Pruefprogramm fuer den schnellen Weg der Bilder (wie
   HOPPLERs BlitAsm): links mit, rechts ohne schnellen Weg; beide Haelften
   muessen gleich aussehen. Bilder: Gesicht, Wagen, Herz (SET/RESET),
   Pfeil (1 Bit, Malfarbe), Haus 64 x 47 mit Maske; Zeilen deckend,
   durchsichtig, XOR. Taste beendet.

   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "DMVGRAF.H"

extern int qbsch;               /* intern: schneller Weg an */
static unsigned char d[2048];
static int f[96];

/* Punkt x,y im Format 4 (Zeile: Maske, Gruen, Rot, Blau) ab d[o]; Farbe c:
   Bit 0 gruen, Bit 1 rot, Bit 2 blau; 0 deckendes Schwarz */
static void pix(o, n, x, y, c)
int o, n, x, y, c;
{
        int e, i;

        i = o + y * 4 * n + (x >> 3);
        for (e = 0; e < 4; e++)
                if (e == 0 || (c >> (e - 1)) & 1)
                        d[i + e * n] |= 1 << (x & 7);
}

static int qd(a, b)
int a, b;
{
        return a * a + b * b;
}

static void bauen()
{
        int x, y, c, r, e;

        for (y = 0; y < 2048; y++)
                d[y] = 0;
        for (y = 0; y < 96; y++)
                f[y] = 0;
        for (y = 0; y < 20; y++)                /* 0: Gesicht 24 x 20 */
                for (x = 0; x < 24; x++) {
                        r = qd(2 * x - 23, 2 * y - 19);
                        if (r >= 440)
                                continue;
                        c = r < 324 ? 3 : 0;
                        if ((x == 7 || x == 8 || x == 15 || x == 16)
                            && y >= 5 && y <= 7)
                                c = 0;
                        if ((y == 13 && (x == 7 || x == 16))
                            || (y == 14 && x >= 8 && x <= 15))
                                c = 2;
                        pix(0, 3, x, y, c);
                }
        for (y = 0; y < 16; y++)                /* 1: Wagen 32 x 16 */
                for (x = 0; x < 32; x++) {
                        c = -1;
                        if (y >= 6 && y <= 11 && x >= 1 && x <= 30)
                                c = 2;
                        if (y >= 2 && y <= 5 && x >= 8 && x <= 23)
                                c = (y >= 3 && ((x >= 10 && x <= 15)
                                     || (x >= 17 && x <= 22))) ? 4 : 2;
                        if (x == 30 && (y == 7 || y == 8))
                                c = 3;
                        if (qd(x - 8, y - 12) < 10 || qd(x - 23, y - 12) < 10)
                                c = (y == 12 && (x == 8 || x == 23)) ? 7 : 0;
                        if (c >= 0)
                                pix(240, 4, x, y, c);
                }
        for (y = 0; y < 16; y++)                /* 2: Herz 16 x 16, SET/RESET */
                for (x = 0; x < 16; x++) {
                        c = 2 * x - 15;
                        r = 2 * y;
                        if (qd(c + 6, r - 9) <= 49 || qd(c - 6, r - 9) <= 49
                            || (r >= 9 && (c < 0 ? -c : c) * 19 <= 13 * (28 - r)))
                                for (e = 0; e < 3; e++)  /* rot setzen, sonst loeschen */
                                        f[(2 * e + (e != 1)) * 16 + y]
                                                |= 1 << x;
                }
        for (y = 0; y < 12; y++)                /* 3: Pfeil 24 x 12, 1 Bit */
                for (x = 0; x < 24; x++) {
                        r = 2 * y - 11;
                        if (r < 0)
                                r = -r;
                        if ((x < 16 && y >= 4 && y <= 7)
                            || (x >= 16 && r <= 2 * (23 - x) + 1))
                                d[496 + y * 3 + (x >> 3)] |= 1 << (x & 7);
                }
        for (y = 0; y < 47; y++)                /* 4: Haus 64 x 47 */
                for (x = 0; x < 64; x++) {
                        c = -1;
                        r = 2 * x - 63;
                        if (r < 0)
                                r = -r;
                        if (y >= 3 && y <= 14 && x >= 44 && x <= 49)
                                c = 0;
                        if (y < 18 && r * 17 <= 136 + y * 56)
                                c = 2;
                        if (y >= 18 && x >= 6 && x <= 57) {
                                c = 7;
                                if (y >= 22 && y <= 31 && ((x >= 12 && x <= 23)
                                    || (x >= 40 && x <= 51)))
                                        c = (x == 17 || x == 18 || x == 45
                                             || x == 46 || y == 26 || y == 27)
                                            ? 7 : 4;
                                if (x >= 28 && x <= 35 && y >= 30)
                                        c = (x == 34 && y == 38) ? 3 : 1;
                        }
                        if (c >= 0)
                                pix(540, 8, x, y, c);
                }
}

int main()
{
        int i, k, a, x, y;

        if (ginit()) return 1;
        gorig(1);
        gmode(3);
        for (i = 0; i < 8; i++) {
                gcolor(i); gpoint(0, i * 50); gbox(640, 50, 0);
        }
        bauen();
        gbdef(0, 24, 20, GB_EBN, d);
        gbdef(1, 32, 16, GB_EBN, d + 240);
        gbdef(2, 16, 16, GB_SETR, f);
        gbdef(3, 24, 12, GB_BIT0, d + 496);
        gbdef(4, 64, 47, GB_EBN, d + 540);
        for (k = 0; k < 2; k++) {
                qbsch = k == 0;
                for (a = 0; a < 4; a++) {
                        if (a == GB_SCHAT)
                                continue;
                        for (i = 0; i < 5; i++) {
                                gcolor(1 + (i + a) % 7);
                                x = k * 320 + i * 64;
                                y = 10 + a * 90 + (i == 4 ? 30 : 0);
                                gpoint(x, y);
                                gbput(i, a);
                        }
                }
        }
        qbsch = 1;
        gmode(3);
        gcolor(7);
        gpoint(8, 220);
        gtext("links: schneller Weg", 0);
        gpoint(328, 220);
        gtext("rechts: alter Weg", 0);
        gmode(2);
        gpoint(8, 370);
        gtext("Zeilen deckend, durchsichtig, XOR. Taste beendet.", 0);
        while (!gkey())
                gwait(1);
        gexit();
        return 0;
}

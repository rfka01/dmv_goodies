/* BTEST.C - Pruefprogramm fuer den schnellen Weg der Bilder (wie
   HOPPLERs BlitAsm): links mit, rechts ohne schnellen Weg; beide Haelften
   muessen gleich aussehen. Taste beendet.

   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "DMVGRAF.H"

extern int qbsch;               /* intern: schneller Weg an */
static unsigned char d[2048];
static int f[96];

int main()
{
        int i, k, a, x, y;

        if (ginit()) return 1;
        gorig(1);
        gmode(3);
        for (i = 0; i < 8; i++) {
                gcolor(i); gpoint(0, i * 50); gbox(640, 50, 0);
        }
        for (i = 0; i < 2048; i++)
                d[i] = (i * 37 + (i >> 3)) & 0xFF;
        for (i = 0; i < 96; i++)
                f[i] = (int)(((long)i * 1237) & 0x7777);
        gbdef(0, 24, 20, GB_EBN, d);
        gbdef(1, 32, 16, GB_EBN, d + 100);
        gbdef(2, 16, 16, GB_SETR, f);
        gbdef(3, 24, 12, GB_BIT0, d + 300);
        gbdef(4, 64, 47, GB_EBN, d + 400);
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
        while (!gkey())
                gwait(1);
        gexit();
        return 0;
}

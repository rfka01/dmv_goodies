/* PTEST.C - Pruefprogramm fuer die Muster: am Schirm verankert
   (aneinanderstossende Flaechen ohne Naht), zweifarbig (gbcol) und
   eigene Muster (gpdef). Taste beendet.

   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "DMVGRAF.H"

static char welle[8] = { 0xFF, 0xEE, 0x55, 0xBB, 0xFF, 0xEE, 0x55, 0xBB };
static char acht[8] = { 0x01, 0x00, 0x10, 0x00, 0x01, 0x00, 0x10, 0x00 };

int main()
{
        int i, k;

        if (ginit()) return 1;
        gorig(1);
        gcolor(7);
        gpoint(8, 4);
        gtext("DMVGRAF - Muster am Schirm verankert, zweifarbig, eigene", 0);
        gmode(3);
        for (k = 0; k < 4; k++) {               /* Streifen aus Stuecken */
                gpatt(k == 0 ? 1 : k == 1 ? 2 : k == 2 ? 6 : 10);
                gcolor(k + 2);
                for (i = 0; i < 7; i++) {
                        gpoint(13 + i * 37 + k, 30 + k * 30 + (i & 1) * 3);
                        gbox(37, 23 + (i & 1) * 2, 0);
                }
        }
        gpdef(14, welle);
        gpdef(15, acht);
        for (k = 0; k < 2; k++) {               /* zweifarbig */
                gpatt(14 + k);
                gbcol(k ? 6 : 0);
                gcolor(k ? 4 : 1);
                for (i = 0; i < 5; i++) {
                        gpoint(300 + i * 51 + k * 5, 200 + k * 60 + i);
                        gbox(51, 47, 0);
                }
        }
        gbcol(-1);
        gpatt(9);                               /* Richtung 2 wie bisher */
        gcolor(7);
        gpoint(100, 330);
        gbox(60, 40, 2);
        while (!gkey())
                gwait(1);
        gexit();
        return 0;
}

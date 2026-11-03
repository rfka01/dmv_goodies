/* TONTEST.C - Pruefprogramm fuer die Tonwarteschlange (tonq, tonidle):
   stellt eine Tonleiter auf und ab (29 Toene) auf einmal in die
   Warteschlange und zeichnet waehrenddessen einen wandernden Balken; die
   Toene laufen ohne Warten weiter. Zeigt "voll" bei GE_PLATZ und "fertig",
   sobald tonidle meldet, dass alles beim 8741 ist. Taste beendet.

   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "DMVGRAF.H"

int main()
{
        int i, x, r, voll, fertig;

        if (ginit()) {
                gputs("Keine DMV-Grafik gefunden.\n");
                return 1;
        }
        gorig(1);
        gcolor(7);
        gpoint(8, 4);
        gtext("TONTEST: Tonleiter ueber tonq, der Balken laeuft weiter", 0);
        voll = 0;
        for (i = 0; i < 15; i++)                /* auf */
                if (tonq(13 + 2 * i, 8) == GE_PLATZ)
                        voll = 1;
        for (i = 13; i >= 0; i--)               /* ab */
                if (tonq(13 + 2 * i, 8) == GE_PLATZ)
                        voll = 1;
        gpoint(8, 30);
        gtext(voll ? "Warteschlange voll (GE_PLATZ)" : "29 Toene eingestellt", 0);
        fertig = 0;
        x = 0;
        gmode(1);                               /* umkehren */
        while (!gkey()) {
                gpoint(x, 200);
                gbox(16, 40, 0);                /* Balken an */
                gwait(1);
                gpoint(x, 200);
                gbox(16, 40, 0);                /* und wieder weg */
                x = (x + 8) % 624;
                if (!fertig && tonidle()) {
                        fertig = 1;
                        gmode(0);
                        gpoint(8, 60);
                        gtext("fertig: alles beim 8741 (tonidle)", 0);
                        gmode(1);
                }
        }
        gexit();
        r = 0;
        return r;
}

/* GZEIT.C - Zeitbasis fuer DMVGRAF unter MS-DOS (8088/V20).
 *
 * Die Arbeit tut der Kern aus src/GZEIT.INC (in GHW86.S, erzeugt von
 * tools/gpicbin.py), derselbe wie in DMVDEMO, DMVRACE und PITTEST.
 * Kanal 2 des 8253 bekommt 500 kHz; DOS betreibt ihn in Modus 3
 * (NCR-DOS 2.11: Startwert 5000, DOS-3.3-Port: 27463), ein Zaehlschritt
 * ist dann 1 us. Ohne DOS steht er; dann setzt gtinit ihn auf Modus 2
 * (2 us je Schritt). Kanal 0 und Bildwechsel werden getrennt und erst auf
 * Anfrage geeicht (je 0,26 s); steht Kanal 0, stoesst gt0per ihn ueber
 * Port 14h an.
 *
 *   gtinit()   Kanal 2 kennenlernen, Uhr auf 0 -> 1, wenn er stand
 *   gtus()     us seit gtinit (long); oefter rufen als einmal je
 *              Halbperiode von Kanal 2 (5 ms bzw. 27 ms)
 *   gtbild()   us je Bild, gt0per() us je Takt von Kanal 0 (0 = steht)
 *   gteich(art, n)  neu eichen ueber n x 65536 us: art 0 Kanal 0, 1 Bild
 *   gt0()      Kanal 0 gelatcht (Rohwert)
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

extern int ghk(), ghkcf;
extern long ghkl();
extern char ghkern[];

#define K_LERN   21
#define K_US     23
#define K_CAL0   24
#define K_CALV   25
#define K_C0RD   26

static int gtok;        /* gtinit gelaufen */
static int gtb, gt0p;   /* us je Bild, je Takt von Kanal 0; -1 ungeeicht */
static char gtv, gt0v;  /* geeicht */

int gtinit()
{
        ghk(K_LERN, 0, 0, 0, 0);
        gtok = 1;
        return ghkcf;
}

long gtus()
{
        if (!gtok)
                gtinit();
        return ghkl(K_US, 0, 0, 0, 0);
}

int gt0()
{
        return ghk(K_C0RD, 0, 0, 0, 0);
}

int gteich(art, n)
int art, n;
{
        int *dur;

        if (!gtok)
                gtinit();
        dur = (int *)(ghkern + ((int *)ghkern)[5] + 10);   /* gz_dur */
        *dur = n > 0 ? n : 4;
        if (art) {
                gtb = ghk(K_CALV, 0, 0, 0, 0);
                gtv = 1;
        } else {
                gt0p = ghk(K_CAL0, 1, 0, 0, 0);
                gt0v = 1;
        }
        *dur = 4;
        return art ? gtb : gt0p;
}

int gtbild()
{
        return gtv ? gtb : gteich(1, 4);
}

int gt0per()
{
        return gt0v ? gt0p : gteich(0, 4);
}

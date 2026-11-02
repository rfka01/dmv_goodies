/* GZEIT.C - Zeitbasis fuer DMVGRAF unter MS-DOS (8088/V20).
 *
 * Nach PITTEST (claude/PITTEST-Ergebnisse.md): Kanal 2 des 8253 bekommt
 * 500 kHz; DOS betreibt ihn in Modus 3 (NCR-DOS 2.11: Startwert 5000,
 * DOS-3.3-Port: 27463), ein Zaehlschritt ist dann 1 us. Ohne DOS ist er
 * nicht programmiert; dann setzt gtinit ihn auf Modus 2 mit vollem
 * Umfang (2 us je Schritt). Kanal 0 laeuft mit dem Bildtakt (Modus 5,
 * Motor-Auszeit); steht er, stoesst gtinit ihn ueber Port 14h an.
 * Kanal 0 und 1 werden nie umprogrammiert.
 *
 *   gtinit()   Kanal 2 kennenlernen, Kanal 0 und Bildwechsel eichen
 *   gtus()     us seit gtinit (long); oefter rufen als einmal je
 *              Halbperiode von Kanal 2 (5 ms bzw. 27 ms)
 *   gtbild()   us je Bild, gt0per() us je Takt von Kanal 0 (0 = steht)
 *   gt0()      Kanal 0 gelatcht (Rohwert)
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

extern int gpit();      /* (k) Kanal k gelatcht */
extern int gpit2();     /* () Kanal 2: Modus 2, voller Umfang */
extern int inb(), outb();

static long gtu;        /* Uhr in us */
static unsigned gtalt;  /* letzter Stand von Kanal 2 */
static long gtper;      /* Halbperiode (Modus 3) bzw. Umfang (Modus 2) */
static int gtschr;      /* us je Zaehlschritt: 1 (Modus 3) oder 2 */
static int gtok;        /* gtinit gelaufen */
static int gtb, gt0p;   /* us je Bild, je Takt von Kanal 0 */

long gtus()
{
        unsigned n;
        long d;

        n = gpit(2);
        if (n <= gtalt)
                d = (long)(gtalt - n);
        else                            /* neu geladen */
                d = (long)gtalt + gtper - n;
        gtalt = n;
        gtu += d * gtschr;
        return gtu;
}

int gt0()
{
        return gpit(0);
}

/* Flanken zaehlen: art 0 Kanal 0 (jede Aenderung), 1 Bildwechsel
   (steigende Flanke von Bit 5 an A0h); ueber etwa dauer us. Liefert us
   je Flanke, 0 wenn in 0,26 s keine kam. */
static int gtflank(art, dauer)
int art;
long dauer;
{
        long t, t0, t1, tz, k;
        int n, a, v;

        a = art ? (inb(0xA0) & 0x20) : gpit(0);
        t = gtus();
        t0 = -1;
        t1 = 0;
        tz = t;
        n = 0;
        for (k = 0; k < 400000L; k++) { /* Grenze, falls die Uhr steht */
                v = art ? (inb(0xA0) & 0x20) : gpit(0);
                t = gtus();
                if (v != a && (!art || v)) {
                        if (t0 < 0)
                                t0 = t;
                        else
                                n++;
                        t1 = t;
                        tz = t;
                }
                a = v;
                if (t - tz > 260000L)
                        return 0;
                if (t0 >= 0 && t - t0 > dauer)
                        return n ? (int)((t1 - t0) / n) : 0;
        }
        return 0;
}

int gtinit()
{
        unsigned v, mx, alt;
        int i, z, neu;

        /* Kanal 2 kennenlernen: hoechster Stand und Neuladungen */
        mx = 0;
        z = 0;
        alt = gpit(2);
        neu = 0;
        for (i = 0; i < 20000; i++) {
                v = gpit(2);
                if (v != alt)
                        z++;
                if (v > alt)
                        neu++;
                if (v > mx)
                        mx = v;
                alt = v;
        }
        if (z < 10) {                   /* steht: ohne DOS, selbst setzen */
                gpit2();
                gtper = 65536L;
                gtschr = 2;
        } else {
                gtschr = 1;
                if (mx > 4900 && mx <= 5000)
                        gtper = 5000;
                else if (mx > 27000 && mx <= 27463)
                        gtper = 27463;
                else
                        gtper = (long)mx + 1;
        }
        gtalt = gpit(2);
        gtu = 0;
        gtok = 1;
        gtb = gtflank(1, 500000L);
        gt0p = gtflank(0, 500000L);
        if (!gt0p) {                    /* Kanal 0 steht: anstossen */
                outb(0x14, 0);
                gt0p = gtflank(0, 500000L);
        }
        return gtb ? 0 : 1;
}

int gtbild()
{
        if (!gtok)
                gtinit();
        return gtb;
}

int gt0per()
{
        if (!gtok)
                gtinit();
        return gt0p;
}

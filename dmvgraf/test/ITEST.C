/* ITEST.C - Hardwareauskunft (ghinfo) und Zeitbasis anzeigen, nur MS-DOS.
   ITEST K probiert dazu den Koprozessor (K234), ITEST U die Uhr K803,
   ITEST KU beides.
   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "DMVGRAF.H"

static char zb[8];
static char *zahl(v)
int v;
{
        int i;

        i = 7;
        zb[i] = 0;
        if (v < 0) v = -v;
        do { zb[--i] = '0' + v % 10; v /= 10; } while (v && i > 1);
        return &zb[i];
}

static char *nam[11] = { "ROM", "Schirm", "Karte", "CPU", "FPU", "Kopro",
                         "Baenke", "Spur A", "Takt 10kHz", "Bild 1/100Hz", "Uhr" };

int main(argc, argv)
int argc;
char **argv;
{
        int r[16], i;
        char *a, z[80];

        r[15] = 0;
        for (a = argc > 1 ? argv[1] : ""; *a; a++) {
                if (*a == 'K' || *a == 'k') r[15] |= GH_MITCOPRO;
                if (*a == 'U' || *a == 'u') r[15] |= GH_MITUHR;
        }
        ghinfo(r);
        for (i = 0; i <= GH_UHR; i++) {
                gputs(nam[i]);
                gputs(": ");
                gputs(zahl(r[i]));
                gputs("\r\n");
        }
        gputs("ghcpu: ");
        gputs(zahl(ghcpu()));
        gputs("\r\nKanal 0 us: ");
        gputs(zahl(gt0per()));
        gputs("\r\n");
        ghzeile(z, 0);
        gputs(z);
        ghzeile(z, 1);
        gputs(z);
        return 0;
}

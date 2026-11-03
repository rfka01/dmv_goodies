/* PICTEST.C - Pruefprogramm fuer gepackte Vollbilder (gpic, gpicm).

     PICTEST datei xw y art [datei xw y art ...]

   Zeigt die Dateien (.PIC) nacheinander an Wortspalte xw, Zeile y; art
   wie gpic (1 dunkel, 2 mono, 4 Farbe), mit angehaengtem S wird die Datei
   erst in den Speicher gelesen (bis 16 KB) und mit gpicm gezeigt (etwa
   "4S"). Eine Taste beendet; danach stehen die Rueckgaben auf der Konsole
   (0 gut, sonst Fehlernummer).

   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "GINTERN.H"
#include "DMVPIC.H"

static char sp[16384];
static char zb[8];

static int zahl(s)
char *s;
{
        int v = 0;

        while (*s >= '0' && *s <= '9')
                v = v * 10 + *s++ - '0';
        return v;
}

static char *text(v)
int v;
{
        int i = 7;

        zb[i] = 0;
        do {
                zb[--i] = '0' + v % 10;
                v /= 10;
        } while (v && i > 0);
        return &zb[i];
}

int main(argc, argv)
int argc;
char **argv;
{
        static int r[8];
        int i, k, xw, y, art;
        char *a;
        unsigned n;

        if (argc < 5) {
                gputs("PICTEST datei xw y art [datei xw y art ...]\n");
                return 1;
        }
        if (ginit()) {
                gputs("Keine DMV-Grafik gefunden.\n");
                return 1;
        }
        for (i = 1, k = 0; i + 3 < argc && k < 8; i += 4, k++) {
                xw = zahl(argv[i + 1]);
                y = zahl(argv[i + 2]);
                a = argv[i + 3];
                art = zahl(a);
                while (*a >= '0' && *a <= '9')
                        a++;
                if (*a == 'S' || *a == 's') {
                        n = 0;
                        if (ghopen(argv[i]) == 0) {
                                while (n + 128 <= sizeof sp && ghread(sp + n))
                                        n += 128;
                                ghclos();
                        }
                        r[k] = gpicm(sp, n, xw, y, art);
                } else
                        r[k] = gpic(argv[i], xw, y, art);
        }
        while (!gkey())
                gwait(1);
        gexit();
        gputs("gpic:");
        for (i = 0; i < k; i++) {
                gputs(" ");
                gputs(text(r[i]));
        }
        gputs("\n");
        return 0;
}

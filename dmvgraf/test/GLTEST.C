/* GLTEST.C - Pruefprogramm fuer Bildsammlungen (gbldv).

     GLTEST datei kopf [d]

   Laedt die Sammlung (ab Byte kopf) mit gbldv und zeichnet alle Bilder
   deckend nebeneinander. Mit d liest das Programm die Datei selbst und
   legt die Bilder mit gbdef an; beide Bilder muessen gleich aussehen. Eine
   Taste beendet, danach "gbldv: Fehler Anzahl".

   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "GINTERN.H"

#define NMAX 40
static char rb[128];
static int rp = 128;
static char bd[20480];
static int bw5[2048];
static int bb[NMAX], bhh[NMAX];

static int zahl(s)
char *s;
{
        unsigned v = 0;

        while (*s >= '0' && *s <= '9')
                v = v * 10 + *s++ - '0';
        return (int)v;
}

static char zb[8];
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

static int lies(d, n)           /* n Byte -> 1 bei Dateiende */
char *d;
unsigned n;
{
        while (n--) {
                if (rp >= 128) {
                        if (!ghread(rb))
                                return 1;
                        rp = 0;
                }
                *d++ = rb[rp++];
        }
        return 0;
}

static int laenge(b, h, f)
int b, h, f;
{
        int n = (b + 7) >> 3;

        switch (f) {
        case 1: case 2: return h * n;
        case 3: return h * ((b + 1) >> 1);
        case 4: return 4 * h * n;
        }
        return 12 * h * ((b + 15) >> 4);
}

/* Datei selbst lesen, Bilder mit gbdef anlegen -> Anzahl */
static int selbst(name, kopf, fe)
char *name;
unsigned kopf;
int *fe;
{
        int i, k, n, b, h, f;
        char k6[6];

        *fe = 0;
        if (ghopen(name)) {
                *fe = GE_DATEI;
                return 0;
        }
        while (kopf > 0) {
                k = kopf > 128 ? 128 : kopf;
                lies(bd, k);
                kopf -= k;
        }
        for (i = 0; i < NMAX; i++) {
                if (lies(k6, 6))
                        break;
                b = (k6[0] & 0xFF) | (k6[1] & 0xFF) << 8;
                h = (k6[2] & 0xFF) | (k6[3] & 0xFF) << 8;
                f = k6[4];
                if (b < 1 || b > 640)
                        break;
                n = laenge(b, h, f);
                if (n > (int)sizeof(bd) || lies(bd, n)) {
                        *fe = GE_KURZ;
                        break;
                }
                if (f == GB_SETR) {
                        for (k = 0; k < n / 2; k++)
                                bw5[k] = (bd[2 * k] & 0xFF)
                                        | (bd[2 * k + 1] & 0xFF) << 8;
                        *fe = gbdef(i, b, h, f, (char *)bw5);
                } else
                        *fe = gbdef(i, b, h, f, bd);
                if (*fe)
                        break;
        }
        ghclos();
        return i;
}

int main(argc, argv)
int argc;
char **argv;
{
        int i, n, x, y, zh, fe, r[6];
        unsigned kopf;

        if (argc < 3)
                return 1;
        kopf = (unsigned)zahl(argv[2]);
        if (ginit())
                return 1;
        gorig(1);
        if (argc > 3 && (argv[3][0] == 'd' || argv[3][0] == 'D'))
                n = selbst(argv[1], kopf, &fe);
        else {
                fe = gbldv(0, argv[1], -1, kopf);
                n = gbanz;
        }
        x = y = zh = 0;
        for (i = 0; i < n && i < NMAX; i++) {
                if (gbinfo(i, r))
                        continue;
                bb[i] = r[0];
                bhh[i] = r[1];
                if (x + bb[i] > 640) {
                        x = 0;
                        y += zh + 4;
                        zh = 0;
                }
                gpoint(x, y);
                gbput(i, GB_DECK);
                x += bb[i] + 4;
                if (bhh[i] > zh)
                        zh = bhh[i];
        }
        while (!gkey())
                gwait(1);
        gexit();
        gputs("gbldv: ");
        gputs(text(fe));
        gputs(" ");
        gputs(text(n));
        gputs("\n");
        return 0;
}

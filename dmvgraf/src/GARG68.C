/* GARG68.C - Kommandozeile der CP/M-68K-Basisseite (Versatz 80h: Laenge,
 * Zeichen) in argc/argv zerlegen und main aufrufen (zu GCRT68.S).
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
extern int main();

#define NARG 16

static char zeile[130];
static char *argv[NARG + 1];

int gcmain(bs)
char *bs;
{
        int n, i, argc;
        char *p;

        n = bs[0x80] & 0xFF;
        if (n > 127)
                n = 127;
        for (i = 0; i < n; i++)
                zeile[i] = bs[0x81 + i];
        zeile[n] = 0;
        argv[0] = "";
        argc = 1;
        p = zeile;
        while (*p && argc < NARG) {
                while (*p == ' ' || *p == '\t')
                        p++;
                if (!*p)
                        break;
                argv[argc++] = p;
                while (*p && *p != ' ' && *p != '\t')
                        p++;
                if (*p)
                        *p++ = 0;
        }
        argv[argc] = 0;
        return main(argc, argv);
}

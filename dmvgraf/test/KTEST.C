/* KTEST.C - Koprozessor und Hardwaretabelle (nur MS-DOS, K235):
   ghinfo mit Koprozessorprobe als Hexzeile (zum Vergleich mit GHWT.COM,
   der MASM-Fassung), die Zeile von ghzeile, dann mit K234 ein Lauf des
   68008 (Summe 1 bis 100 = 5050 im Briefkasten) und immer ein Lauf
   eigenen Z80-Codes. Zum Schluss: sind 8088-Vektoren und Warteplatz des
   Z80 (F10Eh-F11Ch) unveraendert?
   Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT */
#include "DMVGRAF.H"

extern int gfget(), gfput(), gdsg();

static char zb[8];
static char *hex(v)
unsigned v;
{
        int i;

        for (i = 3; i >= 0; i--, v >>= 4)
                zb[i] = "0123456789ABCDEF"[v & 15];
        zb[4] = 0;
        return zb;
}

static char *dez(v)
unsigned v;
{
        int i = 7;

        zb[i] = 0;
        do { zb[--i] = '0' + v % 10; v /= 10; } while (v && i > 0);
        return &zb[i];
}

/* 68008 ab P+100h: Summe 1..100 nach P+0, dann zurueck */
static unsigned char m68[] = {
        0x70, 0x00, 0x72, 0x64, 0xD0, 0x41, 0x53, 0x41, 0x66, 0xFA,
        0x41, 0xFA, 0xFE, 0xF4, 0x30, 0x80, 0x11, 0xC0, 0xFF, 0xDF,
        0x4E, 0x72, 0x27, 0x00
};
/* Z80: LD B,0 / DJNZ $ (256 Runden), laeuft durch */
static unsigned char z80[] = { 0x06, 0x00, 0x10, 0xFE };

static unsigned char vor[40 + 15], nach[40 + 15], stz[8], bk[2];

int main()
{
        int r[16], i, e;
        unsigned seg;
        unsigned long p;
        long us;
        char z[80];

        gfget(vor, 0, 0, 40);
        gfget(vor + 40, 0, 0xF10E, 15);
        r[15] = GH_MITCOPRO;
        ghinfo(r);
        gputs("tab:");
        for (i = 0; i <= GH_UHR; i++) {
                gputs(" ");
                gputs(hex(r[i]));
        }
        gputs("\r\n");
        ghzeile(z, 0);
        gputs(z);
        if (r[GH_COPRO] == 2) {
                if (gcoblk(0x110, &seg))
                        gputs("gcoblk: kein Block\r\n");
                else {
                        gfput(seg, 0x100, m68, sizeof m68);
                        bk[0] = bk[1] = 0;
                        gfput(seg, 0, bk, 2);
                        p = (unsigned long)seg << 4;
                        stz[0] = 0; stz[1] = (p + 0x1000) >> 16;
                        stz[2] = (p + 0x1000) >> 8; stz[3] = p + 0x1000;
                        stz[4] = 0; stz[5] = (p + 0x100) >> 16;
                        stz[6] = (p + 0x100) >> 8; stz[7] = p + 0x100;
                        e = gcorun(2, gdsg(), (unsigned)stz, 8, &us);
                        gfget(bk, seg, 0, 2);
                        gputs("68008: ");
                        gputs(dez(e));
                        gputs(" Summe ");
                        gputs(dez((bk[0] << 8) | bk[1]));
                        gputs("\r\n");
                }
        }
        e = gcorun(1, gdsg(), (unsigned)z80, sizeof z80, &us);
        gputs("Z80: ");
        gputs(dez(e));
        gputs("\r\n");
        gfget(nach, 0, 0, 40);
        gfget(nach + 40, 0, 0xF10E, 15);
        for (i = 0; i < 55 && vor[i] == nach[i]; i++)
                ;
        gputs(i == 55 ? "Vektoren und Warteplatz gleich\r\n"
                      : "Vektoren oder Warteplatz VERAENDERT\r\n");
        return 0;
}

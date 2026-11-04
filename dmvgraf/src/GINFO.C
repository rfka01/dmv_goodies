/* GINFO.C - Hardwareauskunft und Koprozessor fuer DMVGRAF unter MS-DOS.
 *
 * Die Arbeit tut der Kern aus src/GHW.INC (GHW86.S, erzeugt von
 * tools/gpicbin.py), derselbe, den DMVDEMO und DMVRACE aus MASM einbinden;
 * hier nur die Aufrufe. Regeln: README.md, "Hardware erkennen".
 *
 *   ghinfo(r)   r[0..10] nach GH_ROM ... GH_UHR; r[15] vorher:
 *               GH_MITCOPRO (Koprozessor probieren, nur K235) und
 *               GH_MITUHR (K803, dauert 1,2 s)
 *   ghcpu()     kurz: GH_CPU | GH_FPU << 2 | GH_KARTE << 4
 *   ghzeile(p, engl)  "Erkannt: K235 mit V20, 8087, K234 (68008)" nach p
 *               (80 Byte), englisch mit engl = 1; nach ghinfo
 *   gcorun(art, seg, start, len, us)   Koprozessor laufen lassen (K235)
 *   gcoblk(abs, &seg)  DOS-Block fuer den Koprozessor
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

extern int ghk(), ghkcf, gfput(), gfget(), gdos();
extern int gt0per();
extern char ghkern[];

#define K_INFO   14
#define K_CPUK   6
#define K_COPRO  15
#define K_CORUN  16
#define K_CORZ   17
#define K_COBLK  19
#define K_ZEILE  20

static unsigned qsv, qsvp;      /* Sicherung fuer Bank 0: Segment, Absaetze */

static int *ktab()              /* gh_tab im Kern */
{
        return (int *)(ghkern + ((int *)ghkern)[1]);
}

int ghcpu()
{
        return ghk(K_CPUK, 0, 0, 0, 0);
}

int ghinfo(r)
int *r;
{
        int i, *t;

        qstart();
        ghk(K_INFO, r[15], 0, 0, 0);
        t = ktab();
        for (i = 0; i < 15; i++)
                r[i] = t[i];
        r[15] = 0;
        return 0;
}

int ghzeile(p, engl)
char *p;
int engl;
{
        char *q;

        ghk(K_ZEILE, engl ? 1 : 0, 0, 0, 0);
        q = ghkern + ((int *)ghkern)[4];
        while (*q != '$')
                *p++ = *q++;
        *p = 0;
        return 0;
}

/* Programm des Koprozessors laufen lassen (nur K235). art 1 Z80: len
   Byte Code ab seg:start; er wird in Bank 0 unter das Programm gelegt
   (2000h-F100h), laeuft bis zu seinem Ende durch und wird danach wieder
   entfernt. art 2 68008 (K234), 3 Z8001 (K238): ab seg:start 8 Byte
   Startzustand, die nach 0:0000 kommen (68008: SSP und PC, Big Endian;
   Z8001: 0, FCW, PC); das Programm gibt mit move.b d0,$FFDF.w bzw. OUTB
   DFh zurueck. Der 8088 steht so lange, die Unterbrechungen sind
   gesperrt. *us: Laufzeit ueber Kanal 0 (ein Bild genau). 0 gut, 1 kein
   Koprozessorweg (keine K235, kein Platz, Warteplatz belegt), 2 der Z80
   kam nicht zurueck. */
int gcorun(art, seg, start, len, us)
int art;
unsigned seg, start, len;
long *us;
{
        int r, per, *t;
        unsigned char *z;
        struct dosr { unsigned ax, bx, cx, dx; } d;

        if (us)
                *us = 0;
        if (art < 1 || art > 3)
                return 1;
        per = us ? gt0per() : 0;
        if (art == 1) {
                if (len == 0 || len > 0xC000)
                        return 1;
                if (qsvp < ((len + 8 + 15) >> 4)) {     /* bleibt */
                        d.ax = 0x4800;
                        d.bx = (len + 8 + 15) >> 4;
                        if (gdos(&d) & 1)
                                return 1;
                        qsv = d.ax;
                        qsvp = d.bx;
                }
                r = ghk(K_CORZ, seg, qsv, len, start);
        } else {
                z = (unsigned char *)(ghkern + ((int *)ghkern)[3]);
                gfget(z, seg, start, 8);
                r = ghk(K_CORUN, 0, 0, 8, ((int *)ghkern)[3]);
        }
        if (us && !r) {
                t = (int *)(ghkern + ((int *)ghkern)[2]);
                *us = (long)((t[5] - t[6]) & 0xFFFF) * per;
        }
        return r;
}

int gcoblk(abs, seg)
unsigned abs, *seg;
{
        int r;

        r = ghk(K_COBLK, 0, abs, 0, 0);
        if (ghkcf)
                return 1;
        *seg = r;
        return 0;
}

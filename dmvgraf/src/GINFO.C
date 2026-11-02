/* GINFO.C - Hardwareauskunft und Koprozessor fuer DMVGRAF unter MS-DOS.
 *
 * Nach "NCR Decision Mate V - Hardware erkennen" (HWA, 2.11.2026). Jeder
 * Wert 0 heisst "unbekannt" oder "nicht da", nie geraten.
 *
 *   ghinfo(r)   r[0..10] nach GH_ROM ... GH_UHR fuellen; r[15] vorher:
 *               GH_MITCOPRO (Koprozessor probieren, nur K235) und
 *               GH_MITUHR (K803, dauert 1,2 s)
 *   ghcpu()     kurz: GH_CPU | GH_FPU << 2 | GH_KARTE << 4
 *   gcorun(art, seg, start, len, us)   Koprozessor laufen lassen (K235)
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

extern int gcli(), gfl(), gpit(), gcpu(), gfpu(), gaam(), gaam0();
extern int grom(), gdsg(), inb(), outb(), gfput(), gfget(), gdos();
extern int gtinit(), gt0per(), gtbild();
extern long gtus();
extern int ghschirm;
extern int gpsp;

/* ---- Firmware ---- */

static unsigned long qcrc(c, p, n)     /* CRC-32 (wie zip), fortgesetzt */
unsigned long c;
unsigned char *p;
int n;
{
        int k;

        while (n-- > 0) {
                c ^= *p++;
                for (k = 0; k < 8; k++)
                        c = (c & 1) ? (c >> 1) ^ 0xEDB88320L : c >> 1;
        }
        return c;
}

static int qrom()
{
        static unsigned char b[256];
        unsigned long c;
        int v;

        c = 0xFFFFFFFFL;
        for (v = 0; v < 0x2000; v += 256) {
                grom(b, v, 256);
                c = qcrc(c, b, 256);
        }
        c = ~c;
        if (c == 0x120951B6L)
                return 1;               /* M.07.00 (33609) */
        if (c == 0xBF25F3F0L)
                return 2;               /* C.07.00 (33610) */
        grom(b, 0x0FF7, 9);             /* Zaehlbyte 08h, " M.05.00" */
        if (b[0] != 8 || b[4] != '.' || b[5] != '0' || b[7] != '.')
                return 0;
        if (b[2] == 'M' && b[6] == '5') return 3;
        if (b[2] == 'M' && b[6] == '6') return 4;
        if (b[2] == 'C' && b[6] == '6') return 5;
        return 0;
}

/* ---- Prozessoren ---- */

static int qkarte()             /* 1 K235, 2 K230/K231, 0 keine */
{
        int f, alt, a, b;

        if (inb(0x13) & 2)              /* Bit 1 negiert: keine Karte */
                return 0;
        f = gcli();
        alt = inb(0x91);                /* Maske des 8259 */
        outb(0x91, 0xAA);
        a = inb(0x91);
        outb(0x91, 0x55);
        b = inb(0x91);
        outb(0x91, alt);
        gfl(f);
        return a == 0xAA && b == 0x55 ? 1 : 2;
}

static int qcpu()               /* 1 8088, 2 V20, 3 widerspruechlich */
{
        int c;

        c = gcpu();
        return c == 3 ? 2 : c == 0 ? 1 : 3;
}

static int qfpu()               /* 0 keiner, 1 8087, 2 anderer */
{
        unsigned w[2];

        gfpu(w);
        if (w[1] == 0x5A5A)
                return 0;
        return w[1] == 0x03FF ? 1 : 2;
}

int ghcpu()
{
        return qcpu() | (qfpu() << 2) | (qkarte() << 4);
}

/* Takt in 10 kHz: 1600 x AAM gegen Kanal 2, leere Schleife abziehen.
   AAM braucht auf dem V20 15 Takte, auf dem 8088 vermutlich 77. */
static int qtakt(cpu)
int cpu;
{
        long t0, t1, t2;
        int f, i, z;

        if (cpu != 1 && cpu != 2)
                return 0;
        gtinit();
        f = gcli();
        t0 = gtus();
        for (i = 0; i < 100; i++) {
                gaam(16);
                gtus();
        }
        t1 = gtus();
        for (i = 0; i < 100; i++) {
                gaam0(16);
                gtus();
        }
        t2 = gtus();
        gfl(f);
        t1 = (t1 - t0) - (t2 - t1);
        if (t1 <= 0)
                return 0;
        z = cpu == 2 ? 15 : 77;
        return (int)(1600L * z * 100L / t1);
}

/* ---- Speicher, Laufwerk, Uhr ---- */

static int qbaenke()            /* zusammenhaengende Baenke ab 1 */
{
        unsigned alt[8][2], w[2], m[2], ds;
        int f, n, k;

        ds = gdsg();
        f = gcli();
        for (n = 1; n <= 7; n++)
                gfget(alt[n], n << 12, 0xFFF0, 4);
        for (n = 1; n <= 7; n++) {
                m[0] = 0xA500 | n;
                m[1] = ~m[0];
                if (ds != (unsigned)(n << 12))  /* nie in den eigenen Stapel */
                        gfput(n << 12, 0xFFF0, m, 4);
        }
        k = 0;
        for (n = 1; n <= 7; n++) {
                gfget(w, n << 12, 0xFFF0, 4);
                if (ds != (unsigned)(n << 12) &&
                    (w[0] != (0xA500 | n) || w[1] != (unsigned)~(0xA500 | n)))
                        break;
                k = n;
        }
        for (n = 7; n >= 1; n--)
                if (ds != (unsigned)(n << 12))
                        gfput(n << 12, 0xFFF0, alt[n], 4);
        gfl(f);
        return k;
}

static int qpeek(a)             /* Byte bei 0:a */
unsigned a;
{
        unsigned char b[2];

        gfget(b, 0, a, 1);
        return b[0];
}

static int qbcd(v)
int v;
{
        return (v & 15) <= 9 && (v >> 4) <= 5;
}

static int quhr()               /* K803 auf 4B: Sekunden zweimal */
{
        int a, b;
        long t;

        outb(0xC8, 0);
        a = inb(0xCE);
        if (a == 0xCE || !qbcd(a))
                return 0;
        gtinit();
        t = gtus();
        while (gtus() - t < 1200000L)
                ;
        b = inb(0xCE);
        return qbcd(b) && b != a ? 1 : 0;
}

/* ---- Koprozessor ---- */

static int qkenn;               /* Byte bei 0:0000 nach dem Lauf */
static unsigned qsv, qsvp;      /* Sicherung fuer Bank 0: Segment, Absaetze */

/* Programm des Koprozessors laufen lassen (nur K235). art 1 Z80: len
   Byte Code ab seg:start, der Code endet mit OUT (D0h),A und JP F111h;
   er wird in Bank 0 unter das Programm gelegt und danach wieder
   entfernt. art 2 68008 (K234), 3 Z8001 (K238): ab seg:start 8 Byte
   Startblock (68008: SSP und PC, Big Endian), die nach 0:0000 kommen;
   das Programm gibt mit einem Schreibzugriff auf FFFDFh zurueck. Der
   8088 steht so lange, die Unterbrechungen sind gesperrt. *us: Laufzeit
   ueber Kanal 0 (Aufloesung ein Bild). 0 gut, 1 kein Koprozessorweg
   (keine K235, kein Platz), 2 Z80 kam nicht zurueck. */
int gcorun(art, seg, start, len, us)
int art;
unsigned seg, start, len;
long *us;
{
        static unsigned char vek[8], stub[16], b[256], neu[16];
        unsigned c0, c1, platz, n, k, sv;
        long lo, hi, pl;
        int f, i, r, per;
        struct dosr { unsigned ax, bx, cx, dx; } d;

        if (us)
                *us = 0;
        if (art < 1 || art > 3 || qkarte() != 1)
                return 1;
        per = gt0per();
        platz = 0;
        sv = 0;
        if (art == 1) {                 /* Platz in Bank 0 suchen */
                if (len == 0 || len > 0xD000)
                        return 1;
                lo = (long)gpsp << 4;
                hi = ((long)gdsg() << 4) + 0x10000L;
                pl = (0xF100L - len) & ~0xFFL;
                if (pl + len > lo && pl < hi)
                        pl = (lo - len) & ~0xFFL;
                if (pl < 0x2000L)
                        return 1;
                platz = (unsigned)pl;
                if (qsvp < ((len + 15) >> 4)) { /* Sicherung fuer Bank 0, bleibt */
                        d.ax = 0x4800;
                        d.bx = (len + 15) >> 4;
                        if (gdos(&d) & 1)
                                return 1;
                        qsv = d.ax;
                        qsvp = d.bx;
                }
                sv = qsv;
        }
        f = gcli();
        gfget(vek, 0, 0, 8);
        gfget(stub, 0, 0xF111, 16);
        if (art == 1) {
                for (k = 0; k < len; k += n) {
                        n = len - k > 256 ? 256 : len - k;
                        gfget(b, 0, platz + k, n);
                        gfput(sv, k, b, n);
                        gfget(b, seg, start + k, n);
                        gfput(0, platz + k, b, n);
                }
                i = 0;                  /* LD A,5Ah; LD (F11Fh),A; JP platz */
                neu[i++] = 0x3E; neu[i++] = 0x5A;
                neu[i++] = 0x32; neu[i++] = 0x1F; neu[i++] = 0xF1;
                neu[i++] = 0xC3; neu[i++] = platz & 0xFF; neu[i++] = platz >> 8;
        } else {
                gfget(b, seg, start, 8);
                gfput(0, 0, b, 8);
                i = 0;                  /* OUT (DFh),A; LD A,5Ah; LD (F11Fh),A; */
                neu[i++] = 0xD3; neu[i++] = 0xDF;   /* OUT (D0h),A; JP F111h */
                neu[i++] = 0x3E; neu[i++] = 0x5A;
                neu[i++] = 0x32; neu[i++] = 0x1F; neu[i++] = 0xF1;
                neu[i++] = 0xD3; neu[i++] = 0xD0;
                neu[i++] = 0xC3; neu[i++] = 0x11; neu[i++] = 0xF1;
        }
        while (i < 15)
                neu[i++] = 0;
        neu[14] = 0;                    /* F11Fh: Marke */
        gfput(0, 0xF111, neu, 15);
        c0 = gpit(0);
        outb(0xD0, 0);                  /* Bus an den Z80; der 8088 steht */
        c1 = gpit(0);
        gfget(b, 0, 0xF11F, 1);
        r = b[0] == 0x5A ? 0 : 2;
        gfget(b, 0, 0, 1);
        qkenn = b[0];
        gfput(0, 0xF111, stub, 16);
        gfput(0, 0, vek, 8);
        if (art == 1)
                for (k = 0; k < len; k += n) {
                        n = len - k > 256 ? 256 : len - k;
                        gfget(b, sv, k, n);
                        gfput(0, platz + k, b, n);
                }
        gfl(f);
        if (us)
                *us = (long)((c0 - c1) & 0xFFFF) * per;
        return r;
}

/* K234 probieren: der 68008 schreibt 36h nach 0:0000 und gibt zurueck */
static int qcopro()
{
        static unsigned char p[48];
        unsigned long a;
        long us;
        int r;

        if (qkarte() != 1)
                return 0;
        a = ((unsigned long)gdsg() << 4) + (unsigned)p;
        r = 0;
        p[r++] = (a + 48) >> 24; p[r++] = (a + 48) >> 16;      /* SSP */
        p[r++] = (a + 48) >> 8;  p[r++] = a + 48;
        p[r++] = (a + 8) >> 24;  p[r++] = (a + 8) >> 16;       /* PC */
        p[r++] = (a + 8) >> 8;   p[r++] = a + 8;
        p[r++] = 0x11; p[r++] = 0xFC; p[r++] = 0x00; p[r++] = 0x36; /* move.b #$36,$0.w */
        p[r++] = 0x00; p[r++] = 0x00;
        p[r++] = 0x11; p[r++] = 0xC0; p[r++] = 0xFF; p[r++] = 0xDF; /* move.b d0,$FFDF.w */
        p[r++] = 0x4E; p[r++] = 0x72; p[r++] = 0x27; p[r++] = 0x00; /* stop #$2700 */
        r = gcorun(2, gdsg(), (unsigned)p, 8, &us);
        if (r == 2)
                return 4;
        if (r)
                return 0;
        return qkenn == 0x36 ? 2 : qkenn == 0x5A ? 3 : 1;
}

int ghinfo(r)
int *r;
{
        int i, was;

        was = r[15];
        for (i = 0; i < 16; i++)
                r[i] = 0;
        qstart();
        r[GH_ROM] = qrom();
        r[GH_SCHIRM] = ghschirm;
        r[GH_KARTE] = qkarte();
        r[GH_CPU] = qcpu();
        r[GH_FPU] = qfpu();
        if ((was & GH_MITCOPRO) && r[GH_KARTE] == 1)
                r[GH_COPRO] = qcopro();
        r[GH_BAENKE] = qbaenke();
        i = qpeek(0xFE08);
        r[GH_SPUR_A] = i == 0 ? 40 : i == 1 ? 80 : 0;
        r[GH_TAKT] = qtakt(r[GH_CPU]);
        i = gtbild();
        r[GH_BILD] = i ? (int)(100000000L / i) : 0;
        if (was & GH_MITUHR)
                r[GH_UHR] = quhr();
        return 0;
}

/* GTEST.C - Pruef- und Vorfuehrprogramm fuer DMVGRAF.
 *
 * Drei Bilder: Figuren (Linien, Rechtecke, Flaechen mit allen Mustern,
 * Kreise, Boegen), Text (Richtungen, Zoom, Betriebsarten) und Bilder
 * (alle Formate, Arten, Zoom, GBGET, Zusammensetzen). Weiter mit einer
 * Taste; im dritten Bild folgt ein Zeiger der Maus, ESC beendet.
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "DMVGRAF.H"

static int warte()
{
        int k;

        while ((k = gkey()) == 0)
                gwait(1);
        return k;
}

static char *zahl(v)
int v;
{
        static char b[8];
        int i;

        i = 7;
        b[i] = 0;
        do {
                b[--i] = '0' + v % 10;
                v /= 10;
        } while (v && i > 0);
        return &b[i];
}

static int titel(s)
char *s;
{
        gmode(0);
        gcolor(7);
        gzoom(0);
        gpoint(0, 399);
        gorig(1);
        gpoint(8, 0);
        gtext(s, 0);
        gorig(0);
        return 0;
}

/* ---- Bild 1: Figuren ---- */
static int figuren()
{
        int i, x, y;

        gclear();
        titel("DMVGRAF - Figuren: Linien, Rechtecke, Fl\204chen, Kreise, B\224gen");
        gmode(3);
        for (i = 0; i < 16; i++) {          /* Faecher */
                gcolor(1 + i % 7);
                gpoint(120, 200);
                gline(120 + (i < 8 ? 100 : -100 + (i - 8) * 25),
                      200 + (i < 8 ? -100 + i * 25 : 100));
        }
        gcolor(7);
        for (i = 0; i < 8; i++) {           /* Rechtecke in acht Richtungen */
                gpoint(330, 200);
                gcolor(1 + i % 7);
                grect(60, 20, i);
        }
        for (i = 0; i < 16; i++) {          /* Muster */
                x = 440 + (i % 4) * 48;
                y = 250 + (i / 4) * 30;
                gpatt(i);
                gcolor(1 + i % 7);
                gpoint(x, y);
                gbox(40, 24, 0);
                gpatt(0);
                gcolor(7);
                grect(40, 24, 0);
        }
        gpatt(0);
        for (i = 1; i <= 5; i++) {          /* Kreise */
                gcolor(i);
                gpoint(120, 80);
                gcircl(i * 12);
        }
        gcolor(6);                          /* Boegen */
        gpoint(330, 70);
        garc(50, 0, 90);
        gcolor(5);
        garc(40, 90, 270);
        gcolor(3);
        garc(30, 300, 30);
        gpatt(1);                           /* gestrichelt */
        gcolor(7);
        gpoint(330, 70);
        gcircl(60);
        gpatt(0);
        gzoom(3);                           /* Flaeche mit Zoom */
        gpatt(9);
        gcolor(2);
        gpoint(440, 40);
        gbox(40, 12, 0);
        gzoom(0);
        gpatt(0);
        gmode(1);                           /* XOR ueber alles */
        gcolor(4);
        gpoint(200, 150);
        gbox(260, 40, 0);
        return 0;
}

/* ---- Bild 2: Text ---- */
static int texte()
{
        int i;

        gclear();
        titel("DMVGRAF - Text in acht Richtungen, Zoom, Betriebsarten");
        gmode(3);
        for (i = 1; i < 8; i++) {
                gcolor(i);
                gpoint(160, 200);
                gtext("  Richtung", i);
        }
        gcolor(7);
        gpoint(160, 200);
        gtext("  Richtung 0", 0);
        gzoom(1);
        gcolor(6);
        gpoint(330, 300);
        gtext("Zoom 1", 0);
        gzoom(2);
        gcolor(3);
        gpoint(330, 240);
        gtext("Zoom 2", 0);
        gzoom(1);
        gcolor(5);
        gpoint(600, 120);
        gtext("senkrecht", 2);
        gzoom(0);
        gcolor(2);                          /* Betriebsarten ueber Flaeche */
        gpoint(330, 40);
        gbox(280, 100, 0);
        for (i = 0; i < 4; i++) {
                gmode(i);
                gcolor(i == 3 ? 4 : 7);
                gpoint(340, 120 - i * 22);
                gtext("Modus ", 0);
                gtext(zahl(i), 0);
                gtext(i == 0 ? " ersetzen" : i == 1 ? " umkehren"
                      : i == 2 ? " schwarz" : " Farbe (rot)", 0);
        }
        gmode(3);
        gcolor(7);
        gpoint(20, 20);
        gtext("Umlaute: \204\224\201\216\231\232\341", 0);
        return 0;
}

/* ---- Bild 3: Bilder ---- */

/* Zeiger 16 x 16 im Format 4: je Zeile Maske, Gruen, Rot, Blau */
static char zeig[16 * 4 * 2];
static int zkern[16] = {
        0x0000, 0x0002, 0x0006, 0x000E, 0x001E, 0x003E, 0x007E, 0x00FE,
        0x01FE, 0x003E, 0x0036, 0x0062, 0x0060, 0x00C0, 0x00C0, 0x0000 };
static int zrand[16] = {
        0x0003, 0x0005, 0x0009, 0x0011, 0x0021, 0x0041, 0x0081, 0x0101,
        0x0201, 0x03C1, 0x0049, 0x0095, 0x0091, 0x0120, 0x0120, 0x00C0 };

/* Hopplers Frosch (SprLibF) im Format 5 */
static int frosch[3 * 2 * 16] = {
        0x0000,0x0000,0x0540,0x0FF0,0x0BD0,0x0FF0,0x6FF6,0x2BD4,
        0x2C34,0x0FF0,0x0560,0x07E0,0x0140,0x0000,0x0000,0x0000,
        0x1818,0x0FF0,0x0AB0,0x1008,0x342C,0xF00F,0x9009,0x542A,
        0x53CA,0x300C,0x1A98,0x0810,0x06A0,0x0BD0,0x1248,0x366C,
        0x0000,0x0000,0x0000,0x0E70,0x0A50,0x0E70,0x0000,0x0000,
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x1818,0x0FF0,0x0FF0,0x1188,0x35AC,0xF18F,0xFFFF,0x7FFE,
        0x7FFE,0x3FFC,0x1FF8,0x0FF0,0x07E0,0x0BD0,0x1248,0x366C,
        0x0000,0x0000,0x0000,0x0E70,0x2E74,0xEE77,0xE007,0x6006,
        0x6006,0x2004,0x1008,0x0000,0x0000,0x0000,0x0000,0x0000,
        0x1818,0x0FF0,0x0FF0,0x1188,0x1188,0x1188,0x1FF8,0x1FF8,
        0x1FF8,0x1FF8,0x0FF0,0x0FF0,0x07E0,0x0BD0,0x1248,0x366C };

/* Wagen 16 x 8, Format 1 (Bit 7 links) wie DMVRACE */
static char wagen[16] = {
        0x07, 0xE0, 0x0F, 0xF0, 0x1C, 0x38, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0x38, 0x1C, 0x10, 0x08 };

/* Format 3: 32 x 24, die 16 Farbnummern als Streifen, Rand durchsichtig */
static char streif[32 / 2 * 24];

static int bilder()
{
        int i, k, x, y, r[6];

        gclear();
        titel("DMVGRAF - Bilder: Formate 1/3/4/5, Arten, Zoom, GBGET, Ebenen");
        for (y = 0; y < 16; y++)
                for (i = 0; i < 2; i++) {
                        k = (y * 4 + 0) * 2 + i;
                        zeig[k] = ((zkern[y] | zrand[y]) >> (8 * i)) & 0xFF;
                        zeig[k + 2] = zeig[k + 4] = zeig[k + 6]
                                = (zrand[y] >> (8 * i)) & 0xFF;
                }
        for (y = 0; y < 24; y++)
                for (x = 0; x < 16; x++) {
                        i = (y < 2 || y > 21) ? 0 : x;
                        streif[y * 16 + x] = (i << 4) | i;
                }
        gbdef(0, 16, 16, GB_EBN, zeig);
        gbdef(1, 16, 16, GB_SETR, (char *)frosch);
        gbdef(2, 16, 8, GB_BIT7, wagen);
        gbdef(3, 32, 24, GB_NIB, streif);
        gmode(3);
        gcolor(2);                          /* Hintergrund fuer die Arten */
        gpoint(20, 220);
        gbox(600, 150, 0);
        for (k = 0; k < 4; k++) {
                gpoint(40 + k * 150, 300);
                gcolor(7);
                gmode(0);
                gtext(k == 0 ? "deckend" : k == 1 ? "durchsichtig"
                      : k == 2 ? "Schatten" : "XOR", 0);
                for (i = 1; i <= 3; i++) {
                        gcolor(i == 2 ? 6 : 4);
                        gpoint(40 + k * 150 + (i - 1) * 40, 250);
                        gbput(i, k);
                }
                gpoint(40 + k * 150, 228);
                gzoom(1);
                gbput(1, k);
                gzoom(0);
        }
        /* Zoom 3 und GBGET: Frosch vergroessert, dann kopiert */
        gpoint(20, 120);
        gzoom(3);
        gbput(1, GB_DURCH);
        gzoom(0);
        gpoint(20, 120);
        gbget(4, 64, 64);
        gpoint(100, 120);
        gbput(4, GB_DECK);
        gbinfo(1, r);
        gpoint(180, 160);
        gmode(3);
        gcolor(7);
        gtext("Frosch sichtbar ", 0);
        gtext(zahl(r[2]), 0); gtext(",", 0); gtext(zahl(r[3]), 0);
        gtext("-", 0);
        gtext(zahl(r[4]), 0); gtext(",", 0); gtext(zahl(r[5]), 0);
        /* Zusammensetzen: zwoelf Streifenbilder ueberlappend + Zeiger */
        glayer(180, 20, 639, 140);
        for (i = 0; i < 12; i++)
                glput(3, 200 + i * 20 + (i & 1) * 4, 40 + (i % 3) * 12,
                      i == 5 ? GB_SCHAT : GB_DURCH);
        glput(1, 470, 90, GB_DURCH);
        glput(0, 480, 70, GB_DURCH);
        gldraw();
        return 0;
}

static int zeiger()                     /* Maus bis ESC */
{
        int x, y, k, ax, ay, t;

        ax = ay = -1;
        for (;;) {
                if (mpos(&x, &y, &k) && (x != ax || y != ay)) {
                        if (ax >= 0) {          /* alten Zeiger weg (XOR) */
                                gpoint(ax, ay);
                                gbput(0, GB_XOR);
                        }
                        gpoint(x, y);
                        gbput(0, GB_XOR);
                        ax = x;
                        ay = y;
                }
                t = gkey();
                if (t == 27)
                        return 0;
                if (!t)
                        gwait(1);
        }
}

int main()
{
        int f, m, s, c;

        if (ginit()) {
                gputs("Keine DMV-Grafik (DMV-BIOS-Block) gefunden.\n");
                return 1;
        }
        ginfo(&f, &m, &s, &c);
        figuren();
        if (warte() == 27)
                return gexit();
        texte();
        if (warte() == 27)
                return gexit();
        bilder();
        if (m)
                zeiger();
        else
                warte();
        gexit();
        gputs("GTEST Ende.\n");
        return 0;
}

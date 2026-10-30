/* GKERN.C - DMVGRAF: Kern in C (Simulation, CP/M-68K). Unter CP/M-8000
 * steht dasselbe in Z8000-Assembler in GZ8K.8KN.
 *
 *   gkopi(d, s, n)          n Byte kopieren
 *   gnull(d, n)             n Byte loeschen
 *   gnib(z, q, tb, n)       n Gruppen zu vier Byte Format 3 (acht Punkte)
 *                           ueber die Tafel tb (je Byte: Bit 0/1 Ebene 0,
 *                           2/3 Ebene 1, 4/5 Ebene 2, 6/7 sichtbar, je
 *                           linker/rechter Punkt) nach z[k] sichtbar,
 *                           z[80 + k], z[160 + k], z[240 + k] Ebenen 0-2
 *   gzeile(d, z, n, sh, cnt, ns, c01, c23, eb)
 *                           eine Bildzeile (z: Maske, im Abstand n die drei
 *                           Ebenen) um sh Bit (0-7; Bit 0 = linker Punkt,
 *                           also nach rechts auf dem Schirm) versetzt mit
 *                           Maske in cnt Bandbytes ab d einsetzen:
 *                           d[j + e * eb] = (d & ~m) | (Ebene e & m);
 *                           ns Quellbytes, danach Nullen; c01, c23 die
 *                           Uebertraege aus dem Byte vor z (hoch/niedrig:
 *                           Maske/Ebene 0, Ebene 1/Ebene 2)
 *
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

int gkopi(d, s, n)
register char *d, *s;
register int n;
{
        while (n-- > 0)
                *d++ = *s++;
        return 0;
}

int gnull(d, n)
register char *d;
register int n;
{
        while (n-- > 0)
                *d++ = 0;
        return 0;
}

int gzeile(d, z, n, sh, cnt, ns, c01, c23, eb)
char *d, *z;
int n, sh, cnt, ns, c01, c23, eb;
{
        register int j, w, mm;
        int cm, c0, c1, c2, v;

        cm = (c01 >> 8) & 0xFF;
        c0 = c01 & 0xFF;
        c1 = (c23 >> 8) & 0xFF;
        c2 = c23 & 0xFF;
        for (j = 0; j < cnt; j++) {
                v = j < ns ? z[j] & 0xFF : 0;
                w = v << sh;
                mm = (w | cm) & 0xFF;
                cm = (w >> 8) & 0xFF;
                v = j < ns ? z[j + n] & 0xFF : 0;
                w = v << sh;
                v = (w | c0) & 0xFF;
                c0 = (w >> 8) & 0xFF;
                d[j] = (d[j] & ~mm) | (v & mm);
                v = j < ns ? z[j + 2 * n] & 0xFF : 0;
                w = v << sh;
                v = (w | c1) & 0xFF;
                c1 = (w >> 8) & 0xFF;
                d[j + eb] = (d[j + eb] & ~mm) | (v & mm);
                v = j < ns ? z[j + 3 * n] & 0xFF : 0;
                w = v << sh;
                v = (w | c2) & 0xFF;
                c2 = (w >> 8) & 0xFF;
                d[j + 2 * eb] = (d[j + 2 * eb] & ~mm) | (v & mm);
        }
        return 0;
}

int gnib(z, q, tb, n)
char *z, *q, *tb;
int n;
{
        register int w, j;
        int k, e0, e1, e2, f;

        for (k = 0; k < n; k++) {
                e0 = e1 = e2 = f = 0;
                for (j = 0; j < 8; j += 2) {
                        w = tb[*q++ & 0xFF] & 0xFF;
                        e0 |= (w & 3) << j;
                        e1 |= ((w >> 2) & 3) << j;
                        e2 |= ((w >> 4) & 3) << j;
                        f |= ((w >> 6) & 3) << j;
                }
                z[k] = f;
                z[80 + k] = e0;
                z[160 + k] = e1;
                z[240 + k] = e2;
        }
        return 0;
}

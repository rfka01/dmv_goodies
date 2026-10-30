/* simlib.c - die GDC-Nachbildung aus GHWSIM.C als Bibliothek fuer den
 * 68K-Pruefstand pruef68.py (ctypes). Bau: make simlib.so
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GHWSIM.C"
void w_init(int f) { farbe = f; graf = 0; }
void w_graf(int an) { ghgraf(an); }
void w_liste(unsigned char *l) { if (graf) liste(l, 0, 0); }
int w_rdat(unsigned char *l, unsigned char *aus) { int nl = 0; if (graf) liste(l, aus, &nl); return nl; }
void w_bild(char *n) { abzug(n); }

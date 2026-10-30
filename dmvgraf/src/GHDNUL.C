/* GHDNUL.C - DMVGRAF: kein direkter Weg zum 7220 (CP/M-68K, bis dort
 * geklaert ist, ob die K234-Anschluesse aus dem Benutzermodus erreichbar
 * sind); alles laeuft ueber BIOS 66/67.
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "GINTERN.H"

int ghdini() { return 0; }
int ghdlst(l) char *l; { return 0; }
int ghdrd(b) char *b; { return 1; }

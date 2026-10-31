/* DEMO.C - DMVGRAF aus C (alle drei Systeme): Linie von (100,50) nach
 * (500,300), dann auf eine Taste warten.
 * Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
 */
#include "DMVGRAF.H"

int main()
{
        ginit();
        gpoint(100, 50);
        gline(500, 300);
        while (!gkey())
                ;
        gexit();
        return 0;
}

program GTestZ;
{ GTESTZ.PAS - GTEST fuer Turbo Pascal 3 unter CP/M-80.
  Dasselbe wie dmvdiag/grafik/GTEST.C: drei Bilder (Figuren, Text, Bilder), weiter
  mit einer Taste; im dritten Bild folgt ein Zeiger der Maus, ESC beendet.

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

{$I DMVHWC.INC}
{$I DMVFONT.INC}
{$I DMVGRAF.INC}
const BKMAX = 5;                    { Bilder 0-4 }
{$I DMVBKL.INC}
{$I DMVFIG.INC}
{$I DMVMAUS.INC}
const STRN = 0;                     { Format 3 fehlt in DMVBKL.INC }
{$I GTESTK.INC}

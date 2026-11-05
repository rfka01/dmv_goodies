program GTest;
{ GTEST.PAS - Pruef- und Vorfuehrprogramm fuer DMVGRAF, Turbo Pascal 3.
  Dasselbe wie dmvdiag/grafik/GTEST.C: drei Bilder (Figuren, Text, Bilder), weiter
  mit einer Taste; im dritten Bild folgt ein Zeiger der Maus, ESC beendet.

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

{$I DMVHWD.INC}
{$I DMVFONT.INC}
{$I DMVGRAF.INC}
{$I DMVBILD.INC}
{$I DMVFIG.INC}
{$I DMVMAUS.INC}
const STRN = 383;                   { Streifenbild (Format 3) }
{$I GTESTK.INC}

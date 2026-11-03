program PICTEST;
{ PICTEST.PAS - gepackte Vollbilder aus Turbo Pascal 3 (MS-DOS), wie
  test/PICTEST.C:  PICTEST datei xw y art [datei xw y art ...]
  art mit angehaengtem S: erst in den Speicher lesen (bis 32000 Byte), gpicm.
  Taste beendet, danach "gpic: n ..." (0 gut, sonst Fehlernummer).
  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }
{$I DMVHWD.INC}
{$I DMVFONT.INC}
{$I DMVGRAF.INC}
const BKMAX = 1;
{$I DMVBKL.INC}
{$I DMVPIC.INC}
var sp: array[0..31999] of byte;
    r: array[1..8] of integer;
    i, k, xw, y, art, c, n: integer;
    a: GStr;
    f: file;
begin
  gstart; ginit;
  if gerr <> 0 then halt;
  i := 1; k := 0;
  while (i + 3 <= ParamCount) and (k < 8) do begin
    Val(ParamStr(i + 1), xw, c);
    Val(ParamStr(i + 2), y, c);
    a := ParamStr(i + 3);
    n := 0;
    while (n < Length(a)) and (a[n + 1] in ['0'..'9']) do n := n + 1;
    Val(Copy(a, 1, n), art, c);
    k := k + 1;
    if (n < Length(a)) and (UpCase(a[n + 1]) = 'S') then begin
      Assign(f, ParamStr(i));
      {$I-} Reset(f); {$I+}
      n := 0;
      if IOResult = 0 then begin
        c := 0;                    { letzter Satz kann kurz sein }
        while (c = 0) and (not Eof(f)) and (n < 250) do begin
          {$I-} BlockRead(f, sp[n * 128], 1); {$I+}
          c := IOResult; n := n + 1;
        end;
        {$I-} Close(f); {$I+} c := IOResult;
      end;
      gpicm(sp, n * 128, xw, y, art);
    end else
      gpic(ParamStr(i), xw, y, art);
    r[k] := gerr;
    i := i + 4;
  end;
  while gkey = 0 do gwait(1);
  gexit;
  Write('gpic:');
  for i := 1 to k do Write(' ', r[i]);
  WriteLn;
end.

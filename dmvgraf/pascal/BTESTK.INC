{ Hauptteil des Pascal-Pruefprogramms fuer den schnellen Weg der Bilder
  (wie test/BTEST.C): links mit, rechts ohne; beide Haelften gleich.
  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }
var d: array[0..2047] of byte; f: array[0..95] of integer;
    i, k, a, x, y, h: integer;
begin
  gstart; ginit;
  gorig(1);
  gmode(3);
  for i := 0 to 7 do begin gcolor(i); gpoint(0, i * 50); gbox(640, 50, 0); end;
  for i := 0 to 2047 do d[i] := (i * 37 + i shr 3) and $FF;
  for i := 0 to 95 do f[i] := (i * 1237) and $7777;
  gbdef(0, 24, 20, GB_EBN, d);
  gbdef(1, 32, 16, GB_EBN, d[100]);
  gbdef(2, 16, 16, GB_SETR, f);
  gbdef(3, 24, 12, GB_BIT0, d[300]);
  gbdef(4, 64, 47, GB_EBN, d[400]);
  for k := 0 to 1 do begin
    qbsch := k = 0;
    for a := 0 to 3 do
      if a <> 2 then
        for i := 0 to 4 do begin
          gcolor(1 + (i + a) mod 7);
          x := k * 320 + i * 64; y := 10 + a * 90;
          if i = 4 then y := y + 30;
          gpoint(x, y);
          gbput(i, a);
        end;
  end;
  qbsch := true;
  while gkey = 0 do gwait(1);
  gexit;
end.

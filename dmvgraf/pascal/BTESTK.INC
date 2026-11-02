{ Hauptteil des Pascal-Pruefprogramms fuer den schnellen Weg der Bilder
  (wie test/BTEST.C): links mit, rechts ohne; beide Haelften gleich.
  Bilder: Gesicht, Wagen, Herz (SET/RESET), Pfeil (1 Bit), Haus 64 x 47.
  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }
var d: array[0..2047] of byte; f: array[0..95] of integer;
    i, k, a, x, y, c, r, e: integer;

{ Punkt x,y im Format 4 ab d[o]; c: Bit 0 gruen, 1 rot, 2 blau }
procedure pix(o, n, x, y, c: integer);
var e, i: integer;
begin
  i := o + y * 4 * n + (x shr 3);
  for e := 0 to 3 do
    if (e = 0) or (((c shr (e - 1)) and 1) <> 0) then
      d[i + e * n] := d[i + e * n] or (1 shl (x and 7));
end;

function qd(a, b: integer): integer;
begin
  qd := a * a + b * b;
end;

begin
  gstart; ginit;
  gorig(1);
  gmode(3);
  for i := 0 to 7 do begin gcolor(i); gpoint(0, i * 50); gbox(640, 50, 0); end;
  FillChar(d, 2048, 0);
  FillChar(f, 192, 0);
  for y := 0 to 19 do                       { 0: Gesicht 24 x 20 }
    for x := 0 to 23 do begin
      r := qd(2 * x - 23, 2 * y - 19);
      if r < 440 then begin
        if r < 324 then c := 3 else c := 0;
        if ((x = 7) or (x = 8) or (x = 15) or (x = 16)) and (y >= 5) and (y <= 7) then
          c := 0;
        if ((y = 13) and ((x = 7) or (x = 16))) or ((y = 14) and (x >= 8) and (x <= 15)) then
          c := 2;
        pix(0, 3, x, y, c);
      end;
    end;
  for y := 0 to 15 do                       { 1: Wagen 32 x 16 }
    for x := 0 to 31 do begin
      c := -1;
      if (y >= 6) and (y <= 11) and (x >= 1) and (x <= 30) then c := 2;
      if (y >= 2) and (y <= 5) and (x >= 8) and (x <= 23) then
        if (y >= 3) and (((x >= 10) and (x <= 15)) or ((x >= 17) and (x <= 22))) then
          c := 4 else c := 2;
      if (x = 30) and ((y = 7) or (y = 8)) then c := 3;
      if (qd(x - 8, y - 12) < 10) or (qd(x - 23, y - 12) < 10) then
        if (y = 12) and ((x = 8) or (x = 23)) then c := 7 else c := 0;
      if c >= 0 then pix(240, 4, x, y, c);
    end;
  for y := 0 to 15 do                       { 2: Herz 16 x 16, SET/RESET }
    for x := 0 to 15 do begin
      c := 2 * x - 15; r := 2 * y;
      if (qd(c + 6, r - 9) <= 49) or (qd(c - 6, r - 9) <= 49) or
         ((r >= 9) and (Abs(c) * 19 <= 13 * (28 - r))) then
        for e := 0 to 2 do begin            { rot setzen, sonst loeschen }
          if e = 1 then k := 2 else k := 2 * e + 1;
          f[k * 16 + y] := f[k * 16 + y] or (1 shl x);
        end;
    end;
  for y := 0 to 11 do                       { 3: Pfeil 24 x 12, 1 Bit }
    for x := 0 to 23 do begin
      r := Abs(2 * y - 11);
      if ((x < 16) and (y >= 4) and (y <= 7)) or ((x >= 16) and (r <= 2 * (23 - x) + 1)) then
        d[496 + y * 3 + (x shr 3)] := d[496 + y * 3 + (x shr 3)] or (1 shl (x and 7));
    end;
  for y := 0 to 46 do                       { 4: Haus 64 x 47 }
    for x := 0 to 63 do begin
      c := -1;
      r := Abs(2 * x - 63);
      if (y >= 3) and (y <= 14) and (x >= 44) and (x <= 49) then c := 0;
      if (y < 18) and (r * 17 <= 136 + y * 56) then c := 2;
      if (y >= 18) and (x >= 6) and (x <= 57) then begin
        c := 7;
        if (y >= 22) and (y <= 31) and (((x >= 12) and (x <= 23)) or ((x >= 40) and (x <= 51))) then
          if (x = 17) or (x = 18) or (x = 45) or (x = 46) or (y = 26) or (y = 27) then
            c := 7 else c := 4;
        if (x >= 28) and (x <= 35) and (y >= 30) then
          if (x = 34) and (y = 38) then c := 3 else c := 1;
      end;
      if c >= 0 then pix(540, 8, x, y, c);
    end;
  gbdef(0, 24, 20, GB_EBN, d);
  gbdef(1, 32, 16, GB_EBN, d[240]);
  gbdef(2, 16, 16, GB_SETR, f);
  gbdef(3, 24, 12, GB_BIT0, d[496]);
  gbdef(4, 64, 47, GB_EBN, d[540]);
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
  gmode(3); gcolor(7);
  gpoint(8, 220); gtext('links: schneller Weg', 0);
  gpoint(328, 220); gtext('rechts: alter Weg', 0);
  gmode(2);
  gpoint(8, 370); gtext('Zeilen deckend, durchsichtig, XOR. Taste beendet.', 0);
  while gkey = 0 do gwait(1);
  gexit;
end.

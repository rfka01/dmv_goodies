{ Hauptteil des Pascal-Pruefprogramms fuer gsprdef/gspr, wie test/STEST.C 
  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }
const raute: array[0..23] of byte = (
  $18, $3C, $7E, $E7, $E7, $7E, $3C, $18,
  $18, $3C, $7E, $E7, $E7, $7E, $3C, $18,
  $00, $00, $00, $18, $18, $00, $00, $00);
var figur: array[0..95] of byte;
    i, k, x, y, a: integer;
    g, r, b: boolean;
begin
  gstart; ginit;
  if gerr <> 0 then halt;
  gorig(1); gmode(3);
  for i := 0 to 7 do begin
    gcolor(i); gpoint(0, 200 + i * 20); gbox(640, 20, 0);
  end;
  gcolor(7); gpoint(8, 4);
  gtext('DMVGRAF - Figuren ueber GCHRD: XOR, durchsichtig, deckend, Zoom', 0);
  FillChar(figur, 96, 0);
  for y := 0 to 15 do
    for x := 0 to 15 do begin
      k := y * 2 + (x shr 3);
      g := false; r := false; b := false;
      if (x - 8) * (x - 8) + (y - 8) * (y - 8) < 50 then begin
        g := y < 8; r := x >= 8; b := ((x + y) and 1) = 1;
      end;
      if (y = 15) or (x = 0) then begin g := true; r := true; b := true; end;
      if g then figur[k] := figur[k] or (1 shl (x and 7));
      if r then figur[32 + k] := figur[32 + k] or (1 shl (x and 7));
      if b then figur[64 + k] := figur[64 + k] or (1 shl (x and 7));
    end;
  gsprdef(0, 8, 8, raute);
  gsprdef(1, 16, 16, figur);
  for k := 0 to 2 do begin
    if k = 0 then a := GB_XOR else if k = 1 then a := GB_DURCH else a := GB_DECK;
    gspr(0, 20, 60 + k * 110, 0, a);
    gspr(0, 40, 60 + k * 110, 1, a);
    gspr(1, 80, 60 + k * 110, 0, a);
    gspr(1, 110, 60 + k * 110, 1, a);
    gspr(1, 160, 60 + k * 110, 3, a);
    gspr(1, 240, 150 + k * 80, 0, a);
    gspr(0, 280, 150 + k * 80, 2, a);
  end;
  gspr(1, 400, 60, 2, GB_XOR);              { zweimal XOR: wieder weg }
  gwait(50);
  gspr(1, 400, 60, 2, GB_XOR);
  gpoint(460, 76); gtext('XOR 2x: weg = gut', 0);
  gspr(1, 630, 60, 0, GB_DURCH);
  a := gerr;
  gpoint(400, 120);
  if a = GE_LAGE then gtext('Rand: GE_LAGE gut', 0) else gtext('Rand: FALSCH', 0);
  while gkey = 0 do gwait(1);
  gexit;
end.

{ Hauptteil des Pascal-Pruefprogramms fuer gsprdef/gsprset/gspr, wie test/STEST.C 
  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }
const raute: array[0..23] of byte = (
  $18, $3C, $7E, $E7, $E7, $7E, $3C, $18,
  $18, $3C, $7E, $E7, $E7, $7E, $3C, $18,
  $00, $00, $00, $18, $18, $00, $00, $00);
  { HOPPLERs Drachenlibelle (SprLibF) im Format 5 }
  libelle: array[0..95] of integer = (
    $0000,$0000,$0540,$0FF0,$0BD0,$0FF0,$6FF6,$2BD4,
    $2C34,$0FF0,$0560,$07E0,$0140,$0000,$0000,$0000,
    $1818,$0FF0,$0AB0,$1008,$342C,$F00F,$9009,$542A,
    $53CA,$300C,$1A98,$0810,$06A0,$0BD0,$1248,$366C,
    $0000,$0000,$0000,$0E70,$0A50,$0E70,$0000,$0000,
    $0000,$0000,$0000,$0000,$0000,$0000,$0000,$0000,
    $1818,$0FF0,$0FF0,$1188,$35AC,$F18F,$FFFF,$7FFE,
    $7FFE,$3FFC,$1FF8,$0FF0,$07E0,$0BD0,$1248,$366C,
    $0000,$0000,$0000,$0E70,$2E74,$EE77,$E007,$6006,
    $6006,$2004,$1008,$0000,$0000,$0000,$0000,$0000,
    $1818,$0FF0,$0FF0,$1188,$1188,$1188,$1FF8,$1FF8,
    $1FF8,$1FF8,$0FF0,$0FF0,$07E0,$0BD0,$1248,$366C);
var figur: array[0..95] of byte;
    i, k, x, y, a: integer;
    g, r, b: boolean;

{ Bild 2: Format 5, links gspr, rechts gbput; beide Haelften gleich }
procedure seite2;
var i, k, a, z, x, y: integer;
begin
  gclear;
  gmode(3);
  for i := 0 to 15 do begin gcolor(i and 7); gpoint(i * 40, 24); gbox(40, 376, 0); end;
  gmode(0); gcolor(7); gpoint(8, 4);
  gtext('gsprset: links gspr, rechts gbput (Format 5), gleich?', 0);
  gsprset(2, 16, 16, libelle);
  gbdef(0, 16, 16, GB_SETR, libelle);
  for k := 0 to 2 do begin
    if k = 0 then a := GB_DURCH else if k = 1 then a := GB_DECK else a := GB_XOR;
    for i := 0 to 3 do begin
      case i of
        0: begin z := 0; x := 20; end;
        1: begin z := 1; x := 52; end;
        2: begin z := 3; x := 108; end;
        3: begin z := 5; x := 196; end;
      end;
      y := 40 + k * 118;
      gspr(2, x, y, z, a);
      gpoint(320 + x, y); gzoom(z); gbput(0, a); gzoom(0);
    end;
  end;
  while gkey = 0 do gwait(1);
end;
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
  seite2;
  gexit;
end.

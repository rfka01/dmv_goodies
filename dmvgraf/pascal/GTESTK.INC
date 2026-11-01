{ GTESTK.INC - Hauptteil von GTEST (MS-DOS: GTEST.PAS, CP/M-80: GTESTZ.PAS).
  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }


const
  zkern: array[0..15] of integer = (
    $0000, $0002, $0006, $000E, $001E, $003E, $007E, $00FE,
    $01FE, $003E, $0036, $0062, $0060, $00C0, $00C0, $0000);
  zrand: array[0..15] of integer = (
    $0003, $0005, $0009, $0011, $0021, $0041, $0081, $0101,
    $0201, $03C1, $0049, $0095, $0091, $0120, $0120, $00C0);
  { Hopplers Frosch (SprLibF) im Format 5 }
  frosch: array[0..95] of integer = (
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
  { Wagen 16 x 8, Format 1 (Bit 7 links) wie DMVRACE }
  wagen: array[0..15] of byte = (
    $07, $E0, $0F, $F0, $1C, $38, $FF, $FF,
    $FF, $FF, $FF, $FF, $38, $1C, $10, $08);

var
  zeig: array[0..127] of byte;      { Zeiger 16 x 16 im Format 4 }
  streif: array[0..383] of byte;    { Format 3: 32 x 24 }
  f, m, s, c: integer;

function warte: integer;
var k: integer;
begin
  k := gkey;
  while k = 0 do begin gwait(1); k := gkey; end;
  warte := k;
end;

function zahl(v: integer): GStr;
var t: GStr;
begin
  Str(v, t);
  zahl := t;
end;

procedure titel(t: GStr);
begin
  gmode(0); gcolor(7); gzoom(0);
  gpoint(0, 399);
  gorig(1);
  gpoint(8, 0);
  gtext(t, 0);
  gorig(0);
end;

{ ---- Bild 1: Figuren ---- }
procedure figuren;
var i, x, y: integer;
begin
  gclear;
  titel('DMVGRAF - Figuren: Linien, Rechtecke, Fl'#132'chen, Kreise, B'#148'gen');
  gmode(3);
  for i := 0 to 15 do begin                 { Faecher }
    gcolor(1 + i mod 7);
    gpoint(120, 200);
    if i < 8 then gline(220, 100 + i * 25)
    else gline(20 + (i - 8) * 25, 300);
  end;
  gcolor(7);
  for i := 0 to 7 do begin                  { Rechtecke in acht Richtungen }
    gpoint(330, 200);
    gcolor(1 + i mod 7);
    grect(60, 20, i);
  end;
  for i := 0 to 15 do begin                 { Muster }
    x := 440 + (i mod 4) * 48;
    y := 250 + (i div 4) * 30;
    gpatt(i);
    gcolor(1 + i mod 7);
    gpoint(x, y);
    gbox(40, 24, 0);
    gpatt(0);
    gcolor(7);
    grect(40, 24, 0);
  end;
  gpatt(0);
  for i := 1 to 5 do begin                  { Kreise }
    gcolor(i);
    gpoint(120, 80);
    gcircl(i * 12);
  end;
  gcolor(6);                                { Boegen }
  gpoint(330, 70);
  garc(50, 0, 90);
  gcolor(5);
  garc(40, 90, 270);
  gcolor(3);
  garc(30, 300, 30);
  gpatt(1);                                 { gestrichelt }
  gcolor(7);
  gpoint(330, 70);
  gcircl(60);
  gpatt(0);
  gzoom(3);                                 { Flaeche mit Zoom }
  gpatt(9);
  gcolor(2);
  gpoint(440, 40);
  gbox(40, 12, 0);
  gzoom(0);
  gpatt(0);
  gmode(1);                                 { XOR ueber alles }
  gcolor(4);
  gpoint(200, 150);
  gbox(260, 40, 0);
end;

{ ---- Bild 2: Text ---- }
procedure texte;
var i: integer;
begin
  gclear;
  titel('DMVGRAF - Text in acht Richtungen, Zoom, Betriebsarten');
  gmode(3);
  for i := 1 to 7 do begin
    gcolor(i);
    gpoint(160, 200);
    gtext('  Richtung', i);
  end;
  gcolor(7);
  gpoint(160, 200);
  gtext('  Richtung 0', 0);
  gzoom(1); gcolor(6); gpoint(330, 300); gtext('Zoom 1', 0);
  gzoom(2); gcolor(3); gpoint(330, 240); gtext('Zoom 2', 0);
  gzoom(1); gcolor(5); gpoint(600, 120); gtext('senkrecht', 2);
  gzoom(0);
  gcolor(2);                                { Betriebsarten ueber Flaeche }
  gpoint(330, 40);
  gbox(280, 100, 0);
  for i := 0 to 3 do begin
    gmode(i);
    if i = 3 then gcolor(4) else gcolor(7);
    gpoint(340, 120 - i * 22);
    gtext('Modus ', 0);
    gtext(zahl(i), 0);
    case i of
      0: gtext(' ersetzen', 0);
      1: gtext(' umkehren', 0);
      2: gtext(' schwarz', 0);
      3: gtext(' Farbe (rot)', 0);
    end;
  end;
  gmode(3);
  gcolor(7);
  gpoint(20, 20);
  gtext('Umlaute: '#132#148#129#142#153#154#225, 0);
end;

{ ---- Bild 3: Bilder ---- }
procedure bilder;
var i, k, x, y: integer;
    r: GBInfoArr;
begin
  gclear;
  titel('DMVGRAF - Bilder: Formate 1/3/4/5, Arten, Zoom, GBGET, Ebenen');
  for y := 0 to 15 do
    for i := 0 to 1 do begin
      k := y * 8 + i;
      if i = 0 then zeig[k] := Lo(zkern[y] or zrand[y])
      else zeig[k] := Hi(zkern[y] or zrand[y]);
      if i = 0 then zeig[k + 2] := Lo(zrand[y]) else zeig[k + 2] := Hi(zrand[y]);
      zeig[k + 4] := zeig[k + 2];
      zeig[k + 6] := zeig[k + 2];
    end;
  for y := 0 to 23 do
    for x := 0 to 15 do begin
      if (y < 2) or (y > 21) then i := 0 else i := x;
      streif[y * 16 + x] := (i shl 4) or i;
    end;
  gbdef(0, 16, 16, GB_EBN, zeig);
  gbdef(1, 16, 16, GB_SETR, frosch);
  gbdef(2, 16, 8, GB_BIT7, wagen);
  gbdef(3, 32, 24, GB_NIB, streif);
  gmode(3);
  gcolor(2);                                { Hintergrund fuer die Arten }
  gpoint(20, 220);
  gbox(600, 150, 0);
  for k := 0 to 3 do begin
    gpoint(40 + k * 150, 300);
    gcolor(7);
    gmode(0);
    case k of
      0: gtext('deckend', 0);
      1: gtext('durchsichtig', 0);
      2: gtext('Schatten', 0);
      3: gtext('XOR', 0);
    end;
    for i := 1 to 3 do begin
      if i = 2 then gcolor(6) else gcolor(4);
      gpoint(40 + k * 150 + (i - 1) * 40, 250);
      gbput(i, k);
    end;
    gpoint(40 + k * 150, 228);
    gzoom(1);
    gbput(1, k);
    gzoom(0);
  end;
  { Zoom 3 und GBGET: Frosch vergroessert, dann kopiert }
  gpoint(20, 120);
  gzoom(3);
  gbput(1, GB_DURCH);
  gzoom(0);
  gpoint(20, 120);
  gbget(4, 64, 64);
  gpoint(100, 120);
  gbput(4, GB_DECK);
  gbinfo(1, r);
  gpoint(180, 160);
  gmode(3);
  gcolor(7);
  gtext('Frosch sichtbar ', 0);
  gtext(zahl(r[2]), 0); gtext(',', 0); gtext(zahl(r[3]), 0);
  gtext('-', 0);
  gtext(zahl(r[4]), 0); gtext(',', 0); gtext(zahl(r[5]), 0);
  { Zusammensetzen: zwoelf Streifenbilder ueberlappend + Zeiger }
  glayer(180, 20, 639, 140);
  for i := 0 to 11 do
    if i = 5 then glput(3, 200 + i * 20 + (i and 1) * 4, 40 + (i mod 3) * 12, GB_SCHAT)
    else glput(3, 200 + i * 20 + (i and 1) * 4, 40 + (i mod 3) * 12, GB_DURCH);
  glput(1, 470, 90, GB_DURCH);
  glput(0, 480, 70, GB_DURCH);
  gldraw;
end;

procedure zeiger;                           { Maus bis ESC }
var x, y, k, ax, ay, t: integer;
    ende: boolean;
begin
  ax := -1; ay := -1;
  ende := false;
  repeat
    if mpos(x, y, k) then
      if (x <> ax) or (y <> ay) then begin
        if ax >= 0 then begin               { alten Zeiger weg (XOR) }
          gpoint(ax, ay);
          gbput(0, GB_XOR);
        end;
        gpoint(x, y);
        gbput(0, GB_XOR);
        ax := x; ay := y;
      end;
    t := gkey;
    if t = 27 then ende := true
    else if t = 0 then gwait(1);
  until ende;
end;

begin
  gstart;
  ginit;
  if gerr <> 0 then
    gputs('Keine DMV-Grafik gefunden.'#10)
  else begin
    ginfo(f, m, s, c);
    figuren;
    if warte = 27 then gexit
    else begin
      texte;
      if warte = 27 then gexit
      else begin
        bilder;
        if m <> 0 then zeiger
        else if warte = 0 then ;
        gexit;
        gputs('GTEST Ende.'#10);
      end;
    end;
  end;
end.

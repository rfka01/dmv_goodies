{ DMVGRAF.INC - Grafik, Bilder, Maus und Ton fuer die NCR Decision Mate V,
  Fassung fuer Turbo Pascal 3.

  Dieselbe Bibliothek wie die C-Fassung (src/GRAF.C, GBILD.C, GEIN.C,
  GKERN.C), Zeile fuer Zeile uebersetzt; Beschreibung der Funktionen in
  README.md. Vorher einbinden: die Hardwareschicht (DMVHWD.INC fuer
  MS-DOS) und DMVFONT.INC.

  Turbo Pascal 3 kann eine Funktion nicht als Anweisung aufrufen. Was in
  C einen Fehlercode liefert, ist darum hier eine Prozedur; der Code steht
  danach in GERR (0 = gut). Funktionen sind nur gkey, grnd, mpos und
  gdirekt.

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

const
  { Formate fuer gbdef/gbload }
  GB_BIT7 = 1;  GB_BIT0 = 2;  GB_NIB = 3;  GB_EBN = 4;  GB_SETR = 5;
  { Arten fuer gbput/glput }
  GB_DECK = 0;  GB_DURCH = 1;  GB_SCHAT = 2;  GB_XOR = 3;
  { Fehlercodes }
  GE_ZOOM = 10;  GE_RAD = 20;  GE_RICHT = 30;  GE_MUST = 40;
  GE_MODUS = 50; GE_TRICHT = 60; GE_SEITE = 80;
  GE_NR = 90;  GE_PLATZ = 91;  GE_DATEI = 92;  GE_KURZ = 93;
  GE_FORM = 94;  GE_LEER = 95;  GE_LAGE = 96;  GE_HW = 99;

  QLMAX = 1024;  GBMAX = 64;  GLMAX = 200;  BANDH = 8;  QBUF = 1280;
  EB = 640;                         { BANDH * 80: Abstand der Ebenen im Band }
  ZHMAX = 32;  ZKH = 48;  ZKB = 12;    { Mauszeiger }

  qdx: array[0..7] of integer = (0, 1, 1, 1, 0, -1, -1, -1);
  qdy: array[0..7] of integer = (1, 1, 0, -1, -1, -1, 0, 1);

  { 16 Muster, je 8 Zeilen von oben, Bit 0 = links; die ersten zwei Zeilen
    sind zugleich das Linienmuster }
  qmust: array[0..15, 0..7] of byte = (
    ($FF,$FF,$FF,$FF,$FF,$FF,$FF,$FF), ($55,$AA,$55,$AA,$55,$AA,$55,$AA),
    ($11,$44,$11,$44,$11,$44,$11,$44), ($EE,$BB,$EE,$BB,$EE,$BB,$EE,$BB),
    ($FF,$00,$00,$00,$FF,$00,$00,$00), ($11,$11,$11,$11,$11,$11,$11,$11),
    ($80,$40,$20,$10,$08,$04,$02,$01), ($01,$02,$04,$08,$10,$20,$40,$80),
    ($FF,$11,$11,$11,$FF,$11,$11,$11), ($81,$42,$24,$18,$18,$24,$42,$81),
    ($FF,$01,$01,$01,$FF,$10,$10,$10), ($11,$00,$44,$00,$11,$00,$44,$00),
    ($FF,$FF,$00,$00,$FF,$FF,$00,$00), ($33,$33,$33,$33,$33,$33,$33,$33),
    ($0F,$0F,$0F,$0F,$F0,$F0,$F0,$F0), ($00,$00,$00,$00,$00,$00,$00,$00));

  { sin(t) * 256 fuer t = 0..45 Grad }
  qsin: array[0..45] of integer = (
      0,   4,   9,  13,  18,  22,  27,  31,  36,  40,  44,  49,
     53,  58,  62,  66,  71,  75,  79,  83,  88,  92,  96, 100,
    104, 108, 112, 116, 120, 124, 128, 132, 136, 139, 143, 147,
    150, 154, 158, 161, 165, 168, 171, 175, 178, 181);

  { Kreisachtel: 7220-Richtung, Startpunkt in Radien (y nach unten), DM,
    Grundwinkel, Drehsinn }
  qokt: array[0..7, 0..5] of integer = (
    (4,  1,  0, 0,   0,  1), (7,  1,  0, 1,   0, -1),
    (1,  0, -1, 0,  90, -1), (6,  0, -1, 1,  90,  1),
    (0, -1,  0, 0, 180,  1), (3, -1,  0, 1, 180, -1),
    (2,  0,  1, 0, 270,  1), (5,  0,  1, 1, 270, -1));

  bfa: array[0..15] of integer = (0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7);
  bfm: array[0..15] of integer = (0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7);
  bnlo: array[0..15] of byte = (0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0);
  bnhi: array[0..15] of byte = (0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3);

type
  GBInfoArr = array[0..5] of integer;

var
  gerr: integer;                    { Ergebnis des letzten Aufrufs }
  qneb, qhw, qx, qy, qorig, qan, qcol, qmode, qpatt, qzoom, qzs: integer;
  qebad: array[0..2] of integer;
  qebit: array[0..2] of integer;
  ql: array[0..QLMAX] of byte;
  qn, qe: integer;
  qzeig: boolean;

  grz: integer;                     { grnd }
  gmb: array[0..7] of byte;         { Mausblock }


procedure qzhid; forward;
procedure qzwied; forward;
procedure gzeig(nr, x, y: integer); forward;
procedure gbstart; forward;

{ ------------------------------------------------------------------ }
{ Hilfen fuer 16 Bit ohne Vorzeichen                                 }

function uwr(i: integer): real;     { Bitmuster als 0..65535 }
begin
  if i < 0 then uwr := i + 65536.0 else uwr := i;
end;

function rint(r: real): integer;    { 0..65535 -> Bitmuster }
begin
  if r >= 32768.0 then rint := Trunc(r - 65536.0) else rint := Trunc(r);
end;

{ ------------------------------------------------------------------ }
{ Kern (GKERN.C)                                                     }

procedure gnib(var zz; var qq; var tt; n: integer);
var z: GBytes absolute zz;
    q: GBytes absolute qq;
    tb: GBytes absolute tt;
    k, j, w, e0, e1, e2, f, qi: integer;
begin
  qi := 0;
  for k := 0 to n - 1 do begin
    e0 := 0; e1 := 0; e2 := 0; f := 0;
    j := 0;
    while j < 8 do begin
      w := tb[q[qi]];
      qi := qi + 1;
      e0 := e0 or ((w and 3) shl j);
      e1 := e1 or (((w shr 2) and 3) shl j);
      e2 := e2 or (((w shr 4) and 3) shl j);
      f := f or (((w shr 6) and 3) shl j);
      j := j + 2;
    end;
    z[k] := f;
    z[80 + k] := e0;
    z[160 + k] := e1;
    z[240 + k] := e2;
  end;
end;

procedure gzeile(var dd; var zz; n, sh, cnt, ns, c01, c23, eb: integer);
var d: GBytes absolute dd;
    z: GBytes absolute zz;
    j, w, mm, cm, c0, c1, c2, v: integer;
begin
  cm := Hi(c01); c0 := Lo(c01); c1 := Hi(c23); c2 := Lo(c23);
  for j := 0 to cnt - 1 do begin
    if j < ns then v := z[j] else v := 0;
    w := v shl sh;
    mm := (w or cm) and $FF;
    cm := (w shr 8) and $FF;
    if j < ns then v := z[j + n] else v := 0;
    w := v shl sh;
    v := (w or c0) and $FF;
    c0 := (w shr 8) and $FF;
    d[j] := (d[j] and not mm) or (v and mm);
    if j < ns then v := z[j + 2 * n] else v := 0;
    w := v shl sh;
    v := (w or c1) and $FF;
    c1 := (w shr 8) and $FF;
    d[j + eb] := (d[j + eb] and not mm) or (v and mm);
    if j < ns then v := z[j + 3 * n] else v := 0;
    w := v shl sh;
    v := (w or c2) and $FF;
    c2 := (w shr 8) and $FF;
    d[j + 2 * eb] := (d[j + 2 * eb] and not mm) or (v and mm);
  end;
end;

{ ------------------------------------------------------------------ }
{ GDC-Liste (GRAF.C)                                                 }

procedure qflush;
begin
  if qn <> 0 then begin
    ql[qn] := 0;
    if qan <> 0 then ghlist(ql);
    qn := 0;
  end;
  qe := -1;
end;

procedure qplatz(n: integer);
begin
  if qzeig then qzhid;              { Mauszeiger weg, bevor anderes kommt }
  if qn + n + 2 >= QLMAX then qflush;
end;

procedure qcmd(c: integer);
begin
  qplatz(24);
  qe := qn;
  ql[qn] := 1; ql[qn + 1] := c;
  qn := qn + 2;
end;

procedure qpar(b: integer);
begin
  ql[qn] := Lo(b);
  qn := qn + 1;
  ql[qe] := ql[qe] + 1;
end;

procedure qwrd(w: integer);
begin
  qpar(Lo(w)); qpar(Hi(w));
end;

{ Block von n Zeilen zu nc Worten ab Bildzeile sy, Wortspalte c auf den
  Ebenen 0..ne-1 ersetzen; Quelle s, Zeilenabstand k, Ebenenabstand es
  Byte. Zeile fuer Zeile alle Ebenen }
procedure qblk(ne, sy, c, nc, n: integer; var ss; k, es: integer);
var s: GBytes absolute ss;
    adr, m, e, si: integer;
begin
  m := 2 * nc;
  si := 0;
  while n > 0 do begin
    for e := 0 to ne - 1 do begin
      if qn + m + 20 >= QLMAX then qflush;
      adr := qebad[e] + sy * 40 + c;
      ql[qn] := 4; ql[qn + 1] := $49; ql[qn + 2] := Lo(adr);
      ql[qn + 3] := Hi(adr); ql[qn + 4] := 0;
      ql[qn + 5] := 3; ql[qn + 6] := $4A; ql[qn + 7] := $FF; ql[qn + 8] := $FF;
      ql[qn + 9] := 4; ql[qn + 10] := $4C; ql[qn + 11] := 2; ql[qn + 12] := 0;
      ql[qn + 13] := 0;
      ql[qn + 14] := 1 + m; ql[qn + 15] := $20;
      Move(s[si + e * es], ql[qn + 16], m);
      qn := qn + 16 + m;
    end;
    n := n - 1; si := si + k; sy := sy + 1;
  end;
  qe := -1;
end;

{ CURS: Wortadresse, Punkt 0-15 im Wort (setzt MASK auf diesen Punkt) }
procedure qcurs(adr, p: integer);
begin
  qcmd($49);
  qwrd(adr);
  qpar((p and 15) shl 4);
end;

procedure qzm(z: integer);          { GCHRD-Zoom }
begin
  if z <> qzs then begin
    qcmd($46);
    qpar(z and 15);
    qzs := z;
  end;
end;

function qsy(y: integer): integer;
begin
  if qorig <> 0 then qsy := y else qsy := 399 - y;
end;

function qebon(e, c: integer): boolean;
begin
  qebon := (c and qebit[e]) <> 0;
end;

{ Zeichenmodus einer Ebene fuer Figuren: 0 auslassen, 1 mit Muster,
  2 mit vollem Muster; w = WDAT-Befehl }
function qfmod(e: integer; var w: integer): integer;
var an: boolean;
begin
  an := qebon(e, qcol);
  case qmode of
    0: if an then begin w := $20; qfmod := 1; end
       else begin w := $22; qfmod := 2; end;
    1: begin w := $21; if an then qfmod := 1 else qfmod := 0; end;
    2: begin w := $22; qfmod := 1; end;
  else begin
    if an then w := $23 else w := $22;
    qfmod := 1;
  end;
  end;
end;

function qdir(d: integer): integer; { Richtung 0-7 des Programms -> 7220 }
begin
  if qorig <> 0 then qdir := (10 - d) and 7 else qdir := (2 + d) and 7;
end;

{ Ecke, an der der 7220 beginnt }
procedure qank(sx, sy, d, l: integer; var ox, oy: integer);
var k: integer;
begin
  ox := sx; oy := sy;
  if qorig <> 0 then begin
    k := (8 - d) and 7;
    ox := ox + l * qdx[k];
    oy := oy + l * qdy[k];
  end;
end;

function qadr(e, sx, sy: integer): integer;
begin
  qadr := qebad[e] + sy * 40 + (sx shr 4);
end;

{ Figur auf allen Ebenen: Startpunkt, Muster (nm Byte ab PRAM 8),
  FIGS-Parameter, Abschlussbefehl (FIGD 6Ch oder GCHRD 68h) }
procedure qfig(sx, sy: integer; var mm; nm: integer; var ff; nf, ende: integer);
var mu: GBytes absolute mm;
    fp: GBytes absolute ff;
    e, k, i, w: integer;
begin
  for e := 0 to qneb - 1 do begin
    k := qfmod(e, w);
    if k <> 0 then begin
      qplatz(48);
      qcmd(w);
      qcurs(qadr(e, sx, sy), sx and 15);
      qcmd($78);
      for i := 0 to nm - 1 do
        if k = 2 then qpar($FF) else qpar(mu[i]);
      qcmd($4C);
      for i := 0 to nf - 1 do qpar(fp[i]);
      qcmd(ende);
    end;
  end;
end;

procedure qput(var ff; i, w: integer);   { Wort in FIGS-Parameter, 14 Bit }
var p: GBytes absolute ff;
begin
  p[i] := Lo(w);
  p[i + 1] := Hi(w) and $3F;
end;

{ ------------------------------------------------------------------ }
{ Ein- und Ausschalten                                               }

function qstart: integer;
begin
  if qhw = -2 then begin
    qneb := 3;
    qebad[0] := $0000; qebad[1] := $4000; qebad[2] := $8000;
    qebit[0] := 2; qebit[1] := 4; qebit[2] := 1;
    qhw := ghinit;
    if (qhw >= 0) and ((qhw and GH_FARBE) = 0) then begin
      qneb := 1;
      qebit[0] := 7;
    end;
  end;
  if qhw < 0 then qstart := GE_HW else qstart := 0;
end;

procedure ginit;
begin
  gerr := GE_HW;
  if qstart <> 0 then exit;
  qflush;
  ghgraf(true);
  qan := 1;
  qzs := 0; qpatt := 0; qzoom := 0; qmode := 1; qcol := 7;
  qx := 0; qy := 0;
  gerr := 0;
end;

procedure gexit;
begin
  gzeig(-1, 0, 0);
  qflush;
  if qan <> 0 then ghgraf(false);
  qan := 0;
  gerr := 0;
end;

procedure gclear;
const fp: array[0..4] of byte = ($12, $8F, $41, $80, $02);   { 400 x 640 }
var e, i: integer;
begin
  qzm(0);
  for e := 0 to qneb - 1 do begin
    qplatz(40);
    qcmd($22);
    qcurs(qadr(e, 0, 399), 0);
    qcmd($78);
    for i := 1 to 8 do qpar($FF);
    qcmd($4C);
    for i := 0 to 4 do qpar(fp[i]);
    qcmd($68);
  end;
  qflush;
  gerr := 0;
end;

procedure gpoint(x, y: integer);
begin
  if x < 0 then gerr := 1 else if x > 639 then gerr := 2
  else if y < 0 then gerr := 3 else if y > 399 then gerr := 4
  else begin qx := x; qy := y; gerr := 0; end;
end;

procedure where(var x, y: integer);
begin
  x := qx; y := qy; gerr := 0;
end;

procedure gorig(o: integer);
begin
  if o <> 0 then qorig := 1 else qorig := 0;
  gerr := 0;
end;

procedure gcolor(c: integer);
begin
  qcol := c and 7; gerr := 0;
end;

procedure gmode(m: integer);
begin
  if (m < 0) or (m > 3) then gerr := GE_MODUS
  else begin qmode := m; gerr := 0; end;
end;

procedure gpatt(p: integer);
begin
  if (p < 0) or (p > 15) then gerr := GE_MUST
  else begin qpatt := p; gerr := 0; end;
end;

procedure gzoom(z: integer);
begin
  if (z < 0) or (z > 15) then gerr := GE_ZOOM
  else begin qzoom := z; gerr := 0; end;
end;

{ ------------------------------------------------------------------ }
{ Linien, Rechtecke, Kreise                                          }

procedure gline(x, y: integer);
var fp: array[0..8] of byte;
    x0, y0, x1, y1, dx, dy, ax, ay, dir, maj, min: integer;
begin
  if x < 0 then begin gerr := 5; exit; end;
  if x > 639 then begin gerr := 6; exit; end;
  if y < 0 then begin gerr := 7; exit; end;
  if y > 399 then begin gerr := 8; exit; end;
  x0 := qx; y0 := qsy(qy);
  x1 := x;  y1 := qsy(y);
  dx := x1 - x0; dy := y1 - y0;
  ax := Abs(dx); ay := Abs(dy);
  if dy >= 0 then begin
    if dx >= 0 then begin if ay >= ax then dir := 0 else dir := 1; end
    else begin if ay >= ax then dir := 7 else dir := 6; end;
  end else begin
    if dx > 0 then begin if ax >= ay then dir := 2 else dir := 3; end
    else begin if ay >= ax then dir := 4 else dir := 5; end;
  end;
  if (dir = 0) or (dir = 3) or (dir = 4) or (dir = 7) then begin
    maj := ay; min := ax;
  end else begin
    maj := ax; min := ay;
  end;
  fp[0] := dir or $08;
  fp[1] := Lo(maj);
  fp[2] := (Hi(maj) and $3F) or $40;
  qput(fp, 3, 2 * min - maj);
  qput(fp, 5, 2 * (min - maj));
  qput(fp, 7, 2 * min);
  qfig(x0, y0, qmust[qpatt, 0], 2, fp, 9, $6C);
  qflush;
  qx := x; qy := y;
  gerr := 0;
end;

procedure grect(m, n, d: integer);
var fp: array[0..10] of byte;
    sx, sy: integer;
begin
  if (d < 0) or (d > 7) then begin gerr := GE_RICHT; exit; end;
  if m < 2 then begin gerr := 80; exit; end;
  if m > 640 then begin gerr := 81; exit; end;
  if n < 2 then begin gerr := 82; exit; end;
  if n > 640 then begin gerr := 83; exit; end;
  qank(qx, qsy(qy), d, n - 1, sx, sy);
  fp[0] := $40 or qdir(d);
  fp[1] := 3;
  fp[2] := $40;
  qput(fp, 3, m - 1);
  qput(fp, 5, n - 1);
  qput(fp, 7, $3FFF);
  qput(fp, 9, m - 1);
  qfig(sx, sy, qmust[qpatt, 0], 2, fp, 11, $6C);
  qflush;
  gerr := 0;
end;

procedure gbox(m, n, d: integer);
var fp: array[0..4] of byte;
    sx, sy, z: integer;
begin
  z := qzoom + 1;
  if (d < 0) or (d > 7) then begin gerr := GE_RICHT; exit; end;
  if m < 1 then begin gerr := 80; exit; end;
  if 1.0 * m * z > 640 then begin gerr := 81; exit; end;
  if n < 1 then begin gerr := 82; exit; end;
  if 1.0 * n * z > 640 then begin gerr := 83; exit; end;
  qank(qx, qsy(qy), d, n * z - 1, sx, sy);
  qzm(qzoom);
  fp[0] := $10 or qdir(d);
  qput(fp, 1, n - 1);
  fp[2] := fp[2] or $40;
  qput(fp, 3, m);
  qfig(sx, sy, qmust[qpatt, 0], 8, fp, 5, $68);
  qflush;
  gerr := 0;
end;

procedure qbog(cx, cy, r, k, t0, t1: integer);  { Achtel k von t0 bis t1 Grad }
var fp: array[0..10] of byte;
    dm, dc: integer;
begin
  dm := Trunc((1.0 * qsin[t0] * r + 255) / 256);
  dc := Trunc((1.0 * qsin[t1] * r) / 256);
  if t0 = 0 then dm := qokt[k, 3];
  if dm > dc then exit;
  fp[0] := $20 or qokt[k, 0];
  fp[1] := Lo(dc);
  fp[2] := (Hi(dc) and $3F) or $40;
  qput(fp, 3, r - 1);
  qput(fp, 5, 2 * (r - 1));
  qput(fp, 7, $3FFF);
  qput(fp, 9, dm);
  qfig(cx + r * qokt[k, 1], cy + r * qokt[k, 2], qmust[qpatt, 0], 2, fp, 11, $6C);
end;

function qwink(a: integer): integer;   { auf 0..359 }
begin
  { mod nur mit nicht negativen Zahlen: TP3 fuer den Z80 rechnet sonst
    anders als TP3 fuer den 8086 }
  if a >= 0 then qwink := a mod 360
  else qwink := 359 - ((-1 - a) mod 360);
end;

procedure garc(r, a, e: integer);
var cx, cy, k, t, t0, w, l: integer;
    voll, drin: boolean;
begin
  if (r < 1) or (r > 1000) then begin gerr := GE_RAD; exit; end;
  cx := qx;
  cy := qsy(qy);
  voll := (e - a >= 360) or (a - e >= 360);
  if qorig <> 0 then begin          { gespiegelt: Winkel umdrehen }
    t := a; a := -e; e := -t;
  end;
  a := qwink(a);
  l := qwink(e - a);
  if (l = 0) and (e <> a) then voll := true;
  for k := 0 to 7 do
    if voll then qbog(cx, cy, r, k, 0, 45)
    else begin
      t0 := -1;
      for t := 0 to 46 do begin
        drin := false;
        if t <= 45 then begin
          w := qwink(qokt[k, 4] + qokt[k, 5] * t - a);
          drin := w <= l;
        end;
        if drin and (t0 < 0) then t0 := t;
        if (not drin) and (t0 >= 0) then begin
          qbog(cx, cy, r, k, t0, t - 1);
          t0 := -1;
        end;
      end;
    end;
  qflush;
  gerr := 0;
end;

procedure gcircl(r: integer);
begin
  garc(r, 0, 360);
end;

{ ------------------------------------------------------------------ }
{ Zeilen wortweise                                                   }

{ Eine Bitzeile (Bit 0 = Punkt sx) in Ebene e mit WDAT-Befehl w.
  a: 0 Bits, 1 Bits umgekehrt, 2 lauter Einsen, 3 lauter Nullen }
procedure qrow(sy, sx: integer; var bb; nb, e, w, a: integer);
var b: GBytes absolute bb;
    j, v, alt, nw, sh, n8, k, letzt, rest, i, adr, p: integer;
    leer: boolean;
begin
  if (nb <= 0) or (sy < 0) or (sy > 399) then exit;
  sh := sx and 15;
  nw := (sh + nb + 15) shr 4;
  n8 := (nb + 7) shr 3;
  rest := nb and 7;
  if rest <> 0 then letzt := (1 shl rest) - 1 else letzt := $FF;
  qplatz(40 + 2 * nw);              { kein Ausgeben mittendrin }
  adr := qadr(e, sx, sy);
  ql[qn] := 4; ql[qn + 1] := $49;
  ql[qn + 2] := Lo(adr); ql[qn + 3] := Hi(adr); ql[qn + 4] := 0;
  ql[qn + 5] := 3; ql[qn + 6] := $4A; ql[qn + 7] := $FF; ql[qn + 8] := $FF;
  ql[qn + 9] := 4; ql[qn + 10] := $4C; ql[qn + 11] := 2; ql[qn + 12] := 0;
  ql[qn + 13] := 0;
  k := 2 * nw;
  ql[qn + 14] := 1 + k; ql[qn + 15] := w;
  qe := qn + 14;
  qn := qn + 16;
  p := qn;
  if (sh = 0) and (a = 0) and ((nb and 15) = 0) then begin   { ganze Worte }
    Move(b, ql[p], k);
    qn := qn + k;
    exit;
  end;
  ql[qe] := 1;
  i := -(sh shr 3);
  sh := sh and 7;
  alt := 0;
  leer := true;
  for j := 0 to k - 1 do begin
    v := 0;
    if (i >= 0) and (i < n8) then begin
      case a of
        0: v := b[i];
        1: v := (not b[i]) and $FF;
        2: v := $FF;
      end;
      if i = n8 - 1 then v := v and letzt;
    end;
    ql[p] := ((v shl sh) or alt) and $FF;
    if ql[p] <> 0 then leer := false;
    p := p + 1;
    if sh <> 0 then alt := v shr (8 - sh) else alt := 0;
    i := i + 1;
  end;
  if leer and (w <> $20) then begin  { nichts zu tun: Eintrag zurueck }
    qn := qe - 14;
    qe := -1;
    exit;
  end;
  qn := qn + k;
  ql[qe] := ql[qe] + k;
end;

{ Bitzeile in der aktuellen Farbe und Betriebsart auf alle Ebenen }
procedure qbits(sy, sx: integer; var b; nb: integer);
var e: integer;
    an, ganz: boolean;
begin
  ganz := ((sx and 15) = 0) and ((nb and 15) = 0);
  for e := 0 to qneb - 1 do begin
    an := qebon(e, qcol);
    case qmode of
      0: if ganz then begin
           if an then qrow(sy, sx, b, nb, e, $20, 0)
           else qrow(sy, sx, b, nb, e, $20, 3);
         end else if an then begin
           qrow(sy, sx, b, nb, e, $22, 1);
           qrow(sy, sx, b, nb, e, $23, 0);
         end else qrow(sy, sx, b, nb, e, $22, 2);
      1: if an then qrow(sy, sx, b, nb, e, $21, 0);
      2: qrow(sy, sx, b, nb, e, $22, 0);
    else
      if an then qrow(sy, sx, b, nb, e, $23, 0)
      else qrow(sy, sx, b, nb, e, $22, 0);
    end;
  end;
end;

{ ------------------------------------------------------------------ }
{ Text                                                               }

function qglyph(c: integer): integer;   { Zeichen -> Index in fntbit }
var k: integer;
begin
  qglyph := 0;
  if (c >= 32) and (c < 127) then qglyph := c - 32
  else
    for k := 0 to 6 do
      if fntuml[k] = c then qglyph := 95 + k;
end;

var qtz: array[0..79] of byte;

procedure qtxt0(var s: GStr; n: integer);   { waagrecht, wortweise }
var z1, gr, zr, i, j, k, v, sy, bit, nb: integer;
begin
  z1 := qzoom + 1;
  nb := n * 8 * z1;
  sy := qsy(qy);
  if qorig = 0 then sy := sy - (16 * z1 - 1);
  for gr := 0 to 15 do begin
    FillChar(qtz, 80, 0);
    for i := 0 to n - 1 do begin
      v := fntbit[qglyph(Ord(s[i + 1])) * 16 + gr];
      if z1 = 1 then qtz[i] := v
      else
        for j := 0 to 7 do
          if (v and (1 shl j)) <> 0 then begin
            bit := (i * 8 + j) * z1;
            for k := 1 to z1 do begin
              qtz[bit shr 3] := qtz[bit shr 3] or (1 shl (bit and 7));
              bit := bit + 1;
            end;
          end;
    end;
    for zr := 0 to z1 - 1 do qbits(sy + gr * z1 + zr, qx, qtz, nb);
  end;
end;

procedure qtxtd(var s: GStr; n, d: integer);  { je Zeichen zwei 8x8-Muster }
var fp: array[0..4] of byte;
    mu: array[0..7] of byte;
    z1, td, ud, sx, sy, i, h, r, g, x, y: integer;
begin
  z1 := qzoom + 1;
  td := qdir(d);
  ud := (td + 2) and 7;
  qank(qx, qsy(qy), d, 16 * z1 - 1, sx, sy);
  qzm(qzoom);
  fp[0] := $10 or td; fp[1] := 7; fp[2] := $40; fp[3] := 8; fp[4] := 0;
  for i := 0 to n - 1 do begin
    g := qglyph(Ord(s[i + 1])) * 16;
    for h := 0 to 1 do begin
      { PRAM 8 = oberste Zeile, 15 = unterste: untere Haelfte zuerst }
      for r := 0 to 7 do
        if h = 0 then mu[r] := fntbit[g + 8 + r] else mu[r] := fntbit[g + r];
      x := sx + i * 8 * z1 * qdx[td] + h * 8 * z1 * qdx[ud];
      y := sy + i * 8 * z1 * qdy[td] + h * 8 * z1 * qdy[ud];
      qfig(x, y, mu, 8, fp, 5, $68);
    end;
  end;
end;

procedure gtext(s: GStr; d: integer);
var n, l, sx, sy: integer;
begin
  if (d < 0) or (d > 7) then begin gerr := GE_TRICHT; exit; end;
  gerr := 0;
  n := Length(s);
  if n * 8 * (qzoom + 1) > 640 then n := 640 div (8 * (qzoom + 1));
  if n = 0 then exit;
  if d = 0 then qtxt0(s, n) else qtxtd(s, n, d);
  qflush;
  { Zeichenpunkt hinter das letzte Zeichen }
  l := n * 8 * (qzoom + 1);
  sx := qx + l * qdx[qdir(d)];
  sy := qsy(qy) + l * qdy[qdir(d)];
  if (sx >= 0) and (sx < 640) and (sy >= 0) and (sy < 400) then begin
    qx := sx;
    if qorig <> 0 then qy := sy else qy := 399 - sy;
  end;
end;

{ ------------------------------------------------------------------ }
{ Maus, Tastatur, Zeit, Auskunft, Ton (GEIN.C)                       }

procedure ginfo(var f, m, s, c: integer);
var r: integer;
begin
  r := qstart;
  f := 0; m := 0; s := 0; c := 0;
  if r = 0 then begin
    if (qhw and GH_FARBE) <> 0 then f := 1;
    if (qhw and GH_MAUS) <> 0 then m := 1;
    if (qhw and GH_SYN) <> 0 then s := 1;
    if (qhw and GH_COVOX) <> 0 then c := 1;
  end;
  gerr := r;
end;

{ K806 Befehl 00h: X low, X high, Y low, Y high, Schalter; Y zaehlt wie
  NCRGRAF nach oben (0 = unten) }
function mpos(var x, y, k: integer): boolean;
var v: integer;
begin
  mpos := false;
  if qstart <> 0 then exit;
  if (qhw and GH_MAUS) = 0 then exit;
  qzwied;
  gmb[0] := 0; gmb[1] := 5;
  if ghmaus(0, gmb) <> 0 then exit;
  x := gmb[2] + 256 * gmb[3];
  v := gmb[4] + 256 * gmb[5];
  if v < 0 then v := 0;
  if v > 399 then v := 399;
  if qorig <> 0 then y := 399 - v else y := v;
  k := gmb[6] and 7;
  mpos := true;
end;

procedure mset(x, y: integer);
begin
  gerr := GE_HW;
  if qstart <> 0 then exit;
  if (qhw and GH_MAUS) = 0 then exit;
  if qorig <> 0 then y := 399 - y;
  gmb[0] := 4; gmb[1] := Lo(x); gmb[2] := Hi(x);
  gmb[3] := Lo(y); gmb[4] := Hi(y); gmb[5] := 0;
  if ghmaus(4, gmb) <> 0 then gerr := GE_HW else gerr := 0;
end;

function gkey: integer;
begin
  qzwied;
  gkey := ghtast;
end;

procedure gwait(n: integer);
begin
  qzwied;
  qflush;
  while n > 0 do begin ghvsyn; n := n - 1; end;
  gerr := 0;
end;

procedure ton(h, l: integer);
begin
  if (h < 0) or (h > 42) or (l < 1) or (l > 224) then gerr := GE_FORM
  else gerr := ghton(h, l);
end;

function gdirekt(an: integer): integer;
begin
  if qstart <> 0 then gdirekt := -1
  else begin
    qflush;
    gdirekt := ghdset(an);
  end;
end;

procedure gputs(s: GStr);
begin
  ghtext(s);
  gerr := 0;
end;

{ Zufallszahl fuer den Start: Lage des Bildaufbaus, dazu die Maus }
function grnd: integer;
var n: integer;
begin
  if qstart = 0 then begin
    n := ghvzahl;
    grz := grz * 31421 + n + 6927;
    if (qhw and GH_MAUS) <> 0 then begin
      gmb[0] := 0; gmb[1] := 5;
      if ghmaus(0, gmb) = 0 then
        grz := grz * 75 + (gmb[2] shl 7) + gmb[4] * 13 + (gmb[6] and 7);
    end;
  end;
  grnd := grz and $7FFF;
end;

{ Anfangswerte: Turbo Pascal 3 kennt keine vorbelegten Variablen; das
  erste DMVGRAF-Element im Programm muss darum gstart sein }
procedure gstart;
begin
  qhw := -2; qneb := 3; qcol := 7; qmode := 1; qpatt := 0; qzoom := 0;
  qorig := 0; qan := 0; qx := 0; qy := 0; qzs := 0;
  qn := 0; qe := -1; qzeig := false;
  gbstart;
  grz := 1;
  gerr := 0;
end;

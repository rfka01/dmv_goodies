{ DMVFIG.INC - DMVGRAF fuer Turbo Pascal 3: Linien, Rechtecke, Kreise
  und Boegen (gline, grect, gcircl, garc). Nach DMVGRAF.INC einbinden,
  wenn gebraucht; Flaechen (gbox) und Text sind im Kern.

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

const
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


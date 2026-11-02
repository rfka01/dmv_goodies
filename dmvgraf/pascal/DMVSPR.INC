{ DMVSPR.INC - DMVGRAF fuer Turbo Pascal 3: Figuren ueber GCHRD
  ("Sprites"). Nach DMVGRAF.INC einbinden, wenn gebraucht.

  Die Muster stehen 8 x 8 im Parameter-RAM des 7220, der sie selbst
  zeichnet und vergroessert. Schneller als Bilder, gut fuer kleine
  bewegte Figuren.

  gsprdef(nr, b, h, muster)   Figur 0-15 anlegen; b und h je 8 oder 16;
                              muster: je Ebene (gruen, rot, blau) h Zeilen
                              zu b/8 Byte, Bit 0 links. Die Daten bleiben
                              im Programm und muessen erhalten bleiben.
  gspr(nr, x, y, zoom, art)   zeichnen (Ecke wie gbput), zoom 0-15; art
                              GB_XOR umkehren (zweimal = weg), GB_DURCH
                              nur gesetzte Punkte, GB_DECK ganzes Feld.
                              Muss ganz auf den Schirm passen (GE_LAGE).

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

var
  qspd: array[0..15] of GBZeiger;   { Muster des Programms }
  qspb, qsph: array[0..15] of byte;
  qspk: array[0..15] of integer;    { Kennung: angelegt (TP3 loescht nicht) }
  qspm, qspp, qspq: array[0..7] of byte;

procedure gsprdef(nr, b, h: integer; var mu);
begin
  gerr := GE_NR;
  if (nr < 0) or (nr > 15) then exit;
  gerr := GE_FORM;
  if ((b <> 8) and (b <> 16)) or ((h <> 8) and (h <> 16)) then exit;
  qspd[nr] := hwAdr(mu);
  qspb[nr] := b; qsph[nr] := h;
  qspk[nr] := $5A3C + nr;
  gerr := 0;
end;

{ Ein 8 x 8-Feld auf Ebene e mit WDAT-Befehl w; x, y = untere linke
  Ecke (Bildzeile), dort beginnt der 7220 }
procedure qkach(e, w, x, y: integer; var mu);
const fp: array[0..4] of byte = ($12, 7, $40, 8, 0);
begin
  qplatz(48);
  qcmd(w);
  qcurs(qadr(e, x, y), x and 15);
  qcmd($78);
  qpars(mu, 8);
  qcmd($4C);
  qpars(fp, 5);
  qcmd($68);
end;

function qsleer(var mm): boolean;
var m: array[0..7] of byte absolute mm;
    i: integer;
begin
  qsleer := false;
  for i := 0 to 7 do if m[i] <> 0 then exit;
  qsleer := true;
end;

procedure gspr(nr, x, y, z, art: integer);
var nc, z1, sx, sy, c, r, e, i, k, eb, xt, yt: integer;
begin
  gerr := GE_NR;
  if (nr < 0) or (nr > 15) then exit;
  gerr := GE_LEER;
  if qspk[nr] <> $5A3C + nr then exit;
  gerr := GE_ZOOM;
  if (z < 0) or (z > 15) then exit;
  gerr := GE_FORM;
  if (art < 0) or (art > 3) or (art = GB_SCHAT) then exit;
  gerr := qstart;
  if gerr <> 0 then exit;
  nc := qspb[nr] shr 3;
  z1 := z + 1;
  sx := x;
  sy := qsy(y);
  if qorig = 0 then sy := sy - (qsph[nr] * z1 - 1);
  gerr := GE_LAGE;
  if (sx < 0) or (sy < 0) or (sx + qspb[nr] * z1 > 640) or
     (sy + qsph[nr] * z1 > 400) then exit;
  gerr := 0;
  eb := qsph[nr] * nc;              { Byte je Ebene }
  qzm(z);
  for r := 0 to (qsph[nr] shr 3) - 1 do
    for c := 0 to nc - 1 do begin
      xt := sx + c * 8 * z1;
      yt := sy + (r + 1) * 8 * z1 - 1;
      for i := 0 to 7 do begin
        k := (r * 8 + i) * nc + c;
        qspm[i] := qspd[nr]^[k] or qspd[nr]^[eb + k] or qspd[nr]^[2 * eb + k];
      end;
      for e := 0 to qneb - 1 do begin
        for i := 0 to 7 do begin
          k := (r * 8 + i) * nc + c;
          if qneb = 1 then qspp[i] := qspm[i]
          else qspp[i] := qspd[nr]^[e * eb + k];
          qspq[i] := qspm[i] and (not qspp[i]) and $FF;
        end;
        if art = GB_DECK then qkach(e, $20, xt, yt, qspp)
        else if art = GB_XOR then begin
          if not qsleer(qspp) then qkach(e, $21, xt, yt, qspp);
        end else begin
          if not qsleer(qspp) then qkach(e, $23, xt, yt, qspp);
          if not qsleer(qspq) then qkach(e, $22, xt, yt, qspq);
        end;
      end;
    end;
  qflush;
end;

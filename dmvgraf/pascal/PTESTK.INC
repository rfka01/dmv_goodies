{ Hauptteil des Pascal-Pruefprogramms fuer die Muster, wie test/PTEST.C 
  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }
const welle: array[0..7] of byte = ($FF, $EE, $55, $BB, $FF, $EE, $55, $BB);
      acht: array[0..7] of byte = ($01, $00, $10, $00, $01, $00, $10, $00);
var i, k, p: integer;
begin
  gstart; ginit;
  if gerr <> 0 then halt;
  gorig(1);
  gcolor(7); gpoint(8, 4);
  gtext('DMVGRAF - Muster am Schirm verankert, zweifarbig, eigene', 0);
  gmode(3);
  for k := 0 to 3 do begin
    if k = 0 then p := 1 else if k = 1 then p := 2 else if k = 2 then p := 6 else p := 10;
    gpatt(p);
    gcolor(k + 2);
    for i := 0 to 6 do begin
      gpoint(13 + i * 37 + k, 30 + k * 30 + (i and 1) * 3);
      gbox(37, 23 + (i and 1) * 2, 0);
    end;
  end;
  gpdef(14, welle);
  gpdef(15, acht);
  for k := 0 to 1 do begin
    gpatt(14 + k);
    if k = 1 then gbcol(6) else gbcol(0);
    if k = 1 then gcolor(4) else gcolor(1);
    for i := 0 to 4 do begin
      gpoint(300 + i * 51 + k * 5, 200 + k * 60 + i);
      gbox(51, 47, 0);
    end;
  end;
  gbcol(-1);
  gpatt(9);
  gcolor(7);
  gpoint(100, 330);
  gbox(60, 40, 2);
  while gkey = 0 do gwait(1);
  gexit;
end.

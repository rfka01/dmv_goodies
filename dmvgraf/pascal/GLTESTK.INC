{ GLTESTK.INC - Rumpf von GLTEST/GLTESTZ: Bildsammlung mit gbldv laden
  und alle Bilder deckend nebeneinander zeichnen, wie test/GLTEST.C.
    GLTEST datei kopf
  Eine Taste beendet, danach "gbldv: Fehler Anzahl".
  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }
var li, lx, ly, lzh, lc: integer;
    lkopf: real;
    lr: GBInfoArr;
begin
  if ParamCount < 2 then halt;
  Val(ParamStr(2), lkopf, lc);
  gstart; ginit;
  if gerr <> 0 then halt;
  gorig(1);
  gbldv(0, ParamStr(1), -1, lkopf);
  lc := gerr;
  lx := 0; ly := 0; lzh := 0;
  for li := 0 to gbanz - 1 do begin
    gbinfo(li, lr);
    if gerr = 0 then begin
      if lx + lr[0] > 640 then begin lx := 0; ly := ly + lzh + 4; lzh := 0; end;
      gpoint(lx, ly);
      gbput(li, GB_DECK);
      lx := lx + lr[0] + 4;
      if lr[1] > lzh then lzh := lr[1];
    end;
  end;
  while gkey = 0 do gwait(1);
  gexit;
  WriteLn('gbldv: ', lc, ' ', gbanz);
end.

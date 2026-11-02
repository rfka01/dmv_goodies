{ DMVMAUS.INC - DMVGRAF fuer Turbo Pascal 3: Maus K806 (mpos, mset).
  Nach DMVGRAF.INC einbinden, wenn gebraucht. Den Mauszeiger (gzeig)
  enthaelt DMVBILD.INC.

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

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


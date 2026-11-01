{ DMVHWC.INC - DMVGRAF fuer Turbo Pascal 3: Hardwareschicht CP/M-80
  (Z80 der Hauptplatine).

  Wie DMVHWD.INC (MS-DOS): 7220 (A0h-A2h), Maus K806 (30h-33h) und
  Tastatur-8741 (40h/41h) direkt ueber Port[]; Tastatur und Text ueber
  BDOS 6 und 2, Dateien ueber FCB. Grafik aus wie der Konsolentreiber
  von NCRs CP/M (danach loescht die Konsole mit ^Z den Schirm). Der
  Bildspeicher ist ein Block auf dem Heap (GetMem), so gross, wie der
  Speicher hergibt (hoechstens hwPMAX Byte).

  Reihenfolge im Programm: DMVHWC.INC, DMVFONT.INC, DMVGRAF.INC.

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

type
  GBytes  = array[0..32767] of byte;
  GStr    = string[255];
  GBZeiger = ^GBytes;

const
  GH_FARBE = 1;  GH_MAUS = 2;  GH_SYN = 4;  GH_COVOX = 8;
  hwZEIT   = 30000;
  hwPMAX   = 16384;                 { Bildspeicher hoechstens }
  hwPRES   = 2048;                  { so viel Heap bleibt frei }

var
  hwK806, hwFarbe: integer;
  hwBn, hwPn: integer;              { Bildspeicher: Bloecke (-1 noch nicht), Groesse }
  hwPool: ^GBytes;
  hwEad: array[0..3] of byte;
  hwWb: array[0..1] of byte;
  hwFcb: array[0..35] of byte;

{ ---- 7220 ---- }

procedure hwGw;
var n: integer;
begin
  n := 0;
  while ((Port[$A0] and 2) <> 0) and (n < hwZEIT) do n := n + 1;
end;

procedure hwCmd(c: integer);
begin
  hwGw; Port[$A1] := c;
end;

procedure hwPar(c: integer);
begin
  hwGw; Port[$A0] := c;
end;

procedure hwParZ(c: integer);
begin
  hwGw; Port[$A2] := c;
end;

function hwLies(m: integer; var d): integer;
var b: GBytes absolute d;
    i, n: integer;
begin
  hwLies := 0;
  for i := 0 to m - 1 do begin
    n := 0;
    while ((Port[$A0] and 1) = 0) and (n < hwZEIT) do n := n + 1;
    if n >= hwZEIT then begin hwLies := 1; exit; end;
    b[i] := Port[$A1];
  end;
end;

procedure ghlist(var l);
var b: GBytes absolute l;
    i, n: integer;
begin
  i := 0;
  while b[i] <> 0 do begin
    n := b[i];
    while (Port[$A0] and 2) <> 0 do ;
    Port[$A1] := b[i + 1];
    i := i + 2;
    n := n - 1;
    while n > 0 do begin
      while (Port[$A0] and 2) <> 0 do ;
      Port[$A0] := b[i];
      i := i + 1;
      n := n - 1;
    end;
  end;
end;

function ghrdat(var bl): integer;
var b: GBytes absolute bl;
    m, p: integer;
begin
  m := b[0] * 256 + b[1];
  p := 2;
  while b[p] <> 0 do p := p + b[p] + 1;
  ghlist(b[2]);
  ghrdat := hwLies(m, b[p + 1]);
end;

function ghstat: integer;
begin
  ghstat := Port[$A0];
end;

procedure ghvsyn;
var n: integer;
begin
  n := 0;
  while ((Port[$A0] and $20) <> 0) and (n < hwZEIT) do n := n + 1;
  n := 0;
  while ((Port[$A0] and $20) = 0) and (n < hwZEIT) do n := n + 1;
end;

function ghvzahl: integer;
var n: integer;
begin
  n := 0;
  while ((Port[$A0] and $20) <> 0) and (n < 20000) do n := n + 1;
  while ((Port[$A0] and $20) = 0) and (n < 30000) do n := n + 1;
  ghvzahl := n;
end;

function hwWLies(a: integer): integer;
begin
  hwCmd($49); hwPar(Lo(a)); hwPar(Hi(a)); hwPar(0);
  hwCmd($4A); hwPar($FF); hwPar($FF);
  hwCmd($4C); hwPar($02); hwPar(1); hwPar(0);
  hwCmd($A0);
  hwWb[0] := 0; hwWb[1] := 0;
  if hwLies(2, hwWb) = 0 then ;
  hwWLies := hwWb[0] + 256 * hwWb[1];
end;

procedure hwWSchr(a, w: integer);
begin
  hwCmd($49); hwPar(Lo(a)); hwPar(Hi(a)); hwPar(0);
  hwCmd($4A); hwPar($FF); hwPar($FF);
  hwCmd($4C); hwPar($02); hwPar(0); hwPar(0);
  hwCmd($20); hwPar(Lo(w)); hwPar(Hi(w));
end;

function hwFarbtest: integer;
var v: integer;
begin
  hwWSchr($3FF0, $1234);
  hwWSchr($7FF0, $A55A);
  v := hwWLies($3FF0);
  hwWSchr($3FF0, 0);
  hwWSchr($7FF0, 0);
  if v = $1234 then hwFarbtest := 1 else hwFarbtest := 0;
end;

function hwMaustest: integer;
var v, i: integer;
begin
  hwMaustest := 0;
  v := $FF;
  for i := 0 to 3 do v := v and Port[$30 + i];
  if v = $FF then exit;
  if (Port[$33] and $80) = 0 then begin
    Port[$36] := $4E;
    Port[$36] := $7E;
    v := Port[$32];
    v := Port[$32];
    if (v = $7E) or (v = $3E) then exit;
  end;
  i := 0;
  while (i < 64) and ((Port[$31] and 1) <> 0) do begin
    v := Port[$30]; i := i + 1;
  end;
  hwMaustest := $30;
end;

function ghinit: integer;
var a, n: integer;
begin
  hwBn := -1;
  n := 0;
  while (n < 16) and ((Port[$A0] and 1) <> 0) do begin
    a := Port[$A1]; n := n + 1;
  end;
  hwCmd($E0);
  hwEad[0] := 0; hwEad[1] := 0; hwEad[2] := 0;
  if hwLies(3, hwEad) = 0 then ;
  n := 0;
  while (n < 16) and ((Port[$A0] and 1) <> 0) do begin
    a := Port[$A1]; n := n + 1;
  end;
  hwFarbe := hwFarbtest;
  hwCmd($49); hwPar(hwEad[0]); hwPar(hwEad[1]); hwPar(hwEad[2] and 3);
  hwCmd($4A); hwPar($FF); hwPar($FF);
  hwK806 := hwMaustest;
  a := 0;
  if hwFarbe <> 0 then a := a or GH_FARBE;
  if hwK806 <> 0 then a := a or GH_MAUS;
  ghinit := a;
end;

{ Grafik ein wie unter DOS; zurueck wie der Konsolentreiber von NCRs CP/M }
procedure ghgraf(an: boolean);
var i, e: integer;
begin
  if an then begin
    hwCmd($0C);
    hwCmd($70);
    for i := 1 to 2 do begin
      hwPar(0); hwPar(0); hwPar(0); hwPar($59);
    end;
    hwCmd($47); hwPar(80);
    hwCmd($49); hwPar(0); hwPar(0); hwPar(0);
    hwCmd($46); hwParZ(0);
    hwCmd($4A); hwPar($FF); hwPar($FF);
    if hwFarbe <> 0 then e := 3 else e := 1;
    for i := 1 to e do begin
      hwCmd($4C); hwPar($02); hwPar($FF); hwPar($7F);
      hwCmd($22); hwPar($FF); hwPar($FF);
    end;
    hwCmd($6B);
    hwCmd($0D);
  end else begin
    hwCmd($0C);
    hwCmd($46); hwParZ(0);
    hwCmd($78);
    for i := 1 to 8 do hwPar($FF);
    hwCmd($47); hwPar(80);
    hwCmd($70);
    hwPar(0); hwPar(0); hwPar(0); hwPar(25);
    hwPar(0); hwPar(0); hwPar(0); hwPar(0);
    hwCmd($49); hwPar(0); hwPar(0); hwPar(0);
    hwCmd($4A); hwPar($FF); hwPar($FF);
    hwCmd($4C); hwPar($02); hwPar(Lo(1999)); hwPar(Hi(1999));
    hwCmd($20); hwPar($20); hwPar($E8);       { Leerzeichen, normal }
    hwCmd($4C); hwPar($02); hwPar(0); hwPar(0);
    hwCmd($49); hwPar(0); hwPar(0); hwPar(0);
    hwCmd($6B);
    hwCmd($0D);
    Bdos(2, $1A);                             { die Konsole loescht selbst }
  end;
end;

function ghdset(an: integer): integer;
begin
  ghdset := -1;
end;

{ ---- Maus K806 ---- }

function hwMw(bit, soll: integer): integer;
var n: integer;
begin
  hwMw := 0;
  for n := 1 to hwZEIT do
    if ((Port[hwK806 + 1] and bit) <> 0) = (soll <> 0) then exit;
  hwMw := 1;
end;

function ghmaus(c: integer; var bl): integer;
var b: GBytes absolute bl;
    i, p, a, k: integer;
begin
  ghmaus := 1;
  if hwK806 = 0 then exit;
  ghmaus := 2;
  i := 0;
  while (i < 64) and ((Port[hwK806 + 1] and 1) <> 0) do begin
    a := Port[hwK806]; i := i + 1;
  end;
  p := b[0];
  k := 1;
  if hwMw(2, 0) <> 0 then exit;
  Port[hwK806 + 1] := c;
  while p > 0 do begin
    if hwMw(2, 0) <> 0 then exit;
    Port[hwK806] := b[k];
    k := k + 1; p := p - 1;
  end;
  a := b[k];
  k := k + 1;
  while a > 0 do begin
    if hwMw(1, 1) <> 0 then exit;
    b[k] := Port[hwK806];
    k := k + 1; a := a - 1;
  end;
  ghmaus := 0;
end;

{ ---- Ton ueber den 8741 ---- }

procedure hwKw;
var n: integer;
begin
  n := 0;
  while ((Port[$41] and 2) <> 0) and (n < hwZEIT) do n := n + 1;
end;

function ghton(h, l: integer): integer;
begin
  ghton := 1;
  if (Port[$41] and 2) <> 0 then exit;
  Port[$41] := $06;
  hwKw;
  if h > 42 then h := 42;
  Port[$40] := h + $20;
  hwKw;
  if l < 1 then l := 1;
  if l > 224 then l := 224;
  Port[$40] := l + $1F;
  ghton := 0;
end;

{ ---- Tastatur, Text, Dateien ueber BDOS ---- }

function ghtast: integer;           { BDOS 6 mit FFh: holen ohne Warten }
begin
  ghtast := Bdos(6, $FF);
end;

procedure ghtext(s: GStr);
var i: integer;
begin
  for i := 1 to Length(s) do begin
    if s[i] = #10 then Bdos(2, 13);
    Bdos(2, Ord(s[i]));
  end;
end;

function ghopen(name: GStr): integer;
var i, j: integer;
    c: char;
begin
  FillChar(hwFcb, 36, 0);
  FillChar(hwFcb[1], 11, Ord(' '));
  j := 1;
  if (Length(name) >= 2) and (name[2] = ':') then begin
    hwFcb[0] := Ord(UpCase(name[1])) - Ord('A') + 1;
    j := 3;
  end;
  i := 1;
  while (j <= Length(name)) and (name[j] <> '.') and (i < 9) do begin
    hwFcb[i] := Ord(UpCase(name[j])); i := i + 1; j := j + 1;
  end;
  while (j <= Length(name)) and (name[j] <> '.') do j := j + 1;
  if (j <= Length(name)) and (name[j] = '.') then begin
    j := j + 1; i := 9;
    while (j <= Length(name)) and (i < 12) do begin
      hwFcb[i] := Ord(UpCase(name[j])); i := i + 1; j := j + 1;
    end;
  end;
  if Bdos(15, Addr(hwFcb)) = $FF then ghopen := 1 else ghopen := 0;
end;

function ghread(var bl): integer;   { 128 Byte }
begin
  Bdos(26, Addr(bl));
  if Bdos(20, Addr(hwFcb)) = 0 then ghread := 1 else ghread := 0;
  Bdos(26, $80);
end;

procedure ghclos;
begin
  Bdos(16, Addr(hwFcb));
end;

{ ---- Bildspeicher: ein Block auf dem Heap ---- }

function ghsnum: integer;
var f: real;
begin
  if hwBn < 0 then begin
    hwBn := 0;
    f := MaxAvail;
    if f < 0 then f := f + 65536.0;
    f := f - hwPRES;
    if f > hwPMAX then f := hwPMAX;
    if f >= 1024 then begin
      hwPn := Trunc(f);
      GetMem(hwPool, hwPn);
      hwBn := 1;
    end;
  end;
  ghsnum := hwBn;
end;

function ghsgro(b: integer): real;
begin
  ghsgro := hwPn;
end;

procedure ghsput(b, o: integer; var q; n: integer);
begin
  Move(q, hwPool^[o], n);
end;

procedure ghsget(var z; b, o, n: integer);
begin
  Move(hwPool^[o], z, n);
end;

procedure ghsmov(b, z, q, n: integer);
begin
  Move(hwPool^[q], hwPool^[z], n);
end;

function hwAdr(var x): GBZeiger;   { Zeiger auf eine Variable }
begin
  hwAdr := Ptr(Addr(x));
end;

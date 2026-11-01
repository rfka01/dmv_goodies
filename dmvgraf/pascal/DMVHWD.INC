{ DMVHWD.INC - DMVGRAF fuer Turbo Pascal 3: Hardwareschicht MS-DOS
  (8088/V20 der K230/K231/K235).

  Dasselbe wie GHWDOS.C der C-Fassung: 7220 (A0h-A2h), Maus K806
  (Steckplatz 2A, 30h-33h) und Tastatur-8741 (40h/41h) direkt,
  Tastatur, Bildschirmtext und Dateien ueber MsDos, Bildspeicher in
  Bloecken zu 64 KB von DOS (Funktion 48h).

  Reihenfolge im Programm: DMVHWD.INC, DMVFONT.INC, DMVGRAF.INC.

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

type
  GBytes  = array[0..32767] of byte;
  GStr    = string[255];
  GBZeiger = ^GBytes;
  GDosReg = record AX, BX, CX, DX, BP, SI, DI, DS, ES, Flags: integer; end;

const
  GH_FARBE = 1;  GH_MAUS = 2;  GH_SYN = 4;  GH_COVOX = 8;
  hwZEIT   = 30000;                 { Wartegrenze in Abfragen }
  hwGBBLK  = 2;                     { hoechstens so viele 64-KB-Bloecke }

var
  hwK806, hwFarbe: integer;
  hwFh: integer;                    { offene Datei, -1 keine }
  hwBn: integer;                    { Bildspeicherbloecke, -1 = noch nicht }
  hwBseg: array[0..1] of integer;
  hwR: GDosReg;
  hwEad: array[0..3] of byte;
  hwWb: array[0..1] of byte;

{ ---- 7220 ---- }

procedure hwGw;                     { bis das FIFO nicht voll ist }
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

procedure hwParZ(c: integer);       { ZOOM-Parameter ueber A2h }
begin
  hwGw; Port[$A2] := c;
end;

function hwLies(m: integer; var d): integer;  { m Bytes aus dem FIFO }
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

{ GDC-Liste wie BIOS 66: Eintraege n, Befehl, n-1 Parameter; 0 Ende }
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

{ Block wie BIOS 67/2: Wortzahl (hoch, niedrig), Liste, dahinter Platz }
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

procedure ghvsyn;                   { Status Bit 5 von 0 auf 1 }
var n: integer;
begin
  n := 0;
  while ((Port[$A0] and $20) <> 0) and (n < hwZEIT) do n := n + 1;
  n := 0;
  while ((Port[$A0] and $20) = 0) and (n < hwZEIT) do n := n + 1;
end;

function ghvzahl: integer;          { Statusabfragen bis zum Bildwechsel }
var n: integer;
begin
  n := 0;
  while ((Port[$A0] and $20) <> 0) and (n < 20000) do n := n + 1;
  while ((Port[$A0] and $20) = 0) and (n < 30000) do n := n + 1;
  ghvzahl := n;
end;

function hwWLies(a: integer): integer;  { ein Wort aus dem Bildspeicher }
begin
  hwCmd($49); hwPar(Lo(a)); hwPar(Hi(a)); hwPar(0);
  hwCmd($4A); hwPar($FF); hwPar($FF);
  hwCmd($4C); hwPar($02); hwPar(1); hwPar(0);
  hwCmd($A0);
  hwWb[0] := 0; hwWb[1] := 0;
  if hwLies(2, hwWb) = 0 then ;
  hwWLies := hwWb[0] + 256 * hwWb[1];
end;

procedure hwWSchr(a, w: integer);   { ein Wort schreiben }
begin
  hwCmd($49); hwPar(Lo(a)); hwPar(Hi(a)); hwPar(0);
  hwCmd($4A); hwPar($FF); hwPar($FF);
  hwCmd($4C); hwPar($02); hwPar(0); hwPar(0);
  hwCmd($20); hwPar(Lo(w)); hwPar(Hi(w));
end;

{ Farbe: die Monochromkarte dekodiert nur A0-A13 und findet ihre gruene
  Ebene bei 4000h und 8000h gespiegelt }
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

{ K806 auf 2A (30h-33h), wie MSPROBE im BIOS von CP/M-8000 }
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
    if (v = $7E) or (v = $3E) then exit;    { 2661 (K801), 2651 }
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
  hwFh := -1;
  hwBn := -1;
  { Die DOS-Konsole schreibt nach dem Farbtest an der alten Stelle
    weiter: Adresse (EAD) vorher mit CURD lesen, hinterher mit CURS
    zurueck, MASK FFFFh }
  n := 0;
  while (n < 16) and ((Port[$A0] and 1) <> 0) do begin
    a := Port[$A1]; n := n + 1;
  end;
  hwCmd($E0);
  hwEad[0] := 0; hwEad[1] := 0; hwEad[2] := 0;
  if hwLies(3, hwEad) = 0 then ;
  n := 0;
  while (n < 16) and ((Port[$A0] and 1) <> 0) do begin
    a := Port[$A1]; n := n + 1;            { der echte 7220 schickt noch MASK }
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

{ Grafik ein und aus wie GINIT/GEXIT von NCRs GRAPH-Bibliothek unter DOS }
procedure ghgraf(an: boolean);
var i, e: integer;
begin
  if an then begin
    hwCmd($0C);                             { Anzeige aus }
    hwCmd($4B); hwPar(0);                   { CCHAR: kein Cursor }
    hwCmd($70);                             { PRAM: zwei Bildbereiche }
    for i := 1 to 2 do begin
      hwPar(0); hwPar(0); hwPar(0); hwPar($59);
    end;
    hwCmd($47); hwPar(80);                  { PITCH }
    hwCmd($49); hwPar(0); hwPar(0); hwPar(0);
    hwCmd($46); hwParZ(0);                  { ZOOM 0 }
    hwCmd($4A); hwPar($FF); hwPar($FF);     { MASK }
    if hwFarbe <> 0 then e := 3 else e := 1;
    for i := 1 to e do begin
      hwCmd($4C); hwPar($02); hwPar($FF); hwPar($7F);
      hwCmd($22); hwPar($FF); hwPar($FF);
    end;
    hwCmd($6B);                             { START }
    hwCmd($0D);                             { Anzeige an }
  end else begin
    hwCmd($0C);
    hwCmd($46); hwParZ(0);
    hwCmd($4B); hwPar($8F);                 { CCHAR wie NCRs GEXIT }
    hwCmd($70);
    hwPar(0); hwPar(0); hwPar(0); hwPar($19);
    hwPar(0); hwPar(0); hwPar(0); hwPar($19);
    for i := 1 to 8 do hwPar($FF);
    hwCmd($47); hwPar(80);
    hwCmd($49); hwPar(0); hwPar(0); hwPar(0);
    hwCmd($4A); hwPar($FF); hwPar($FF);
    hwCmd($6B);
    hwCmd($0D);
    hwR.AX := $0200; hwR.DX := $1A;         { die Konsole loescht selbst }
    MsDos(hwR);
  end;
end;

function ghdset(an: integer): integer;     { unter DOS nur der direkte Weg }
begin
  ghdset := -1;
end;

{ ---- Maus K806 (wie BIOS 68) ---- }

function hwMw(bit, soll: integer): integer;  { Status-Bit abwarten }
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

{ ---- Ton ueber den 8741 (wie BIOS 69, DMVTRAK) ---- }

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

{ ---- Tastatur, Text, Dateien ueber DOS ---- }

function ghtast: integer;           { 06h mit DL = FFh: holen ohne Warten }
begin
  hwR.AX := $0600; hwR.DX := $FF;
  MsDos(hwR);
  if (hwR.Flags and $40) <> 0 then ghtast := 0
  else ghtast := Lo(hwR.AX);
end;

procedure ghtext(s: GStr);
var i: integer;
begin
  for i := 1 to Length(s) do begin
    if s[i] = #10 then begin
      hwR.AX := $0200; hwR.DX := 13; MsDos(hwR);
    end;
    hwR.AX := $0200; hwR.DX := Ord(s[i]); MsDos(hwR);
  end;
end;

function ghopen(name: GStr): integer;
var z: array[0..80] of byte;
    i: integer;
begin
  for i := 1 to Length(name) do z[i - 1] := Ord(name[i]);
  z[Length(name)] := 0;
  hwR.AX := $3D00;                  { nur lesen }
  hwR.DS := Seg(z); hwR.DX := Ofs(z);
  MsDos(hwR);
  if (hwR.Flags and 1) <> 0 then ghopen := 1
  else begin hwFh := hwR.AX; ghopen := 0; end;
end;

function ghread(var bl): integer;   { 128 Byte; ein kurzer Rest mit ^Z }
var b: GBytes absolute bl;
    i: integer;
begin
  hwR.AX := $3F00; hwR.BX := hwFh; hwR.CX := 128;
  hwR.DS := Seg(b); hwR.DX := Ofs(b);
  MsDos(hwR);
  if ((hwR.Flags and 1) <> 0) or (hwR.AX = 0) then ghread := 0
  else begin
    for i := hwR.AX to 127 do b[i] := $1A;
    ghread := 1;
  end;
end;

procedure ghclos;
begin
  if hwFh >= 0 then begin
    hwR.AX := $3E00; hwR.BX := hwFh; MsDos(hwR);
    hwFh := -1;
  end;
end;

{ ---- Bildspeicher: Bloecke zu 64 KB von DOS ---- }

function ghsnum: integer;
begin
  if hwBn < 0 then begin
    hwBn := 0;
    while hwBn < hwGBBLK do begin
      hwR.AX := $4800; hwR.BX := $1000;     { 4096 Absaetze = 64 KB }
      MsDos(hwR);
      if (hwR.Flags and 1) <> 0 then begin
        ghsnum := hwBn; exit;
      end;
      hwBseg[hwBn] := hwR.AX;
      hwBn := hwBn + 1;
    end;
  end;
  ghsnum := hwBn;
end;

function ghsgro(b: integer): real;
begin
  ghsgro := 65536.0;
end;

procedure ghsput(b, o: integer; var q; n: integer);
begin
  Move(q, Mem[hwBseg[b]:o], n);
end;

procedure ghsget(var z; b, o, n: integer);
begin
  Move(Mem[hwBseg[b]:o], z, n);
end;

procedure ghsmov(b, z, q, n: integer);     { z < q, vorwaerts }
begin
  Move(Mem[hwBseg[b]:q], Mem[hwBseg[b]:z], n);
end;

function hwAdr(var x): GBZeiger;   { Zeiger auf eine Variable }
begin
  hwAdr := Ptr(Seg(x), Ofs(x));
end;

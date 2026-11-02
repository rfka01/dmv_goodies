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
  GStr    = string[80];            { CP/M-80: eine Bildzeile, spart Platz }
  GBZeiger = ^GBytes;

const
  GH_FARBE = 1;  GH_MAUS = 2;  GH_SYN = 4;  GH_COVOX = 8;
  hwZEIT   = 30000;
  hwPMAX   = 16384;                 { Bildspeicher hoechstens }
  hwPRES   = 256;                   { so viel Heap bleibt frei (TP3 haelt selbst 1 KB) }
  hwQL     = 256;                   { GDC-Liste }
  hwQB     = 384;  hwQB1 = 383;     { Bildzeilenpuffer (mindestens 320) }

var
  hwK806, hwFarbe: integer;
  hwBn, hwPn: integer;              { Bildspeicher: Bloecke (-1 noch nicht), Groesse }
  hwPool: ^GBytes;
  hwEad: array[0..3] of byte;
  hwWb: array[0..1] of byte;
  hwFcb: array[0..35] of byte;
  hwSrc, hwDst, hwN: integer;       { fuer die Maschinenbefehle }
  hwL, hwQn, hwQe, hwAd, hwSx, hwNb, hwW, hwAa, hwMsk,
  hwTpl, hwTn, hwHl: integer;
  hwWst: boolean;                   { MASK FFFF und FIGS fuer WDAT gesetzt }
  hwRomz, hwSchirm: integer;        { Kennbuchstabe der Firmware, Schirm 1-3 }
  hwT, hwFF: array[0..79] of byte;
  hwXe, hwXp, hwXa, hwXw, hwXm: integer;   { fuer ghblit }
  hwK: array[0..15] of byte;

{ ---- 7220 ---- }

procedure hwGw;
var n: integer;
begin
  n := 0;
  while ((Port[$A0] and 2) <> 0) and (n < hwZEIT) do n := n + 1;
end;

procedure hwCmd(c: integer);
begin
  hwWst := false;
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

{ GDC-Liste ausgeben (n, Befehl, n-1 Parameter; 0 = Ende). In
  Maschinensprache, sonst bremst der Z80 hier alles aus. }
procedure ghlist(var l);
begin
  hwSrc := Addr(l);
  inline(
    $2A/>hwSrc/                       { LD HL,(hwSrc) }
    { satz: }
    $7E/                              { LD A,(HL)      Anzahl }
    $B7/                              { OR A }
    $28/$1D/                          { JR Z,ende }
    $47/                              { LD B,A }
    $23/                              { INC HL }
    { w1: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2          FIFO voll? }
    $20/$FA/                          { JR NZ,w1 }
    $7E/                              { LD A,(HL) }
    $D3/$A1/                          { OUT (A1h),A    Befehl }
    $23/                              { INC HL }
    $05/                              { DEC B }
    $28/$ED/                          { JR Z,satz }
    { w2: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w2 }
    $7E/                              { LD A,(HL) }
    $D3/$A0/                          { OUT (A0h),A    Parameter }
    $23/                              { INC HL }
    $10/$F4/                          { DJNZ w2 }
    $18/$DF                           { JR satz }
    { ende: }
  );
end;

{ Eintrag fuer eine Bitzeile an die GDC-Liste lst (ab qn) haengen:
  CURS adr, MASK FFFF, FIGS, WDAT w mit den Worten (MASK und FIGS
  entfallen, solange sie vom letzten Eintrag her gelten). nb Bit ab
  Punkt sx aus bb; a: 0 Bits, 1 umgekehrt, 2 lauter Einsen, 3 lauter
  Nullen, 4 mm and not bb, 5 mm and bb (mm: Maske, sonst unbenutzt),
  6 beides nacheinander: w mit mm and not bb, dann w + 1 mit mm and bb.
  Bleibt bei w <> 20h nichts zu tun, faellt der Eintrag wieder weg
  (qe := -1). Platz muss vorher geschaffen sein. In Maschinensprache:
  das ist die meistbenutzte Stelle der Bibliothek. }
const hwKopf: array[0..15] of byte =   { CURS, MASK FFFF, FIGS, WDAT }
        (4, $49, 0, 0, 0, 3, $4A, $FF, $FF, 4, $4C, 2, 0, 0, 0, 0);
      hwKurz: array[0..5] of byte =    { CURS, WDAT (MASK, FIGS gelten noch) }
        (3, $49, 0, 0, 0, 0);

procedure hwEin;                    { ein Eintrag nach den hw-Variablen }
begin
  if hwAa = 0 then hwK[7] := 0 else hwK[7] := $FF;  { X: umkehren }
  if hwWst then begin hwTpl := Addr(hwKurz); hwTn := 6; hwHl := 4; end
  else begin hwTpl := Addr(hwKopf); hwTn := 16; hwHl := 14; end;
  inline(
    $DD/$E5/                          { PUSH IX }
    $3A/>hwSx/                        { LD A,(hwSx) }
    $E6/$0F/                          { AND 15 }
    $32/>hwK+0/                       { LD (hwK+0),A  sh }
    $6F/                              { LD L,A }
    $26/$00/                          { LD H,0 }
    $ED/$5B/>hwNb/                    { LD DE,(hwNb) }
    $19/                              { ADD HL,DE }
    $11/$0F/$00/                      { LD DE,15 }
    $19/                              { ADD HL,DE }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $7D/                              { LD A,L }
    $87/                              { ADD A,A }
    $32/>hwK+1/                       { LD (hwK+1),A  k }
    $3C/                              { INC A }
    $32/>hwKopf+14/                   { LD (hwKopf+14),A  1+k }
    $32/>hwKurz+4/                    { LD (hwKurz+4),A }
    $2A/>hwNb/                        { LD HL,(hwNb) }
    $11/$07/$00/                      { LD DE,7 }
    $19/                              { ADD HL,DE }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $7D/                              { LD A,L }
    $32/>hwK+2/                       { LD (hwK+2),A  n8 }
    $3A/>hwNb/                        { LD A,(hwNb) }
    $E6/$07/                          { AND 7 }
    $47/                              { LD B,A }
    $3E/$FF/                          { LD A,FFh }
    $28/$05/                          { JR Z,lz }
    $AF/                              { XOR A }
    { lzs: }
    $37/                              { SCF }
    $17/                              { RLA }
    $10/$FC/                          { DJNZ lzs }
    { lz: }
    $32/>hwK+3/                       { LD (hwK+3),A  letzt }
    $2A/>hwAd/                        { LD HL,(hwAd) }
    $22/>hwKopf+2/                    { LD (hwKopf+2),HL }
    $22/>hwKurz+2/                    { LD (hwKurz+2),HL }
    $3A/>hwW/                         { LD A,(hwW) }
    $32/>hwKopf+15/                   { LD (hwKopf+15),A }
    $32/>hwKurz+5/                    { LD (hwKurz+5),A }
    $2A/>hwL/                         { LD HL,(hwL) }
    $ED/$5B/>hwQn/                    { LD DE,(hwQn) }
    $19/                              { ADD HL,DE }
    $EB/                              { EX DE,HL }
    $2A/>hwTpl/                       { LD HL,(hwTpl)  Kopf lang oder kurz }
    $ED/$4B/>hwTn/                    { LD BC,(hwTn) }
    $ED/$B0/                          { LDIR }
    $2A/>hwQn/                        { LD HL,(hwQn) }
    $ED/$4B/>hwHl/                    { LD BC,(hwHl) }
    $09/                              { ADD HL,BC }
    $22/>hwQe/                        { LD (hwQe),HL }
    $23/                              { INC HL }
    $23/                              { INC HL }
    $22/>hwQn/                        { LD (hwQn),HL }
    $ED/$53/>hwDst/                   { LD (hwDst),DE }
    $DD/$E1                           { POP IX }
  );
  if (((hwSx or hwNb) and 15) = 0) and (hwAa = 0) and (hwMsk = Addr(hwFF)) then
      inline(
        $ED/$5B/>hwDst/                   { LD DE,(hwDst) }
        $2A/>hwSrc/                       { LD HL,(hwSrc) }
        $3A/>hwK+1/                       { LD A,(hwK+1) }
        $4F/                              { LD C,A }
        $06/$00/                          { LD B,0 }
        $ED/$B0/                          { LDIR }
        $2A/>hwQn/                        { LD HL,(hwQn) }
        $3A/>hwK+1/                       { LD A,(hwK+1) }
        $5F/                              { LD E,A }
        $16/$00/                          { LD D,0 }
        $19/                              { ADD HL,DE }
        $22/>hwQn                         { LD (hwQn),HL }
      )
  else
      inline(
        $DD/$E5/                          { PUSH IX }
        $FD/$E5/                          { PUSH IY }
        $FD/$2A/>hwMsk/                   { LD IY,(hwMsk) }
        $ED/$5B/>hwDst/                   { LD DE,(hwDst) }
        $AF/                              { XOR A }
        $32/>hwK+6/                       { LD (hwK+6),A  oder }
        $4F/                              { LD C,A        alt }
        $3A/>hwK+1/                       { LD A,(hwK+1) }
        $47/                              { LD B,A }
        $3A/>hwK+0/                       { LD A,(hwK+0) }
        $FE/$08/                          { CP 8 }
        $38/$04/                          { JR C,nv }
        $AF/                              { XOR A }
        $12/                              { LD (DE),A }
        $13/                              { INC DE }
        $05/                              { DEC B }
        { nv: }
        $78/                              { LD A,B }
        $32/>hwK+4/                       { LD (hwK+4),A  rest }
        $3A/>hwK+0/                       { LD A,(hwK+0) }
        $E6/$07/                          { AND 7 }
        $32/>hwK+5/                       { LD (hwK+5),A  sh7 }
        $3A/>hwK+2/                       { LD A,(hwK+2) }
        $47/                              { LD B,A        n8 }
        $3A/>hwK+4/                       { LD A,(hwK+4) }
        $B8/                              { CP B }
        $30/$09/                          { JR NC,vok }
        $47/                              { LD B,A }
        $3E/$FF/                          { LD A,FFh }
        $32/>hwK+3/                       { LD (hwK+3),A }
        $3A/>hwK+4/                       { LD A,(hwK+4) }
        { vok: }
        $90/                              { SUB B }
        $32/>hwK+4/                       { LD (hwK+4),A }
        $DD/$2A/>hwSrc/                   { LD IX,(hwSrc) }
        $05/                              { DEC B }
        $28/$27/                          { JR Z,le }
        { lp: }
        $DD/$7E/$00/                      { LD A,(IX+0) }
        $DD/$23/                          { INC IX }
        $21/>hwK+7/                       { LD HL,hwK+7 }
        $AE/                              { XOR (HL)       X }
        $FD/$A6/$00/                      { AND (IY+0)     Maske }
        $FD/$23/                          { INC IY }
        $6F/                              { LD L,A }
        $26/$00/                          { LD H,0 }
        $3A/>hwK+5/                       { LD A,(hwK+5)   sh7 }
        $B7/                              { OR A }
        $28/$04/                          { JR Z,n0 }
        { s0: }
        $29/                              { ADD HL,HL }
        $3D/                              { DEC A }
        $20/$FC/                          { JR NZ,s0 }
        { n0: }
        $7D/                              { LD A,L }
        $B1/                              { OR C }
        $4C/                              { LD C,H }
        $12/                              { LD (DE),A }
        $13/                              { INC DE }
        $21/>hwK+6/                       { LD HL,hwK+6 }
        $B6/                              { OR (HL) }
        $77/                              { LD (HL),A }
        $10/$D9/                          { DJNZ lp }
        { le: }
        $DD/$7E/$00/                      { LD A,(IX+0) }
        $DD/$23/                          { INC IX }
        $21/>hwK+7/                       { LD HL,hwK+7 }
        $AE/                              { XOR (HL)       X }
        $FD/$A6/$00/                      { AND (IY+0)     Maske }
        $FD/$23/                          { INC IY }
        $21/>hwK+3/                       { LD HL,hwK+3 }
        $A6/                              { AND (HL)       letzt }
        $6F/                              { LD L,A }
        $26/$00/                          { LD H,0 }
        $3A/>hwK+5/                       { LD A,(hwK+5)   sh7 }
        $B7/                              { OR A }
        $28/$04/                          { JR Z,n1 }
        { s1: }
        $29/                              { ADD HL,HL }
        $3D/                              { DEC A }
        $20/$FC/                          { JR NZ,s1 }
        { n1: }
        $7D/                              { LD A,L }
        $B1/                              { OR C }
        $4C/                              { LD C,H }
        $12/                              { LD (DE),A }
        $13/                              { INC DE }
        $21/>hwK+6/                       { LD HL,hwK+6 }
        $B6/                              { OR (HL) }
        $77/                              { LD (HL),A }
        $3A/>hwK+4/                       { LD A,(hwK+4) }
        $B7/                              { OR A }
        $28/$11/                          { JR Z,rf }
        $47/                              { LD B,A }
        $79/                              { LD A,C }
        $12/                              { LD (DE),A }
        $13/                              { INC DE }
        $21/>hwK+6/                       { LD HL,hwK+6 }
        $B6/                              { OR (HL) }
        $77/                              { LD (HL),A }
        $05/                              { DEC B }
        $28/$05/                          { JR Z,rf }
        $AF/                              { XOR A }
        { rz: }
        $12/                              { LD (DE),A }
        $13/                              { INC DE }
        $10/$FC/                          { DJNZ rz }
        { rf: }
        $3A/>hwK+6/                       { LD A,(hwK+6) }
        $B7/                              { OR A }
        $20/$1C/                          { JR NZ,dazu }
        $3A/>hwW/                         { LD A,(hwW) }
        $FE/$20/                          { CP 20h }
        $28/$15/                          { JR Z,dazu }
        $2A/>hwQe/                        { LD HL,(hwQe)  leer: zurueck }
        $ED/$5B/>hwHl/                    { LD DE,(hwHl) }
        $B7/                              { OR A }
        $ED/$52/                          { SBC HL,DE }
        $22/>hwQn/                        { LD (hwQn),HL }
        $21/$FF/$FF/                      { LD HL,-1 }
        $22/>hwQe/                        { LD (hwQe),HL }
        $18/$0D/                          { JR ende }
        { dazu: }
        $2A/>hwQn/                        { LD HL,(hwQn) }
        $3A/>hwK+1/                       { LD A,(hwK+1) }
        $5F/                              { LD E,A }
        $16/$00/                          { LD D,0 }
        $19/                              { ADD HL,DE }
        $22/>hwQn/                        { LD (hwQn),HL }
        { ende: }
        $FD/$E1/                          { POP IY }
        $DD/$E1                           { POP IX }
      );
  if hwQe <> -1 then hwWst := true;
end;

procedure ghrow(var lst; var qn, qe: integer; adr, sx, nb, w, a: integer;
                var bb; var mm);
begin
  hwL := Addr(lst); hwQn := qn; hwQe := qe; hwAd := adr;
  hwSx := sx; hwNb := nb; hwW := w; hwSrc := Addr(bb);
  hwMsk := Addr(hwFF);
  case a of
    2: begin FillChar(hwT, 80, $FF); hwSrc := Addr(hwT); a := 0; end;
    3: begin FillChar(hwT, 80, 0); hwSrc := Addr(hwT); a := 0; end;
    4: begin hwMsk := Addr(mm); a := 1; end;
    5: begin hwMsk := Addr(mm); a := 0; end;
  end;
  if a = 6 then begin
    hwMsk := Addr(mm);
    hwAa := 1; hwEin;               { Maske ohne Punkte: aus }
    hwAa := 0; hwW := w + 1; hwEin; { Maske mit Punkten: an }
  end else begin
    hwAa := a; hwEin;
  end;
  qn := hwQn; qe := hwQe;
end;

{ Eine Bildzeile fuer alle Ebenen ab e0 an die Liste haengen: Ebene e
  aus pl + e * st, Maske mk; Wortadresse ebad[e] + ad. art wie gbput
  (deckend nur fuer ganze Worte). Liefert die naechste Ebene; ist die
  Liste voll, kleiner als ne (dann ausgeben und weiter). }
function ghbrow(var lst; var qn, qe: integer; var pl; var mk;
                e0, ne, st, ad, sx, nb, art: integer; var ebad): integer;
var eb: array[0..2] of integer absolute ebad;
    e, p: integer;
begin
  hwL := Addr(lst); hwQn := qn; hwQe := qe;
  hwSx := sx; hwNb := nb;
  p := Addr(pl) + e0 * st;
  e := e0;
  while (e < ne) and (hwQn + 48 + (nb shr 2) < hwQL) do begin
    hwAd := eb[e] + ad; hwSrc := p;
    if (art = 0) and (((sx or nb) and 15) = 0) then begin
      hwMsk := Addr(hwFF); hwAa := 0; hwW := $20; hwEin;
    end else if art = 3 then begin
      hwMsk := Addr(mk); hwAa := 0; hwW := $21; hwEin;
    end else begin
      hwMsk := Addr(mk);
      hwAa := 1; hwW := $22; hwEin;
      hwAa := 0; hwW := $23; hwEin;
    end;
    e := e + 1; p := p + st;
  end;
  qn := hwQn; qe := hwQe;
  ghbrow := e;
end;

{ Ein Eintrag (ab hwXe) fuer eine Zeile (ab hwXp) direkt an den 7220 }
procedure hwBlitZ;
begin
  inline(
    $DD/$E5/                          { PUSH IX }
    $FD/$E5/                          { PUSH IY }
    $FD/$2A/>hwXe/                    { LD IY,(hwXe)  Eintrag }
    $0E/$A0/                          { LD C,A0h }
    $3E/$49/                          { LD A,49h }
    $57/                              { LD D,A }
    { w1: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w1 }
    $7A/                              { LD A,D }
    $D3/$A1/                          { OUT (A1h),A  CURS }
    $2A/>hwXa/                        { LD HL,(hwXa) }
    $FD/$5E/$00/                      { LD E,(IY+0) }
    $FD/$56/$01/                      { LD D,(IY+1) }
    $19/                              { ADD HL,DE }
    { w2: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w2 }
    $ED/$69/                          { OUT (C),L }
    { w3: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w3 }
    $ED/$61/                          { OUT (C),H }
    $FD/$7E/$02/                      { LD A,(IY+2) }
    $57/                              { LD D,A }
    { w4: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w4 }
    $7A/                              { LD A,D }
    $D3/$A1/                          { OUT (A1h),A  WDAT }
    $FD/$6E/$04/                      { LD L,(IY+4)  Maske }
    $FD/$66/$05/                      { LD H,(IY+5) }
    $7C/                              { LD A,H }
    $B5/                              { OR L }
    $20/$03/                          { JR NZ,mz }
    $2A/>hwXp/                        { LD HL,(hwXp) Maske aus der Zeile }
    { mz: }
    $DD/$2A/>hwXp/                    { LD IX,(hwXp) }
    $FD/$5E/$08/                      { LD E,(IY+8)  Versatz }
    $FD/$56/$09/                      { LD D,(IY+9) }
    $DD/$19/                          { ADD IX,DE }
    $FD/$5E/$06/                      { LD E,(IY+6)  X }
    $3A/>hwXw/                        { LD A,(hwXw) }
    $47/                              { LD B,A }
    $FD/$7E/$0A/                      { LD A,(IY+10) Nullen? }
    $B7/                              { OR A }
    $20/$38/                          { JR NZ,null }
    { wort: }
    $DD/$7E/$00/                      { LD A,(IX+0)  Ebene }
    $AB/                              { XOR E }
    $A6/                              { AND (HL)     Maske }
    $57/                              { LD D,A }
    $78/                              { LD A,B }
    $3D/                              { DEC A }
    $20/$05/                          { JR NZ,k0 }
    $3A/>hwXm+0/                      { LD A,(hwXm+0) letztes Wort }
    $A2/                              { AND D }
    $57/                              { LD D,A }
    { k0: }
    { w5: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w5 }
    $ED/$51/                          { OUT (C),D }
    $23/                              { INC HL }
    $DD/$23/                          { INC IX }
    $DD/$7E/$00/                      { LD A,(IX+0)  Ebene }
    $AB/                              { XOR E }
    $A6/                              { AND (HL)     Maske }
    $57/                              { LD D,A }
    $78/                              { LD A,B }
    $3D/                              { DEC A }
    $20/$05/                          { JR NZ,k1 }
    $3A/>hwXm+1/                      { LD A,(hwXm+1) letztes Wort }
    $A2/                              { AND D }
    $57/                              { LD D,A }
    { k1: }
    { w6: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w6 }
    $ED/$51/                          { OUT (C),D }
    $23/                              { INC HL }
    $DD/$23/                          { INC IX }
    $10/$CA/                          { DJNZ wort }
    $18/$14/                          { JR ende }
    { null: }
    $16/$00/                          { LD D,0 }
    { nw: }
    { w7: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w7 }
    $ED/$51/                          { OUT (C),D }
    { w8: }
    $DB/$A0/                          { IN A,(A0h) }
    $E6/$02/                          { AND 2 }
    $20/$FA/                          { JR NZ,w8 }
    $ED/$51/                          { OUT (C),D }
    $10/$EE/                          { DJNZ nw }
    { ende: }
    $FD/$E1/                          { POP IY }
    $DD/$E1                           { POP IX }
  );
end;

{ Ganzes Bild direkt an den 7220, wie HOPPLERs BlitAsm: einmal MASK FFFF
  und FIGS, dann je Zeile und je Eintrag der Tabelle tu (nt Eintraege zu
  6 Worten: Ebenenadresse, WDAT-Befehl, Maske 0 = aus der Zeile oder
  FFh, XOR fuer die Ebene, Versatz der Ebene in der Zeile, 1 = lauter
  Nullen) CURS ohne Punkt, WDAT und die nw Worte. Quelle: Block b ab o,
  n Zeilen im Abstand rs; lm maskiert das letzte Wort jeder Zeile; ad =
  Wortadresse der ersten Zeile ohne Ebene. Maschinensprache. }
procedure ghblit(b, o, n, rs, nw, lm, ad: integer; var tu; nt: integer);
var t: array[0..35] of integer absolute tu;
    r, k: integer;
begin
  hwCmd($4A); hwPar($FF); hwPar($FF);         { MASK FFFF }
  hwCmd($4C); hwPar(2); hwPar(0); hwPar(0);   { FIGS: rechts, DC 0 }
  hwXw := nw; hwXm := lm; hwXa := ad;
  hwXp := Addr(hwPool^[o]);
  for r := 1 to n do begin
    for k := 0 to nt - 1 do begin
      hwXe := Addr(t[k * 6]);
      hwBlitZ;
    end;
    hwXp := hwXp + rs;
    hwXa := hwXa + 40;
  end;
  hwWst := false;
end;

{ n Bits ab Bit off von ss nach dd, jedes z1-mal (hoechstens 640 Bit).
  Maschinensprache. }
procedure ghzbits(var dd; var ss; off, n, z1: integer);
var m: integer;
begin
  if (n <= 0) or (z1 <= 0) then exit;
  m := n * z1;
  if m > 640 then m := 640;
  hwDst := Addr(dd); hwSrc := Addr(ss);
  hwSx := off; hwNb := n; hwAa := z1; hwW := m;
  inline(
    $DD/$E5/                          { PUSH IX }
    $DD/$2A/>hwDst/                   { LD IX,(hwDst) }
    $2A/>hwSx/                        { LD HL,(hwSx)   off }
    $7D/                              { LD A,L }
    $E6/$07/                          { AND 7 }
    $47/                              { LD B,A }
    $3E/$01/                          { LD A,1 }
    $28/$03/                          { JR Z,m0 }
    { m1: }
    $07/                              { RLCA }
    $10/$FD/                          { DJNZ m1 }
    { m0: }
    $4F/                              { LD C,A        Quellmaske }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $CB/$3C/$CB/$1D/                  { SRL H / RR L }
    $ED/$5B/>hwSrc/                   { LD DE,(hwSrc) }
    $19/                              { ADD HL,DE }
    $56/                              { LD D,(HL) }
    $1E/$00/                          { LD E,0 }
    $06/$01/                          { LD B,1        Zielmaske }
    { bit: }
    $7A/                              { LD A,D }
    $A1/                              { AND C }
    $28/$02/                          { JR Z,b0 }
    $3E/$FF/                          { LD A,FFh }
    { b0: }
    $32/>hwK+0/                       { LD (hwK+0),A }
    $3A/>hwAa/                        { LD A,(hwAa)   z1 }
    $32/>hwK+1/                       { LD (hwK+1),A }
    { zl: }
    $3A/>hwK+0/                       { LD A,(hwK+0) }
    $A0/                              { AND B }
    $B3/                              { OR E }
    $5F/                              { LD E,A }
    $CB/$00/                          { RLC B }
    $30/$07/                          { JR NC,nw }
    $DD/$73/$00/                      { LD (IX+0),E }
    $DD/$23/                          { INC IX }
    $1E/$00/                          { LD E,0 }
    { nw: }
    $E5/                              { PUSH HL }
    $2A/>hwW/                         { LD HL,(hwW)   m }
    $2B/                              { DEC HL }
    $22/>hwW/                         { LD (hwW),HL }
    $7C/                              { LD A,H }
    $B5/                              { OR L }
    $E1/                              { POP HL }
    $28/$1C/                          { JR Z,fertig }
    $3A/>hwK+1/                       { LD A,(hwK+1) }
    $3D/                              { DEC A }
    $32/>hwK+1/                       { LD (hwK+1),A }
    $20/$D9/                          { JR NZ,zl }
    $CB/$01/                          { RLC C }
    $30/$02/                          { JR NC,ns }
    $23/                              { INC HL }
    $56/                              { LD D,(HL) }
    { ns: }
    $E5/                              { PUSH HL }
    $2A/>hwNb/                        { LD HL,(hwNb)  n }
    $2B/                              { DEC HL }
    $22/>hwNb/                        { LD (hwNb),HL }
    $7C/                              { LD A,H }
    $B5/                              { OR L }
    $E1/                              { POP HL }
    $20/$B7/                          { JR NZ,bit }
    { fertig: }
    $78/                              { LD A,B }
    $FE/$01/                          { CP 1 }
    $28/$03/                          { JR Z,raus }
    $DD/$73/$00/                      { LD (IX+0),E }
    { raus: }
    $DD/$E1                           { POP IX }
  );
end;

function ghrdat(var bl): integer;
var b: GBytes absolute bl;
    m, p: integer;
begin
  hwWst := false;
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

{ Kennbuchstabe der Firmware (0FF9h, 'C' Farbe, 'M' Mono): OUT 11h legt
  das ROM ueber 0-1FFFh, OUT 10h zurueck, dazwischen keine Unterbrechung.
  Der Code muss ueber 2000h liegen. }
procedure hwRom;
begin
  hwRomz := 0;
  inline(
    $ED/$57/                          { LD A,I: P/V = Unterbrechungen an }
    $F5/                              { PUSH AF }
    $F3/                              { DI }
    $D3/$11/                          { OUT (11h),A: ROM ueber 0-1FFFh }
    $3A/$F9/$0F/                      { LD A,(0FF9h) }
    $32/>hwRomz/                      { LD (hwRomz),A }
    $D3/$10/                          { OUT (10h),A: zurueck }
    $C1/                              { POP BC }
    $CB/$51/                          { BIT 2,C }
    $28/$01/                          { JR Z,+1 }
    $FB                               { EI }
  );
end;

{ Bildschirm nach HWA 3: die Farbkarte hat drei Ebenen zu 16 K Worten,
  die Monokarte eine (sie spiegelt 4000h auf 0000h). Alte Worte sichern,
  A5C3h nach 3FF0h und 3C5Ah nach 7FF0h, in der Reihenfolge 7FF0h,
  3FF0h zuruecklesen, alte Worte zurueck. Bleiben beide stehen: Farbe.
  Mit dem Kennbuchstaben verglichen; hwSchirm: 1 Mono, 2 Farbe, 3
  widerspruechlich. }
function hwFarbtest: integer;
var a3, a7, v3, v7, r: integer;
begin
  a3 := hwWLies($3FF0); a7 := hwWLies($7FF0);
  hwWSchr($3FF0, $A5C3); hwWSchr($7FF0, $3C5A);
  v7 := hwWLies($7FF0); v3 := hwWLies($3FF0);
  hwWSchr($7FF0, a7); hwWSchr($3FF0, a3);
  if (v3 = $A5C3) and (v7 = $3C5A) then r := 2
  else if (v3 = $3C5A) and (v7 = $3C5A) then r := 1
  else r := 0;
  hwRom;
  if r = 0 then begin               { Probe unklar: nach der Firmware }
    hwSchirm := 3;
    if hwRomz = Ord('C') then r := 2 else r := 1;
  end else if ((hwRomz = Ord('C')) and (r = 1)) or
              ((hwRomz = Ord('M')) and (r = 2)) then hwSchirm := 3
  else hwSchirm := r;
  if r = 2 then hwFarbtest := 1 else hwFarbtest := 0;
end;

{ Ist das eine DMV? Statusbits des 7220 an A0h: Bildwechsel (Bit 5) und
  Zeilenruecklauf (Bit 6) wechseln, FIFO leer (Bit 2) kommt vor, nie
  FFh. Liest nur. }
function hwDMV: boolean;
var i, s, m: integer;
begin
  hwDMV := false;
  m := 0; i := 0;
  while (i < 30000) and (m <> $1F) do begin
    s := Port[$A0];
    if s = $FF then exit;
    if (s and $20) <> 0 then m := m or 1 else m := m or 2;
    if (s and $40) <> 0 then m := m or 4 else m := m or 8;
    if (s and 4) <> 0 then m := m or $10;
    i := i + 1;
  end;
  hwDMV := m = $1F;
end;

{ K806 auf 2A (30h-33h) nach HWA 5. Erst nur lesen: ein leerer Platz
  liefert das untere Adressbyte zurueck (MAME: FFh). Dann Befehl 00h an
  31h: Statusbit 3 (F1) zeigt Adressbit 0 des letzten Schreibens, also
  1, und es kommen genau fuenf Datenbytes. Nichts sonst wird beschrieben. }
function hwMaustest: integer;
const MW = 20000;
var v, i, n, t: integer;
begin
  hwMaustest := 0;
  n := 0;
  for i := 0 to 3 do begin
    v := Port[$30 + i];
    if (v = $30 + i) or (v = $FF) then n := n + 1;
  end;
  if n = 4 then exit;
  i := 0;
  while (i < 64) and ((Port[$31] and 1) <> 0) do begin   { alte Daten }
    v := Port[$30]; i := i + 1;
  end;
  t := 0;
  while (t < MW) and ((Port[$31] and 2) <> 0) do t := t + 1;
  if t >= MW then exit;
  Port[$31] := 0;
  if (Port[$31] and 8) = 0 then exit;                   { F1 }
  n := 0;
  repeat
    t := 0;
    if n < 5 then i := MW else i := MW div 8;
    while (t < i) and ((Port[$31] and 1) = 0) do t := t + 1;
    if (Port[$31] and 1) <> 0 then begin v := Port[$30]; n := n + 1; end
    else t := -1;
  until (t < 0) or (n > 5);
  if n = 5 then hwMaustest := $30;
end;

function ghinit: integer;
var a, n: integer;
begin
  ghinit := -1;
  if not hwDMV then exit;             { keine DMV: nichts anfassen }
  hwBn := -1;
  FillChar(hwFF, 80, $FF);
  hwWst := false;
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

{ Tastatur-8741 fuer die Tonwarteschlange: Eingangspuffer frei (IBF 0)?
  Und ein Byte an 41h (Befehl) oder 40h (Daten), ohne zu warten. }
function ghkfrei: boolean;
begin
  ghkfrei := (Port[$41] and 2) = 0;
end;

procedure ghkaus(befehl: boolean; v: integer);
begin
  if befehl then Port[$41] := v else Port[$40] := v;
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
    if f >= 256 then begin
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

function hwAdrI(var x): integer;   { Adresse im Datensegment als Zahl }
begin
  hwAdrI := Addr(x);
end;

function hwAdr(var x): GBZeiger;   { Zeiger auf eine Variable }
begin
  hwAdr := Ptr(Addr(x));
end;

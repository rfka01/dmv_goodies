{ DMVBILD.INC - DMVGRAF fuer Turbo Pascal 3: Bilder, Zusammensetzen und
  Mauszeiger (vollstaendig, wie GBILD.C). Nach DMVGRAF.INC einbinden;
  fuer CP/M-80 gibt es die kleine Fassung DMVBKL.INC.

  Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT }

var
  { Bilder }
  bw, bh, bbpr, bnp, bblk, boff, bx0, by0, bx1, by1: array[0..63] of integer;
  btop: array[0..7] of real;
  znr, bnblk: integer;
  mpa, mpb, mpm: array[0..15] of integer;
  mpok: boolean;
  bpuf: array[0..1279] of byte;
  bm, bt, bt2, bein, bnul: array[0..79] of byte;
  bp: array[0..2, 0..79] of byte;
  ptab: array[0..1, 0..255] of byte;
  hnr, hn, hpos: integer;
  rmem: GBZeiger;
  rmpos: integer;
  rdat: boolean;                    { Quelle Datei (sonst Speicher) }
  rbuf: array[0..127] of byte;
  rpos: integer;
  rraw: array[0..319] of byte;
  ra, rb: array[0..319] of byte;
  brest: integer;
  bzr: array[0..319] of byte;
  gbblk: array[0..107] of byte;

  { Zusammensetzen }
  lan: boolean;
  lnz, lx0, ly0, lx1, ly1: integer;
  lnr, lart: array[0..199] of byte;
  lsx, lsy: array[0..199] of integer;
  band: array[0..2, 0..7, 0..79] of byte;
  gbanda: GBytes absolute band;

  { Mauszeiger }
  zaus: boolean;
  zx, zsy, zh, znn, zbnp, zr0, zr1, zc0, znc, zsh: integer;
  zbild: array[0..511] of byte;
  zsich: array[0..2, 0..191] of byte;
  zpm: array[0..191] of byte;
  zpe: array[0..2, 0..191] of byte;
  zblk: array[0..115] of byte;


{ ------------------------------------------------------------------ }
{ Bilder (GBILD.C)                                                   }

function bnrok(nr: integer): boolean;
begin
  bnrok := (nr >= 0) and (nr < GBMAX);
end;

procedure bmini;
var i: integer;
begin
  if mpok then exit;
  if qstart <> 0 then ;
  for i := 0 to 15 do begin
    if qneb = 3 then mpa[i] := bfa[i] else mpa[i] := bfm[i];
    mpb[i] := mpa[i];
    mpm[i] := 0;
  end;
  { Farbe: braun -> Raster gelb/rot, dunkelgrau -> Raster schwarz/blau;
    mono: dunkelgrau im Raster }
  if qneb = 3 then begin
    mpa[6] := 6; mpb[6] := 4; mpm[6] := 1;
  end;
  mpa[8] := 0;
  if qneb = 3 then mpb[8] := 1 else mpb[8] := 7;
  mpm[8] := 1;
  mpa[0] := -1; mpb[0] := 0;        { 0 durchsichtig }
  mpok := true;
end;

procedure gbmap(f, a, b, m: integer);
begin
  bmini;
  if (f < 0) or (f > 15) or (a < -1) or (a > 7) or (b < 0) or (b > 7)
     or (m < 0) or (m > 3) then gerr := GE_FORM
  else begin
    mpa[f] := a; mpb[f] := b; mpm[f] := m;
    gerr := 0;
  end;
end;

function bfarb(c, x, y: integer): integer;   { Farbnummer -> DMV-Farbe }
var r: integer;
begin
  if mpa[c] < 0 then bfarb := -1
  else begin
    case mpm[c] of
      1: r := (x + y) and 1;
      2: r := x and y and 1;
      3: r := y and 1;
    else r := 0;
    end;
    if r <> 0 then bfarb := mpb[c] else bfarb := mpa[c];
  end;
end;

procedure ptbau;
var y, v, e, w, c0, c1: integer;
begin
  for y := 0 to 1 do
    for v := 0 to 255 do begin
      c0 := bfarb(v shr 4, 0, y);
      c1 := bfarb(v and 15, 1, y);
      w := 0;
      for e := 0 to qneb - 1 do begin
        if (c0 >= 0) and ((c0 and qebit[e]) <> 0) then w := w or (1 shl (2 * e));
        if (c1 >= 0) and ((c1 and qebit[e]) <> 0) then w := w or (2 shl (2 * e));
      end;
      if c0 >= 0 then w := w or $40;
      if c1 >= 0 then w := w or $80;
      ptab[y, v] := w;
    end;
end;

{ ---- Bildspeicher ---- }

function balloc(nr: integer; n: real): integer;
var k: integer;
begin
  if bnblk < 0 then begin
    bnblk := ghsnum;
    if bnblk > 8 then bnblk := 8;
    for k := 0 to bnblk - 1 do btop[k] := 0.0;
  end;
  balloc := GE_PLATZ;
  for k := 0 to bnblk - 1 do
    if btop[k] + n <= ghsgro(k) then begin
      bblk[nr] := k;
      boff[nr] := rint(btop[k]);
      btop[k] := btop[k] + n;
      balloc := 0;
      exit;
    end;
end;

function bgr(nr: integer): real;    { Groesse eines Bildes }
begin
  bgr := 1.0 * bh[nr] * bnp[nr] * bbpr[nr];
end;

procedure gbfree(nr: integer);
var k, b: integer;
    n, o, rest: real;
begin
  gerr := GE_NR;
  if not bnrok(nr) then exit;
  gerr := 0;
  if bnp[nr] = 0 then exit;
  if nr = znr then gzeig(-1, 0, 0);
  b := bblk[nr];
  n := bgr(nr);
  o := uwr(boff[nr]);
  rest := btop[b] - o - n;
  if rest > 0 then                  { dahinter liegendes nachruecken }
    ghsmov(b, rint(o), rint(o + n), rint(rest));
  for k := 0 to GBMAX - 1 do
    if (bnp[k] <> 0) and (k <> nr) and (bblk[k] = b) and (uwr(boff[k]) > o) then
      boff[k] := rint(uwr(boff[k]) - n);
  btop[b] := btop[b] - n;
  bnp[nr] := 0;
  gerr := 0;
end;

procedure hflush;
begin
  if hn <> 0 then begin
    ghsput(bblk[hnr], boff[hnr] + hpos, bpuf, hn);
    hpos := hpos + hn;
    hn := 0;
  end;
end;

procedure hput(var s; n: integer);
begin
  if hn + n > QBUF then hflush;
  Move(s, bpuf[hn], n);
  hn := hn + n;
end;

procedure bhol(nr, y, n: integer; var d);   { n Zeilen ab y holen }
var rs: integer;
begin
  rs := bnp[nr] * bbpr[nr];
  ghsget(d, bblk[nr], boff[nr] + y * rs, n * rs);
end;

{ ---- Quelle: Speicher oder Datei ---- }

function brd(var dd; n: integer): boolean;  { n Byte lesen; true = Ende }
var d: GBytes absolute dd;
    i, di: integer;
begin
  brd := false;
  if not rdat then begin
    Move(rmem^[rmpos], d, n);
    rmpos := rmpos + n;
    exit;
  end;
  di := 0;
  while n > 0 do begin
    if rpos >= 128 then begin
      if ghread(rbuf) = 0 then begin brd := true; exit; end;
      rpos := 0;
    end;
    i := 128 - rpos;
    if i > n then i := n;
    Move(rbuf[rpos], d[di], i);
    di := di + i;
    rpos := rpos + i;
    n := n - i;
  end;
end;

{ ---- Bild anlegen ---- }

{ erster und letzter gesetzter Punkt von n Byte, -1 = keiner }
procedure bspan(var zz; n: integer; var lo, hi: integer);
var z: GBytes absolute zz;
    k, v: integer;
begin
  lo := -1; hi := -1;
  k := 0;
  while (k < n) and (lo < 0) do begin
    v := z[k];
    if v <> 0 then
      if (v and 15) <> 0 then lo := k * 8 + bnlo[v and 15]
      else lo := k * 8 + 4 + bnlo[v shr 4];
    k := k + 1;
  end;
  if lo < 0 then exit;
  k := n - 1;
  while (k >= 0) and (hi < 0) do begin
    v := z[k];
    if v <> 0 then
      if (v shr 4) <> 0 then hi := k * 8 + 4 + bnhi[v shr 4]
      else hi := k * 8 + bnhi[v and 15];
    k := k - 1;
  end;
end;

procedure bbox(nr: integer; var z; y: integer);
var lo, hi: integer;
begin
  bspan(z, bbpr[nr], lo, hi);
  if hi < 0 then exit;
  if lo < bx0[nr] then bx0[nr] := lo;
  if hi > bx1[nr] then bx1[nr] := hi;
  if y < by0[nr] then by0[nr] := y;
  if y > by1[nr] then by1[nr] := y;
end;

{ Format 3: Zeile y nach z (je 80 Byte: sichtbar, Ebene 0, 1, 2) }
procedure bnib(nr, y: integer; var zz);
var z: GBytes absolute zz;
    n, k, e: integer;
begin
  n := bbpr[nr];
  for k := (bw[nr] + 1) shr 1 to 4 * n - 1 do rraw[k] := 0;
  gnib(z, rraw, ptab[y and 1, 0], n);
  z[n - 1] := z[n - 1] and brest;
  for e := 1 to 3 do z[80 * e + n - 1] := z[80 * e + n - 1] and z[n - 1];
end;

{ Format 3: Zeile fertig machen (Maske nach Fuellart) und ablegen }
procedure bfin(nr, y: integer; var zz; lo, hi, plo, phi, nlo2, nhi2, art, fb: integer);
var z: GBytes absolute zz;
    k, n, e: integer;
begin
  n := bbpr[nr];
  if hi >= 0 then begin
    if lo < bx0[nr] then bx0[nr] := lo;
    if hi > bx1[nr] then bx1[nr] := hi;
    if y < by0[nr] then by0[nr] := y;
    if y > by1[nr] then by1[nr] := y;
  end;
  for k := 0 to n - 1 do
    if art <> 0 then bm[k] := 0 else bm[k] := z[k];
  if art <> 0 then begin
    if art = 2 then begin
      { Umriss um einen Punkt erweitern, auch senkrecht }
      if phi >= 0 then begin
        if (hi < 0) or (plo < lo) then lo := plo;
        if phi > hi then hi := phi;
      end;
      if nhi2 >= 0 then begin
        if (hi < 0) or (nlo2 < lo) then lo := nlo2;
        if nhi2 > hi then hi := nhi2;
      end;
    end;
    if hi >= 0 then begin
      if art = 2 then begin lo := lo - 1; hi := hi + 1; end;
      if lo < 0 then lo := 0;
      if hi > bw[nr] - 1 then hi := bw[nr] - 1;
      for k := lo shr 3 to hi shr 3 do bm[k] := $FF;
      bm[lo shr 3] := bm[lo shr 3] and (($FF shl (lo and 7)) and $FF);
      bm[hi shr 3] := bm[hi shr 3] and ($FF shr (7 - (hi and 7)));
    end;
  end;
  hput(bm, n);
  for e := 0 to qneb - 1 do
    if (fb and qebit[e]) <> 0 then begin
      for k := 0 to n - 1 do
        bt[k] := z[80 * (e + 1) + k] or (bm[k] and (not z[k]) and $FF);
      hput(bt, n);
    end else hput(z[80 * (e + 1)], n);
end;

function bfmt3(nr: integer): integer;
var y, lo, hi, plo, phi, clo, chi, art, fb, i: integer;
    da, aufa: boolean;              { aufa: aktuelle Zeile in ra }
begin
  bfmt3 := GE_KURZ;
  art := 0; fb := 0;
  for i := 0 to 15 do
    if (mpa[i] < 0) and (mpm[i] > art) then begin
      if mpm[i] > 2 then art := 2 else art := mpm[i];
      fb := mpb[i];
    end;
  aufa := true;                     { cur = ra, nxt = rb }
  plo := -1; phi := -1; clo := -1; chi := -1;
  da := false;
  for y := 0 to bh[nr] - 1 do begin
    if brd(rraw, (bw[nr] + 1) shr 1) then exit;
    if aufa then begin
      bnib(nr, y, rb);
      bspan(rb, bbpr[nr], lo, hi);
      if da then bfin(nr, y - 1, ra, clo, chi, plo, phi, lo, hi, art, fb);
    end else begin
      bnib(nr, y, ra);
      bspan(ra, bbpr[nr], lo, hi);
      if da then bfin(nr, y - 1, rb, clo, chi, plo, phi, lo, hi, art, fb);
    end;
    if da then begin plo := clo; phi := chi; end;
    aufa := not aufa;
    clo := lo; chi := hi;
    da := true;
  end;
  if aufa then bfin(nr, bh[nr] - 1, ra, clo, chi, plo, phi, -1, -1, art, fb)
  else bfin(nr, bh[nr] - 1, rb, clo, chi, plo, phi, -1, -1, art, fb);
  bfmt3 := 0;
end;

function brev(v: integer): integer;  { Bitfolge eines Bytes umdrehen }
var r, j: integer;
begin
  r := 0;
  for j := 0 to 7 do
    if (v and (1 shl j)) <> 0 then r := r or ($80 shr j);
  brev := r;
end;

function bfmt12(nr, fmt: integer): integer;
var y, k, n: integer;
begin
  bfmt12 := GE_KURZ;
  n := bbpr[nr];
  for y := 0 to bh[nr] - 1 do begin
    if brd(rraw, n) then exit;
    for k := 0 to n - 1 do begin
      if fmt = GB_BIT7 then rraw[k] := brev(rraw[k]);
      if k = n - 1 then rraw[k] := rraw[k] and brest;
    end;
    bbox(nr, rraw, y);
    hput(rraw, n);
  end;
  bfmt12 := 0;
end;

function bfmt4(nr: integer): integer;
var y, k, n, e: integer;
begin
  bfmt4 := GE_KURZ;
  n := bbpr[nr];
  for y := 0 to bh[nr] - 1 do begin
    if brd(rraw, 4 * n) then exit;
    rraw[n - 1] := rraw[n - 1] and brest;
    bbox(nr, rraw, y);
    hput(rraw, n);
    if qneb = 3 then
      for e := 0 to 2 do begin
        for k := 0 to n - 1 do bt[k] := rraw[(e + 1) * n + k] and rraw[k];
        hput(bt, n);
      end
    else begin
      for k := 0 to n - 1 do
        bt[k] := (rraw[n + k] or rraw[2 * n + k] or rraw[3 * n + k]) and rraw[k];
      hput(bt, n);
    end;
  end;
  bfmt4 := 0;
end;

{ Format 5: Worte, [Ebene gruen/rot/blau][SET, RESET][Zeile][Wort] }
function bfmt5(nr: integer): integer;
var y, k, n, e, wpr, s, r, i, ws, wr, pe: integer;
begin
  n := bbpr[nr];
  wpr := (bw[nr] + 15) shr 4;
  for y := 0 to bh[nr] - 1 do begin
    for k := 0 to n - 1 do begin
      bm[k] := 0; bp[0, k] := 0; bp[1, k] := 0; bp[2, k] := 0;
    end;
    for e := 0 to 2 do
      for k := 0 to wpr - 1 do begin
        i := ((e * 2) * bh[nr] + y) * wpr + k;
        ws := rmem^[rmpos + 2 * i] + 256 * rmem^[rmpos + 2 * i + 1];
        i := i + bh[nr] * wpr;
        wr := rmem^[rmpos + 2 * i] + 256 * rmem^[rmpos + 2 * i + 1];
        if qneb = 3 then pe := e else pe := 0;
        s := 0;
        while (s < 2) and (2 * k + s < n) do begin
          if s <> 0 then r := Hi(ws) else r := Lo(ws);
          if s <> 0 then bm[2 * k + s] := bm[2 * k + s] or r or Hi(wr)
          else bm[2 * k + s] := bm[2 * k + s] or r or Lo(wr);
          bp[pe, 2 * k + s] := bp[pe, 2 * k + s] or r;
          s := s + 1;
        end;
      end;
    bm[n - 1] := bm[n - 1] and brest;
    bbox(nr, bm, y);
    hput(bm, n);
    for e := 0 to qneb - 1 do hput(bp[e, 0], n);
  end;
  bfmt5 := 0;
end;

{ Bild nr anlegen; die Daten kommen ueber brd() oder bei Format 5 aus
  rmem (Worte niederwertiges Byte zuerst, wie Turbo Pascal sie ablegt) }
function bdef(nr, b, h, fmt: integer): integer;
var r: integer;
begin
  if not bnrok(nr) then begin bdef := GE_NR; exit; end;
  if (b < 1) or (b > 640) or (h < 1) or (h > 400) or (fmt < 1) or (fmt > 5) then begin
    bdef := GE_FORM; exit;
  end;
  gbfree(nr);
  bw[nr] := b;
  bh[nr] := h;
  bbpr[nr] := (b + 7) shr 3;
  if (fmt = GB_BIT7) or (fmt = GB_BIT0) then bnp[nr] := 1 else bnp[nr] := qneb + 1;
  if (b and 7) <> 0 then brest := (1 shl (b and 7)) - 1 else brest := $FF;
  if balloc(nr, bgr(nr)) <> 0 then begin
    bnp[nr] := 0; bdef := GE_PLATZ; exit;
  end;
  bx0[nr] := 9999; by0[nr] := 9999;
  bx1[nr] := -1; by1[nr] := -1;
  hnr := nr; hn := 0; hpos := 0;
  case fmt of
    GB_NIB:  r := bfmt3(nr);
    GB_EBN:  r := bfmt4(nr);
    GB_SETR: r := bfmt5(nr);
  else r := bfmt12(nr, fmt);
  end;
  hflush;
  if r <> 0 then gbfree(nr);
  bdef := r;
end;

procedure gbdef(nr, b, h, fmt: integer; var daten);
begin
  gerr := GE_HW;
  if qstart <> 0 then exit;
  bmini;
  if fmt = GB_NIB then ptbau;
  rdat := false;
  rmem := hwAdr(daten);
  rmpos := 0;
  gerr := bdef(nr, b, h, fmt);
end;

procedure gbload(nr0: integer; name: GStr; b, h, anz, fmt, kopf: integer);
var i, r: integer;
    c: byte;
begin
  gerr := GE_HW;
  if qstart <> 0 then exit;
  bmini;
  gerr := GE_FORM;
  if fmt = GB_SETR then exit;
  if fmt = GB_NIB then ptbau;
  gerr := GE_DATEI;
  if ghopen(name) <> 0 then exit;
  rdat := true;
  rpos := 128;
  r := 0;
  i := 0;
  while (i < kopf) and (r = 0) do begin
    if brd(c, 1) then r := GE_KURZ;
    i := i + 1;
  end;
  i := 0;
  while (i < anz) and (r = 0) do begin
    r := bdef(nr0 + i, b, h, fmt);
    i := i + 1;
  end;
  ghclos;
  rdat := false;
  gerr := r;
end;

procedure gbinfo(nr: integer; var r: GBInfoArr);
begin
  gerr := GE_NR;
  if not bnrok(nr) then exit;
  gerr := GE_LEER;
  if bnp[nr] = 0 then exit;
  r[0] := bw[nr]; r[1] := bh[nr];
  r[2] := bx0[nr]; r[3] := by0[nr]; r[4] := bx1[nr]; r[5] := by1[nr];
  gerr := 0;
end;

{ ---- Zeichnen ---- }

{ Geholte Zeile z fuer Art art als zusammenhaengende Zeile in bzr:
  Maske, im Abstand n die Ebenen }
procedure bzei(nr: integer; var zz; art, sx, sy: integer);
var z: GBytes absolute zz;
    k, n, e, pat, s: integer;
begin
  n := bbpr[nr];
  if bnp[nr] = 1 then begin
    Move(z, bzr, n);
    for e := 0 to qneb - 1 do
      if qebon(e, qcol) then Move(z, bzr[(e + 1) * n], n)
      else Move(bnul, bzr[(e + 1) * n], n);
  end else Move(z, bzr, (qneb + 1) * n);
  if art = GB_DECK then begin
    for k := 0 to n - 1 do bzr[k] := $FF;
    if (bw[nr] and 7) <> 0 then bzr[n - 1] := (1 shl (bw[nr] and 7)) - 1
    else bzr[n - 1] := $FF;
  end;
  if art = GB_SCHAT then begin
    if ((sx + sy) and 1) <> 0 then pat := $55 else pat := $AA;
    for k := 0 to n - 1 do
      if qneb = 3 then begin
        s := bzr[n + k] and bzr[2 * n + k] and bzr[3 * n + k] and pat;
        bzr[n + k] := bzr[n + k] and (not s) and $FF;
        bzr[2 * n + k] := bzr[2 * n + k] and (not s) and $FF;
      end else bzr[n + k] := bzr[n + k] and (not pat) and $FF;
  end;
end;

{ bzei liefert in C die Zeile selbst, wo keine Kopie noetig ist
  (Ebenenbild, Art 1, 3); hier: true = z unveraendert benutzen }
function bdirekt(nr, art: integer): boolean;
begin
  bdirekt := (bnp[nr] <> 1) and ((art = GB_DURCH) or (art = GB_XOR));
end;

procedure bkopz(var zz; n: integer);   { Zeile nach bm, bp (fuer gbput) }
var z: GBytes absolute zz;
    e: integer;
begin
  Move(z, bm, n);
  for e := 0 to qneb - 1 do Move(z[(e + 1) * n], bp[e, 0], n);
end;

procedure bcut(var dd; var ss; off, n: integer);   { Bits off.. von s }
var d: GBytes absolute dd;
    s: GBytes absolute ss;
    i, j: integer;
begin
  for i := 0 to ((n + 7) shr 3) - 1 do d[i] := 0;
  for i := 0 to n - 1 do begin
    j := off + i;
    if (s[j shr 3] and (1 shl (j and 7))) <> 0 then
      d[i shr 3] := d[i shr 3] or (1 shl (i and 7));
  end;
end;

procedure bzoom(var dd; var ss; n, z1: integer);   { jedes Bit z1-mal }
var d: GBytes absolute dd;
    s: GBytes absolute ss;
    i, k, j, m: integer;
begin
  m := n * z1;
  if m > 640 then m := 640;
  for i := 0 to ((m + 7) shr 3) - 1 do d[i] := 0;
  for i := 0 to n - 1 do
    if (s[i shr 3] and (1 shl (i and 7))) <> 0 then
      for k := 0 to z1 - 1 do begin
        j := i * z1 + k;
        if j < m then d[j shr 3] := d[j shr 3] or (1 shl (j and 7));
      end;
end;

{ Zeile (bm, bp; nb Bit ab Punkt sx) auf den Schirm }
procedure bzeig(sy, sx, nb, art: integer);
var e, k, n: integer;
    ganz: boolean;
begin
  n := (nb + 7) shr 3;
  ganz := (art = GB_DECK) and ((sx and 15) = 0) and ((nb and 15) = 0);
  for e := 0 to qneb - 1 do
    if ganz then qrow(sy, sx, bp[e, 0], nb, e, $20, 0)
    else if art = GB_XOR then begin
      for k := 0 to n - 1 do bt[k] := bm[k] and bp[e, k];
      qrow(sy, sx, bt, nb, e, $21, 0);
    end else begin
      for k := 0 to n - 1 do begin
        bt[k] := bm[k] and (not bp[e, k]) and $FF;
        bt2[k] := bm[k] and bp[e, k];
      end;
      qrow(sy, sx, bt, nb, e, $22, 0);
      qrow(sy, sx, bt2, nb, e, $23, 0);
    end;
end;

function bvor(nr, art: integer): integer;
begin
  if not bnrok(nr) then bvor := GE_NR
  else if bnp[nr] = 0 then bvor := GE_LEER
  else if (art < 0) or (art > 3) then bvor := GE_FORM
  else begin
    FillChar(bein, 80, $FF);
    FillChar(bnul, 80, 0);
    bvor := 0;
  end;
end;

procedure gbput(nr, art: integer);
var z1, ww, hh, sx, sy, y, n, zr, rs, k, e, c0, c1, ys, nb, zo: integer;
begin
  gerr := bvor(nr, art);
  if gerr <> 0 then exit;
  z1 := qzoom + 1;
  ww := bw[nr] * z1;
  hh := bh[nr] * z1;
  sx := qx;
  sy := qsy(qy);
  if qorig = 0 then sy := sy - (hh - 1);
  if sx < 0 then c0 := -sx else c0 := 0;
  if sx + ww > 640 then c1 := 640 - sx else c1 := ww;
  if c0 >= c1 then exit;
  rs := bnp[nr] * bbpr[nr];
  y := 0;
  while y < bh[nr] do begin
    n := QBUF div rs;
    if n > bh[nr] - y then n := bh[nr] - y;
    bhol(nr, y, n, bpuf);
    for k := 0 to n - 1 do begin
      zo := k * rs;
      for zr := 0 to z1 - 1 do begin
        ys := sy + (y + k) * z1 + zr;
        if (ys >= 0) and (ys <= 399) then begin
          if bdirekt(nr, art) then bkopz(bpuf[zo], bbpr[nr])
          else begin
            bzei(nr, bpuf[zo], art, sx, ys);
            bkopz(bzr, bbpr[nr]);
          end;
          nb := bw[nr];
          if z1 > 1 then begin
            bzoom(bt, bm, nb, z1);
            Move(bt, bm, 80);
            for e := 0 to qneb - 1 do begin
              bzoom(bt, bp[e, 0], nb, z1);
              Move(bt, bp[e, 0], 80);
            end;
            if ww > 640 then nb := 640 else nb := ww;
          end;
          if (c0 > 0) or (c1 < nb) then begin
            bcut(bt, bm, c0, c1 - c0);
            Move(bt, bm, 80);
            for e := 0 to qneb - 1 do begin
              bcut(bt, bp[e, 0], c0, c1 - c0);
              Move(bt, bp[e, 0], 80);
            end;
          end;
          bzeig(ys, sx + c0, c1 - c0, art);
        end;
      end;
    end;
    y := y + n;
  end;
  qflush;
end;

{ Schirmausschnitt am Zeichenpunkt als Bild nr }
procedure gbget(nr, b, h: integer);
var sx, sy, y, e, nw, i, k, n, sh, a: integer;
begin
  gerr := GE_NR;
  if not bnrok(nr) then exit;
  if qzeig then qzhid;
  sx := qx;
  sy := qsy(qy);
  if qorig = 0 then sy := sy - (h - 1);
  gerr := GE_FORM;
  if (b < 1) or (h < 1) or (sx < 0) or (sx + b > 640) or (sy < 0)
     or (sy + h > 400) then exit;
  gbfree(nr);
  bw[nr] := b;
  bh[nr] := h;
  n := (b + 7) shr 3;
  bbpr[nr] := n;
  bnp[nr] := qneb + 1;
  gerr := GE_PLATZ;
  if balloc(nr, bgr(nr)) <> 0 then begin bnp[nr] := 0; exit; end;
  bx0[nr] := 0; by0[nr] := 0; bx1[nr] := b - 1; by1[nr] := h - 1;
  hnr := nr; hn := 0; hpos := 0;
  sh := sx and 15;
  nw := (sh + b + 15) shr 4;
  qflush;
  for k := 0 to n - 1 do bm[k] := $FF;
  if (b and 7) <> 0 then bm[n - 1] := (1 shl (b and 7)) - 1 else bm[n - 1] := $FF;
  for y := 0 to h - 1 do begin
    hput(bm, n);
    for e := 0 to qneb - 1 do begin
      a := qebad[e] + (sy + y) * 40 + (sx shr 4);
      { Liste CURS, MASK, FIGS, RDAT, Ende; DC = Zahl der Worte }
      gbblk[0] := 0; gbblk[1] := 2 * nw;
      gbblk[2] := 4; gbblk[3] := $49; gbblk[4] := Lo(a); gbblk[5] := Hi(a);
      gbblk[6] := 0;
      gbblk[7] := 3; gbblk[8] := $4A; gbblk[9] := $FF; gbblk[10] := $FF;
      gbblk[11] := 4; gbblk[12] := $4C; gbblk[13] := 2; gbblk[14] := nw;
      gbblk[15] := 0;
      gbblk[16] := 1; gbblk[17] := $A0;
      gbblk[18] := 0;
      for i := 0 to 2 * nw - 1 do gbblk[19 + i] := 0;
      if ghrdat(gbblk) = 0 then ;
      bcut(bt, gbblk[19], sh, b);
      hput(bt, n);
    end;
  end;
  hflush;
  gerr := 0;
end;

{ ------------------------------------------------------------------ }
{ Zusammensetzen                                                     }

procedure glayer(x0, y0, x1, y1: integer);
var t: integer;
begin
  y0 := qsy(y0);
  y1 := qsy(y1);
  if x0 > x1 then begin t := x0; x0 := x1; x1 := t; end;
  if y0 > y1 then begin t := y0; y0 := y1; y1 := t; end;
  if x0 < 0 then x0 := 0;
  if y0 < 0 then y0 := 0;
  if x1 > 639 then x1 := 639;
  if y1 > 399 then y1 := 399;
  lan := (x0 <= x1) and (y0 <= y1);
  lx0 := x0 and not 15;
  lx1 := (x1 or 15) + 1;
  ly0 := y0;
  ly1 := y1 + 1;
  lnz := 0;
  gerr := 0;
end;

procedure glput(nr, x, y, art: integer);
var sy: integer;
begin
  gerr := GE_LAGE;
  if not lan then exit;
  gerr := GE_NR;
  if not bnrok(nr) then exit;
  gerr := GE_LEER;
  if bnp[nr] = 0 then exit;
  gerr := 0;
  sy := qsy(y);
  if qorig = 0 then sy := sy - (bh[nr] - 1);
  if (x >= lx1) or (x + bw[nr] <= lx0) or (sy >= ly1) or (sy + bh[nr] <= ly0) then exit;
  gerr := GE_LAGE;
  if lnz >= GLMAX then exit;
  lnr[lnz] := nr;
  lsx[lnz] := x;
  lsy[lnz] := sy;
  lart[lnz] := art;
  lnz := lnz + 1;
  gerr := 0;
end;

{ Zeile z (Maske, Ebenen im Abstand n) um sh Punkte versetzt in das Band
  ab Bandversatz d: cnt Bytes ab Quellbyte k0, allgemein (mono, XOR) }
procedure lgen(d: integer; var zz; n, sh, k0, cnt, art: integer);
var z: GBytes absolute zz;
    k, mm, v, j, e, w, q: integer;
begin
  for j := 0 to cnt - 1 do begin
    k := k0 + j;
    if k < n then w := z[k] shl sh else w := 0;
    if k > 0 then w := w or (z[k - 1] shr (8 - sh));
    mm := w and $FF;
    if mm <> 0 then
      for e := 0 to qneb - 1 do begin
        q := (e + 1) * n;
        if k < n then w := z[q + k] shl sh else w := 0;
        if k > 0 then w := w or (z[q + k - 1] shr (8 - sh));
        v := w and mm;
        if art = GB_XOR then
          gbanda[d + j + e * EB] := gbanda[d + j + e * EB] xor v
        else
          gbanda[d + j + e * EB] := (gbanda[d + j + e * EB] and not mm) or v;
      end;
  end;
end;

function lueb(var zz; k0, sh: integer): integer;  { Uebertrag vor k0 }
var z: GBytes absolute zz;
begin
  if (sh <> 0) and (k0 > 0) then lueb := z[k0 - 1] shr (8 - sh)
  else lueb := 0;
end;

procedure gldraw;
var nb, yb, yz, e, r, k, nr, r0, r1, rs, y, n, i, art: integer;
    bx, b0, sh, k0, cnt, ns, nn, c01, c23, dz, zo: integer;
    schnell, direkt: boolean;
begin
  gerr := GE_LAGE;
  if not lan then exit;
  FillChar(bein, 80, $FF);
  FillChar(bnul, 80, 0);
  nb := (lx1 - lx0) shr 3;
  yb := ly0;
  while yb < ly1 do begin
    yz := ly1 - yb;
    if yz > BANDH then yz := BANDH;
    if nb = 80 then FillChar(band, 3 * EB, 0)
    else
      for e := 0 to qneb - 1 do
        for r := 0 to yz - 1 do FillChar(band[e, r, 0], nb, 0);
    for k := 0 to lnz - 1 do begin
      nr := lnr[k];
      if (lsy[k] < yb + yz) and (lsy[k] + bh[nr] > yb) then begin
        nn := bbpr[nr];
        bx := lsx[k] - lx0;
        b0 := ((bx + 8192) shr 3) - 1024;
        sh := bx - 8 * b0;
        if b0 < 0 then k0 := -b0 else k0 := 0;
        if sh <> 0 then cnt := nn + 1 else cnt := nn;
        if cnt > nb - b0 then cnt := nb - b0;
        cnt := cnt - k0;
        if cnt > 0 then begin
          ns := nn - k0;
          art := lart[k];
          schnell := (qneb = 3) and (art <> GB_XOR);
          if lsy[k] > yb then r0 := lsy[k] else r0 := yb;
          r1 := lsy[k] + bh[nr];
          if r1 > yb + yz then r1 := yb + yz;
          rs := bnp[nr] * nn;
          direkt := bdirekt(nr, art);
          dz := (r0 - yb) * 80 + b0 + k0;
          y := r0;
          while y < r1 do begin
            n := QBUF div rs;
            if n > r1 - y then n := r1 - y;
            bhol(nr, y - lsy[k], n, bpuf);
            zo := 0;
            for i := 0 to n - 1 do begin
              if not direkt then begin
                bzei(nr, bpuf[zo], art, lsx[k], y + i);
                Move(bzr, rraw, 4 * nn);
              end else Move(bpuf[zo], rraw, rs);
              if not schnell then lgen(dz, rraw, nn, sh, k0, cnt, art)
              else begin
                c01 := 0; c23 := 0;
                if (sh <> 0) and (k0 <> 0) then begin
                  c01 := (lueb(rraw, k0, sh) shl 8) or lueb(rraw[nn], k0, sh);
                  c23 := (lueb(rraw[2 * nn], k0, sh) shl 8) or lueb(rraw[3 * nn], k0, sh);
                end;
                gzeile(gbanda[dz], rraw[k0], nn, sh, cnt, ns, c01, c23, EB);
              end;
              zo := zo + rs;
              dz := dz + 80;
            end;
            y := y + n;
          end;
        end;
      end;
    end;
    for r := 0 to yz - 1 do
      for e := 0 to qneb - 1 do qrow(yb + r, lx0, band[e, r, 0], nb * 8, e, $20, 0);
    yb := yb + BANDH;
  end;
  qflush;
  lan := false;
  gerr := 0;
end;

{ ------------------------------------------------------------------ }
{ Mauszeiger mit gesichertem Hintergrund (Kasten im Band)           }

{ ZBUF(e) = Versatz im Band: e * ZKH * ZKB }

procedure zvers(sh: integer);       { Bild um sh Punkte versetzt }
var i, v, r, e, b0, s, t, n, qo, d: integer;
begin
  zsh := sh;
  b0 := sh shr 3;
  s := sh and 7;
  t := 8 - s;
  n := znn;
  for e := 0 to qneb do begin
    if e = 0 then FillChar(zpm, zh * 6, 0) else FillChar(zpe[e - 1, 0], zh * 6, 0);
    qo := e * ZHMAX * 4;
    d := 0;
    for r := 0 to zh - 1 do begin
      for i := 0 to n - 1 do begin
        v := zbild[qo + i];
        if e = 0 then begin
          zpm[d + b0 + i] := zpm[d + b0 + i] or ((v shl s) and $FF);
          zpm[d + b0 + i + 1] := zpm[d + b0 + i + 1] or (v shr t);
        end else begin
          zpe[e - 1, d + b0 + i] := zpe[e - 1, d + b0 + i] or ((v shl s) and $FF);
          zpe[e - 1, d + b0 + i + 1] := zpe[e - 1, d + b0 + i + 1] or (v shr t);
        end;
      end;
      d := d + 6;
      qo := qo + 4;
    end;
  end;
end;

procedure zlad(nr: integer);        { Bild holen: Maske, Ebenen maskiert }
var r, e, i, v: integer;
begin
  znr := nr;
  zh := bh[nr];
  znn := bbpr[nr];
  zbnp := bnp[nr];
  FillChar(zbild, SizeOf(zbild), 0);
  for r := 0 to zh - 1 do begin
    bhol(nr, r, 1, bpuf);
    for i := 0 to znn - 1 do begin
      zbild[r * 4 + i] := bpuf[i];
      for e := 0 to qneb - 1 do begin
        if zbnp = 1 then begin
          if qebon(e, qcol) then v := bpuf[i] else v := 0;
        end else v := bpuf[(e + 1) * znn + i] and bpuf[i];
        zbild[(e + 1) * ZHMAX * 4 + r * 4 + i] := v;
      end;
    end;
  end;
  zsh := -1;
end;

procedure zlage(x, sy: integer; var l0, l1, l2, l3: integer);
begin
  if sy < 0 then l0 := 0 else l0 := sy;
  if sy + zh > 400 then l1 := 400 else l1 := sy + zh;
  l2 := x shr 4;
  l3 := ((x and 15) + bw[znr] + 15) shr 4;
  if l3 > 40 - l2 then l3 := 40 - l2;
end;

procedure zles(y0, n, c0, nc: integer);   { Kasten vom Schirm ins Band }
var r, e, c, a, d: integer;
begin
  qflush;
  for e := 0 to qneb - 1 do
    for c := 0 to nc - 1 do begin
      a := qebad[e] + y0 * 40 + c0 + c;
      zblk[0] := 0; zblk[1] := 2 * n;
      zblk[2] := 4; zblk[3] := $49; zblk[4] := Lo(a); zblk[5] := Hi(a);
      zblk[6] := 0;
      zblk[7] := 3; zblk[8] := $4A; zblk[9] := $FF; zblk[10] := $FF;
      { nach unten, DC wie gbget; GD: Grafikzeilen }
      zblk[11] := 4; zblk[12] := $4C; zblk[13] := 0; zblk[14] := n;
      zblk[15] := $40;
      zblk[16] := 1; zblk[17] := $A0;
      zblk[18] := 0;
      FillChar(zblk[19], 2 * n, 0);
      if ghrdat(zblk) = 0 then ;
      d := e * ZKH * ZKB + 2 * c;
      for r := 0 to n - 1 do begin
        gbanda[d] := zblk[19 + 2 * r];
        gbanda[d + 1] := zblk[20 + 2 * r];
        d := d + ZKB;
      end;
    end;
end;

{ Rechteck tauschen zwischen Kasten (Abstand ZKB) und Sicherung
  (Abstand 6); nach = true: Kasten -> Sicherung }
procedure zkopi(ky, kc, sy, n, c, nc: integer; nach: boolean);
var k, s, e, r, m: integer;
begin
  m := 2 * nc;
  if (n <= 0) or (m <= 0) then exit;
  for e := 0 to qneb - 1 do begin
    k := e * ZKH * ZKB + (sy - ky) * ZKB + 2 * (c - kc);
    s := (sy - zsy) * 6 + 2 * (c - zc0);
    for r := 1 to n do begin
      if nach then Move(gbanda[k], zsich[e, s], m)
      else Move(zsich[e, s], gbanda[k], m);
      k := k + ZKB;
      s := s + 6;
    end;
  end;
end;

procedure zweg;                     { Hintergrund zurueck (in die Liste) }
begin
  qzeig := false;
  if (zr1 > zr0) and (znc > 0) then
    qblk(qneb, zr0, zc0, znc, zr1 - zr0, zsich[0, (zr0 - zsy) * 6], 6, ZHMAX * 6);
end;

{ Zeiger an x, sy setzen; alt = true: steht noch am alten Platz }
procedure zhin(x, sy: integer; alt: boolean);
var j, v, l0, l1, l2, l3, y0, y1, c0, c1, e, r, n, nc, sh, k, d, m, p: integer;
begin
  qzeig := false;
  zlage(x, sy, l0, l1, l2, l3);
  if (l1 <= l0) or (l3 <= 0) then begin     { nicht zu sehen }
    if alt then zweg;
    zx := x; zsy := sy; zr0 := l0; zr1 := l0; znc := 0;
    qflush;
    qzeig := true;
    exit;
  end;
  y0 := l0; y1 := l1; c0 := l2; c1 := l2 + l3;
  if alt and (znc > 0) and (zr1 > zr0) then begin
    if zr0 < y0 then y0 := zr0;
    if zr1 > y1 then y1 := zr1;
    if zc0 < c0 then c0 := zc0;
    if zc0 + znc > c1 then c1 := zc0 + znc;
    if (c1 - c0 > 6) or (y1 - y0 > ZKH) then begin   { zu weit: getrennt }
      zweg;
      alt := false;
      y0 := l0; y1 := l1; c0 := l2; c1 := l2 + l3;
    end;
  end else alt := false;
  n := y1 - y0;
  nc := c1 - c0;
  zles(y0, n, c0, nc);
  if alt then zkopi(y0, c0, zr0, zr1 - zr0, zc0, znc, false);
  zx := x; zsy := sy; zr0 := l0; zr1 := l1; zc0 := l2; znc := l3;
  zkopi(y0, c0, zr0, zr1 - zr0, zc0, znc, true);
  sh := x and 15;
  if sh <> zsh then zvers(sh);
  k := 2 * znc;
  for e := 0 to qneb - 1 do begin
    d := e * ZKH * ZKB + (zr0 - y0) * ZKB + 2 * (zc0 - c0);
    m := (zr0 - sy) * 6;
    p := m;
    for r := 1 to zr1 - zr0 do begin
      for j := 1 to k do begin
        v := gbanda[d] and (not zpm[m]) and $FF;
        gbanda[d] := v or zpe[e, p];
        d := d + 1; m := m + 1; p := p + 1;
      end;
      d := d + ZKB - k;
      m := m + 6 - k;
      p := p + 6 - k;
    end;
  end;
  qblk(qneb, y0, c0, nc, n, band, ZKB, ZKH * ZKB);
  qflush;
  qzeig := true;
end;

procedure qzhid;                    { von qplatz: Zeiger wegnehmen }
begin
  zweg;
  zaus := true;
end;

procedure qzwied;                   { von mpos, gkey, gwait: wieder zeigen }
begin
  if zaus and (znr >= 0) and (qan <> 0) then begin
    zaus := false;
    zhin(zx, zsy, false);
  end;
end;

procedure gzeig;
var sy: integer;
begin
  gerr := 0;
  if nr < 0 then begin
    if qzeig then zweg;
    qflush;
    zaus := false;
    znr := -1;
    exit;
  end;
  gerr := GE_NR;
  if not bnrok(nr) then exit;
  gerr := GE_LEER;
  if bnp[nr] = 0 then exit;
  gerr := GE_FORM;
  if (bw[nr] > 32) or (bh[nr] > ZHMAX) then exit;
  gerr := GE_HW;
  if (qstart <> 0) or (qan = 0) then exit;
  gerr := 0;
  if x < 0 then x := 0;
  if x > 639 then x := 639;
  sy := qsy(y);
  if qorig = 0 then sy := sy - (bh[nr] - 1);
  if nr <> znr then begin
    if qzeig then zweg;
    zaus := false;
    zlad(nr);
  end;
  if (x = zx) and (sy = zsy) and (qzeig or zaus) then begin
    qzwied;
    exit;
  end;
  zaus := false;
  zhin(x, sy, qzeig);
end;

procedure gbstart;                  { Anfangswerte (aus gstart) }
begin
  znr := -1; bnblk := -1; mpok := false;
  FillChar(bnp, SizeOf(bnp), 0);
  lan := false; lnz := 0;
  zaus := false; zx := 0; zsy := 0; zh := 0; znn := 0; zbnp := 0;
  zr0 := 0; zr1 := 0; zc0 := 0; znc := 0; zsh := -1;
  rdat := false; rmpos := 0;
end;

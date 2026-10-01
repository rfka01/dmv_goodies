| RACE68.S - Rechenkerne fuer den 68008 (K234) in DMVRACE
| (GNU as, m68k:  m68k-linux-gnu-as -m68000 --register-prefix-optional)
|
| Wird vom 8088 an die physische Adresse P+100h kopiert und per Zugriff auf
| Port D8h gestartet (Reset: SSP/PC ab Adresse 0). Je Start ein Test.
| Der Code ist lageunabhaengig (nur PC-relativ).
|
| Briefkasten ab P (Bytes, Langworte im 68000-Format = Big Endian):
|   +0  Status   0 = nicht gestartet, 1 = rechnet, 2 = fertig, 3 = abgebrochen
|   +1  fertige Zeilen      +2  Abbruchwunsch      +3  Test (1..5)
|   +4  x0 (4.28)  +8 y0 (4.28)  +12 dx (4.28)  +16 dy (4.28)
|   +20 W  +22 H  +24 maxit (Worte)  +26/+28 Arbeitsvariablen
|   +32 Ergebnis (Langwort)
| Speicher: P+1000h Mandelbrot-Ergebnisse (1 Byte je Zelle, darunter der
| Stapel), P+1400h Sieb-Flags (8191 Bytes), P+3400h Sierpinski-Zeile (32 Bytes)
| Bilder: P+3500h Sierpinski (100 Zeilen zu 16 Bytes = 4 Langworte),
| P+3C00h Division (Faktoren je n, 400 Bytes), P+3E00h Kreis (150 Worte)
| Rechenvorschrift wie tools/benchref.py.

        .equ    SIER_ROWS, 200
        .equ    SIER_REPS, 40
        .equ    SIEVE_SIZE, 8190
        .equ    SIEVE_ITER, 8
        .equ    DIV_N0, 50000
        .equ    DIV_K, 400
        .equ    SQ_R, 6000

        .text
| Quadrat: \r = \r*\r >> 28 (4.28), \r >= 0 vorausgesetzt, zerstoert d3,d4
        .macro  SQ r
        move.l  \r,d3
        swap    d3                      | d3.w = ah
        move.w  d3,d4
        mulu.w  d4,d4                   | ah*ah
        lsl.l   #4,d4
        mulu.w  d3,\r                   | al*ah
        moveq   #11,d3
        lsr.l   d3,\r                   | 2*ah*al >> 12
        add.l   d4,\r
        .endm

start:  lea     start-256(pc),a5        | Briefkasten
        move.b  #1,(a5)                 | laeuft
        clr.l   32(a5)
        move.b  3(a5),d0
        cmp.b   #1,d0
        beq     mandel
        cmp.b   #2,d0
        beq     sier
        cmp.b   #3,d0
        beq     sieve
        cmp.b   #4,d0
        beq     divt
        cmp.b   #5,d0
        beq     circ
fin:    move.b  #2,(a5)                 | fertig
halt:   stop    #0x2700                 | anhalten
        bra.s   halt

|------------------------------------------------------------------------
| 1: Mandelbrot (32 Bit Festkomma 4.28)
|------------------------------------------------------------------------
mandel:
        lea     4096(a5),a4             | Ergebnisse
        move.l  #0x20000000,a3          | 2.0
        move.l  #0x40000000,a6          | 4.0
        clr.b   1(a5)
        move.l  8(a5),a2                | ci = y0
        move.w  22(a5),28(a5)           | Zeilen
row:    move.l  4(a5),a1                | cr = x0
        move.w  20(a5),26(a5)           | Spalten
pix:    moveq   #0,d0                   | zr
        moveq   #0,d1                   | zi
        moveq   #0,d5                   | n
        move.w  24(a5),d6               | maxit
it:     move.l  d0,d2                   | d2 = |zr| < 2 ?
        bpl.s   1f
        neg.l   d2
1:      cmp.l   a3,d2
        bcc.s   esc
        move.l  d1,d7                   | d7 = |zi| < 2 ?
        bpl.s   2f
        neg.l   d7
2:      cmp.l   a3,d7
        bcc.s   esc
        | Produkt |zr|*|zi| >> 28 aus d2 (a) und d7 (b), bevor sie quadriert werden
        move.l  d2,d4
        swap    d4                      | ah
        move.w  d4,d6                   | ah merken (d6 danach neu laden)
        mulu.w  d7,d4                   | ah*bl
        move.l  d7,d3
        swap    d3                      | bh
        mulu.w  d2,d3                   | al*bh
        add.l   d4,d3
        moveq   #12,d4
        lsr.l   d4,d3
        move.l  d7,d4
        swap    d4
        mulu.w  d6,d4                   | ah*bh
        lsl.l   #4,d4
        add.l   d3,d4                   | d4 = |zr*zi|
        move.l  d4,-(sp)
        SQ      d2                      | d2 = zr^2
        SQ      d7                      | d7 = zi^2
        move.l  d2,d3
        add.l   d7,d3
        cmp.l   a6,d3                   | |z|^2 > 4 ?
        bhi.s   esc1
        sub.l   d7,d2                   | zr^2 - zi^2
        move.l  (sp)+,d4
        add.l   d4,d4                   | 2|zr zi|
        move.l  d0,d3
        eor.l   d1,d3
        bpl.s   3f
        neg.l   d4
3:      add.l   a2,d4
        move.l  d4,d1                   | zi = 2 zr zi + ci
        add.l   a1,d2
        move.l  d2,d0                   | zr = zr^2 - zi^2 + cr
        move.w  24(a5),d6
        addq.w  #1,d5
        cmp.w   d6,d5
        bcs     it
        bra.s   esc
esc1:   addq.l  #4,sp
esc:    move.b  d5,(a4)+
        add.l   12(a5),a1               | cr += dx
        subq.w  #1,26(a5)
        bne     pix
        add.l   16(a5),a2               | ci += dy
        addq.b  #1,1(a5)                | Zeile fertig
        tst.b   2(a5)                   | Abbruch?
        bne.s   abort
        subq.w  #1,28(a5)
        bne     row
        bra     fin
abort:  move.b  #3,(a5)
        bra     halt

|------------------------------------------------------------------------
| 2: Sierpinski: Zeile = Zeile XOR (Zeile << 1), 256 Bit als 8 Langworte
|    (Langwort 0 = niedrigste Bits); X-Flag traegt das Bit weiter
|------------------------------------------------------------------------
sier:   lea     0x3400(a5),a1
        lea     0x3500(a5),a2           | Bild
        lea     0x3B40(a5),a3           | Bildende (100 Zeilen)
        move.w  #SIER_REPS-1,d7
srep:   move.l  a1,a0
        moveq   #7,d1
sclr:   clr.l   (a0)+
        dbra    d1,sclr
        move.l  #1,(a1)
        bsr     scopy
        move.w  #SIER_ROWS-2,d6
srow:   move.l  a1,a0
        moveq   #7,d1
        move    #0,ccr                  | X = 0
s1:     move.l  (a0),d0
        move.l  d0,d2
        roxl.l  #1,d2                   | X rein, oberstes Bit nach X
        eor.l   d2,d0                   | (EOR laesst X stehen)
        move.l  d0,(a0)+
        dbra    d1,s1
        bsr     scopy
        dbra    d6,srow
        dbra    d7,srep
        move.l  a1,a0                   | Summe der 32 Bytes
        moveq   #0,d0
        moveq   #0,d2
        moveq   #31,d1
ssum:   move.b  (a0)+,d2
        add.l   d2,d0
        dbra    d1,ssum
        move.l  d0,32(a5)
        bra     fin

| im letzten Durchgang (d7 = 0) die ersten 4 Langworte der Zeile ablegen
scopy:  tst.w   d7
        bne.s   sc9
        cmp.l   a3,a2
        bcc.s   sc9
        move.l  (a1),(a2)+
        move.l  4(a1),(a2)+
        move.l  8(a1),(a2)+
        move.l  12(a1),(a2)+
sc9:    rts

|------------------------------------------------------------------------
| 3: Sieb (BYTE 1981): 8190 Flags, 8 Durchgaenge -> 1899
|------------------------------------------------------------------------
sieve:  lea     0x1400(a5),a1
        moveq   #SIEVE_ITER-1,d7
sit:    move.l  a1,a0
        move.w  #SIEVE_SIZE,d1
sfl:    move.b  #1,(a0)+
        dbra    d1,sfl
        moveq   #0,d5                   | Zaehler
        moveq   #0,d1                   | i
sl:     tst.b   0(a1,d1.l)
        beq.s   snext
        move.l  d1,d2                   | Primzahl = 2i+3
        add.l   d2,d2
        addq.l  #3,d2
        move.l  d1,d3
        add.l   d2,d3                   | k = i + Primzahl
skl:    cmp.l   #SIEVE_SIZE,d3
        bhi.s   sdone
        clr.b   0(a1,d3.l)
        add.l   d2,d3
        bra.s   skl
sdone:  addq.l  #1,d5
snext:  addq.l  #1,d1
        cmp.l   #SIEVE_SIZE,d1
        bls.s   sl
        dbra    d7,sit
        move.l  d5,32(a5)
        bra     fin

|------------------------------------------------------------------------
| 4: Primfaktoren durch Probedivision (DIVU.W), n = 50000..50399
|------------------------------------------------------------------------
divt:   move.l  #DIV_N0,d6              | n
        move.w  #DIV_K-1,d7
        moveq   #0,d5                   | Summe
        lea     0x3C00(a5),a2           | Bild
dn:     move.l  d5,d4                   | Summe vorher
        move.l  d6,d0                   | m
        moveq   #2,d1                   | d
dd:     move.l  d0,d2
        divu.w  d1,d2                   | d2 = Rest:Quotient
        cmp.w   d1,d2
        bcs.s   dbrk                    | Quotient < d
        swap    d2
        tst.w   d2
        bne.s   dnext
        swap    d2                      | teilbar: m = q
        moveq   #0,d0
        move.w  d2,d0
        addq.l  #1,d5
        bra.s   dd
dnext:  addq.w  #1,d1
        bra.s   dd
dbrk:   addq.l  #1,d5                   | Rest ist prim
        move.l  d5,d3                   | Bild: Zahl der Faktoren von n
        sub.l   d4,d3
        move.b  d3,(a2)+
        addq.l  #1,d6
        dbra    d7,dn
        move.l  d5,32(a5)
        bra     fin

|------------------------------------------------------------------------
| 5: Gitterpunkte im Viertelkreis: Summe isqrt(R^2 - x^2), x = 0..R-1
|------------------------------------------------------------------------
circ:   moveq   #0,d6                   | x
        moveq   #0,d5                   | Summe
        lea     0x3E00(a5),a2           | Bild
        moveq   #0,d0
        move.w  d0,a3                   | Zaehler bis 40
cx1:    move.w  d6,d0
        mulu.w  d0,d0                   | x^2
        move.l  #SQ_R*SQ_R,d1
        sub.l   d0,d1                   | N
        | Wurzel Bit fuer Bit: d2 = Wurzel, d3 = Rest, d4 = Probe
        moveq   #0,d2
        moveq   #0,d3
        moveq   #15,d7
iq1:    lsl.l   #1,d1                   | 2 Bits von N in den Rest
        roxl.l  #1,d3
        lsl.l   #1,d1
        roxl.l  #1,d3
        add.l   d2,d2
        move.l  d2,d4
        add.l   d4,d4
        addq.l  #1,d4
        cmp.l   d4,d3
        bcs.s   iq2
        sub.l   d4,d3
        addq.l  #1,d2
iq2:    dbra    d7,iq1
        add.l   d2,d5
        move.w  a3,d0                   | Bild: jede 40. Wurzel
        bne.s   cx2
        move.w  d2,(a2)+
        moveq   #40,d0
cx2:    subq.w  #1,d0
        move.w  d0,a3
        addq.w  #1,d6
        cmp.w   #SQ_R,d6
        bcs.s   cx1
        move.l  d5,32(a5)
        bra     fin
        .end

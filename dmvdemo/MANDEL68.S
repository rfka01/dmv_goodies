| MANDEL68.S - Mandelbrot-Rechenkern fuer den 68008 auf dem K234
| (GNU as, m68k:  m68k-linux-gnu-as -m68000 --register-prefix-optional)
|
| Wird vom 8088-Programm DMV68 an die physische Adresse P+100h kopiert
| und per Zugriff auf Port D8h gestartet (Reset: SSP/PC ab Adresse 0).
| Der Code ist lageunabhaengig (nur PC-relativ).
|
| Briefkasten ab P (Bytes, Langworte im 68000-Format = Big Endian):
|   +0  Status   0 = nicht gestartet, 1 = rechnet, 2 = fertig, 3 = abgebrochen
|   +1  fertige Zeilen (0..H)
|   +2  Abbruchwunsch vom 8088 (<> 0)
|   +4  x0  (4.28)     +8  y0 (4.28)
|   +12 dx  (4.28)     +16 dy (4.28)
|   +20 W (Wort)       +22 H (Wort)      +24 maxit (Wort, <= 255)
|   +26 Spaltenzaehler +28 Zeilenzaehler (Arbeitsvariablen)
| Ergebnis ab P+1000h: ein Byte je Zelle = Iterationen (maxit = innen)

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
        lea     4096(a5),a4             | Ergebnisse
        move.l  #0x20000000,a3          | 2.0
        move.l  #0x40000000,a6          | 4.0
        move.b  #1,(a5)                 | laeuft
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
        move.b  #2,(a5)                 | fertig
halt:   stop    #0x2700                 | anhalten
        bra.s   halt
abort:  move.b  #3,(a5)
        bra.s   halt
        .end

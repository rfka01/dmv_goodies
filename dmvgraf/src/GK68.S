| GK68.S - DMVGRAF fuer CP/M-68K: Kern in 68000-Assembler (statt GKERN.C)
| und die Ausgabe einer GDC-Liste an den 7220.
|
|   gkopi(d, s, n)           n Byte kopieren
|   gnull(d, n)              n Byte loeschen
|   gnib(z, q, tb, n)        Format 3 -> sichtbar und Ebenen (wie GKERN.C)
|   gzeile(d, z, n, sh, cnt, ns, c01, c23, eb)   wie GKERN.C
|   gdlst(liste)             Liste wie BIOS 66 an den 7220 (A1h Befehl,
|                            A0h Parameter, vor jedem Byte warten, bis
|                            das FIFO nicht voll ist); nur im
|                            Supervisorzustand aufrufen (GHD68K.C)
|
| Aufruf aus gcc: Argumente als Langworte ab 4(sp), Ergebnis d0, d2-d7 und
| a2-a6 bleiben erhalten.
|
| Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT

        .text
        .global gkopi
        .global gnull
        .global gnib
        .global gzeile
        .global gdlst

gkopi:  move.l  4(sp),a0
        move.l  8(sp),a1
        move.l  12(sp),d0
        ble.s   gk9
        move.l  a0,d1                   | beide gerade: Langworte
        btst    #0,d1
        bne.s   gk2
        move.l  a1,d1
        btst    #0,d1
        bne.s   gk2
        move.l  d0,d1
        lsr.l   #2,d1
        beq.s   gk2
        and.w   #3,d0
        subq.l  #1,d1
gk1:    move.l  (a1)+,(a0)+
        dbra    d1,gk1
        tst.w   d0
        beq.s   gk9
gk2:    subq.l  #1,d0
gk3:    move.b  (a1)+,(a0)+
        dbra    d0,gk3
gk9:    moveq   #0,d0
        rts

gnull:  move.l  4(sp),a0
        move.l  8(sp),d0
        ble.s   gn9
        move.l  a0,d1
        btst    #0,d1
        bne.s   gn2
        move.l  d0,d1
        lsr.l   #2,d1
        beq.s   gn2
        and.w   #3,d0
        subq.l  #1,d1
gn1:    clr.l   (a0)+
        dbra    d1,gn1
        tst.w   d0
        beq.s   gn9
gn2:    subq.l  #1,d0
gn3:    clr.b   (a0)+
        dbra    d0,gn3
gn9:    moveq   #0,d0
        rts

| gnib: je Gruppe vier Quellbytes; Tafelwert w: Bit 0/1 Ebene 0, 2/3
| Ebene 1, 4/5 Ebene 2, 6/7 sichtbar (je linker/rechter Punkt)
gnib:   movem.l d2-d7/a2,-(sp)
        move.l  32(sp),a0               | z
        move.l  36(sp),a1               | q
        move.l  40(sp),a2               | tb
        move.l  44(sp),d7               | n
        ble.s   gb9
        subq.l  #1,d7
gb1:    moveq   #0,d2                   | Ebene 0
        moveq   #0,d3                   | Ebene 1
        moveq   #0,d4                   | Ebene 2
        moveq   #0,d5                   | sichtbar
        moveq   #3,d6                   | vier Quellbytes, von rechts
        addq.l  #4,a1
gb2:    moveq   #0,d0
        move.b  -(a1),d0
        move.b  0(a2,d0.w),d0
        lsl.b   #2,d2                   | bisheriges um zwei Punkte weiter
        lsl.b   #2,d3
        lsl.b   #2,d4
        lsl.b   #2,d5
        move.b  d0,d1
        and.b   #3,d1
        or.b    d1,d2
        lsr.b   #2,d0
        move.b  d0,d1
        and.b   #3,d1
        or.b    d1,d3
        lsr.b   #2,d0
        move.b  d0,d1
        and.b   #3,d1
        or.b    d1,d4
        lsr.b   #2,d0
        or.b    d0,d5
        dbra    d6,gb2
        addq.l  #4,a1
        move.b  d5,(a0)
        move.b  d2,80(a0)
        move.b  d3,160(a0)
        move.b  d4,240(a0)
        addq.l  #1,a0
        dbra    d7,gb1
gb9:    movem.l (sp)+,d2-d7/a2
        moveq   #0,d0
        rts

| gzeile(d, z, n, sh, cnt, ns, c01, c23, eb)
|   d3 Uebertrag Maske, d4/d5/d6 Uebertrag Ebene 0/1/2, d7 sh
|   a1 Maske, a2/a3/a4 Ebenen, a0/a5/a6 Ziel Ebene 0/1/2
        .equ    ZA, 48                  | 11 Register + Ruecksprung
gzeile: movem.l d2-d7/a2-a6,-(sp)
        move.l  ZA(sp),a0               | d
        move.l  ZA+4(sp),a1             | z
        move.l  ZA+8(sp),d0             | n
        lea     0(a1,d0.l),a2
        lea     0(a2,d0.l),a3
        lea     0(a3,d0.l),a4
        move.l  ZA+12(sp),d7            | sh
        move.l  ZA+32(sp),d0            | eb
        lea     0(a0,d0.l),a5
        lea     0(a5,d0.l),a6
        move.l  ZA+24(sp),d0            | c01
        move.w  d0,d4
        and.w   #0xFF,d4
        lsr.w   #8,d0
        move.w  d0,d3
        and.w   #0xFF,d3
        move.l  ZA+28(sp),d0            | c23
        move.w  d0,d6
        and.w   #0xFF,d6
        lsr.w   #8,d0
        move.w  d0,d5
        and.w   #0xFF,d5
        move.l  ZA+16(sp),d0            | cnt
        move.l  ZA+20(sp),d1            | ns
        cmp.l   d1,d0
        ble.s   gz0
        move.l  d1,d0                   | erst min(cnt, ns) mit Quelle
        move.w  #1,-(sp)                | danach ein Schritt nur Uebertrag
        bra.s   gz1
gz0:    clr.w   -(sp)
gz1:    move.w  d0,-(sp)                | Zaehler
        beq     gz5
gz2:    moveq   #0,d0                   | Maske
        move.b  (a1)+,d0
        lsl.w   d7,d0
        move.w  d0,d1
        lsr.w   #8,d1
        or.b    d3,d0
        move.b  d1,d3
        move.b  d0,d2                   | mm
        moveq   #0,d0                   | Ebene 0
        move.b  (a2)+,d0
        lsl.w   d7,d0
        move.w  d0,d1
        lsr.w   #8,d1
        or.b    d4,d0
        move.b  d1,d4
        move.b  (a0),d1
        eor.b   d1,d0
        and.b   d2,d0
        eor.b   d0,(a0)+
        moveq   #0,d0                   | Ebene 1
        move.b  (a3)+,d0
        lsl.w   d7,d0
        move.w  d0,d1
        lsr.w   #8,d1
        or.b    d5,d0
        move.b  d1,d5
        move.b  (a5),d1
        eor.b   d1,d0
        and.b   d2,d0
        eor.b   d0,(a5)+
        moveq   #0,d0                   | Ebene 2
        move.b  (a4)+,d0
        lsl.w   d7,d0
        move.w  d0,d1
        lsr.w   #8,d1
        or.b    d6,d0
        move.b  d1,d6
        move.b  (a6),d1
        eor.b   d1,d0
        and.b   d2,d0
        eor.b   d0,(a6)+
        subq.w  #1,(sp)
        bne     gz2
gz5:    addq.l  #2,sp
        tst.w   (sp)+                   | noch ein Schritt mit Quelle 0?
        beq.s   gz9
        move.b  d3,d2                   | mm = Uebertrag der Maske
        move.b  (a0),d1
        move.b  d4,d0
        eor.b   d1,d0
        and.b   d2,d0
        eor.b   d0,(a0)
        move.b  (a5),d1
        move.b  d5,d0
        eor.b   d1,d0
        and.b   d2,d0
        eor.b   d0,(a5)
        move.b  (a6),d1
        move.b  d6,d0
        eor.b   d1,d0
        and.b   d2,d0
        eor.b   d0,(a6)
gz9:    movem.l (sp)+,d2-d7/a2-a6
        moveq   #0,d0
        rts

| gdlst(liste): Eintraege n, Befehl, n-1 Parameter; n = 0 Ende
gdlst:  move.l  4(sp),a0
        lea     0xFFFA0,a1              | Status lesen, Parameter schreiben
        move.l  a2,-(sp)
        lea     0xFFFA1,a2              | Befehl schreiben
gd1:    moveq   #0,d0
        move.b  (a0)+,d0
        beq.s   gd9
        move.b  (a0)+,d1
gd2:    btst    #1,(a1)                 | FIFO voll?
        bne.s   gd2
        move.b  d1,(a2)
        subq.w  #1,d0
        bra.s   gd5
gd3:    move.b  (a0)+,d1
gd4:    btst    #1,(a1)
        bne.s   gd4
        move.b  d1,(a1)
gd5:    dbra    d0,gd3
        bra.s   gd1
gd9:    move.l  (sp)+,a2
        moveq   #0,d0
        rts

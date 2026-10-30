| GCRT68.S - kleines Startstueck fuer C-Programme unter CP/M-68K, mit
| gcc (m68k-linux-gnu-gcc -m68000) uebersetzt; ohne DRs Bibliothek.
|
|   start              Einsprung (vorn im Text): Basisseite aus 4(sp),
|                      Kommandozeile zu argc/argv (GARG68.C), main,
|                      Ende ueber BDOS 0
|   long __BDOS(int fn, long par)   TRAP #2, d0 = fn, d1 = par
|   memcpy, memset     falls gcc sie selbst einsetzt
|   __mulsi3, __divsi3, __modsi3, __udivsi3, __umodsi3  (statt libgcc)
|
| Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT

        .text
        .global start
        .global __BDOS
        .global memcpy
        .global memset
        .global memmove

start:  move.l  4(sp),a0                | Basisseite
        move.l  a0,-(sp)
        jsr     gcmain                  | GARG68.C: argc, argv, main
        addq.l  #4,sp
        moveq   #0,d0                   | BDOS 0: Warmstart
        trap    #2
        bra.s   start

__BDOS: move.l  4(sp),d0
        move.l  8(sp),d1
        movem.l d2-d7/a2-a6,-(sp)       | was der BDOS anfasst, gehoert uns
        trap    #2
        movem.l (sp)+,d2-d7/a2-a6
        rts

memmove:
memcpy: move.l  4(sp),a0
        move.l  8(sp),a1
        move.l  12(sp),d1
        move.l  a0,d0
        cmp.l   a1,a0
        bhi.s   mc3                     | Ziel hinter der Quelle: von hinten
        bra.s   mc2
mc1:    move.b  (a1)+,(a0)+
mc2:    subq.l  #1,d1
        bcc.s   mc1
        rts
mc3:    add.l   d1,a0
        add.l   d1,a1
        bra.s   mc5
mc4:    move.b  -(a1),-(a0)
mc5:    subq.l  #1,d1
        bcc.s   mc4
        rts

memset: move.l  4(sp),a0
        move.l  8(sp),d1
        move.l  12(sp),d0
        move.l  a0,-(sp)
        bra.s   ms2
ms1:    move.b  d1,(a0)+
ms2:    subq.l  #1,d0
        bcc.s   ms1
        move.l  (sp)+,d0
        rts

| ---- Rechnen mit 32 Bit fuer gcc (libgcc enthaelt 68020-Befehle) ----

        .global __mulsi3
        .global __udivsi3
        .global __umodsi3
        .global __divsi3
        .global __modsi3

__mulsi3:
        move.w  4(sp),d0                | a hoch * b tief
        mulu.w  10(sp),d0
        move.w  6(sp),d1                | a tief * b hoch
        mulu.w  8(sp),d1
        add.w   d1,d0
        swap    d0
        clr.w   d0
        move.w  6(sp),d1                | a tief * b tief
        mulu.w  10(sp),d1
        add.l   d1,d0
        rts

| udiv: d0 / d1 -> d0 Quotient, d1 Rest (d2, d3 bleiben)
udiv:   movem.l d2-d3,-(sp)
        cmp.l   #0xFFFF,d1
        bhi.s   ud2
        move.l  d0,d2                   | Teiler passt in 16 Bit: zweimal DIVU
        clr.w   d2
        swap    d2
        divu.w  d1,d2                   | hohes Wort
        move.w  d2,d3
        move.w  d0,d2
        divu.w  d1,d2                   | tiefes Wort mit Rest davor
        swap    d3
        move.w  d2,d3
        clr.w   d2
        swap    d2
        move.l  d3,d0
        move.l  d2,d1
        bra.s   ud9
ud2:    move.l  d1,d3                   | sonst Stelle fuer Stelle
        moveq   #0,d1
        moveq   #31,d2
ud3:    add.l   d0,d0
        addx.l  d1,d1
        cmp.l   d3,d1
        bcs.s   ud4
        sub.l   d3,d1
        addq.l  #1,d0
ud4:    dbra    d2,ud3
ud9:    movem.l (sp)+,d2-d3
        rts

__udivsi3:
        move.l  4(sp),d0
        move.l  8(sp),d1
        bra     udiv

__umodsi3:
        move.l  4(sp),d0
        move.l  8(sp),d1
        bsr     udiv
        move.l  d1,d0
        rts

__divsi3:
        move.l  d2,-(sp)
        move.l  8(sp),d0
        move.l  12(sp),d1
        move.l  d0,d2
        eor.l   d1,d2                   | Vorzeichen des Ergebnisses
        tst.l   d0
        bpl.s   dv1
        neg.l   d0
dv1:    tst.l   d1
        bpl.s   dv2
        neg.l   d1
dv2:    bsr     udiv
        tst.l   d2
        bpl.s   dv3
        neg.l   d0
dv3:    move.l  (sp)+,d2
        rts

__modsi3:
        move.l  d2,-(sp)
        move.l  8(sp),d0
        move.l  12(sp),d1
        move.l  d0,d2                   | Vorzeichen des Rests: das des Dividenden
        tst.l   d0
        bpl.s   mo1
        neg.l   d0
mo1:    tst.l   d1
        bpl.s   mo2
        neg.l   d1
mo2:    bsr     udiv
        move.l  d1,d0
        tst.l   d2
        bpl.s   mo3
        neg.l   d0
mo3:    move.l  (sp)+,d2
        rts

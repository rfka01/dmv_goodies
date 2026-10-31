| DEMO68.S - DMVGRAF aus GNU as (CP/M-68K): Linie von (100,50) nach (500,300)
| Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
        .text
        .global main
main:   jsr     ginit
        move.l  #50,-(sp)            | gpoint(100, 50): jedes Argument ein Langwort
        move.l  #100,-(sp)
        jsr     gpoint
        addq.l  #8,sp
        move.l  #300,-(sp)           | gline(500, 300)
        move.l  #500,-(sp)
        jsr     gline
        addq.l  #8,sp
warte:  jsr     gkey            | auf eine Taste warten
        tst.l   d0
        beq.s   warte
        jsr     gexit
        moveq   #0,d0           | main liefert 0
        rts

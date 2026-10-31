; DEMO.ASM - DMVGRAF aus NASM (MS-DOS): Linie von (100,50) nach (500,300)
; Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
        cpu     8086
        bits    16
        section .text
        global  main
        extern  ginit, gexit, gpoint, gline, gkey
main:   call    ginit
        mov     ax, 50          ; gpoint(100, 50): Argumente von rechts
        push    ax
        mov     ax, 100
        push    ax
        call    gpoint
        add     sp, 4           ; der Aufrufer raeumt den Stapel ab
        mov     ax, 300         ; gline(500, 300)
        push    ax
        mov     ax, 500
        push    ax
        call    gline
        add     sp, 4
.warte: call    gkey            ; auf eine Taste warten
        or      ax, ax
        jz      .warte
        call    gexit
        xor     ax, ax          ; main liefert 0 (Errorlevel)
        ret

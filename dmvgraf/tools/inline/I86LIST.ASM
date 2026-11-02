; GDC-Liste an den 7220 (ghlist in DMVHWD.INC). Variablen: hwSrc
        cld                     ; Richtung vorwaerts (TP3 laesst DF manchmal gesetzt)
        mov si,[hwSrc]
        mov ax,[hwSrc+2]
        push ds
        mov ds,ax
satz:   lodsb
        or al,al
        jz ende
        mov cl,al
w1:     in al,0A0h
        test al,2
        jnz w1
        lodsb
        out 0A1h,al
        dec cl
        jz satz
w2:     in al,0A0h
        test al,2
        jnz w2
        lodsb
        out 0A0h,al
        dec cl
        jnz w2
        jmp short satz
ende:   pop ds

; Bits vervielfachen und ausschneiden (ghzbits in DMVHWD.INC).
; Variablen: hwSrc,hwDst,hwSx,hwNb,hwW,hwAa
        cld                     ; Richtung vorwaerts (TP3 laesst DF manchmal gesetzt)
; n Bits ab Bit off von Quelle nach Ziel, jedes z1-mal, hoechstens m Bit
; hwSrc (ofs,seg), hwDst (ofs,seg), hwSx = off, hwNb = n, hwAa = z1, hwW = m
        push bp
        push ds
        mov bx,[hwSx]
        mov cx,bx
        and cl,7
        mov dl,1
        shl dl,cl               ; dl = Quellmaske
        mov cl,3
        shr bx,cl
        mov bp,[hwNb]           ; Quellbits
        mov cx,[hwW]            ; Zielbits
        mov dh,[hwAa]           ; z1
        les di,[hwDst]
        lds si,[hwSrc]
        add si,bx
        lodsb
        mov bh,al               ; Quellbyte
        xor bl,bl               ; Zielbyte
        mov ah,1                ; Zielmaske
bit:    mov al,bh
        and al,dl
        jz b0
        mov al,0FFh
b0:     push dx
zl:     mov dl,al
        and dl,ah
        or bl,dl
        rol ah,1
        jnc nw
        xchg al,bl
        stosb
        xchg al,bl
        xor bl,bl
nw:     dec cx
        jz fertig
        dec dh
        jnz zl
        pop dx
        rol dl,1
        jnc ns
        mov al,[si]
        inc si
        mov bh,al
ns:     dec bp
        jnz bit
        jmp short raus
fertig: pop dx
raus:   cmp ah,1
        jz ende
        mov al,bl
        stosb
ende:   pop ds
        pop bp

; Variablen: hwL,hwQn,hwQe,hwAd,hwSx,hwNb,hwW,hwAa,hwSrc,hwLz,hwX,hwSh,hwRe,hwK2,hwMsk,hwFFo,hwOr,hwWst,hwHl
; Eintrag fuer eine Bitzeile an die GDC-Liste haengen (wie ghrow in Pascal)
; hwL: Liste (nur ofs, liegt im Datensegment), hwQn, hwQe, hwAd, hwSx,
; hwNb, hwW, hwAa (0 = Bits, 1 = umgekehrt; je mit Maske hwMsk),
; hwSrc: Quelle (ofs,seg), hwMsk: Maske (ofs, im Datensegment).
; Je Byte: v = (Quelle xor X) and Maske. Ergebnis in hwQn, hwQe.
        cld
        push bp
        mov ax,ds
        mov es,ax
        mov bx,[hwSx]
        and bx,15               ; bx = sh 0..15
        mov ax,[hwNb]
        add ax,bx
        add ax,15
        mov cl,4
        shr ax,cl
        shl ax,1
        mov bp,ax               ; bp = k
        mov [hwK2],ax
        mov dx,[hwNb]           ; dx = nb
        mov di,[hwL]
        add di,[hwQn]
        cmp byte [hwWst],0
        jz kl
        mov ax,4903h            ; kurz: CURS ohne Punkt, MASK und FIGS gelten noch
        stosw
        mov ax,[hwAd]
        stosw
        mov cx,4
        jmp short kw
kl:     mov ax,4904h            ; CURS
        stosw
        mov ax,[hwAd]
        stosw
        xor al,al
        stosb
        mov ax,4A03h            ; MASK FFFF
        stosw
        mov ax,0FFFFh
        stosw
        mov ax,4C04h            ; FIGS 02 00 00
        stosw
        mov ax,2
        stosw
        xor al,al
        stosb
        mov cx,14
kw:     mov [hwHl],cx
        mov ax,bp
        inc al
        stosb                   ; 1+k
        mov al,[hwW]
        stosb
        mov ax,[hwQn]
        add ax,cx
        mov [hwQe],ax
        add ax,2
        mov [hwQn],ax
        or bx,bx
        jnz lang
        cmp byte [hwAa],0
        jnz lang
        mov ax,[hwMsk]
        cmp ax,[hwFFo]
        jnz lang
        test dl,15
        jnz lang
        mov cx,bp               ; ganze Worte: kopieren
        push ds
        lds si,[hwSrc]
        rep movsb
        pop ds
        jmp dazu
lang:   ; n8 und letzt
        mov ax,dx
        add ax,7
        mov cl,3
        shr ax,cl               ; ax = n8
        mov cx,dx
        and cl,7
        mov dl,0FFh
        jz lz
        mov dl,1
        shl dl,cl
        dec dl
lz:     mov [hwLz],dl           ; letzt
        mov dh,0
        cmp byte [hwAa],0
        jz ux
        mov dh,0FFh             ; dh = X (umkehren)
ux:     mov [hwX],dh
        mov cx,bp               ; cx = rest
        cmp bl,8
        jb nv
        mov byte [di],0         ; erstes Byte leer
        inc di
        dec cx
nv:     and bl,7
        mov [hwSh],bl
        mov si,ax               ; si = n8
        cmp cx,si
        jae vok
        mov si,cx
        mov byte [hwLz],0FFh
vok:    sub cx,si
        mov [hwRe],cx           ; Rest danach
        mov byte [hwOr],0
        mov dl,[hwLz]
        mov dh,[hwX]
        mov cl,[hwSh]
        mov bp,si               ; Zaehler
        xor ch,ch               ; Uebertrag
        mov bx,[hwMsk]
        les si,[hwSrc]
        dec bp
        jz letzt
lp:     es lodsb
        xor al,dh
        and al,[bx]
        inc bx
        xor ah,ah
        shl ax,cl
        or al,ch
        mov ch,ah
        mov [di],al
        inc di
        or [hwOr],al
        dec bp
        jnz lp
letzt:  es lodsb
        xor al,dh
        and al,[bx]
        and al,dl
        xor ah,ah
        shl ax,cl
        or al,ch
        mov ch,ah
        mov [di],al
        inc di
        or [hwOr],al
        mov al,ch
        mov cx,[hwRe]
        jcxz rf
        mov [di],al
        inc di
        or [hwOr],al
        dec cx
        jcxz rf
        xor al,al
rz:     mov [di],al
        inc di
        loop rz
rf:     cmp byte [hwOr],0
        jnz dazu
        cmp byte [hwW],20h
        jz dazu
        mov ax,[hwQe]           ; nichts zu tun: Eintrag zurueck
        sub ax,[hwHl]
        mov [hwQn],ax
        mov word [hwQe],0FFFFh
        jmp short ende
dazu:   mov ax,[hwQn]
        add ax,[hwK2]
        mov [hwQn],ax
ende:   pop bp

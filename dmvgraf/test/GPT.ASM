; GPT.ASM - Pruefprogramm fuer GPIC.INC aus MASM (COM), wie PICTEST:
;   GPT datei xw y art [datei xw y art ...]
; art als Zahl (1 dunkel, 2 mono, 4 Farbe; 8: das Programm schaltet selbst
; dunkel und hell statt GPIC.INC). Zeigt die Bilder, wartet auf
; eine Taste und schreibt "gpic: n ..." (0 gut, sonst Fehlernummer).
; Schaltet wie DMVGRAF (ghgraf) die Grafik ein und am Ende wieder auf Text;
; Mono, wenn das erste art Bit 1 hat, sonst Farbe.
;
; Bauen: jwasm -Zm -bin -Fo=GPT.COM GPT.ASM   (GPIC.INC im Suchpfad, -I)
; Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT

_TEXT   SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:_TEXT, DS:_TEXT, ES:_TEXT, SS:_TEXT
        ORG     100h
start:  cld
        call    grafan
        mov     si, 81h
        mov     di, OFFSET codes
naechst:call    wort                    ; Dateiname
        jc      warte
        mov     bp, dx                  ; Anfang des Namens
        call    wort
        jc      warte
        call    zahl
        push    ax                      ; xw
        call    wort
        jc      warte2
        call    zahl
        push    ax                      ; y
        call    wort
        jc      warte3
        call    zahl
        pop     cx
        pop     bx
        push    si
        push    di
        mov     dx, bp
        test    al, 8                   ; art + 8: das Programm schaltet selbst
        jz      g_n                     ; dunkel und hell (wie DMVGRAF)
        and     al, 0F6h
        push    ax
        mov     al, 0Ch
        call    bef
        pop     ax
        call    gp_pic
        pushf
        push    ax
        mov     al, 6Bh
        call    bef
        mov     al, 0Dh
        call    bef
        pop     ax
        popf
        jmp     short g_m
g_n:    call    gp_pic
g_m:    pop     di
        pop     si
        jnc     gut
        jmp     short merk
gut:    xor     ax, ax
merk:   mov     [di], ax
        add     di, 2
        jmp     naechst
warte3: pop     ax
warte2: pop     ax
warte:  mov     ah, 06h                 ; Taste abwarten
        mov     dl, 0FFh
        int     21h
        jz      warte
        call    textan
        mov     dx, OFFSET text
        mov     ah, 09h
        int     21h
        mov     si, OFFSET codes
aus:    cmp     si, di
        jae     ende
        mov     dl, ' '
        mov     ah, 02h
        int     21h
        lodsw
        call    dezimal
        jmp     aus
ende:   mov     dl, 13
        mov     ah, 02h
        int     21h
        mov     dl, 10
        int     21h
        mov     ax, 4C00h
        int     21h

; naechstes Wort der Kommandozeile ab SI: DX = Anfang, mit 0 abgeschlossen;
; CF = 1 kein Wort mehr
wort:   cmp     byte ptr zende, 0
        jne     w_nix
w_0:    lodsb
        cmp     al, ' '
        je      w_0
        cmp     al, 9
        je      w_0
        cmp     al, 13
        jbe     w_nix
        lea     dx, [si-1]
w_1:    lodsb
        cmp     al, ' '
        je      w_2
        cmp     al, 9
        je      w_2
        cmp     al, 13
        ja      w_1
        mov     byte ptr zende, 1       ; Zeilenende
w_2:    mov     byte ptr [si-1], 0
        clc
        ret
w_nix:  stc
        ret

; Zahl ab DX nach AX
zahl:   push    si
        push    bx
        mov     si, dx
        xor     bx, bx
z_1:    lodsb
        sub     al, '0'
        jb      z_2
        cmp     al, 9
        ja      z_2
        xor     ah, ah
        push    ax
        mov     ax, 10
        mul     bx
        pop     bx
        add     bx, ax
        jmp     z_1
z_2:    mov     ax, bx
        pop     bx
        pop     si
        ret

; AX dezimal ausgeben
dezimal:push    ax
        push    bx
        push    cx
        push    dx
        mov     bx, 10
        xor     cx, cx
d_1:    xor     dx, dx
        div     bx
        push    dx
        inc     cx
        or      ax, ax
        jnz     d_1
d_2:    pop     dx
        add     dl, '0'
        mov     ah, 02h
        int     21h
        loop    d_2
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; ---- Grafik ein und aus wie GHWDOS.C (grafan, textan) ----
bef:    push    ax
b_1:    in      al, 0A0h
        test    al, 2
        jnz     b_1
        pop     ax
        out     0A1h, al
        ret
par:    push    ax
p_1:    in      al, 0A0h
        test    al, 2
        jnz     p_1
        pop     ax
        out     0A0h, al
        ret
par2:   push    ax
p_2:    in      al, 0A0h
        test    al, 2
        jnz     p_2
        pop     ax
        out     0A2h, al
        ret
folge:  lodsb                           ; Liste ab SI: n, Befehl, n-1 Parameter
        or      al, al
        jz      f_e
        mov     cl, al
        xor     ch, ch
        lodsb
        mov     bl, al
        call    bef
        dec     cx
        jz      folge
f_1:    lodsb
        cmp     bl, 46h                 ; ZOOM-Parameter gehen an A2h
        jne     f_2
        call    par2
        jmp     short f_3
f_2:    call    par
f_3:    loop    f_1
        jmp     folge
f_e:    ret
grafan: mov     si, 80h                 ; Kommandozeile sichern
        mov     di, OFFSET kzeile
        mov     cx, 128
        rep     movsb
        mov     si, 81h                 ; Mono? erstes art suchen
        call    wort
        jc      g_f
        call    wort
        call    wort
        call    wort
        jc      g_f
        call    zahl
        test    al, 2
        jz      g_f
        mov     byte ptr ebenen, 1
g_f:    mov     si, OFFSET kzeile       ; Kommandozeile zurueck
        mov     di, 80h
        mov     cx, 128
        rep     movsb
        mov     byte ptr zende, 0
        push    si
        mov     si, OFFSET lgraf
        call    folge
        mov     dl, ebenen
g_5:    mov     si, OFFSET lfuell       ; je Ebene loeschen (FIGS, WDAT 22h)
        call    folge
        dec     dl
        jnz     g_5
        mov     si, OFFSET lstart
        call    folge
        pop     si
        ret
textan: mov     si, OFFSET ltext
        call    folge
        mov     ah, 02h                 ; Strg-Z: die Konsole loescht selbst
        mov     dl, 1Ah
        int     21h
        ret
lgraf   db      1, 0Ch, 2, 4Bh, 0
        db      9, 70h, 0, 0, 0, 59h, 0, 0, 0, 59h
        db      2, 47h, 80, 4, 49h, 0, 0, 0, 2, 46h, 0, 3, 4Ah, 0FFh, 0FFh, 0
lfuell  db      4, 4Ch, 02h, 0FFh, 7Fh, 3, 22h, 0FFh, 0FFh, 0
lstart  db      1, 6Bh, 1, 0Dh, 0
ltext   db      1, 0Ch, 2, 46h, 0, 2, 4Bh, 8Fh
        db      17, 70h, 0, 0, 0, 19h, 0, 0, 0, 19h
        db      0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
        db      2, 47h, 80, 4, 49h, 0, 0, 0, 3, 4Ah, 0FFh, 0FFh, 1, 6Bh, 1, 0Dh, 0
ebenen  db      3
kzeile  db      128 dup (0)

text    db      'gpic:$'
zende   db      0
codes   dw      8 dup (0)

INCLUDE GPIC.INC

_TEXT   ENDS
        END     start

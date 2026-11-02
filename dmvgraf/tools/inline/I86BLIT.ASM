; Ganzes Bild direkt an den 7220 (ghblit in DMVHWD.INC), wie HOPPLERs
; BlitAsm: einmal MASK FFFF und FIGS, dann je Zeile und Eintrag der
; Tabelle CURS (ohne Punkt), WDAT und die Worte.
; Variablen: hwXs,hwXo,hwXn,hwXr,hwXw,hwXm,hwXa,hwXt,hwXu,hwXk,hwXz
; Quelle hwXs:hwXo, hwXn Zeilen im Abstand hwXr, je Ebene hwXw Worte,
; hwXm maskiert das letzte Wort. hwXu: hwXt Eintraege zu 6 Worten:
; Ebenenadresse, WDAT-Befehl, Maske (0 = Zeile, sonst Adresse von FFh-Bytes
; im Datensegment), XOR fuer die Ebene (0/FFFFh), Versatz der Ebene in der
; Zeile, 1 = lauter Nullen. Je Wort: ((Ebene xor X) and Maske).
%macro  par 0                   ; AL an A0h, vorher FIFO abwarten (AH frei)
%%w:    mov     ah, al
        in      al, 0A0h
        test    al, 2
        mov     al, ah
        jnz     %%w
        out     0A0h, al
%endmacro
%macro  pdl 0                   ; DL an A0h
%%w:    in      al, 0A0h
        test    al, 2
        jnz     %%w
        mov     al, dl
        out     0A0h, al
%endmacro
%macro  pdh 0                   ; DH an A0h
%%w:    in      al, 0A0h
        test    al, 2
        jnz     %%w
        mov     al, dh
        out     0A0h, al
%endmacro
%macro  bef 0                   ; AL an A1h
%%w:    mov     ah, al
        in      al, 0A0h
        test    al, 2
        mov     al, ah
        jnz     %%w
        out     0A1h, al
%endmacro
anfang: push    bp
        push    es
        mov     es, [hwXs]
        mov     si, [hwXo]
        mov     al, 4Ah         ; MASK FFFF
        bef
        mov     al, 0FFh
        par
        par
        mov     al, 4Ch         ; FIGS 02 00 00
        bef
        mov     al, 2
        par
        xor     al, al
        par
        par
        mov     ax, [hwXn]
        mov     [hwXz], ax
zeile:  mov     bx, hwXu
        mov     ax, [hwXt]
        mov     [hwXk], ax
tupel:  mov     al, 49h         ; CURS Wortadresse
        bef
        mov     dx, [hwXa]
        add     dx, [bx]
        mov     al, dl
        par
        mov     al, dh
        par
        mov     al, [bx+2]      ; WDAT
        bef
        mov     cx, [hwXw]
        dec     cx              ; letztes Wort extra (Maske hwXm)
        push    bx
        push    si
        cmp     word [bx+10], 0
        jne     null
        mov     di, [bx+4]      ; Maske: 0 = die Zeile selbst
        mov     bp, [bx+6]      ; X
        mov     bx, [bx+8]      ; Versatz der Ebene
        or      di, di
        jnz     nurp
        jcxz    lz
wortz:  mov     dx, [es:si+bx]  ; (Ebene xor X) and Maske
        xor     dx, bp
        and     dx, [es:si]
        pdl
        pdh
        add     si, 2
        loop    wortz
lz:     mov     dx, [es:si+bx]
        xor     dx, bp
        and     dx, [es:si]
        and     dx, [hwXm]
        pdl
        pdh
        jmp     short fertig
nurp:   jcxz    lp
wortp:  mov     dx, [es:si+bx]  ; Maske FFh: nur die Ebene
        xor     dx, bp
        pdl
        pdh
        add     si, 2
        loop    wortp
lp:     mov     dx, [es:si+bx]
        xor     dx, bp
        and     dx, [hwXm]
        pdl
        pdh
        jmp     short fertig
null:   inc     cx
        xor     dx, dx
n1:     pdl
        pdh
        loop    n1
fertig: pop     si
        pop     bx
        add     bx, 12
        dec     word [hwXk]
        jnz     tupel
        add     si, [hwXr]
        add     word [hwXa], 40
        dec     word [hwXz]
        jnz     zeile
        pop     es
        pop     bp

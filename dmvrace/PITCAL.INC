;-----------------------------------------------------------------------
; PITCAL.INC - Zeitbasis eichen (DMVRACE, DMVDEMO, PITTEST)
;
; Bezug ist 8253 Kanal 2: 500 kHz von der Hauptplatine, von DOS in
; Modus 3 betrieben (NCR 2.11: Startwert 5000 = 10 ms, DOS-3.3-Port:
; 27463). In Modus 3 zaehlt der Kanal in Zweierschritten abwaerts, ein
; Schritt ist also genau 1 us. Wer ihn oft genug liest (oefter als alle
; Startwert us), kann daraus eine lueckenlose Uhr in us machen, ohne ihn
; umzuprogrammieren.
;
; Damit werden gemessen:
;   Kanal 0 (Motor-Auszeit, Modus 5, CLK0 = Bildtakt, ca. 56 Hz) -> c_c0per
;   Bildwechsel des GDC (Status Bit 5, steigende Flanke)         -> c_vper
; jeweils als us je Takt bzw. je Bild, 0 = keine Flanke gesehen.
; Die Messdauer steht in c_dur in Einheiten von 65536 us (8 = 0,52 s).
; Auf echter Hardware gemessen (PITTEST, 26.10.2026): Kanal 0 und
; Bildwechsel 56,09 Hz (Farbe) bzw. 56,19 Hz (Mono).
;-----------------------------------------------------------------------
c_n      dw     0               ; Startwert Kanal 2 (us je Halbperiode)
c_nmax   dw     0               ; hoechster gesehener Wert
c_nsnap  db     0               ; 1 = auf bekannten Startwert gerundet
c_old    dw     0
c_lo     dw     0               ; vergangene us seit c2start
c_hi     dw     0
c_dur    dw     8
c_t0lo   dw     0
c_t0hi   dw     0
c_t1lo   dw     0
c_t1hi   dw     0
c_cnt    dw     0               ; Flanken nach der ersten
c_first  dw     0               ; erster und letzter Wert (Kanal 0)
c_last   dw     0
c_rise   db     0
c_c0per  dw     0               ; us je Takt Kanal 0
c_c0cnt  dw     0
c_vper   dw     0               ; us je Bild
c_vcnt   dw     0

; Kanal 2 festhalten und lesen -> AX
c2rd    PROC    NEAR
        mov     al,80h
        out     83h,al
        in      al,82h
        mov     ah,al
        in      al,82h
        xchg    al,ah
        ret
c2rd    ENDP

; Kanal 0 festhalten und lesen -> AX
c0rd    PROC    NEAR
        mov     al,00h
        out     83h,al
        in      al,80h
        mov     ah,al
        in      al,80h
        xchg    al,ah
        ret
c0rd    ENDP

; Bildwechsel: GDC-Status Bit 5 -> AX (0 oder 20h)
cvrd    PROC    NEAR
        in      al,0A0h
        and     al,20h
        mov     ah,0
        ret
cvrd    ENDP

; Startwert von Kanal 2 bestimmen. CF = 1: Kanal 2 laeuft nicht.
; Die Abfrage trifft den Startwert selten genau; liegt das Maximum
; knapp unter 5000 oder 27463, wird darauf gerundet.
c2learn PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        pushf
        cli
        call    c2rd
        mov     bx,ax                   ; letzter Wert
        mov     si,ax                   ; Maximum
        xor     dx,dx                   ; Rueckspruenge
        xor     cx,cx                   ; 65536 Abfragen
cl1:    call    c2rd
        cmp     ax,si
        jbe     cl2
        mov     si,ax
cl2:    cmp     ax,bx
        jbe     cl3
        inc     dx                      ; Wert stieg: neu geladen
cl3:    mov     bx,ax
        cmp     dx,8
        jae     cl4
        loop    cl1
        cmp     dx,2
        jb      clf
cl4:    popf
        mov     c_nmax,si
        mov     c_nsnap,1
        mov     ax,5000
        cmp     si,ax
        ja      cl5
        cmp     si,5000-48
        jae     cl7
cl5:    mov     ax,27463
        cmp     si,ax
        ja      cl6
        cmp     si,27463-48
        jae     cl7
cl6:    mov     c_nsnap,0               ; unbekannt: Maximum plus halbe Luecke
        mov     ax,si
        add     ax,8
cl7:    mov     c_n,ax
        clc
        jmp     short cl9
clf:    popf
        mov     c_n,0
        stc
cl9:    pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
c2learn ENDP

; Uhr auf 0 setzen
c2start PROC    NEAR
        call    c2rd
        mov     c_old,ax
        mov     c_lo,0
        mov     c_hi,0
        ret
c2start ENDP

; Uhr fortschreiben (oefter als alle c_n us aufrufen). Zerstoert AX.
c2tick  PROC    NEAR
        push    dx
        call    c2rd
        mov     dx,c_old
        mov     c_old,ax
        sub     dx,ax                   ; alt - neu
        jae     ct1
        add     dx,c_n                  ; neu geladen: + Startwert
ct1:    add     c_lo,dx
        adc     c_hi,0
        pop     dx
        ret
c2tick  ENDP

; Flanken zaehlen. DI = Abfrage (c0rd, cvrd), c_rise = 1: nur steigend.
; -> AX = us je Flanke (0 = keine; 0FFFFh = zu langsam), c_cnt
caledge PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        pushf
        cli
        mov     c_cnt,0
        call    c2start
        call    di
        mov     bx,ax                   ; letzter Wert
        mov     c_first,ax
        mov     c_last,ax
        xor     si,si                   ; erste Flanke gesehen?
ce1:    call    c2tick
        call    di
        cmp     ax,bx
        je      ce5
        mov     bx,ax
        mov     c_last,ax
        cmp     c_rise,0
        je      ce2
        or      ax,ax
        jz      ce5                     ; fallende Flanke
ce2:    mov     ax,c_lo
        mov     dx,c_hi
        or      si,si
        jnz     ce3
        inc     si
        mov     c_t0lo,ax
        mov     c_t0hi,dx
        jmp     short ce5
ce3:    inc     c_cnt
        mov     c_t1lo,ax
        mov     c_t1hi,dx
        sub     ax,c_t0lo
        sbb     dx,c_t0hi
        cmp     dx,c_dur
        jae     ce7                     ; lang genug gemessen
ce5:    mov     ax,c_hi                 ; Zeitgrenzen
        or      si,si
        jnz     ce6
        cmp     ax,4                    ; 0,26 s ohne Flanke: steht
        jae     ce7
        jmp     ce1
ce6:    mov     cx,c_dur
        add     cx,8
        cmp     ax,cx
        jb      ce1
ce7:    popf
        xor     ax,ax
        mov     cx,c_cnt
        jcxz    ce9
        mov     ax,c_t1lo
        mov     dx,c_t1hi
        sub     ax,c_t0lo
        sbb     dx,c_t0hi
        cmp     dx,cx
        jb      ce8
        mov     ax,0FFFFh
        jmp     short ce9
ce8:    div     cx
ce9:    pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
caledge ENDP

; Kanal 0 messen -> c_c0per, c_c0cnt
cal0    PROC    NEAR
        push    di
        mov     di,offset c0rd
        mov     c_rise,0
        call    caledge
        mov     c_c0per,ax
        mov     ax,c_cnt
        mov     c_c0cnt,ax
        pop     di
        ret
cal0    ENDP

; Bildwechsel messen -> c_vper, c_vcnt
calv    PROC    NEAR
        push    di
        mov     di,offset cvrd
        mov     c_rise,1
        call    caledge
        mov     c_vper,ax
        mov     ax,c_cnt
        mov     c_vcnt,ax
        pop     di
        ret
calv    ENDP

; us-Wert AX -> Frequenz in 1/100 Hz (AX), 0 bei AX < 1526
calhz   PROC    NEAR
        push    bx
        push    dx
        mov     bx,ax
        xor     ax,ax
        cmp     bx,1526
        jb      chz9
        mov     dx,05F5h                ; 100.000.000
        mov     ax,0E100h
        div     bx
chz9:   pop     dx
        pop     bx
        ret
calhz   ENDP

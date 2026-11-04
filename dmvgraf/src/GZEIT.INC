;-----------------------------------------------------------------------
; GZEIT.INC - Zeitbasis der NCR Decision Mate V (8253), fuer MASM 5.1
; und JWasm -Zm. Gemeinsame Fassung fuer DMVGRAF (Kern der DOS-Bibliothek,
; tools/gpicbin.py), DMVDEMO, DMVRACE und PITTEST; aus PITCAL.INC.
;
; Bezug ist Kanal 2: 500 kHz von der Hauptplatine, von DOS in Modus 3
; betrieben (NCR 2.11: Startwert 5000 = 10 ms, DOS-3.3-Port: 27463). In
; Modus 3 zaehlt der Kanal in Zweierschritten, ein Schritt ist 1 us. Wer
; ihn oefter liest als einmal je Startwert us, bekommt eine lueckenlose
; Uhr in us, ohne ihn umzuprogrammieren. Steht er (ohne DOS), setzt
; gz_lern ihn auf Modus 2 mit vollem Umfang (2 us je Schritt).
;
; Getrennt und nur auf Anfrage geeicht werden
;   Kanal 0 (Motor-Auszeit, Modus 5, CLK0 = Bildtakt, ca. 56 Hz) gz_cal0
;   Bildwechsel des 7220 (Status Bit 5, steigende Flanke)         gz_calv
; als us je Takt bzw. je Bild; 0 = keine Flanke oder nicht plausibel
; (Kanal 0: 5000-62500 us, Bild: 10000-40000 us). Messdauer gz_dur in
; Einheiten von 65536 us (Vorgabe 4 = 0,26 s). Gemessen (PITTEST,
; 26.10.2026): 56,09 Hz Farbe, 56,19 Hz Mono, Kanal 2 499,75 kHz.
;
; Prozeduren (alle Register bleiben, ausser den genannten Ergebnissen):
;   gz_lern   Startwert von Kanal 2 lernen, Uhr auf 0. CF = 1: Kanal 2
;             stand, jetzt Modus 2. -> gz_n, gz_nmax, gz_snap, gz_schr
;   gz_start  Uhr auf 0
;   gz_tick   Uhr fortschreiben (oefter rufen als alle gz_n us)
;   gz_us     gz_tick, dann DX:AX = us seit gz_start/gz_lern
;   gz_cal0   AL = 1: steht Kanal 0, ueber Port 14h anstossen, 10 Bilder
;             warten, noch einmal messen. -> AX = gz_c0per, gz_c0cnt,
;             gz_stoss (1 = angestossen)
;   gz_calv   -> AX = gz_vper, gz_vcnt
;   gz_hz     AX us -> AX in 1/100 Hz (0 bei AX < 1526)
;   gz_bildw  einen Bildwechsel abwarten (hoechstens etwa 0,1 s)
;   gz_c0rd, gz_c2rd  Kanal 0 bzw. 2 gelatcht -> AX
; Unterbrechungen sind beim Lernen und Messen gesperrt; danach gilt
; wieder der alte Zustand.
;
; DS muss auf die Daten zeigen (wie bei PITCAL.INC).
; Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
;-----------------------------------------------------------------------
GZ_DA    =      1               ; fuer GHW.INC: Zeitbasis vorhanden

gz_daten LABEL  BYTE
gz_n     dw     0               ; Startwert Kanal 2 (0: Modus 2, 65536)
gz_nmax  dw     0               ; hoechster gesehener Wert
gz_snap  db     0               ; 1 = auf bekannten Startwert gerundet
gz_schr  db     0               ; us je Schritt: 1 (Modus 3), 2 (Modus 2); 0 ungelernt
gz_lo    dw     0               ; Uhr in us
gz_hi    dw     0
gz_dur   dw     4               ; Messdauer in 65536 us
gz_c0per dw     0               ; us je Takt Kanal 0
gz_c0cnt dw     0
gz_vper  dw     0               ; us je Bild
gz_vcnt  dw     0
gz_stoss db     0               ; 1 = Kanal 0 wurde angestossen
gz_rise  db     0
gz_old   dw     0
gz_cnt   dw     0               ; Flanken nach der ersten
gz_first dw     0               ; erster und letzter Wert (Kanal 0)
gz_last  dw     0
gz_blo   dw     0               ; Uhr zu Beginn einer Messung
gz_bhi   dw     0
gz_t0lo  dw     0
gz_t0hi  dw     0
gz_t1lo  dw     0
gz_t1hi  dw     0
gz_grz   dw     0               ; Abfragen, falls die Uhr steht
gz_grh   dw     0

gz_c2rd PROC    NEAR
        pushf
        cli
        mov     al,80h
        out     83h,al
        in      al,82h
        mov     ah,al
        in      al,82h
        xchg    al,ah
        popf
        ret
gz_c2rd ENDP

gz_c0rd PROC    NEAR
        pushf
        cli
        mov     al,00h
        out     83h,al
        in      al,80h
        mov     ah,al
        in      al,80h
        xchg    al,ah
        popf
        ret
gz_c0rd ENDP

gz_cvrd PROC    NEAR                    ; Status Bit 5 -> AX (0 oder 20h)
        in      al,0A0h
        and     al,20h
        mov     ah,0
        ret
gz_cvrd ENDP

; Startwert lernen: hoechster Wert ueber hoechstens 65536 Abfragen, Ende
; nach 8 Neuladungen; knapp unter 5000 oder 27463 wird gerundet
gz_lern PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        pushf
        cli
        call    gz_c2rd
        mov     bx,ax                   ; letzter Wert
        mov     si,ax                   ; Maximum
        xor     dx,dx                   ; Rueckspruenge
        xor     cx,cx
gzl1:   call    gz_c2rd
        cmp     ax,si
        jbe     gzl2
        mov     si,ax
gzl2:   cmp     ax,bx
        jbe     gzl3
        inc     dx                      ; Wert stieg: neu geladen
gzl3:   mov     bx,ax
        cmp     dx,8
        jae     gzl4
        loop    gzl1
        cmp     dx,2
        jb      gzlf
gzl4:   mov     gz_nmax,si
        mov     gz_snap,1
        mov     ax,5000
        cmp     si,ax
        ja      gzl5
        cmp     si,5000-48
        jae     gzl7
gzl5:   mov     ax,27463
        cmp     si,ax
        ja      gzl6
        cmp     si,27463-48
        jae     gzl7
gzl6:   mov     gz_snap,0               ; unbekannt: Maximum plus halbe Luecke
        mov     ax,si
        add     ax,8
gzl7:   mov     gz_n,ax
        mov     gz_schr,1
        popf
        call    gz_start
        clc
        jmp     short gzl9
gzlf:   mov     al,0B4h                 ; steht: Modus 2, voller Umfang
        out     83h,al
        xor     al,al
        out     82h,al
        out     82h,al
        mov     gz_n,0
        mov     gz_nmax,0
        mov     gz_snap,0
        mov     gz_schr,2
        popf
        call    gz_start
        stc
gzl9:   pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
gz_lern ENDP

gz_start PROC   NEAR
        push    ax
        call    gz_c2rd
        mov     gz_old,ax
        mov     gz_lo,0
        mov     gz_hi,0
        pop     ax
        ret
gz_start ENDP

gz_tick PROC    NEAR
        push    ax
        push    dx
        call    gz_c2rd
        mov     dx,gz_old
        mov     gz_old,ax
        sub     dx,ax                   ; alt - neu
        jae     gzt1
        add     dx,gz_n                 ; neu geladen: + Startwert (Modus 2: 0)
gzt1:   add     gz_lo,dx
        adc     gz_hi,0
        cmp     gz_schr,2
        jne     gzt2
        add     gz_lo,dx
        adc     gz_hi,0
gzt2:   pop     dx
        pop     ax
        ret
gz_tick ENDP

gz_us   PROC    NEAR
        cmp     gz_schr,0
        jne     gzu1
        call    gz_lern
gzu1:   call    gz_tick
        mov     ax,gz_lo
        mov     dx,gz_hi
        ret
gz_us   ENDP

; DX:AX = Uhr - Beginn der Messung
gz_rel  PROC    NEAR
        mov     ax,gz_lo
        mov     dx,gz_hi
        sub     ax,gz_blo
        sbb     dx,gz_bhi
        ret
gz_rel  ENDP

; Flanken zaehlen. DI = Abfrage (gz_c0rd, gz_cvrd), gz_rise = 1: nur
; steigend. -> AX us je Flanke (0 keine, 0FFFFh zu langsam), gz_cnt
gz_flank PROC   NEAR
        push    bx
        push    cx
        push    dx
        push    si
        pushf
        cli
        cmp     gz_schr,0
        jne     gzf0
        call    gz_lern
gzf0:   mov     gz_cnt,0
        mov     gz_grz,0
        mov     gz_grh,0
        call    gz_tick
        mov     ax,gz_lo
        mov     gz_blo,ax
        mov     ax,gz_hi
        mov     gz_bhi,ax
        call    di
        mov     bx,ax
        mov     gz_first,ax
        mov     gz_last,ax
        xor     si,si                   ; erste Flanke gesehen?
gzf1:   call    gz_tick
        call    di
        cmp     ax,bx
        je      gzf5
        mov     bx,ax
        mov     gz_last,ax
        cmp     gz_rise,0
        je      gzf2
        or      ax,ax
        jz      gzf5                    ; fallende Flanke
gzf2:   call    gz_rel
        or      si,si
        jnz     gzf3
        inc     si
        mov     gz_t0lo,ax
        mov     gz_t0hi,dx
        jmp     short gzf5
gzf3:   inc     gz_cnt
        mov     gz_t1lo,ax
        mov     gz_t1hi,dx
        sub     ax,gz_t0lo
        sbb     dx,gz_t0hi
        cmp     dx,gz_dur
        jae     gzf7                    ; lang genug gemessen
gzf5:   inc     gz_grz                  ; Uhr steht: nach 8 x 65536 aufgeben
        jnz     gzf5a
        inc     gz_grh
        cmp     gz_grh,8
        jae     gzf6x
gzf5a:  call    gz_rel
        or      si,si
        jnz     gzf6
        cmp     dx,4                    ; 0,26 s ohne Flanke: steht
        jae     gzf7
        jmp     gzf1
gzf6:   mov     cx,gz_dur
        add     cx,8
        cmp     dx,cx
        jb      gzf1
gzf7:   popf
        xor     ax,ax
        mov     cx,gz_cnt
        jcxz    gzf9
        mov     ax,gz_t1lo
        mov     dx,gz_t1hi
        sub     ax,gz_t0lo
        sbb     dx,gz_t0hi
        cmp     dx,cx
        jb      gzf8
        mov     ax,0FFFFh
        jmp     short gzf9
gzf8:   div     cx
gzf9:   pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
gzf6x:  popf
        mov     gz_cnt,0
        xor     ax,ax
        jmp     gzf9
gz_flank ENDP

; AX im Bereich BX..CX lassen, sonst 0
gz_plaus PROC   NEAR
        cmp     ax,bx
        jb      gzp0
        cmp     ax,cx
        jbe     gzp9
gzp0:   xor     ax,ax
gzp9:   ret
gz_plaus ENDP

gz_cal0 PROC    NEAR
        push    bx
        push    cx
        push    di
        mov     gz_stoss,0
        mov     bl,al
        mov     di,offset gz_c0rd
        mov     gz_rise,0
        call    gz_flank
        push    bx
        mov     bx,5000
        mov     cx,62500
        call    gz_plaus
        pop     bx
        or      ax,ax
        jnz     gzc8
        cmp     bl,1
        jne     gzc8
        out     14h,al                  ; Gate von Kanal 0 (Motor ein)
        mov     gz_stoss,1
        mov     cx,10
gzc1:   call    gz_bildw
        loop    gzc1
        call    gz_flank
        mov     bx,5000
        mov     cx,62500
        call    gz_plaus
gzc8:   mov     gz_c0per,ax
        push    ax
        mov     ax,gz_cnt
        mov     gz_c0cnt,ax
        pop     ax
        pop     di
        pop     cx
        pop     bx
        ret
gz_cal0 ENDP

gz_calv PROC    NEAR
        push    bx
        push    cx
        push    di
        mov     di,offset gz_cvrd
        mov     gz_rise,1
        call    gz_flank
        mov     bx,10000
        mov     cx,40000
        call    gz_plaus
        mov     gz_vper,ax
        push    ax
        mov     ax,gz_cnt
        mov     gz_vcnt,ax
        pop     ax
        pop     di
        pop     cx
        pop     bx
        ret
gz_calv ENDP

gz_hz   PROC    NEAR
        push    bx
        push    dx
        mov     bx,ax
        xor     ax,ax
        cmp     bx,1526
        jb      gzh9
        mov     dx,05F5h                ; 100.000.000
        mov     ax,0E100h
        div     bx
gzh9:   pop     dx
        pop     bx
        ret
gz_hz   ENDP

gz_bildw PROC   NEAR
        push    ax
        push    cx
        xor     cx,cx
gzb1:   in      al,0A0h
        test    al,20h
        loopnz  gzb1
        xor     cx,cx
gzb2:   in      al,0A0h
        test    al,20h
        loopz   gzb2
        pop     cx
        pop     ax
        ret
gz_bildw ENDP

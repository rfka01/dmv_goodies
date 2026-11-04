;=====================================================================
;  Taste M auf der Laufwerksseite: Motoranlauf und Schrittzeit von A:
;=====================================================================
; 1. Motoranlauf. Laufwerk A: anwaehlen (SENSE DRIVE STATUS), dann
;    warten, bis die Hardware den Motor abschaltet (13h Bit 0 = 1; der
;    8253 Kanal 0 schaltet ihn 14,9 s nach dem letzten OUT 14h ab), bis
;    20 s. Dann OUT 14h und die Zeit messen, bis 13h Bit 0 = 0 (Motor
;    an) und bis 13h Bit 2 = 1 (READY des Laufwerks), bis 2 s.
; 2. Schrittzeit. Mit SPECIFY 03 D6 02 (DOS 2.11, 48 tpi) und 03 E6 02
;    (96 tpi): RECALIBRATE, SEEK auf Zylinder 39, SEEK auf 0; gemessen
;    wird je SEEK die Zeit bis zum Interrupt. Beim 8272 ist die Schritt-
;    zeit (16 - SRT) ms bei 8 MHz, das Doppelte bei 4 MHz:
;    D6 -> 3 oder 6 ms, E6 -> 2 oder 4 ms je Schritt.
;    Am Ende bleibt SPECIFY 03 D6 02 stehen (wie DOS 2.11).
; Braucht eine Diskette in A: (sonst kein READY, kein Schritt).

MT_CYL  equ     39

mt_run:
        call    cls
        mov     si,s_mt_go
        call    puts
        call    nl
        mov     byte [m_ran],1
        ; A: anwaehlen, damit RDY das READY von A: zeigt
        xor     bl,bl
        call    fd_st3
        mov     [m_st3],al
        ; 1. warten, bis der Motor aus ist
        call    tk_start
mt_off: in      al,P_SYSSTAT
        test    al,1                    ; MOTOR/ = 1: aus
        jnz     mt_isoff
        mov     ax,10000                ; 20 s
        call    tk_over_ms2
        jnc     mt_off
        mov     byte [m_off],0          ; Motor ging nicht aus
        jmp     mt_seek
mt_isoff:
        call    tk_since
        mov     [m_toff],ax
        mov     [m_toff+2],dx
        mov     byte [m_off],1
        ; Motor an, Zeiten bis "an" und bis READY
        mov     word [m_ton],0xFFFF
        mov     word [m_ton+2],0xFFFF
        mov     word [m_trdy],0xFFFF
        mov     word [m_trdy+2],0xFFFF
        call    tk_start
        out     P_FDMOTOR,al
mt_on:  in      al,P_SYSSTAT
        mov     [m_s13],al
        test    al,1
        jnz     mt_on1
        cmp     word [m_ton+2],0xFFFF
        jne     mt_on1
        call    tk_since
        mov     [m_ton],ax
        mov     [m_ton+2],dx
mt_on1: test    byte [m_s13],SS_READY
        jz      mt_on2
        call    tk_since
        mov     [m_trdy],ax
        mov     [m_trdy+2],dx
        jmp     short mt_seek
mt_on2: mov     ax,1000                 ; 2 s
        call    tk_over_ms2
        jnc     mt_on

        ; 2. Schrittzeit mit D6 und mit E6
mt_seek:
        call    fd_motor
        mov     al,0xD6
        mov     di,m_d6
        call    mt_one
        mov     al,0xE6
        mov     di,m_e6
        call    mt_one
        mov     al,0xD6                 ; wie DOS 2.11 stehen lassen
        call    mt_spec
        ret

mt_spec:                                ; AL = SRT/HUT
        push    ax
        call    fdc_ready
        mov     al,0x03
        call    fdc_out
        pop     ax
        call    fdc_out
        mov     al,0x02                 ; HLT 1, DMA
        call    fdc_out
        ret

mt_one:                                 ; AL = SRT/HUT, DI = 12 Byte Ergebnis
        call    mt_spec
        call    fdc_ready               ; RECALIBRATE A:
        mov     al,0x07
        call    fdc_out
        xor     al,al
        call    fdc_out
        call    fd_cmdwait
        mov     al,MT_CYL               ; SEEK 39
        call    mt_sk
        mov     [di],ax                 ; ST0/PCN
        mov     [di+2],cx               ; Zeit in Schritten zu 2 us, tief
        mov     [di+4],dx               ; hoch
        xor     al,al                   ; SEEK 0
        call    mt_sk
        mov     [di+6],ax
        mov     [di+8],cx
        mov     [di+10],dx
        ret

mt_sk:                                  ; AL = Zylinder -> AX ST0/PCN, DX:CX Zeit
        push    ax
        call    fdc_ready
        mov     al,0x0F
        call    fdc_out
        xor     al,al
        call    fdc_out
        pop     ax
        call    fdc_out
        call    fd_cmdwait              ; startet die Uhr selbst
        push    ax
        call    tk_since
        mov     cx,ax
        pop     ax
        ret

;--- Anzeige (auf der Laufwerksseite) ---------------------------------------
pg_drv7:                                ; Reihenfolge wie in der Testanleitung: M, T, B
        mov     si,s_dv1
        call    puts
        call    nl
        call    nl
        call    mt_show
        call    nl
        jmp     pg_drives

mt_show:
        cmp     byte [m_ran],1
        je      pm_r
        mov     si,s_mt_k
        call    puts
        call    nl
        ret
pm_r:   mov     si,s_mt_h1
        call    puts
        cmp     byte [m_off],1
        je      pm_o
        mov     si,s_mt_noff
        call    puts
        call    nl
        jmp     short pm_s
pm_o:   mov     ax,[m_toff]
        mov     dx,[m_toff+2]
        call    pr_ms
        mov     si,s_mt_h2
        call    puts
        mov     ax,[m_ton]
        mov     dx,[m_ton+2]
        call    pr_msx
        mov     si,s_mt_h3
        call    puts
        mov     ax,[m_trdy]
        mov     dx,[m_trdy+2]
        call    pr_msx
        mov     si,s_mt_ms
        call    puts
        call    nl
pm_s:   mov     si,s_mt_d6
        call    puts
        mov     si,m_d6
        call    pm_line
        mov     si,s_mt_e6
        call    puts
        mov     si,m_e6
        call    pm_line
        ; Deutung: je Schritt mit D6
        mov     si,s_cpj
        call    puts
        ; Zeit je Schritt in 1/100 ms = Schritte * 2 us / 39 / 10 us
        mov     ax,[m_d6+2]             ; SEEK 0->39, tief
        mov     dx,[m_d6+4]             ; hoch
        mov     cx,195
        call    div32
        mov     [m_step],ax
        call    dec2c
        mov     si,s_mt_ps
        call    puts
        mov     si,s_mt_4
        mov     ax,[m_step]
        cmp     ax,540
        jb      pm_j8
        cmp     ax,660
        jbe     pm_jp
pm_j8:  mov     si,s_mt_8
        cmp     ax,270
        jb      pm_ju
        cmp     ax,330
        jbe     pm_jp
pm_ju:  mov     si,s_mt_u
pm_jp:  call    puts
        call    nl
        ret

pm_line:                                ; SI = 12 Byte Ergebnis
        lodsw                           ; ST0/PCN nach SEEK 39
        push    ax
        call    hex8
        mov     al,'/'
        call    putc
        pop     ax
        mov     al,ah
        call    hex8
        call    space
        lodsw                           ; Zeit tief
        mov     cx,ax
        lodsw                           ; Zeit hoch
        mov     dx,ax
        mov     ax,cx
        call    pr_ms
        mov     al,' '
        call    putc
        mov     al,'/'
        call    putc
        call    space
        lodsw
        push    ax
        call    hex8
        mov     al,'/'
        call    putc
        pop     ax
        mov     al,ah
        call    hex8
        call    space
        lodsw
        mov     cx,ax
        lodsw
        mov     dx,ax
        mov     ax,cx
        call    pr_ms
        mov     si,s_mt_ms
        call    puts
        call    nl
        ret

pr_msx:                                 ; wie pr_ms, FFFFFFFF = "-"
        cmp     ax,0xFFFF
        jne     pr_ms
        cmp     dx,0xFFFF
        jne     pr_ms
        mov     al,'-'
        jmp     putc

pr_ms:                                  ; DX:AX Schritte zu 2 us -> "n,nn" ms
        push    ax
        push    bx
        push    cx
        push    dx
        mov     cx,5                    ; -> 1/100 ms
        call    div32
        mov     cx,100
        call    div32                   ; DX:AX ganze ms, BX Rest
        push    bx
        call    dec32
        mov     al,','
        call    putc
        pop     ax
        mov     bl,10
        div     bl
        add     ax,0x3030
        call    putc
        mov     al,ah
        call    putc
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

div32:                                  ; DX:AX / CX -> DX:AX, Rest BX
        push    ax
        mov     ax,dx
        xor     dx,dx
        div     cx
        mov     bx,ax
        pop     ax
        div     cx
        xchg    dx,bx
        ret

s_mt_go db      'Motor- und Schrittmessung an A: (bis 20 s Warten auf Motor aus) ...',0
s_mt_k  db      2,'Taste M: Motoranlauf und Schrittzeit an A: (Diskette in A:, bis 25 s)',0
s_mt_h1 db      2,'M: Warten auf Motor aus ',1,0
s_mt_noff db    3,'Motor ging in 20 s nicht aus (13h Bit 0)',0
s_mt_h2 db      2,' ms; OUT 14h -> an ',1,0
s_mt_h3 db      2,' READY ',1,0
s_mt_ms db      2,' ms',0
s_mt_d6 db      2,'   SPECIFY D6: SEEK 39 ST0/PCN Zeit / SEEK 0  ',1,0
s_mt_e6 db      2,'   SPECIFY E6: SEEK 39 ST0/PCN Zeit / SEEK 0  ',1,0
s_mt_ps db      ' ms je Schritt (D6) -> ',0
s_mt_4  db      '8272 mit 4 MHz (6 ms)',0
s_mt_8  db      '8272 mit 8 MHz (3 ms)',0
s_mt_u  db      'unklar',0
m_ran   db      0
m_off   db      0
m_st3   db      0
m_s13   db      0
m_step  dw      0
m_toff  dd      0
m_ton   dd      0
m_trdy  dd      0
m_d6    times 12 db 0
m_e6    times 12 db 0

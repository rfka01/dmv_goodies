;***********************************************************************
;*                                                                     *
;*  PITTEST  -  Zeitbasis und Prozessor der NCR Decision Mate V        *
;*              (HWCONFIRM, Punkt 5)                                   *
;*                                                                     *
;*  Misst ohne Stoppuhr, alles gegen 8253 Kanal 2 (500 kHz, von DOS    *
;*  in Modus 3 betrieben, ein Zaehlschritt = 1 us):                    *
;*    - ob Kanal 0 zaehlt und mit welchem Takt (NCR: CLK0 56 Hz),      *
;*      vor und nach einem Anstoss seines Gates ueber Port 14h          *
;*    - die Bildfrequenz (GDC-Status Bit 5) - Gegenstueck zu BILDFREQ  *
;*    - mit K803 auf 4B: Kanal 2 selbst gegen die Echtzeituhr           *
;*  Dazu: V20 oder 8088 (zwei Verfahren), 8087, 8259 (K235 / K230),    *
;*  und wo DMVRACE den Z80-Kern ablegen wuerde.                         *
;*  CPU, 8087 und K235 erkennt HWDET.INC, wie in DMVRACE und DMVDEMO.  *
;*                                                                     *
;*  Stand 3.11.2026: 8087 nach "Hardware erkennen" (Steuerwort <>      *
;*  5A5Ah), Status- und Steuerwort roh angezeigt; Groesse von DMVRACE  *
;*  und Warteplatz des Z80 (F10Eh-F11Ch) wie DMVRACE vom 2.11.2026.    *
;*                                                                     *
;*  Waehrend der Messungen sind die Interrupts aus; die DOS-Uhr geht   *
;*  danach um die Messdauer (gut 15 s) nach.                            *
;*                                                                     *
;*  Uebersetzen: jwasm -Zm -Zne -bin -Fo PITTEST.COM PITTEST.ASM       *
;*  Code: Claude, Prompts: rfka01                                       *
;***********************************************************************

        .8087
CSEG    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  CS:CSEG,DS:CSEG,ES:CSEG,SS:CSEG
        ORG     100h

; Werte aus DMVRACE (bei Aenderungen an DMVRACE nachziehen, siehe dort
; ENDPROG und Z80SIZE)
R_END   EQU     62754           ; Programm samt Puffern und Stapel (ENDPROG)
R_Z80   EQU     12338           ; Z80-Kern (Z80SIZE)
R_TRAMP EQU     0F10Eh          ; Warteplatz des Z80 samt K234-Stummel
R_TLEN  EQU     15              ; (HZLEN in HWDET.INC)

START:  jmp     main

T_HEAD  db      'PITTEST - Zeitbasis und Prozessor der NCR DMV (HWCONFIRM 5, 3.11.2026)',0Dh,0Ah
        db      'Code: Claude, Prompts: rfka01',0Dh,0Ah,0Dh,0Ah,'$'
T_DOS   db      'MS-DOS $'
T_SEG   db      ', Programm ab Segment $'
T_CPU   db      0Dh,0Ah,0Dh,0Ah,'PROZESSOR',0Dh,0Ah,'$'
T_PUSH  db      '  PUSH 0 (6Ah):        $'
T_PV20  db      'SP -2  -> V20',0Dh,0Ah,'$'
T_P88   db      'SP bleibt -> 8088',0Dh,0Ah,'$'
T_PXX   db      'SP anders -> unbekannt',0Dh,0Ah,'$'
T_AAD   db      '  AAD 16 auf 0102h:    AL = $'
T_A88   db      'h  -> 8088 (12h)',0Dh,0Ah,'$'
T_AV20  db      'h  -> V20 (0Ch, Basis immer 10)',0Dh,0Ah,'$'
T_A00   db      'h  -> unbekannt',0Dh,0Ah,'$'
T_RES   db      '  Ergebnis:            $'
T_RV20  db      'V20',0Dh,0Ah,'$'
T_R88   db      '8088',0Dh,0Ah,'$'
T_RX    db      'WIDERSPRUCH',0Dh,0Ah,'$'
T_87    db      '  8087:                $'
T_87J   db      'vorhanden$'
T_87N   db      'nicht gefunden$'
T_87S   db      ' (Status $'
T_87C   db      'h, Steuerwort $'
T_87E   db      'h; vorbelegt 5A5Ah)',0Dh,0Ah,'$'
T_PIC   db      '  8259 auf 90h/91h:    $'
T_PICJ  db      'vorhanden -> K235',0Dh,0Ah,'$'
T_PICN  db      'fehlt -> K230/K231',0Dh,0Ah,'$'
T_C2    db      0Dh,0Ah,'KANAL 2 (Bezug, 500 kHz, Modus 3)',0Dh,0Ah,'  Startwert:  $'
T_C2R   db      ' (gerundet, hoechster Wert $'
T_C2E   db      ' (geschaetzt, hoechster Wert $'
T_C2Z   db      ')',0Dh,0Ah,'$'
T_C2N   db      'STEHT - ohne Kanal 2 keine weiteren Messungen',0Dh,0Ah,'$'
T_C0    db      0Dh,0Ah,'KANAL 0 (Motor-Auszeit, Modus 5, NCR: CLK0 56 Hz)',0Dh,0Ah,'$'
T_C0V   db      '  vorher:     $'
T_C0K   db      '  Port 14h angestossen (Diskettenmotor laeuft etwa 15 s an)',0Dh,0Ah,'$'
T_C0A   db      '  nachher:    $'
T_ZAEHL db      'zaehlt, $'
T_HZ    db      ' Hz  ($'
T_TAKT  db      ' Takte, Stand $'
T_TO    db      'h -> $'
T_HZE   db      'h)',0Dh,0Ah,'$'
T_STEHT db      'STEHT (Stand $'
T_SLOW  db      'zu langsam (unter 16 Hz)',0Dh,0Ah,'$'
T_VS    db      0Dh,0Ah,'BILDWECHSEL (GDC-Status Bit 5)',0Dh,0Ah,'  $'
T_VS2   db      ' Hz  ($'
T_VS3   db      ' Bilder; erwartet Mono 56,15, Farbe 56,06 oder 67,27)',0Dh,0Ah,'$'
T_VSN   db      'keine Bildwechsel gesehen',0Dh,0Ah,'$'
T_RTC   db      0Dh,0Ah,'K803 ECHTZEITUHR (4B, C8h-CFh)',0Dh,0Ah,'  $'
T_RTCN  db      'nicht gefunden - Kanal 2 bitte mit der Stoppuhr pruefen (S)',0Dh,0Ah,'$'
T_RTC1  db      'Kanal 2 ueber 5 Sekunden der K803: $'
T_RTC2  db      ' kHz (Soll 500,00)',0Dh,0Ah,'$'
T_RTCX  db      'Sekunden der K803 laufen nicht weiter',0Dh,0Ah,'$'
T_Z     db      0Dh,0Ah,'DMVRACE: PLATZ FUER DEN Z80-KERN (Bank 0)',0Dh,0Ah,'  Programm phys. $'
T_Z1    db      'h bis $'
T_Z2    db      'h',0Dh,0Ah,'  $'
T_ZOK   db      'Z80-Kern bei $'
T_ZMV   db      'zieht um nach Segment $'
T_ZMV2  db      ', dann $'
T_ZNO   db      'KEIN PLATZ - Z80 bliebe in der Box',0Dh,0Ah,'$'
T_ZMEM  db      'KEIN SPEICHER fuer den Umzug - Z80 bliebe in der Box',0Dh,0Ah,'$'
T_ZH    db      'h',0Dh,0Ah,'$'
T_ASK   db      0Dh,0Ah,'S = Stoppuhr fuer Kanal 2, andere Taste = Ende $'
T_SW1   db      0Dh,0Ah,0Dh,0Ah,'STOPPUHR: Taste druecken und zugleich die Armbanduhr starten.',0Dh,0Ah
        db      'Nach etwa einer Minute wieder eine Taste druecken.',0Dh,0Ah,'$'
T_SW2   db      '  laeuft ...',0Dh,0Ah,'$'
T_SW3   db      '  Kanal 2 hat gemessen: $'
T_SW4   db      ' s  - mit der Armbanduhr vergleichen',0Dh,0Ah,'$'
T_CRLF  db      0Dh,0Ah,'$'
T_SPC   db      ' $'

v_psp   dw      0
v_s0    db      0
v_key   db      0

        INCLUDE PITCAL.INC
        INCLUDE HWDET.INC

;-----------------------------------------------------------------------
; Ausgabe
;-----------------------------------------------------------------------
puts    PROC    NEAR                    ; DX = Text
        push    ax
        mov     ah,09h
        int     21h
        pop     ax
        ret
puts    ENDP

putc    PROC    NEAR                    ; DL = Zeichen
        push    ax
        mov     ah,02h
        int     21h
        pop     ax
        ret
putc    ENDP

; AX dezimal
putdec  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bx,10
        xor     cx,cx
pd1:    xor     dx,dx
        div     bx
        push    dx
        inc     cx
        or      ax,ax
        jnz     pd1
pd2:    pop     dx
        add     dl,'0'
        call    putc
        loop    pd2
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
putdec  ENDP

; DX:AX dezimal, Komma vor den letzten CL Stellen (CL = 0: keins)
putfix  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     di,cx                   ; Kommastellen
        and     di,0FFh
        xor     si,si                   ; Ziffern
        mov     bx,10
pf1:    mov     cx,ax                   ; DX:AX / 10
        mov     ax,dx
        xor     dx,dx
        div     bx
        xchg    ax,cx
        div     bx                      ; CX:AX Quotient, DX Rest
        push    dx
        inc     si
        mov     dx,cx
        mov     cx,ax
        or      cx,dx
        jnz     pf1
        mov     cx,si
        cmp     cx,di                   ; mindestens eine Stelle vor dem Komma
        ja      pf3
        mov     cx,di
        sub     cx,si
        inc     cx
pf2:    mov     dx,0                    ; fuehrende Nullen (auf den Stapel)
        push    dx
        inc     si
        loop    pf2
pf3:    mov     cx,si
pf4:    cmp     cx,di
        jne     pf5
        or      di,di
        jz      pf5
        mov     dl,','
        call    putc
pf5:    pop     dx
        add     dl,'0'
        call    putc
        loop    pf4
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
putfix  ENDP

; AX in 1/100 als nnn,nn
puthz   PROC    NEAR
        push    cx
        push    dx
        xor     dx,dx
        mov     cl,2
        call    putfix
        pop     dx
        pop     cx
        ret
puthz   ENDP

; AX hexadezimal, 4 Stellen
puthex  PROC    NEAR
        push    ax
        mov     al,ah
        call    puthx2
        pop     ax
        push    ax
        call    puthx2
        pop     ax
        ret
puthex  ENDP

puthx2  PROC    NEAR                    ; AL
        push    ax
        push    cx
        mov     ah,al
        mov     cl,4
        shr     al,cl
        call    puthx1
        mov     al,ah
        call    puthx1
        pop     cx
        pop     ax
        ret
puthx2  ENDP

puthx1  PROC    NEAR
        push    dx
        and     al,0Fh
        add     al,'0'
        cmp     al,'9'
        jbe     ph1
        add     al,7
ph1:    mov     dl,al
        call    putc
        pop     dx
        ret
puthx1  ENDP

; phys. Adresse DX:AX als 5 Hexstellen
putphys PROC    NEAR
        push    ax
        mov     al,dl
        call    puthx1
        pop     ax
        call    puthex
        ret
putphys ENDP



;-----------------------------------------------------------------------
; Kanal 0 einmal messen und ausgeben (Text DX vorneweg)
;-----------------------------------------------------------------------
showc0  PROC    NEAR
        call    puts
        call    cal0
        mov     ax,c_c0per
        or      ax,ax
        jnz     sc1
        mov     dx,offset T_STEHT
        call    puts
        mov     ax,c_first
        call    puthex
        mov     dx,offset T_HZE
        call    puts
        ret
sc1:    cmp     ax,62500
        jb      sc2
        mov     dx,offset T_SLOW
        call    puts
        ret
sc2:    mov     dx,offset T_ZAEHL
        call    puts
        call    calhz
        call    puthz
        mov     dx,offset T_HZ
        call    puts
        mov     ax,c_c0cnt
        call    putdec
        mov     dx,offset T_TAKT
        call    puts
        mov     ax,c_first
        call    puthex
        mov     dx,offset T_TO
        call    puts
        mov     ax,c_last
        call    puthex
        mov     dx,offset T_HZE
        call    puts
        ret
showc0  ENDP

;-----------------------------------------------------------------------
; K803: Sekundenregister lesen -> AL (stabil), CF = 1 kein Baustein
;-----------------------------------------------------------------------
rtcsec  PROC    NEAR
rs1:    in      al,0CEh                 ; Register 2 = Sekunden (BCD)
        mov     ah,al
        in      al,0CEh
        cmp     al,ah
        jne     rs1
        ret
rtcsec  ENDP

dortc   PROC    NEAR
        mov     dx,offset T_RTC
        call    puts
        mov     al,0                    ; Registergruppe 0, Interrupt aus
        out     0C8h,al
        call    rtcsec
        cmp     al,0FFh
        je      rtn
        mov     ah,al
        and     ah,0Fh
        cmp     ah,9
        ja      rtn
        cmp     al,59h
        ja      rtn
        ; Messung: auf Sekundenwechsel warten, dann fuenf weitere
        pushf
        cli
        call    c2start
        call    rtcsec
        mov     bl,al
        mov     cx,6                    ; erster Wechsel = Start
        mov     si,0
rt1:    call    c2tick
        cmp     c_hi,140                ; gut 9 s ohne Ergebnis: Abbruch
        jae     rtx
        call    rtcsec
        cmp     al,bl
        je      rt1
        mov     bl,al
        or      si,si
        jnz     rt2
        inc     si
        mov     ax,c_lo
        mov     c_t0lo,ax
        mov     ax,c_hi
        mov     c_t0hi,ax
rt2:    loop    rt1
        popf
        mov     ax,c_lo
        mov     dx,c_hi
        sub     ax,c_t0lo
        sbb     dx,c_t0hi               ; us-Schritte in 5 s
        mov     bx,100
        cmp     dx,bx
        jae     rtx2
        div     bx                      ; 1/100 kHz
        push    ax
        mov     dx,offset T_RTC1
        call    puts
        pop     ax
        call    puthz
        mov     dx,offset T_RTC2
        call    puts
        ret
rtx:    popf
rtx2:   mov     dx,offset T_RTCX
        call    puts
        ret
rtn:    mov     dx,offset T_RTCN
        call    puts
        ret
dortc   ENDP

;-----------------------------------------------------------------------
; Z80-Platz wie DMVRACE: zplace / zmove nachgebildet
;-----------------------------------------------------------------------
; Programmanfang phys. in v_ps (DX:AX); Bereich [BX, BX+CX) beruehrt es? CF
zs_lo   dw      0
zs_hi   dw      0
zhit2   PROC    NEAR
        push    ax
        push    dx
        mov     ax,zs_lo
        mov     dx,zs_hi
        or      dx,dx
        jnz     zf                      ; Programm ab 64 KB
        mov     dx,ax
        add     dx,R_END
        jc      zc1                     ; reicht ueber 64 KB
        cmp     bx,dx
        jae     zf
zc1:    mov     dx,bx
        add     dx,cx
        jc      zh                      ; Bereichsende 10000h
        cmp     dx,ax
        jbe     zf
zh:     pop     dx
        pop     ax
        stc
        ret
zf:     pop     dx
        pop     ax
        clc
        ret
zhit2   ENDP

; -> AX = Basis, CF = 1 kein Platz
zplace2 PROC    NEAR
        push    bx
        push    cx
        mov     bx,R_TRAMP
        mov     cx,R_TLEN
        call    zhit2
        jc      zp9
        mov     bx,(0F000h-R_Z80) AND 0FF00h
zp1:    cmp     bx,2000h
        jb      zp8
        mov     cx,R_Z80
        call    zhit2
        jnc     zp7
        sub     bx,100h
        jmp     zp1
zp7:    mov     ax,bx
        clc
        jmp     short zp9
zp8:    stc
zp9:    pop     cx
        pop     bx
        ret
zplace2 ENDP

setzs   PROC    NEAR                    ; AX = Segment
        push    ax
        push    cx
        mov     dx,ax
        mov     cl,4
        shl     ax,cl
        mov     cl,12
        shr     dx,cl
        mov     zs_lo,ax
        mov     zs_hi,dx
        pop     cx
        pop     ax
        ret
setzs   ENDP

dozone  PROC    NEAR
        mov     dx,offset T_Z
        call    puts
        mov     ax,v_psp
        call    setzs
        mov     ax,zs_lo
        mov     dx,zs_hi
        call    putphys
        mov     dx,offset T_Z1
        call    puts
        mov     ax,zs_lo
        mov     dx,zs_hi
        add     ax,R_END
        adc     dx,0
        call    putphys
        mov     dx,offset T_Z2
        call    puts
        call    zplace2
        jc      dz2
dz1:    push    ax
        mov     dx,offset T_ZOK
        call    puts
        pop     ax
        call    puthex
        mov     dx,offset T_ZH
        call    puts
        ret
dz2:    ; Umzug: neuer Block direkt hinter dem verkleinerten
        mov     ax,v_psp
        add     ax,(R_END+15)/16+1
        mov     bx,ax
        add     bx,(R_END+15)/16
        mov     cx,es:[2]               ; Speicherende laut PSP
        cmp     bx,cx
        ja      dz3
        push    ax
        mov     dx,offset T_ZMV
        call    puts
        pop     ax
        call    puthex
        push    ax
        mov     dx,offset T_ZMV2
        call    puts
        pop     ax
        call    setzs
        call    zplace2
        jnc     dz1
        mov     dx,offset T_ZNO
        call    puts
        ret
dz3:    mov     dx,offset T_ZMEM
        call    puts
        ret
dozone  ENDP

;-----------------------------------------------------------------------
; Taste direkt vom 8741 (Interrupts aus) -> AL
;-----------------------------------------------------------------------
kbget   PROC    NEAR
kg1:    in      al,41h
        test    al,1
        jz      kg1
        in      al,40h
        ret
kbget   ENDP

kbhit   PROC    NEAR                    ; ZF = 0: Taste da
        in      al,41h
        test    al,1
        ret
kbhit   ENDP

stopw   PROC    NEAR
        mov     dx,offset T_SW1
        call    puts
        pushf
        cli
        call    kbget
        call    c2start
        popf
        mov     dx,offset T_SW2
        call    puts
        pushf
        cli
        call    c2start
sw1:    call    c2tick
        call    kbhit
        jz      sw1
        in      al,40h
        popf
        mov     dx,offset T_SW3
        call    puts
        mov     ax,c_lo                 ; us -> 1/100 s (bis 655 s)
        mov     dx,c_hi
        mov     bx,10000
        cmp     dx,bx
        jae     sw9
        div     bx
        xor     dx,dx
        mov     cl,2
        call    putfix
        mov     dx,offset T_SW4
        call    puts
sw9:    ret
stopw   ENDP

;-----------------------------------------------------------------------
main:   mov     v_psp,cs
        mov     dx,offset T_HEAD
        call    puts
        mov     dx,offset T_DOS
        call    puts
        mov     ah,30h
        int     21h
        push    ax
        mov     ah,0
        call    putdec
        mov     dl,'.'
        call    putc
        pop     ax
        mov     al,ah
        mov     ah,0
        cmp     al,10
        jae     m0
        mov     dl,'0'
        call    putc
m0:     call    putdec
        mov     dx,offset T_SEG
        call    puts
        mov     ax,v_psp
        call    puthex
        ; Prozessor
        mov     dx,offset T_CPU
        call    puts
        call    hcpu                    ; HWDET.INC
        call    h87
        call    hpic
        mov     dx,offset T_PUSH
        call    puts
        mov     dx,offset T_P88
        cmp     h_spd,0
        je      m1
        mov     dx,offset T_PV20
        cmp     h_spd,2
        je      m1
        mov     dx,offset T_PXX
m1:     call    puts
        mov     dx,offset T_AAD
        call    puts
        mov     al,h_aad
        call    puthx2
        mov     dx,offset T_A88
        cmp     al,12h
        je      m2
        mov     dx,offset T_AV20
        cmp     al,0Ch
        je      m2
        mov     dx,offset T_A00
m2:     call    puts
        mov     dx,offset T_RES
        call    puts
        mov     dx,offset T_R88
        cmp     h_cpu,1
        je      m3
        mov     dx,offset T_RV20
        cmp     h_cpu,2
        je      m3
        mov     dx,offset T_RX
m3:     call    puts
        mov     dx,offset T_87
        call    puts
        mov     dx,offset T_87N
        cmp     h_87,0
        je      m4
        mov     dx,offset T_87J
m4:     call    puts
        mov     dx,offset T_87S
        call    puts
        mov     ax,h_sw
        call    puthex
        mov     dx,offset T_87C
        call    puts
        mov     ax,h_cw
        call    puthex
        mov     dx,offset T_87E
        call    puts
        mov     dx,offset T_PIC
        call    puts
        mov     dx,offset T_PICN
        cmp     h_k235,0
        je      m5
        mov     dx,offset T_PICJ
m5:     call    puts
        ; Kanal 2
        mov     dx,offset T_C2
        call    puts
        call    c2learn
        jnc     m6
        mov     dx,offset T_C2N
        call    puts
        jmp     m_zone
m6:     mov     ax,c_n
        call    putdec
        mov     dx,offset T_C2R
        cmp     c_nsnap,0
        jne     m7
        mov     dx,offset T_C2E
m7:     call    puts
        mov     ax,c_nmax
        call    putdec
        mov     dx,offset T_C2Z
        call    puts
        ; Kanal 0 vorher, Anstoss, nachher (je 2 s)
        mov     c_dur,31
        mov     dx,offset T_C0
        call    puts
        mov     dx,offset T_C0V
        call    showc0
        mov     dx,offset T_C0K
        call    puts
        out     14h,al                  ; Gate von Kanal 0 (Motor ein)
        mov     cx,10                   ; 10 Bildwechsel warten
m8:     call    calwv
        loop    m8
        mov     dx,offset T_C0A
        call    showc0
        ; Bildwechsel
        mov     dx,offset T_VS
        call    puts
        call    calv
        mov     ax,c_vper
        or      ax,ax
        jnz     m9
        mov     dx,offset T_VSN
        call    puts
        jmp     short m10
m9:     call    calhz
        call    puthz
        mov     dx,offset T_VS2
        call    puts
        mov     ax,c_vcnt
        call    putdec
        mov     dx,offset T_VS3
        call    puts
m10:    call    dortc
m_zone: call    dozone
        ; Stoppuhr?
        mov     dx,offset T_ASK
        call    puts
        mov     ah,0Ch                  ; Puffer leeren, dann Taste
        mov     al,08h
        int     21h
        and     al,0DFh
        cmp     al,'S'
        jne     m_end
        cmp     c_n,0
        je      m_end
        call    stopw
m_end:  mov     ah,0Ch                  ; Tastenpuffer leeren
        mov     al,0
        int     21h
        mov     dx,offset T_CRLF
        call    puts
        mov     ax,4C00h
        int     21h

; einen Bildwechsel abwarten (steigende Flanke, hoechstens ~0,1 s)
calwv   PROC    NEAR
        push    cx
        xor     cx,cx
cw1:    in      al,0A0h
        test    al,20h
        loopnz  cw1
        xor     cx,cx
cw2:    in      al,0A0h
        test    al,20h
        loopz   cw2
        pop     cx
        ret
calwv   ENDP

CSEG    ENDS
        END     START

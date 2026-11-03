;-----------------------------------------------------------------------
; HWDET.INC - Hardware erkennen nach "NCR Decision Mate V - Hardware
; erkennen" (Anleitung vom 2.11.2026, gemessen auf zwei echten DMV).
; Gemeinsam fuer DMVDEMO und DMVRACE.
;
;   hcpu   8088 oder V20: PUSH 0 (6Ah) und AAD 16 muessen uebereinstimmen
;   h87    8087: Status- und Steuerwort mit 5A5Ah vorbelegen, FNINIT,
;          FNSTSW, FNSTCW; CW <> 5A5Ah heisst vorhanden (8087: 03FFh)
;   hpic   K235: 13h Bit 1 = 0 (16-Bit-Karte) und 8259 auf 90h/91h
;          (AAh/55h ins Maskenregister, alter Wert zurueck)
;   hprobe Koprozessor auf Steckplatz 7 suchen: K234 (68008) oder K238
;          (Z8001, nur MAME-Branch homebrew). Zweisprachige Probe wie
;          DMVDIAG: ab 0:0000 stehen SSP/PC fuer den 68008 und zugleich
;          FCW/PC fuer den Z8001; der 68008 schreibt 36h nach 0:0000, der
;          Z8001 5Ah. -> h_k234 (1 keiner, 2 K234, 3 K238)
;   hrun68 Koprozessor einmal laufen lassen: der Z80 startet ihn mit
;          OUT DFh (nie D8h, damit lief die echte K234 nicht an), der
;          68008 gibt mit einem Schreiben auf FFFDFh zurueck, der Z8001
;          mit OUTB DFh, dann gibt der Z80 mit OUT D0h an den 8088
;          zurueck. Der 8088 steht so lange. Nur mit K235 (auf K230/K231
;          ist jedes OUT D8h-DFh eine Prozessorumschaltung).
;-----------------------------------------------------------------------
h_cpu   db      0               ; 1 = 8088, 2 = V20, 3 = Proben uneins
h_spd   db      0               ; SP-Differenz nach PUSH 0 (V20: 2)
h_aad   db      0               ; AL nach AAD 16 (8088 12h, V20 0Ch)
h_87    db      0               ; 1 = 8087 vorhanden
h_sw    dw      0
h_cw    dw      0
h_k235  db      0               ; 1 = K235 (8259 antwortet)
h_zok   db      0               ; 1 = Z80 kam zurueck (Marke)
h_k234  db      0               ; 0 nicht geprueft, 1 keiner, 2 K234, 3 K238
h_res   db      0               ; Byte 0:0000 nach dem Lauf (Probe)
h_vec   db      8 dup (0)       ; Startzustand (68000: SSP, PC; Z8001: 0, FCW, PC)
h_ivt   db      40 dup (0)      ; gesicherte 8088-Vektoren 0..9
; zweisprachige Probe (DMVDIAG, K238-Design-Stand 7.2), 40 Byte ab 0:0000:
; 68008: SSP 0000C000h, PC 8: BRA 0Eh; MOVE.B #36h,0000h; MOVE.B D0,FFDFh
; Z8001: FCW C000h, PC 0:0008; ab 000Ch: JR; LDB RL0,#5Ah; LDB 0000h,RL0;
;        OUTB DFh,RL0; HALT
h_probe db      000h,000h,0C0h,000h,000h,000h,000h,008h,060h,004h,000h,000h
        db      0E8h,007h,011h,0FCh,000h,036h,000h,000h,011h,0C0h,0FFh,0DFh
        db      04Eh,072h,027h,000h,0C8h,05Ah,06Eh,008h,000h,000h,03Ah,086h
        db      000h,0DFh,07Ah,000h
HZLO    EQU     0F10Eh          ; Warteplatz des Z80: F10Eh..F11Ch
HZLEN   EQU     15
HZMARK  EQU     0F11Ch
h_zsav  db      HZLEN dup (0)
; F10Eh: OUT (D0h),A / NOP  - hier steht der Z80, solange der 8088 laeuft
; F111h: DI / OUT (DFh),A / LD A,5Ah / LD (F11Ch),A / JP F10Eh
; DI zuerst: das Z80-ROM arbeitet mit IM 1 und EI; ein Interrupt, der
; waehrend des Laufs ansteht, saehe sonst bei 0038h die 8088-Vektoren als
; Code (in MAME: Zeichenmuell oben links auf dem Textschirm). Der Z80
; bleibt danach gesperrt, wie nach den Z80-Kernen von DMVRACE.
h_zstub db      0D3h,0D0h,000h
        db      0F3h,0D3h,0DFh,03Eh,05Ah,032h,01Ch,0F1h,0C3h,00Eh,0F1h
        db      000h

hcpu    PROC    NEAR
        mov     bx,sp
        db      6Ah,00h                 ; V20: PUSH 0   8088: JPE $+2
        mov     ax,bx
        sub     ax,sp
        mov     sp,bx
        mov     h_spd,al
        mov     ax,0102h
        db      0D5h,10h                ; AAD 16
        mov     h_aad,al
        mov     ah,1                    ; 8088?
        cmp     h_spd,0
        jne     hc1
        cmp     al,12h
        je      hc3
        jmp     short hc2
hc1:    mov     ah,2                    ; V20?
        cmp     h_spd,2
        jne     hc2
        cmp     al,0Ch
        je      hc3
hc2:    mov     ah,3
hc3:    mov     h_cpu,ah
        ret
hcpu    ENDP

h87     PROC    NEAR
        mov     h_87,0
        mov     h_sw,5A5Ah
        mov     h_cw,5A5Ah
        fninit
        mov     cx,40
h87a:   loop    h87a
        fnstsw  h_sw
        mov     cx,40
h87b:   loop    h87b
        fnstcw  h_cw
        mov     cx,40
h87c:   loop    h87c
        cmp     h_cw,5A5Ah
        je      h87x
        mov     h_87,1
h87x:   ret
h87     ENDP

hpic    PROC    NEAR
        mov     h_k235,0
        in      al,13h
        test    al,02h                  ; negiert: 0 = 16-Bit-Karte da
        jnz     hp9
        pushf
        cli
        in      al,91h
        mov     ah,al
        mov     al,0AAh
        out     91h,al
        in      al,91h
        mov     bl,al
        mov     al,55h
        out     91h,al
        in      al,91h
        mov     bh,al
        mov     al,ah
        out     91h,al
        popf
        cmp     bx,55AAh
        jne     hp9
        mov     h_k235,1
hp9:    ret
hpic    ENDP

; Liegt [AX*16, AX*16+CX) ueber dem Z80-Warteplatz? CF = 1: ja
hzhit   PROC    NEAR
        push    ax
        push    bx
        push    dx
        mov     bx,ax
        mov     dx,ax
        push    cx
        mov     cl,4
        shl     bx,cl
        mov     cl,12
        shr     dx,cl
        pop     cx
        or      dx,dx
        jnz     hzh0                    ; ab 64 KB: nein
        mov     ax,bx
        add     ax,cx
        jc      hzh1                    ; reicht ueber 64 KB
        cmp     ax,HZLO
        jbe     hzh0                    ; endet davor
hzh1:   cmp     bx,HZLO+HZLEN
        jae     hzh0                    ; beginnt dahinter
        stc
        jmp     short hzh9
hzh0:   clc
hzh9:   pop     dx
        pop     bx
        pop     ax
        ret
hzhit   ENDP

; Startzustand fuer einen Block bei Segment AX (der Code liegt ab P+100h,
; der Briefkasten ab P): 68008 SSP = P+1000h, PC = P+100h; Z8001 (h_k234
; = 3) FCW = C000h, PC = Segment P/64K, Versatz P+100h (der Block liegt
; in einem Segment, hcoblk)
hvec68  PROC    NEAR
        cmp     h_k234,3
        jne     hv0
        jmp     hvz8
hv0:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     dx,ax
        mov     cl,4
        shl     ax,cl
        mov     cl,12
        shr     dx,cl
        mov     bx,ax
        mov     cx,dx
        add     bx,1000h
        adc     cx,0
        mov     h_vec[0],0
        mov     h_vec[1],cl
        mov     h_vec[2],bh
        mov     h_vec[3],bl
        mov     bx,ax
        mov     cx,dx
        add     bx,100h
        adc     cx,0
        mov     h_vec[4],0
        mov     h_vec[5],cl
        mov     h_vec[6],bh
        mov     h_vec[7],bl
hv9:    pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
hvz8:   push    ax
        push    bx
        push    cx
        push    dx
        mov     dx,ax
        mov     cl,4
        shl     ax,cl                   ; Versatz
        mov     cl,12
        shr     dx,cl                   ; Segment
        add     ax,100h
        mov     h_vec[0],0
        mov     h_vec[1],0
        mov     h_vec[2],0C0h           ; FCW: segmentiert, System
        mov     h_vec[3],0
        or      dl,80h
        mov     h_vec[4],dl             ; Segment (Langform)
        mov     h_vec[5],0
        mov     h_vec[6],ah
        mov     h_vec[7],al
        jmp     hv9
hvec68  ENDP

; Block fuer den Koprozessor holen: BX Absaetze. Er darf nicht ueber dem
; Z80-Warteplatz liegen und fuer den Z8001 keine 64-KB-Grenze kreuzen.
; -> AX Segment, CF = 1 kein Speicher (hoechstens vier Versuche)
hcoblk  PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        mov     si,4
hcb1:   push    bx
        mov     ah,48h
        int     21h
        pop     bx
        jc      hcb9
        mov     dx,bx
        mov     cl,4
        shl     dx,cl
        mov     cx,dx                   ; Groesse in Bytes (Block < 64 KB)
        call    hzhit
        jc      hcb2
        cmp     h_k234,3
        jne     hcb8
        mov     dx,ax                   ; Z8001: Versatz + Groesse <= 10000h
        push    cx
        mov     cl,4
        shl     dx,cl
        pop     cx
        add     dx,cx
        jnc     hcb8
        or      dx,dx
        jz      hcb8
hcb2:   dec     si                      ; liegt ungeschickt: noch einer
        jnz     hcb1
        stc
        jmp     short hcb9
hcb8:   clc
hcb9:   pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
hcoblk  ENDP

; Koprozessor suchen (nur mit K235 aufrufen) -> h_k234 1/2/3
hprobe  PROC    NEAR
        push    si
        push    cx
        mov     si,offset h_probe
        mov     cx,40
        call    hrunco
        mov     h_k234,1
        jc      hpr9
        cmp     h_res,36h
        jne     hpr1
        mov     h_k234,2
hpr1:   cmp     h_res,5Ah
        jne     hpr9
        mov     h_k234,3
hpr9:   pop     cx
        pop     si
        ret
hprobe  ENDP

; 68008 einmal laufen lassen (Vektoren aus hvec68). Interrupts sind
; waehrenddessen aus. CF = 1: der Z80 kam nicht zurueck (h_zok = 0).
; Wer die Zeit braucht, liest Kanal 0 davor und danach.
hrun68  PROC    NEAR
        push    si
        push    cx
        mov     si,offset h_vec
        mov     cx,8
        call    hrunco
        pop     cx
        pop     si
        ret
hrun68  ENDP

; Startzustand DS:SI (CX Bytes, hoechstens 40) nach 0:0000 legen und den
; Koprozessor laufen lassen; h_res = Byte 0:0000 danach
hrunco  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    si
        push    di
        push    es
        pushf
        cli
        cld
        xor     ax,ax
        mov     es,ax
        mov     h_ctxn,cx
        mov     bx,0                    ; 8088-Vektoren <-> Startzustand
hr1:    mov     al,es:[bx]
        mov     h_ivt[bx],al
        mov     al,[si+bx]
        mov     es:[bx],al
        inc     bx
        cmp     bx,cx
        jb      hr1
        mov     si,HZLO                 ; Warteplatz sichern, Stummel hin
        mov     di,offset h_zsav
        mov     cx,HZLEN
hr2:    mov     al,es:[si]
        mov     [di],al
        inc     si
        inc     di
        loop    hr2
        mov     si,offset h_zstub
        mov     di,HZLO
        mov     cx,HZLEN
        rep     movsb
        out     0E0h,al                 ; Z80 auf Bank 0
        out     0D0h,al                 ; Bus an den Z80; der 8088 steht,
        nop                             ; bis der Z80 zurueckgibt
        nop
        mov     al,es:[0]
        mov     h_res,al
        mov     h_zok,0
        cmp     byte ptr es:[HZMARK],5Ah
        jne     hr3
        mov     h_zok,1
hr3:    mov     si,offset h_zsav        ; alles zuruecklegen
        mov     di,HZLO
        mov     cx,HZLEN
        rep     movsb
        mov     bx,0
hr4:    mov     al,h_ivt[bx]
        mov     es:[bx],al
        inc     bx
        cmp     bx,h_ctxn
        jb      hr4
        popf
        pop     es
        pop     di
        pop     si
        pop     cx
        pop     bx
        pop     ax
        cmp     h_zok,1                 ; CF = 1, wenn nicht
        ret
hrunco  ENDP
h_ctxn  dw      8

; erkannte Hardware in einer Zeile melden (DOS-Ausgabe)
T_HW    db      'Erkannt: $'
T_HW235 db      'K235 mit $'
T_HW230 db      'K230/K231 mit $'
T_HW88  db      '8088$'
T_HWV20 db      'V20$'
T_HWCPU db      'CPU? (PUSH 0 und AAD 16 uneins)$'
T_HW87  db      ', 8087$'
T_HW68  db      ', K234 (68008)$'
T_HWZ8  db      ', K238 (Z8001)$'
T_HW68N db      ', keine K234/K238$'
T_HWNL  db      0Dh,0Ah,'$'
hwline  PROC    NEAR
        mov     dx,offset T_HW
        call    hwp
        mov     dx,offset T_HW230
        cmp     h_k235,0
        je      hw1
        mov     dx,offset T_HW235
hw1:    call    hwp
        mov     dx,offset T_HW88
        cmp     h_cpu,1
        je      hw2
        mov     dx,offset T_HWV20
        cmp     h_cpu,2
        je      hw2
        mov     dx,offset T_HWCPU
hw2:    call    hwp
        cmp     h_87,0
        je      hw3
        mov     dx,offset T_HW87
        call    hwp
hw3:    mov     dx,offset T_HW68
        cmp     h_k234,2
        je      hw4
        mov     dx,offset T_HWZ8
        cmp     h_k234,3
        je      hw4
        mov     dx,offset T_HW68N
        cmp     h_k234,1
        jne     hw5
hw4:    call    hwp
hw5:    mov     dx,offset T_HWNL
hwp:    mov     ah,09h
        int     21h
        ret
hwline  ENDP

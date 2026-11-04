;=====================================================================
;  Seite "Interrupts": Leitungen der Steckplaetze, Taste I
;=====================================================================
; Ohne DOS steht der 8259 so, wie das ROM 33473 ihn hinterlaesst:
; pegelgesteuert, alles gesperrt. Im Pegelbetrieb zeigt das IRR (OCW3 0Ah
; an 90h, dann 90h lesen) den Stand der Eingaenge, auch wenn sie gesperrt
; sind. Eine leere oder nicht treibende eigene Leitung steht wegen der
; Pull-ups der K235 auf 1.
;
; Taste I misst erst den Ruhestand (13h, IRR, Maske) und dann jede Karte,
; die die Bausteinproben (Taste S, laeuft hier mit) gefunden haben und
; deren Leitung sich per Befehl schalten laesst:
;   K210 (8255): Steuerwort 8Ah (Port C unten Ausgang), PC1 = 1 (INIT
;                aus), dann PC3 an / aus. MAME legt den Interrupt der K210
;                auf PC3; ob die Platine das auch tut, zeigt die Messung.
;                Danach Steuerwort AAh und PC0-PC2 wie vorher.
;   2651/2661 (K211/K212/K213/K801): Befehlsregister 01h (Sender an, der
;                leere Sendepuffer meldet TxRDY) und 00h; danach der alte
;                Wert.
; Je Zustand werden 13h und IRR gelesen. Was sich mit "an" setzt und mit
; "aus" wieder loescht, ist die Leitung dieses Platzes. Fuer jede
; Bestueckung einen Start; die Dateien tragen die Laufnummer (06IRQTST.L1,
; .L2, ...). Aus dem Platz der Karte und der geschalteten IR folgen die
; Bruecken S1 (IR7: Platz 5 oder 6) und S2 (IR5: Platz 2a oder 2).

IQ_MAX  equ     4

iq_irr:                                 ; -> AL = IRR (nur K235), sonst FFh
        mov     al,0xFF
        cmp     word [j_card],s_k235
        jne     ii_e
        mov     al,0x0A
        out     0x90,al
        call    pt_dly
        in      al,0x90
ii_e:   ret

iq_snap:                                ; DI -> 2 Byte: 13h, IRR
        call    pt_dly
        call    pt_dly
        in      al,P_SYSSTAT
        mov     [di],al
        call    iq_irr
        mov     [di+1],al
        ret

irq_run:
        cli
        call    ports_run               ; Bausteine finden
        cli
        cmp     word [j_card],s_k235    ; 8259 wie nach dem ROM: pegelgesteuert, gesperrt
        jne     ir_0
        mov     al,PIC_FWICW1
        out     P_PICCMD,al
        mov     al,PIC_ICW2
        out     P_PICDATA,al
        mov     al,PIC_FWICW4
        out     P_PICDATA,al
        mov     al,0xFF
        out     P_PICDATA,al
ir_0:
        mov     byte [iq_ran],1
        mov     byte [iq_n],0
        mov     di,iq_rest
        call    iq_snap
        mov     al,0xFF
        cmp     word [j_card],s_k235
        jne     ir_m
        in      al,P_PICDATA
ir_m:   mov     [iq_mask],al
        ; Karten aus den Bausteinproben
        xor     cx,cx                   ; Haelfte 0-7
ir_l:   mov     ax,cx
        mov     bl,PH_SIZE
        mul     bl
        add     ax,pt_res
        mov     si,ax
        cmp     byte [iq_n],IQ_MAX
        jae     ir_e
        mov     al,[iq_n]
        mov     bl,8
        mul     bl
        add     ax,iq_tab
        mov     di,ax                   ; Eintrag: Haelfte, Art, 6 Byte Messung
        mov     [di],cl
        cmp     byte [si+PH_P2],2
        jne     ir_s
        mov     byte [di+1],1           ; 8255
        push    cx
        push    si
        call    iq_8255
        pop     si
        pop     cx
        inc     byte [iq_n]
        jmp     short ir_n
ir_s:   mov     al,[si+PH_SIO]
        cmp     al,2
        jb      ir_n
        mov     byte [di+1],2           ; 2651/2661
        push    cx
        push    si
        call    iq_2651
        pop     si
        pop     cx
        inc     byte [iq_n]
ir_n:   inc     cx
        cmp     cx,8
        jb      ir_l
ir_e:   ret

iq_8255:                                ; DI = Eintrag, SI = Ergebnis 0A
        mov     dx,IF_0A+3
        mov     al,0x8A                 ; A Ausgang, B Eingang, C oben Eingang, C unten Ausgang
        call    pt_out
        mov     al,0x03                 ; PC1 = 1: INIT aus
        call    pt_out
        push    di
        add     di,2
        call    iq_snap                 ; aus
        mov     dx,IF_0A+3
        mov     al,0x07                 ; PC3 an
        call    pt_out
        add     di,2
        call    iq_snap
        mov     dx,IF_0A+3
        mov     al,0x06                 ; PC3 aus
        call    pt_out
        add     di,2
        call    iq_snap
        pop     di
        ; wie vorher: AAh, PC0-PC2 aus dem Ruhestand vor den Proben
        mov     dx,IF_0A+3
        mov     al,0xAA
        call    pt_out
        mov     ah,[si+PH_PC]
        xor     cx,cx
i8_l:   mov     al,cl
        shl     al,1
        test    ah,1
        jz      i8_0
        or      al,1
i8_0:   call    pt_out
        shr     ah,1
        inc     cx
        cmp     cx,3
        jb      i8_l
        mov     al,0x0C                 ; INTE A aus
        call    pt_out
        ret

iq_2651:                                ; DI = Eintrag, CL = Haelfte
        mov     bx,cx
        mov     dl,[pt_bases+bx]
        xor     dh,dh
        add     dx,7                    ; Befehlsregister schreiben
        mov     al,0x00
        call    pt_out
        push    di
        add     di,2
        push    dx
        call    iq_snap                 ; aus
        pop     dx
        mov     al,0x01                 ; Sender an
        call    pt_out
        add     di,2
        push    dx
        call    iq_snap
        pop     dx
        mov     al,0x00
        call    pt_out
        add     di,2
        push    dx
        call    iq_snap
        pop     dx
        pop     di
        mov     al,[si+PH_SC]           ; alter Wert
        call    pt_out
        ret

;--- Anzeige -----------------------------------------------------------------
pg_irq:
        mov     si,s_iq
        call    puts
        call    nl
        cmp     byte [iq_ran],1
        je      pq_r
        mov     si,s_iq0
        call    puts
        call    nl
        mov     si,s_iq1
        call    puts
        call    nl
        mov     si,s_iq2
        call    puts
        call    nl
        mov     si,s_iq3
        call    puts
        call    nl
        ret
pq_r:   mov     si,s_iqr
        call    puts
        mov     al,[iq_rest]
        call    hex8
        mov     si,s_iqb
        call    puts
        mov     al,[iq_rest]
        call    bin8
        mov     si,s_iqi
        call    puts
        mov     al,[iq_rest+1]
        call    pq_irr
        mov     si,s_iqm
        call    puts
        mov     al,[iq_mask]
        call    hex8
        call    nl
        cmp     word [j_card],s_k235
        jne     pq_np
        mov     si,s_cpj                ; Ruhe: welche IR stehen
        call    puts
        mov     si,s_iqru
        call    puts
        mov     al,[iq_rest+1]
        call    pq_bits
        call    nl
pq_np:  call    nl
        cmp     byte [iq_n],0
        jne     pq_t
        mov     si,s_iqnone
        call    puts
        call    nl
        jmp     pq_h
pq_t:   mov     si,s_iqh
        call    puts
        call    nl
        xor     cx,cx
pq_l:   mov     al,cl
        mov     bl,8
        mul     bl
        add     ax,iq_tab
        mov     di,ax
        push    cx
        mov     bl,[di]                 ; Haelfte
        xor     bh,bh
        shl     bx,1
        mov     si,pt_names
        add     si,bx
        lodsb
        call    putc
        lodsb
        call    putc
        mov     si,s_iq55
        cmp     byte [di+1],1
        je      pq_k
        mov     si,s_iq51
pq_k:   call    puts
        mov     si,s_iq13
        call    puts
        mov     al,[di+2]
        call    hex8
        call    space
        mov     al,[di+4]
        call    hex8
        call    space
        mov     al,[di+6]
        call    hex8
        mov     si,s_iqir
        call    puts
        mov     al,[di+3]
        call    pq_irr
        call    space
        mov     al,[di+5]
        call    pq_irr
        call    space
        mov     al,[di+7]
        call    pq_irr
        call    nl
        ; Deutung: was mit "an" kommt und mit "aus" geht
        mov     si,s_cpj
        call    puts
        mov     al,[di+4]
        xor     al,[di+2]
        mov     ah,[di+4]
        xor     ah,[di+6]
        and     al,ah
        and     al,0xF0
        mov     si,s_iq13b
        call    puts
        call    pq_bits
        cmp     word [j_card],s_k235
        jne     pq_n
        mov     al,[di+5]
        xor     al,[di+3]
        mov     ah,[di+5]
        xor     ah,[di+7]
        and     al,ah
        mov     si,s_iqirb
        call    puts
        call    pq_bits
        call    pq_slot
pq_n:   call    nl
        pop     cx
        inc     cx
        cmp     cl,[iq_n]
        jb      pq_l
pq_h:   call    nl
        cmp     word [j_card],s_k235
        jne     pq_h1
        mov     si,s_iqs1
        call    puts
        call    nl
        mov     si,s_iqs2
        call    puts
        call    nl
pq_h1:  mov     si,s_iq3
        call    puts
        call    nl
        call    nl
        mov     si,s_foto
        call    puts
        call    nl
        ret

pq_slot:                                ; AL = geschaltete IR -> Platz laut K235-Schaltplan
        push    si
        mov     si,s_iqp3
        cmp     al,0x08
        je      qs_p
        mov     si,s_iqp4
        cmp     al,0x10
        je      qs_p
        mov     si,s_iqp5
        cmp     al,0x20
        je      qs_p
        mov     si,s_iqp7
        cmp     al,0x80
        je      qs_p
        mov     si,s_iqp2
        cmp     al,0x04
        jne     qs_e
qs_p:   call    puts
qs_e:   pop     si
        ret

pq_irr:                                 ; AL = IRR oder FFh ohne 8259
        cmp     word [j_card],s_k235
        jne     pi_no
        jmp     hex8
pi_no:  mov     al,'-'
        call    putc
        jmp     putc

pq_bits:                                ; AL = Bitmaske -> "7 5 3" oder "keine"
        push    ax
        push    cx
        or      al,al
        jnz     pb_s
        mov     si,s_iqk
        call    puts
        jmp     short pb_e
pb_s:   mov     cx,8
        mov     ah,al
pb_l:   dec     cx
        mov     al,ah
        shr     al,cl
        test    al,1
        jz      pb_n
        mov     al,cl
        add     al,'0'
        call    putc
        call    space
pb_n:   or      cx,cx
        jnz     pb_l
pb_e:   pop     cx
        pop     ax
        ret

s_iq    db      4,'INTERRUPTS - Leitungen der Steckplaetze (Taste I)',0
s_iq0   db      2,'Taste I: Bausteinproben, dann Ruhestand von 13h und IRR (8259),',0
s_iq1   db      2,'dann je K210 / 2651 / 2661 die eigene Leitung an und aus.',0
s_iq2   db      2,'Ohne DOS ist der 8259 pegelgesteuert: IRR = Stand der Eingaenge.',0
s_iq3   db      2,'Je Bestueckung ein Start; Dateien 06IRQTST.Ln mit Laufnummer n.',0
s_iqr   db      2,'Ruhe: 13h ',1,0
s_iqb   db      2,' (',1,0
s_iqi   db      2,')  IRR ',1,0
s_iqm   db      2,'  Maske ',1,0
s_iqru  db      'IRR in Ruhe gesetzt (Pull-ups/leere Plaetze): IR ',0
s_iqnone db     2,'keine schaltbare Karte gefunden (K210, K211-K213, K801)',0
s_iqh   db      2,'IF  Karte          13h aus an aus    IRR aus an aus',0
s_iq55  db      2,'  K210 PC3   ',1,0
s_iq51  db      2,'  2651 TxRDY ',1,0
s_iq13  db      2,' 13h ',1,0
s_iqir  db      2,'   IRR ',1,0
s_iq13b db      '13h Bit ',0
s_iqirb db      '  IR ',0
s_iqk   db      'keine ',0
s_iqp3  db      '= Platz 3',0
s_iqp4  db      '= Platz 4',0
s_iqp5  db      '= Platz 2 (S2 1-3) oder 2a (S2 1-2)',0
s_iqp7  db      '= Platz 5 (S1 1-2) oder 6 (S1 1-3)',0
s_iqp2  db      '= INT/ aller Plaetze',0
s_iqs1  db      2,'Bruecken: Karte in Platz 5 schaltet IR7 -> S1 1-2, in Platz 6 -> S1 1-3;',0
s_iqs2  db      2,'Karte in Platz 2 schaltet IR5 -> S2 1-3, sonst steht IR5 auf 2a (1-2).',0
iq_ran  db      0
iq_n    db      0
iq_mask db      0
iq_rest db      0,0
iq_tab  times IQ_MAX*8 db 0

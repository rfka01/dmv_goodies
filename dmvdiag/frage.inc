;=====================================================================
;  DMVFRAGE: die offenen Fragen nach DMVDIAG 0.16 in einem Lauf
;=====================================================================
; Feste Belegung (Maschine A, Farbe, K235):
;   Platz 2 K801 (IFSEL 2B)   Platz 3 K211 (IFSEL 1)   Platz 4 K803 (4B)
;   Platz 5 K210 (IFSEL 0)    Platz 6 leer
; Fragen und wie dieser Lauf sie beantwortet:
;   S1      K210 in Platz 5 schaltet IR7 -> S1 1-2. Schaltet sie nicht
;           und steht IR7 in Ruhe (leerer Platz 6, Pull-up) -> S1 1-3.
;   S2      K801 in Platz 2 schaltet IR5 -> S2 1-3. Schaltet sie nur ihr
;           13h-Bit und steht IR5 in Ruhe (leerer Platz 2a) -> S2 2a.
;   13h     welches Bit Platz 2 meldet (Bit, das die K801 schaltet).
;   Leitung K211 in Platz 3, K803 in Platz 4: IR3/IR4 in Ruhe 0 -> die
;           Karte haelt ihre Leitung inaktiv, 1 -> sie treibt sie nicht.
;   K211    Interrupt ueber TxRDY/TxEMT (Sender an) oder ueber RxRDY
;           (Rueckschleife im 2651: Byte senden, es kommt zurueck).
; Gemessen wird nur, was die Bausteinproben finden; die Seite sagt, ob
; die Karten an der erwarteten Stelle stehen.

fr_main:
        mov     byte [rs_off],1         ; kein Sichern (auch nicht bei X)
        call    cls
        call    header
        mov     si,fr_t
        call    puts
        call    nl
        call    nl
        mov     si,fr_i1
        call    puts
        call    nl
        mov     si,fr_i2
        call    puts
        call    nl
        mov     si,fr_i3
        call    puts
        call    nl
        call    nl
        mov     si,fr_i4
        call    puts
        call    nl
        call    getkey
        or      al,0x20
        cmp     al,'x'
        jne     fr_go
        jmp     pg_exit
fr_go:  call    cls
        mov     si,fr_run
        call    puts
        call    fr_measure
        call    cls
        call    header
        call    fr_show
fr_k:   call    getkey
        or      al,0x20
        cmp     al,'x'
        jne     fr_go
        jmp     pg_exit

;--- Messen -----------------------------------------------------------------
fr_measure:
        mov     byte [fq_ok],0
        cmp     word [j_card],s_k235
        jne     fq_e
        mov     byte [fq_ok],1
        call    ports_run               ; Bausteine finden (schreibt pt_res)
        cli
        ; Uhr: laeuft sie? (K803 auf 4B)
        xor     al,al
        out     RTC_LATCH,al
        call    rtc_sec
        mov     [fr_rtc1],al
        mov     ax,600
        call    wait_ms2
        call    rtc_sec
        mov     [fr_rtc2],al
        cli
        ; 8259 wie nach dem ROM: pegelgesteuert, alles gesperrt
        mov     al,PIC_FWICW1
        out     P_PICCMD,al
        mov     al,PIC_ICW2
        out     P_PICDATA,al
        mov     al,PIC_FWICW4
        out     P_PICDATA,al
        mov     al,0xFF
        out     P_PICDATA,al
        mov     di,fr_rest
        call    iq_snap
        ; K210 (0A): PC3
        mov     di,fr_e210
        mov     byte [di],0xFF
        cmp     byte [pt_res+PH_P2],2
        jne     fq_1
        mov     byte [di],0
        mov     si,pt_res
        call    iq_8255
fq_1:   ; K801 (2B, Haelfte 5): TxRDY
        mov     di,fr_e801
        mov     byte [di],0xFF
        cmp     byte [pt_res+5*PH_SIZE+PH_SIO],2
        jb      fq_2
        mov     byte [di],5
        mov     si,pt_res+5*PH_SIZE
        mov     cx,5
        call    iq_2651
fq_2:   ; K211 (1A, Haelfte 2): Sender an, dann Rueckschleife
        mov     di,fr_e211
        mov     byte [di],0xFF
        mov     byte [fr_e211l],0xFF
        cmp     byte [pt_res+2*PH_SIZE+PH_SIO],2
        jb      fq_e
        mov     byte [di],2
        mov     si,pt_res+2*PH_SIZE
        mov     cx,2
        call    iq_2651
        mov     byte [fr_e211l],2
        call    fr_loop
fq_e:   ret

fr_loop:                                ; K211: RxRDY ueber die Rueckschleife
        mov     dx,IF_1A+3              ; Befehlsregister lesen: Zeiger auf MR1
        in      al,dx
        mov     dx,IF_1A+2
        in      al,dx
        mov     [fr_mr1],al
        in      al,dx
        mov     [fr_mr2],al
        mov     dx,IF_1A+7
        xor     al,al                   ; alles aus
        call    pt_out
        mov     dx,IF_1A+3
        in      al,dx
        mov     dx,IF_1A+6
        mov     al,0x4E                 ; asynchron, 16x, 8 Bit, ohne Paritaet, 1 Stopp
        call    pt_out
        mov     al,0x3E                 ; interne Takte, 9600 Bd
        call    pt_out
        mov     dx,IF_1A
        in      al,dx                   ; Empfaenger leeren
        in      al,dx
        mov     di,fr_e211l+2
        call    iq_snap                 ; aus
        mov     dx,IF_1A+7
        mov     al,0xA7                 ; Rueckschleife, RTS, Empfaenger an, DTR, Sender an
        call    pt_out
        mov     dx,IF_1A+4
        mov     al,0x55                 ; ein Byte senden
        call    pt_out
        mov     ax,3                    ; 6 ms (ein Zeichen bei 9600 Bd: 1 ms)
        call    wait_ms2
        cli
        mov     di,fr_e211l+4
        call    iq_snap                 ; an
        mov     dx,IF_1A+1
        in      al,dx
        mov     [fr_lst],al             ; Status (Bit 1 RxRDY)
        mov     dx,IF_1A
        in      al,dx
        mov     [fr_ldat],al            ; zurueckgekommenes Byte
        mov     dx,IF_1A+7
        xor     al,al
        call    pt_out
        mov     di,fr_e211l+6
        call    iq_snap                 ; aus
        ; wie vorher: MR1, MR2, Befehlsregister
        mov     dx,IF_1A+3
        in      al,dx
        mov     dx,IF_1A+6
        mov     al,[fr_mr1]
        call    pt_out
        mov     al,[fr_mr2]
        call    pt_out
        mov     dx,IF_1A+7
        mov     al,[pt_res+2*PH_SIZE+PH_SC]
        call    pt_out
        ret

;--- Auswertung -------------------------------------------------------------
fr_d13:                                 ; DI = Eintrag -> AL = 13h-Bits, die mit "an" kommen und mit "aus" gehen
        mov     al,[di+4]
        xor     al,[di+2]
        mov     ah,[di+4]
        xor     ah,[di+6]
        and     al,ah
        and     al,0xF0
        ret
fr_dir:                                 ; DI = Eintrag -> AL = IR, ebenso
        mov     al,[di+5]
        xor     al,[di+3]
        mov     ah,[di+5]
        xor     ah,[di+7]
        and     al,ah
        ret

fr_line:                                ; SI = Name, DI = Eintrag: Rohwerte einer Probe
        call    puts
        cmp     byte [di],0xFF
        jne     fl_m
        mov     si,fr_nf
        call    puts
        jmp     nl
fl_m:   mov     si,s_iq13
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
        call    hex8
        call    space
        mov     al,[di+5]
        call    hex8
        call    space
        mov     al,[di+7]
        call    hex8
        mov     si,fr_ch
        call    puts
        call    fr_d13
        call    pq_bits
        mov     si,fr_chir
        call    puts
        call    fr_dir
        call    pq_bits
        jmp     nl

fr_show:
        mov     si,fr_t
        call    puts
        call    nl
        mov     si,fr_i2
        call    puts
        call    nl
        cmp     byte [fq_ok],1
        je      fs_1
        mov     si,fr_nok
        call    puts
        call    nl
        ret
fs_1:   ; gefunden?
        mov     si,fr_g
        call    puts
        mov     si,fr_g210
        call    puts
        mov     al,[pt_res+PH_P2]
        cmp     al,2
        call    fr_jn
        mov     si,fr_g211
        call    puts
        cmp     byte [pt_res+2*PH_SIZE+PH_SIO],2
        call    fr_jn
        mov     si,fr_g801
        call    puts
        cmp     byte [pt_res+5*PH_SIZE+PH_SIO],3
        call    fr_jn
        mov     si,fr_g803
        call    puts
        mov     al,[fr_rtc1]
        call    bcd59
        jc      fs_rn
        mov     al,[fr_rtc2]
        call    bcd59
        jc      fs_rn
        mov     al,[fr_rtc1]
        cmp     al,[fr_rtc2]
        je      fs_rn
        xor     al,al                   ; ZF = 1: ja
        jmp     short fs_rj
fs_rn:  or      al,1                    ; ZF = 0: nein
        mov     al,1
        or      al,al
fs_rj:  call    fr_jn
        call    nl
        call    nl
        ; Rohwerte
        mov     si,fr_ru
        call    puts
        mov     al,[fr_rest]
        call    hex8
        mov     si,s_iqb
        call    puts
        mov     al,[fr_rest]
        call    bin8
        mov     si,s_iqi
        call    puts
        mov     al,[fr_rest+1]
        call    hex8
        mov     si,fr_ru2
        call    puts
        mov     al,[fr_rest+1]
        call    pq_bits
        call    nl
        mov     si,fr_n801
        mov     di,fr_e801
        call    fr_line
        mov     si,fr_n210
        mov     di,fr_e210
        call    fr_line
        mov     si,fr_n211
        mov     di,fr_e211
        call    fr_line
        mov     si,fr_n211l
        mov     di,fr_e211l
        call    fr_line
        cmp     byte [fr_e211l],0xFF
        je      fs_2
        mov     si,fr_lb
        call    puts
        mov     al,[fr_lst]
        call    hex8
        mov     si,fr_lb2
        call    puts
        mov     al,[fr_ldat]
        call    hex8
        mov     si,fr_lb3
        call    puts
        call    nl
fs_2:   call    nl
        mov     si,fr_ant
        call    puts
        call    nl
        ; S1
        mov     si,fr_s1
        call    puts
        mov     di,fr_e210
        cmp     byte [di],0xFF
        je      fs_s1u
        call    fr_dir
        test    al,0x80
        jz      fs_s1b
        mov     si,fr_s1a
        jmp     short fs_s1p
fs_s1b: test    byte [fr_rest+1],0x80
        jz      fs_s1u
        mov     si,fr_s1c
        jmp     short fs_s1p
fs_s1u: mov     si,fr_unk
fs_s1p: call    puts
        call    nl
        ; S2 und 13h-Bit von Platz 2
        mov     si,fr_s2
        call    puts
        mov     di,fr_e801
        cmp     byte [di],0xFF
        je      fs_s2u
        call    fr_dir
        test    al,0x20
        jz      fs_s2b
        mov     si,fr_s2a
        jmp     short fs_s2p
fs_s2b: call    fr_d13
        or      al,al
        jz      fs_s2u
        test    byte [fr_rest+1],0x20
        jz      fs_s2u
        mov     si,fr_s2c
        jmp     short fs_s2p
fs_s2u: mov     si,fr_unk
fs_s2p: call    puts
        call    nl
        mov     si,fr_b2
        call    puts
        mov     di,fr_e801
        cmp     byte [di],0xFF
        je      fs_b2u
        call    fr_d13
        call    pq_bits
        jmp     short fs_b2e
fs_b2u: mov     si,fr_unk
        call    puts
fs_b2e: call    nl
        ; K211 in Platz 3: Leitung in Ruhe, Interrupt
        mov     si,fr_l211
        call    puts
        test    byte [fr_rest+1],0x08
        call    fr_drv
        mov     si,fr_q211
        call    puts
        mov     di,fr_e211
        cmp     byte [di],0xFF
        je      fs_qu
        call    fr_dir
        mov     bl,al
        mov     di,fr_e211l
        call    fr_dir
        mov     bh,al
        mov     si,fr_qtx
        test    bl,0x08
        jnz     fs_qp
        mov     si,fr_qrx
        test    bh,0x08
        jnz     fs_qp
        mov     si,fr_qno
        cmp     byte [fr_ldat],0x55     ; Rueckschleife hat gearbeitet?
        je      fs_qp
        mov     si,fr_qlb
        jmp     short fs_qp
fs_qu:  mov     si,fr_unk
fs_qp:  call    puts
        call    nl
        ; K803 in Platz 4
        mov     si,fr_l803
        call    puts
        test    byte [fr_rest+1],0x10
        call    fr_drv
        call    nl
        mov     si,s_foto
        call    puts
        call    nl
        ret

fr_drv:                                 ; ZF aus test: 0 -> treibt, sonst Pull-up
        mov     si,fr_dr1
        jz      fd_p
        mov     si,fr_dr0
fd_p:   call    puts
        jmp     nl

fr_jn:                                  ; ZF = 1 -> "ja", sonst "NEIN"
        mov     si,fr_ja
        je      fj_p
        mov     si,fr_nein
fj_p:   jmp     puts

fr_t    db      4,'DMVFRAGE - offene Fragen (Bruecken, Leitungen, K211)',0
fr_i1   db      2,'Belegung vor dem Einschalten (Maschine A, Farbe, mit K235):',0
fr_i2   db      1,'Platz 2 K801 (2B)  3 K211 (1)  4 K803 (4B)  5 K210 (0)  6 leer',0
fr_i3   db      2,'K806, K215, C3282 heraus. Laufzeit etwa 3 s, danach eine Seite.',0
fr_i4   db      3,'Taste: Lauf starten   X: Ende',0
fr_run  db      'DMVFRAGE laeuft ...',0
fr_nok  db      3,'Keine K235 erkannt - die Fragen brauchen den 8259 der K235.',0
fr_g    db      2,'gefunden: ',0
fr_g210 db      2,'K210 ',0
fr_g211 db      2,'  K211 ',0
fr_g801 db      2,'  K801 ',0
fr_g803 db      2,'  K803 ',0
fr_ja   db      1,'ja',0
fr_nein db      3,'NEIN',0
fr_ru   db      2,'Ruhe:            13h ',1,0
fr_ru2  db      2,'  gesetzt: IR ',1,0
fr_n801 db      2,'K801 TxRDY   ',0
fr_n210 db      2,'K210 PC3     ',0
fr_n211 db      2,'K211 Sender  ',0
fr_n211l db     2,'K211 Rueck.  ',0
fr_nf   db      3,'nicht gefunden',0
fr_ch   db      2,'  13h ',1,0
fr_chir db      2,' IR ',1,0
fr_lb   db      2,'  Rueckschleife: Status ',1,0
fr_lb2  db      2,'  Byte ',1,0
fr_lb3  db      2,'  (55 = angekommen)',0
fr_ant  db      4,'ANTWORTEN',0
fr_s1   db      2,'S1 (IR7):           ',3,0
fr_s1a  db      'S1 1-2 (Platz 5)',0
fr_s1c  db      'S1 1-3 (Platz 6)',0
fr_s2   db      2,'S2 (IR5):           ',3,0
fr_s2a  db      'S2 1-3 (Platz 2)',0
fr_s2c  db      'S2 1-2 (Platz 2a)',0
fr_b2   db      2,'Platz 2 in 13h:     ',3,'Bit ',0
fr_l211 db      2,'K211 Platz 3:       ',3,0
fr_q211 db      2,'K211 Interrupt:     ',3,0
fr_qtx  db      'ueber TxRDY/TxEMT (Sender an)',0
fr_qrx  db      'ueber RxRDY (Rueckschleife)',0
fr_qno  db      'weder TxRDY/TxEMT noch RxRDY (Bruecke? nicht angeschlossen?)',0
fr_qlb  db      'kein Interrupt; Rueckschleife kam nicht an - unklar',0
fr_l803 db      2,'K803 Platz 4:       ',3,0
fr_dr1  db      'haelt ihre Leitung inaktiv (IR in Ruhe 0)',0
fr_dr0  db      'treibt ihre Leitung nicht (IR in Ruhe 1, Pull-up)',0
fr_unk  db      'unklar - Belegung pruefen',0
fq_ok   db      0
fr_rtc1 db      0
fr_rtc2 db      0
fr_mr1  db      0
fr_mr2  db      0
fr_lst  db      0
fr_ldat db      0
fr_rest db      0,0
fr_e210 times 8 db 0
fr_e801 times 8 db 0
fr_e211 times 8 db 0
fr_e211l times 8 db 0

;=====================================================================
;  DMVINFO: Hardware-Uebersicht (mit INFO gebaut, siehe dmvinfo.nasm)
;=====================================================================
; Seite 1 Uebersicht, Seite 2 Steckplaetze und Interrupts; R zeigt die
; Rohwertseiten von DMVDIAG (X kehrt zurueck), D schreibt beide Seiten
; als DMVINFO.TXT auf A:, W misst neu, X beendet.
; Gemessen wird nach den Regeln der Anleitung "DMV - Hardware erkennen":
; Adressecho des leeren Busses = frei, Platte nur mit 55/AA, Uhr nur mit
; gueltigem BCD, das sich aendert, K234/K238 nur mit erkannter K235.

inf_main:
        mov     byte [rs_off],1         ; kein automatisches Sichern
        call    inf_measure
        mov     byte [inf_pg],0
inf_show:
        mov     byte [inf_raw],0
        call    cls
        call    inf_page
        call    inf_foot
inf_key:
        call    getkey
        or      al,0x20
        cmp     al,'x'
        je      inf_exit
        cmp     al,'r'
        je      inf_rawgo
        cmp     al,'d'
        je      inf_save
        cmp     al,'w'
        je      inf_again
        xor     byte [inf_pg],1         ; jede andere Taste: andere Seite
        jmp     inf_show
inf_again:
        call    cls
        mov     si,s_hello
        call    puts
        call    measure_all
        call    inf_measure
        jmp     inf_show
inf_rawgo:
        mov     byte [inf_raw],1
        mov     byte [page],0
        jmp     pg_show
inf_exit:
        jmp     reboot                  ; X: Neustart ueber das Z80-ROM

inf_page:                               ; aktuelle Seite mit Kopfzeile
        call    header
        cmp     byte [inf_pg],0
        jne     ip_2
        jmp     inf_p1
ip_2:   jmp     inf_p2

inf_foot:
        mov     byte [FW_CURX],0
        mov     byte [FW_CURY],24
        mov     al,A_HEAD
        call    setattr
        mov     si,s_if_foot
        call    puts
        call    pad80
        mov     al,A_VAL
        call    setattr
        ret

;--- Messen -----------------------------------------------------------------
inf_measure:
        call    cls
        mov     si,s_if_run
        call    puts
        call    nl
        call    ports_run               ; Bausteinproben (Echo = frei)
        call    inf_k806
        call    inf_hd
        call    inf_rtc
        call    inf_irq
        call    inf_k233
        ; Koprozessor nur mit K235 (die K230/K231 naehme DFh als D0h)
        mov     byte [c_ran],0
        cmp     word [j_card],s_k235
        jne     im_e
        mov     si,s_if_cop
        call    puts
        call    nl
        call    copro_run
im_e:   cli
        ret

; K233 (16 KB gemeinsames RAM, kippt mit Schreiben auf 7Eh/7Fh): nur
; pruefen, wenn auf 7Fh nichts Fremdes hoert - IFSEL 1B frei, oder ein
; 2651/2661 (K211-Gruppe auf IFSEL 1, K801 auf 1B), dem k233_kipp sein
; ueber 7Bh gelesenes Befehlsregister zurueckschreibt. Eine 8741-Karte
; (K215, K806) oder Unbekanntes auf 1B: nicht pruefen.
inf_k233:
        mov     byte [k_ran],0
        mov     word [inf_k3b],0
        mov     cx,2                    ; 1A: K211-Gruppe belegt auch 1B
        call    inf_card
        cmp     si,s_if_c211
        je      ik3_go
        mov     cx,3                    ; 1B
        call    inf_card
        or      si,si
        jz      ik3_go
        cmp     si,s_if_c801
        je      ik3_go
        mov     [inf_k3b],si            ; diese Karte verhindert die Probe
        ret
ik3_go: call    k233_run
        cli
        ret

inf_k233ok:                             ; ZF = 1: K233 gefunden
        cmp     byte [k_ran],1
        jne     ik3o_e
        cmp     byte [k_r2],0x5A
        jne     ik3o_e
        cmp     byte [k_r1],0xA5
ik3o_e: ret

inf_k806:                               ; K806 in freien, belegten Haelften ab 1A
        mov     di,inf_k8
        mov     cx,8
        xor     al,al
        rep     stosb
        mov     cx,2
ik_l:   mov     ax,cx
        mov     bl,PH_SIZE
        mul     bl
        add     ax,pt_res
        mov     si,ax
        cmp     byte [si+PH_CLM],0
        jne     ik_n
        cmp     byte [si+PH_OCC1],2
        jne     ik_n
        mov     bx,cx
        mov     dl,[pt_bases+bx]
        xor     dh,dh
        inc     dx                      ; Status x1
        push    cx
        call    in3
        pop     cx
        test    al,0x33                 ; OBF, IBF, ROM- und RAM-Fehler aus
        jnz     ik_n
        cmp     al,ah
        jne     ik_n
        mov     ah,dl                   ; kein Adressecho
        cmp     al,ah
        je      ik_n
        mov     byte [pw_mask],2
        mov     ax,0x0200
        call    pt_wait                 ; Eingang frei
        jc      ik_n
        xor     al,al                   ; Befehl 00: absolute Position, fuenf Bytes
        call    pt_out
        xor     bx,bx                   ; Bytes zaehlen
ik_r:   mov     byte [pw_mask],1
        mov     ax,0x0101
        call    pt_wait
        jc      ik_d
        dec     dx
        in      al,dx
        inc     dx
        inc     bx
        cmp     bx,7
        jb      ik_r
ik_d:   cmp     bx,5
        jne     ik_n
        mov     bx,cx
        mov     byte [inf_k8+bx],1
        mov     byte [si+PH_CLM],1
ik_n:   inc     cx
        cmp     cx,8
        jb      ik_l
        ret

inf_hd:                                 ; Plattencontroller auf 4A
        mov     byte [inf_hdc],0
        mov     al,0x55
        out     HD_COUNT,al
        mov     al,0xAA
        out     HD_SECTOR,al
        in      al,HD_COUNT
        cmp     al,0x55
        jne     ih_e
        in      al,HD_SECTOR
        cmp     al,0xAA
        jne     ih_e
        mov     byte [inf_hdc],1
        mov     al,HD_SDHBASE           ; Einheit 0, Kopf 0
        out     HD_SDH,al
        in      al,HD_CMDSTAT
        mov     [inf_hds],al
        mov     al,HD_DESEL
        out     HD_SDH,al
ih_e:   ret

inf_rtc:                                ; K803: laeuft? Zeit und Datum
        mov     byte [inf_rtcok],0
        xor     al,al
        out     RTC_LATCH,al
        call    rtc_sec
        mov     bl,al
        mov     ax,600                  ; 1,2 s
        call    wait_ms2
        cli
        call    rtc_sec
        mov     bh,al
        mov     al,bl
        call    bcd59
        jc      irt_e
        mov     al,bh
        call    bcd59
        jc      irt_e
        cmp     bl,bh
        je      irt_e
        mov     byte [inf_rtcok],1
        mov     [inf_t+2],bh            ; Sekunden
        in      al,0xCF                 ; Minuten (Gruppe 0, Adresse 3)
        mov     [inf_t+1],al
        mov     al,1
        out     RTC_LATCH,al
        in      al,0xCC                 ; Stunden (Adresse 4)
        mov     [inf_t],al
        in      al,0xCE                 ; Tag (Adresse 6)
        mov     [inf_t+3],al
        in      al,0xCF                 ; Monat (Adresse 7)
        mov     [inf_t+4],al
        xor     al,al
        out     RTC_LATCH,al
irt_e:   ret

inf_irq:                                ; Ruhestand der Leitungen (nur K235)
        in      al,P_SYSSTAT
        mov     [inf_13],al
        mov     byte [inf_irr],0
        cmp     word [j_card],s_k235
        jne     iq2_e
        mov     al,PIC_FWICW1           ; wie nach dem ROM: pegelgesteuert, gesperrt
        out     P_PICCMD,al
        mov     al,PIC_ICW2
        out     P_PICDATA,al
        mov     al,PIC_FWICW4
        out     P_PICDATA,al
        mov     al,0xFF
        out     P_PICDATA,al
        call    pt_dly
        call    iq_irr
        mov     [inf_irr],al
        in      al,P_SYSSTAT
        mov     [inf_13],al
iq2_e:  ret

;--- Seite 1: Uebersicht ----------------------------------------------------
inf_p1:
        mov     si,s_if_t1
        call    puts
        call    nl
        call    nl
        ; Firmware und Bildschirm
        mov     si,s_if_fw
        call    puts
        mov     si,[j_rom]
        call    puts
        mov     si,s_if_sep
        call    puts
        mov     si,[j_gfx]
        call    puts
        call    nl
        ; Prozessoren
        mov     si,s_if_cpu
        call    puts
        mov     si,s_z80
        call    puts
        mov     si,[j_cpu]
        call    puts
        mov     ax,[j_mhz]
        or      ax,ax
        jz      i1_nm
        call    space
        call    dec2c
        mov     si,s_mhz
        call    puts
i1_nm:  call    space
        mov     si,[j_fpu]
        call    puts
        call    nl
        mov     si,s_if_card
        call    puts
        mov     si,[j_card]
        call    inf_mod
        cmp     word [j_card],s_k235
        jne     i1_c1
        mov     si,s_if_pic
        call    puts
i1_c1:  call    nl
        ; Koprozessor
        mov     si,s_if_cop2
        call    puts
        cmp     byte [c_ran],1
        je      i1_cr
        mov     si,s_if_copn
        jmp     short i1_cp
i1_cr:  mov     si,s_if_copz
        cmp     byte [c_z80],0x5A
        jne     i1_cp
        mov     al,[c_res]
        mov     si,s_if_c68
        cmp     al,'6'
        je      i1_cp
        mov     si,s_if_cz8
        cmp     al,'Z'
        je      i1_cp
        mov     si,s_if_cop0
i1_cp:  call    puts
        call    nl
        ; Speicher
        mov     si,s_if_mem
        call    puts
        mov     al,[j_banks]            ; erst die Karte, dann die Groesse
        xor     di,di                   ; DI = Nachsatz
        mov     si,s_if_k200
        cmp     al,1
        je      i1_mk
        mov     si,s_if_k202
        cmp     al,3
        je      i1_mk
        mov     si,s_if_k208
        cmp     al,7
        je      i1_mk
        xor     si,si
        mov     di,s_if_knone
        or      al,al
        jz      i1_mk
        mov     di,s_if_k0
i1_mk:  or      si,si
        jz      i1_ms
        call    puts
i1_ms:  push    di
        mov     al,[j_banks]
        cbw
        inc     ax
        mov     cl,6
        shl     ax,cl
        call    dec16
        mov     si,s_kb
        call    puts
        pop     si
        or      si,si
        jz      i1_mz
        call    puts
i1_mz:  call    nl
        ; gemeinsames RAM K233
        mov     si,s_if_k3
        call    puts
        call    inf_k233ok
        jne     i1_k3n
        mov     si,s_if_k3y
        call    puts
        mov     si,s_if_k3w1
        cmp     byte [k_wt],0x3C
        je      i1_k3w
        mov     si,s_if_k3w0
        cmp     byte [k_wt],0xA5
        je      i1_k3w
        mov     si,s_if_k3wu
i1_k3w: call    puts
        jmp     i1_k3e
i1_k3n: mov     si,s_if_k30
        cmp     byte [k_ran],1
        je      i1_k3p
        mov     si,s_if_k3x
        call    puts
        mov     si,[inf_k3b]
        call    puts                    ; Modulnummer der Karte auf 1B (rot)
        mov     si,s_if_k3x2
i1_k3p: call    puts
i1_k3e: call    nl
        ; Tastatur
        mov     si,s_if_kbd
        call    puts
        mov     al,[r_kbst]
        test    al,KBD_LANG
        jz      i1_kn
        mov     al,[r_kbst+1]
        sub     al,KBD_LANGBAS
        cmp     al,8
        jae     i1_kn
        cbw
        mov     bx,ax
        shl     bx,1
        mov     si,[countries+bx]
        jmp     short i1_kp
i1_kn:  mov     si,s_unk
i1_kp:  call    puts
        call    nl
        ; Laufwerke
        mov     si,s_if_drv
        call    puts
        mov     si,s_if_a40
        cmp     byte [MB_Z80A],0
        je      i1_dp
        mov     si,s_if_a80
        cmp     byte [MB_Z80A],1
        je      i1_dp
        mov     si,s_unk
i1_dp:  call    puts
        call    nl
        ; Platte
        mov     si,s_if_hd
        call    puts
        mov     si,s_if_hd0
        cmp     byte [inf_hdc],1
        jne     i1_hp
        mov     si,s_if_hd1
        test    byte [inf_hds],0x40     ; READY von Einheit 0
        jz      i1_hp
        mov     si,s_if_hd2
i1_hp:  call    puts
        call    nl
        ; Uhr
        mov     si,s_if_rtc
        call    puts
        cmp     byte [inf_rtcok],1
        je      i1_rt
        mov     si,s_if_rtc0
        call    puts
        jmp     short i1_re
i1_rt:  mov     si,s_if_rtc1
        call    puts
        mov     al,[inf_t]
        call    hex8
        mov     al,':'
        call    putc
        mov     al,[inf_t+1]
        call    hex8
        mov     al,':'
        call    putc
        mov     al,[inf_t+2]
        call    hex8
        mov     si,s_if_rtc2
        call    puts
        mov     al,[inf_t+3]
        call    hex8
        mov     al,'.'
        call    putc
        mov     al,[inf_t+4]
        call    hex8
        mov     al,'.'
        call    putc
i1_re:  call    nl
        call    nl
        ; Steckkarten
        mov     si,s_if_mod
        call    puts
        call    nl
        xor     cx,cx
        mov     byte [inf_n],0
i1_ml:  call    inf_card                ; SI = Modulnummer, dahinter Text; oder 0
        or      si,si
        jz      i1_mn
        inc     byte [inf_n]
        push    si
        mov     si,s_ind
        call    puts
        pop     si
        call    puts                    ; Modulnummer (rot)
        mov     dl,IF_COLS
        call    inf_tab
        mov     al,A_NOTE
        call    setattr
        mov     bx,cx
        shl     bx,1
        mov     al,[pt_names+bx]
        call    putc
        mov     al,[pt_names+bx+1]
        call    putc
        cmp     byte [inf_grp],1        ; Gruppenkarte: beide Haelften
        jne     i1_m1
        mov     al,'/'
        call    putc
        mov     al,[pt_names+bx+2]
        call    putc
        mov     al,[pt_names+bx+3]
        call    putc
i1_m1:  mov     dl,IF_COLT
        call    inf_tab
        call    inf_next                ; SI = Text hinter der Modulnummer
        call    puts
        call    nl
i1_mn:  cmp     cx,3                    ; nach 1B: K233 (kein Echo, eigene Probe)
        jne     i1_mn2
        call    inf_k233ok
        jne     i1_mn2
        inc     byte [inf_n]
        mov     si,s_if_l233
        call    puts
        call    nl
i1_mn2: inc     cx
        cmp     cx,8
        jb      i1_ml
        cmp     byte [inf_hdc],1        ; Plattencontroller (fest auf 4A)
        jne     i1_mh
        inc     byte [inf_n]
        mov     si,s_if_lhd
        call    puts
        call    nl
i1_mh:  cmp     byte [inf_rtcok],1      ; Uhr K803 (fest auf 4B gemessen)
        jne     i1_m8
        inc     byte [inf_n]
        mov     si,s_if_l803
        call    puts
        call    nl
i1_m8:  cmp     byte [inf_n],0
        jne     i1_me
        mov     si,s_if_mod0
        call    puts
        call    nl
i1_me:  ret

inf_card:                               ; CX = Haelfte -> SI = Kartentext oder 0, [inf_grp]
        mov     byte [inf_grp],0
        mov     ax,cx
        mov     bl,PH_SIZE
        mul     bl
        add     ax,pt_res
        mov     di,ax
        mov     si,s_if_c210
        cmp     byte [di+PH_P2],2
        je      ic_g
        mov     si,s_if_c211
        cmp     byte [di+PH_SIO],2
        je      ic_g
        mov     si,s_if_c801
        cmp     byte [di+PH_SIO],3
        je      ic_e
        mov     si,s_if_c215
        cmp     byte [di+PH_K5],3
        je      ic_e
        mov     si,s_if_c216
        cmp     byte [di+PH_Z1],2
        je      ic_e
        cmp     byte [di+PH_Z2],2
        je      ic_e
        mov     bx,cx
        mov     si,s_if_c806
        cmp     byte [inf_k8+bx],1
        je      ic_e
        cmp     byte [di+PH_CLM],1      ; gehoert zur Haelfte darueber
        je      ic_0
        mov     si,s_if_cunk
        cmp     byte [di+PH_OCC1],2
        je      ic_e
        cmp     byte [di+PH_OCC2],2
        je      ic_e
ic_0:   xor     si,si
        ret
ic_g:   mov     byte [inf_grp],1
ic_e:   ret

;--- Seite 2: Steckplaetze und Interrupts -----------------------------------
inf_p2:
        mov     si,s_if_t2
        call    puts
        call    nl
        call    nl
        cmp     word [j_card],s_k235
        je      i2_k
        mov     si,s_if_nopic
        call    puts
        call    nl
        mov     si,s_if_nopic2
        call    puts
        call    nl
        ret
i2_k:   mov     si,s_if_br
        call    puts
        call    nl
        mov     si,s_if_rest
        call    puts
        mov     al,[inf_13]
        call    hex8
        mov     si,s_if_irr
        call    puts
        mov     al,[inf_irr]
        call    hex8
        call    nl
        call    nl
        ; je Platz: Leitung ruhig (Karte haelt sie) oder meldet dauernd
        mov     si,s_if_p2
        mov     al,[inf_13]
        and     al,0x40
        call    inf_line
        mov     si,s_if_p2a
        mov     al,[inf_irr]
        and     al,0x20
        call    inf_line
        mov     si,s_if_p3
        mov     al,[inf_irr]
        and     al,0x08
        call    inf_line
        mov     si,s_if_p4
        mov     al,[inf_irr]
        and     al,0x10
        call    inf_line
        mov     si,s_if_p5
        mov     al,[inf_irr]
        and     al,0x80
        call    inf_line
        call    nl
        ; gefundene Karten und ihre Leitung
        mov     si,s_if_ct
        call    puts
        call    nl
        xor     cx,cx
i2_l:   call    inf_card
        or      si,si
        jz      i2_n
        push    si
        mov     si,s_ind
        call    puts
        pop     si
        call    puts                    ; Modulnummer (rot)
        mov     dl,IF_COLS
        call    inf_tab
        mov     al,A_NOTE
        call    setattr
        mov     bx,cx
        shl     bx,1
        mov     al,[pt_names+bx]
        call    putc
        mov     al,[pt_names+bx+1]
        call    putc
        mov     dl,IF_COLT
        call    inf_tab
        call    inf_irqtxt
        call    puts
        call    nl
i2_n:   inc     cx
        cmp     cx,8
        jb      i2_l
        cmp     byte [inf_hdc],1        ; 4A vor 4B
        jne     i2_r
        mov     si,s_if_qhd
        call    puts
        call    nl
i2_r:   cmp     byte [inf_rtcok],1
        jne     i2_h
        mov     si,s_if_q803
        call    puts
        call    nl
i2_h:   call    nl
        mov     si,s_if_hint
        call    puts
        call    nl
        mov     si,s_if_hint2
        call    puts
        call    nl
        ret

IF_COLS equ     18                      ; Spalte der Haelfte in den Kartenlisten
IF_COLT equ     25                      ; Spalte des Textes

inf_next:                               ; SI hinter die naechste 0
        push    ax
in_l:   lodsb
        or      al,al
        jnz     in_l
        pop     ax
        ret

inf_mod:                                ; SI = Text; beginnt er mit K: Modulnummer rot
        cmp     byte [si],'K'
        jne     imd_p
        push    ax
        mov     al,A_MOD
        call    setattr
        call    puts
        mov     al,A_JUDGE
        call    setattr
        pop     ax
        ret
imd_p:  jmp     puts

inf_tab:                                ; mit Leerzeichen bis Spalte DL (auch in der Datei)
        cmp     [FW_CURX],dl
        jae     itb_e
        call    space
        jmp     short inf_tab
itb_e:  ret

inf_line:                               ; SI = Platztext, AL = Bit (0 = ruhig)
        push    ax
        call    puts
        pop     ax
        mov     si,s_if_quiet
        or      al,al
        jz      il_p
        mov     si,s_if_loud
il_p:   call    puts
        jmp     nl

inf_irqtxt:                             ; SI = Kartentext -> SI = Text mit Interrupt
        cmp     si,s_if_c210
        jne     it_1
        mov     si,s_if_q210
        ret
it_1:   cmp     si,s_if_c211
        jne     it_2
        mov     si,s_if_q211
        ret
it_2:   cmp     si,s_if_c801
        jne     it_3
        mov     si,s_if_q801
        ret
it_3:   cmp     si,s_if_c215
        jne     it_4
        mov     si,s_if_q215
        ret
it_4:   cmp     si,s_if_c806
        jne     it_5
        mov     si,s_if_q806
        ret
it_5:   cmp     si,s_if_c216
        jne     it_6
        mov     si,s_if_q216
        ret
it_6:   mov     si,s_if_qunk
        ret

;--- Sichern: beide Seiten als DMVINFO.TXT ----------------------------------
inf_save:
        call    cap_begin
        call    cls
        mov     byte [inf_pg],0
        call    inf_page
        call    cls
        mov     byte [inf_pg],1
        call    inf_page
        call    cap_end
        mov     byte [inf_pg],0
        mov     byte [rs_file],0
        mov     byte [rs_run],1         ; feste Datei, keine Laufnummer
        call    cls
        call    rs_write
        jnc     is_ok
        call    rs_report
        jmp     inf_show
is_ok:  call    cls
        mov     si,s_if_saved
        call    puts
        mov     ax,[rs_len]
        call    dec16
        mov     si,s_if_bytes
        call    puts
        call    nl
        call    nl
        mov     si,s_rs_key
        call    puts
        call    getkey
        jmp     inf_show

s_if_run  db    'DMVINFO ',VERSION,' - erkenne die Hardware ...',0
s_if_cop  db    2,'pruefe ',6,'K234',2,'/',6,'K238',2,' (bleibt das stehen, haengt der Koprozessor) ...',0
s_if_foot db    ' Taste: Seite 1/2   R: Rohwerte   D: sichern   W: neu messen   X: Ende',0
s_if_t1   db    4,'HARDWARE-UEBERSICHT',0
s_if_t2   db    4,'STECKPLAETZE UND INTERRUPTS',0
s_if_fw   db    2,'Firmware:      ',3,0
s_if_sep  db    ', ',0
s_if_cpu  db    2,'Prozessoren:   ',3,0
s_if_card db    2,'CPU-Karte:     ',3,0
s_if_pic  db    ' (mit 8259)',0
s_if_cop2 db    2,'Koprozessor:   ',3,0
s_if_copn db    'nicht pruefbar (nur mit ',6,'K235',3,')',0
s_if_copz db    'Z80 kam nicht zurueck - Ablauf pruefen',0
s_if_cop0 db    'keiner (',6,'K234',3,'/',6,'K238',3,' antwortet nicht)',0
s_if_c68  db    6,'K234',3,'  68008 antwortet',0
s_if_cz8  db    6,'K238',3,'  Z8001 antwortet',0
s_if_mem  db    2,'Speicher:      ',3,0
s_if_k200 db    6,'K200',3,'  ',0
s_if_k202 db    6,'K202',3,'  ',0
s_if_k208 db    6,'K208',3,'  ',0
s_if_k0   db    ' (keine bekannte Karte)',0
s_if_knone db   ' (keine Speicherkarte)',0
s_if_kbd  db    2,'Tastatur:      ',3,0
s_if_drv  db    2,'Laufwerk A:    ',3,0
s_if_a40  db    '40 Spuren (B: auf den Rohwertseiten, Seite 8, Taste B)',0
s_if_a80  db    '80 Spuren',0
s_if_hd   db    2,'Festplatte:    ',3,0
s_if_hd0  db    'kein Controller auf 4A',0
s_if_hd1  db    6,'K012/C3282',3,'  auf 4A, Einheit 0 nicht bereit',0
s_if_hd2  db    6,'K012/C3282',3,'  auf 4A, Einheit 0 bereit',0
s_if_rtc  db    2,'Uhr:           ',3,0
s_if_k3   db    2,'Gem. RAM:      ',3,0
s_if_k3y  db    6,'K233',3,'  16 KB auf C000h, ',0
s_if_k3w1 db    'Hauptplatine schreibt mit',0
s_if_k3w0 db    'nur in die Karte',0
s_if_k3wu db    'Schreiben unklar',0
s_if_k30  db    'keine ',6,'K233',3,0
s_if_k3x  db    'nicht pruefbar (',0
s_if_k3x2 db    3,' auf 1B)',0
s_if_l233 db    '  ',6,'K233',2,'            1B     ',3,'gemeinsames RAM (16 KB, C000h)',0
s_if_rtc0 db    'keine ',6,'K803',3,' (oder steht)',0
s_if_rtc1 db    6,'K803',3,'  laeuft, ',0
s_if_rtc2 db    ' am ',0
s_if_mod  db    2,'Steckkarten:',0
s_if_mod0 db    2,'  keine',0
s_if_slash db   '/',0
; Kartentexte: Modulnummer, 0, Beschreibung, 0 (inf_next springt weiter)
s_if_c210 db    6,'K210',0,3,'Centronics (8255)',0
s_if_c211 db    6,'K211/K212/K213',0,3,'RS-232 (2651)',0
s_if_c801 db    6,'K801',0,3,'RS-232 umschaltbar (2661)',0
s_if_c215 db    6,'K215',0,3,'RS-232 gepuffert (8741)',0
s_if_c216 db    6,'K216',0,3,'SCC (Z8530)',0
s_if_c806 db    6,'K806',0,3,'Maus (8741)',0
s_if_cunk db    6,'?',0,3,'belegt, Karte unbekannt',0
s_if_lhd  db    '  ',6,'K012/C3282',2,'      4A     ',3,'Plattencontroller (WD1001-Art)',0
s_if_l803 db    '  ',6,'K803',2,'            4B     ',3,'Uhr (MM58167)',0
s_if_nopic db   2,'Keine ',6,'K235',2,': ohne 8259 gibt es keine Steckplatz-Interrupts;',0
s_if_nopic2 db  2,'alle Geraete werden abgefragt.',0
s_if_br   db    2,'Brueckenstellung angenommen wie gemessen: S1 = Platz 5, S2 = Platz 2a.',0
s_if_rest db    2,'Ruhestand: 13h ',1,0
s_if_p2   db    2,'Platz 2  (13h Bit 6, ohne IR):  ',3,0
s_if_p2a  db    2,'Platz 2a (IR5, Festplatte):     ',3,0
s_if_p3   db    2,'Platz 3  (IR3):                 ',3,0
s_if_p4   db    2,'Platz 4  (IR4):                 ',3,0
s_if_p5   db    2,'Platz 5  (IR7):                 ',3,0
s_if_quiet db   'ruhig: eine Karte haelt die Leitung',0
s_if_loud db    'meldet dauernd: leer/',6,'K215',3,', sperren',0
s_if_irr  db    2,'   IRR ',1,0
s_if_ct   db    2,'Karten und ihr Interrupt:',0
s_if_q210 db    3,'eigene Leitung (PC3) - IR je Platz',0
s_if_q211 db    3,'eigene Leitung, nur bei Empfang (RxRDY)',0
s_if_q801 db    3,'eigene Leitung (TxRDY, RxRDY)',0
s_if_q215 db    3,'kein Interrupt (abfragen)',0
s_if_q806 db    3,'INT/ aller Plaetze = IR2',0
s_if_q216 db    3,'INT/ aller Plaetze = IR2 (nicht gemessen)',0
s_if_qunk db    3,'unbekannt',0
s_if_q803 db    '  ',6,'K803',2,'            4B     ',3,'INT/ aller Plaetze = IR2 (wenn freigegeben)',0
s_if_qhd  db    '  ',6,'K012/C3282',2,'      4A     ',3,'eigene Leitung (IR5 nur in Platz 2a)',0
s_if_hint db    2,'Welche Karte in welchem Platz steckt, zeigt R, Seite 6, Taste I.',0
s_if_hint2 db   2,'IR3/IR4/IR7 nur freigeben, wenn der Platz "ruhig" ist und ein Treiber da ist.',0
s_if_saved db   3,'DMVINFO.TXT gespeichert, ',1,0
s_if_bytes db   ' Bytes',0
inf_pg    db    0
inf_raw   db    0
inf_n     db    0
inf_grp   db    0
inf_k3b   dw    0                       ; Karte auf 1B, die die K233-Probe verhindert
inf_hdc   db    0
inf_hds   db    0
inf_rtcok db    0
inf_t     times 5 db 0
inf_13    db    0
inf_irr   db    0
inf_k8    times 8 db 0

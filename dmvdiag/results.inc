;=====================================================================
;  Ergebnisse auf die Diskette: automatisch je Seite und je Sondertest
;=====================================================================
; Beim Aufbau einer Seite schreibt putc jedes Zeichen zusaetzlich in einen
; Puffer (Kopfzeile und Seite, ohne Fusszeile; Zeilenende CR LF,
; Leerzeichen am Zeilenende gestrichen). Verlaesst man die Seite, kommt
; der Puffer unter dem festen Namen der Seite auf die Startdiskette in A:
; (01SYSTEM ... 10DEUTNG). Laeuft auf einer Seite ein Sondertest
; (K, S, I, M, T, B, P), wird die Seite danach mit dem Ergebnis neu
; aufgebaut; dieser Stand kommt zusaetzlich unter dem Namen des Tests
; auf die Diskette (z. B. 08MOTOR), bevor die Seite verlassen oder der
; naechste Test gestartet wird. Die Erweiterung ist die Laufnummer
; dieses Starts (L1, L2, ...): nach Umstecken und Neustart bleiben die
; Dateien des vorigen Laufs stehen. Unveraenderte Inhalte (Pruefsumme wie
; beim letzten Schreiben) werden nicht noch einmal geschrieben.
; Schlaegt das Schreiben fehl, erscheint der Grund einmal, danach ist das
; Sichern aus; Taste D schaltet es wieder ein und schreibt sofort.
;
; Nur auf die eigene Diskette: Der erste Verzeichnissektor muss als
; Datentraegername "DMVDIAG" tragen, und im Verzeichnis duerfen nur
; unsere Dateien stehen. Das Programm selbst liegt in den reservierten
; Sektoren. Dateien DMVDIAG*.TXT (0.12-0.14) und nn*.TXT (0.15-Vorab)
; werden dabei geloescht.
;
; Lage auf der Diskette (Bootsektor, PROGSECS Programmsektoren, 2 FATs
; zu 2 Sektoren, 7 Verzeichnissektoren, Daten; Cluster = 2 Sektoren):
RS_RES  equ     1+PROGSECS
RS_FAT  equ     RS_RES
RS_ROOT equ     RS_RES+4
RS_DATA equ     RS_RES+11

%ifdef INFO
RS_NF   equ     1                       ; DMVINFO: eine Datei DMVINFO.TXT
%else
RS_NF   equ     17                      ; 10 Seiten, 7 Sondertests
%endif
RS_DIRN equ     111                     ; Eintraege 1-111 (7 Verzeichnissektoren)
RS_TOT  equ     720                     ; 360-KB-Diskette
RS_NCL  equ     (RS_TOT-RS_DATA)/2      ; Cluster 2 .. RS_NCL+1
RS_RMAX equ     99                      ; Laufnummern 1-99
CAP_BUF equ     0xA000                  ; Textpuffer einer Seite, 4 KB
%ifdef INFO
CAP_MAX equ     0x2000                  ; zwei Seiten
%else
CAP_MAX equ     0x1000
%endif
FAT_BUF equ     0xD800                  ; FAT 1, 1 KB
SEC_BUF equ     0xDC00                  ; Verzeichnis, 7 Sektoren (bis E9FFh)

; Dateinummern der Sondertests (Seiten: 0-9 = Seitennummer)
RF_K233 equ     10
RF_PORT equ     11
RF_IRQ  equ     12
RF_MOT  equ     13
RF_STEP equ     14
RF_BRD  equ     15
RF_COP  equ     16

cap_put:                                ; AL = Zeichen (aus putc)
        push    di
        mov     di,[cap_ptr]
        cmp     al,0x88
        jne     cp_c
cp_s:   cmp     di,[cap_line]           ; Leerzeichen am Zeilenende weg
        jbe     cp_nl
        cmp     byte [di-1],' '
        jne     cp_nl
        dec     di
        jmp     short cp_s
cp_nl:  cmp     di,CAP_BUF+CAP_MAX-2
        jae     cp_e
        mov     word [di],0x0A0D
        add     di,2
        mov     [cap_line],di
        jmp     short cp_e
cp_c:   cmp     di,CAP_BUF+CAP_MAX-2
        jae     cp_e
        cmp     al,' '
        jb      cp_q
        cmp     al,0x7E
        jbe     cp_st
cp_q:   mov     al,'?'
cp_st:  mov     [di],al
        inc     di
cp_e:   mov     [cap_ptr],di
        pop     di
        ret

cap_begin:                              ; Seitenaufbau mitschreiben
        mov     word [cap_ptr],CAP_BUF
        mov     word [cap_line],CAP_BUF
        mov     byte [cap_on],1
        ret

cap_end:
        cmp     byte [cap_on],0
        je      ce_e
        mov     al,0x88                 ; letzte Zeile abschliessen
        call    cap_put
        mov     byte [cap_on],0
        mov     ax,[cap_ptr]
        sub     ax,CAP_BUF
        mov     [rs_len],ax
ce_e:   ret

;--- Ablauf im Seitenwechsel --------------------------------------------------
rs_leave:                               ; Seite wird verlassen
        call    rs_flush
        mov     al,[page]
        jmp     rs_auto

rs_flush:                               ; Ergebnis des letzten Tests sichern
        mov     al,[rs_test]
        cmp     al,0xFF
        je      rf_e
        mov     byte [rs_test],0xFF
        jmp     rs_auto
rf_e:   ret

rs_force:                               ; Taste D: wieder an, sofort schreiben
        mov     byte [rs_off],0
        mov     di,rs_have
        mov     cx,RS_NF
        xor     al,al
        rep     stosb
        jmp     rs_leave

rs_auto:                                ; AL = Dateinummer
        cmp     byte [rs_off],0
        jne     ra_e
        mov     [rs_file],al
        ; Pruefsumme des Puffers
        mov     si,CAP_BUF
        mov     cx,[rs_len]
        mov     dx,cx
        jcxz    ra_s
ra_l:   rol     dx,1
        xor     dl,[si]
        inc     si
        loop    ra_l
ra_s:   mov     bl,[rs_file]
        xor     bh,bh
        cmp     byte [rs_have+bx],0
        je      ra_w
        shl     bx,1
        cmp     [rs_sum+bx],dx
        je      ra_e                    ; unveraendert
ra_w:   push    dx
        call    rs_write
        pop     dx
        jc      ra_bad
        mov     bl,[rs_file]
        xor     bh,bh
        mov     byte [rs_have+bx],1
        shl     bx,1
        mov     [rs_sum+bx],dx
ra_e:   ret
ra_bad: mov     byte [rs_off],1
        jmp     rs_report

;--- eine Datei schreiben -----------------------------------------------------
; Laufnummer: beim ersten Schreiben nach dem Start die hoechste Nummer
; auf der Diskette + 1; alle Dateien dieses Starts tragen sie in der
; Erweiterung (05PORTS.L1, 05PORTS.L2, ...). Cluster werden aus der FAT
; vergeben: erst die Daten in freie Cluster, dann FAT, dann Verzeichnis;
; die alte Fassung derselben Datei wird erst danach freigegeben.
rs_write:                               ; rs_file, Puffer CAP_BUF/rs_len -> CF Fehler
        ; Hinweis in der Fusszeile
        mov     byte [FW_CURX],0
        mov     byte [FW_CURY],24
        mov     al,A_HEAD
        call    setattr
        mov     si,s_rs_now
        call    puts
        call    rs_pname
        mov     si,s_rs_now1
        call    puts
        call    pad80
        mov     al,A_VAL
        call    setattr
        mov     byte [rs_step],0

        ; 1. Laufwerk A: bereit, Kopf auf Spur 0
        call    fd_motor
        call    fdc_ready
        mov     al,0x07
        call    fdc_out
        xor     al,al
        call    fdc_out
        call    fd_cmdwait
        mov     byte [fx_dbl],0         ; 40-Spur-Diskette im 80-Spur-Werk?
        cmp     byte [MB_Z80A],1        ; das Z80-ROM hat es beim Start gesehen
        jne     rs_rd
        mov     byte [fx_dbl],1

        ; 2. Verzeichnis (7 Sektoren) und FAT 1 (2 Sektoren) lesen
rs_rd:  mov     byte [rs_step],1
        mov     ax,RS_ROOT
        mov     bx,SEC_BUF
        mov     cx,7
        mov     dl,0
rs_rr:  call    fd_xfer
        jc      rs_fail
        inc     ax
        add     bx,512
        loop    rs_rr
        mov     ax,RS_FAT
        mov     bx,FAT_BUF
        call    fd_xfer
        jc      rs_fail
        inc     ax
        add     bx,512
        call    fd_xfer
        jc      rs_fail
        mov     byte [rs_step],2
        mov     si,SEC_BUF
        mov     di,s_rs_lab
        mov     cx,7
        repe    cmpsb
        jne     rs_fail
        test    byte [SEC_BUF+11],0x08
        jz      rs_fail

        ; 3. Eintraege 1-111: nur frei, geloescht oder unsere Dateien
        mov     byte [rs_step],3
        mov     si,SEC_BUF+32
        mov     cx,RS_DIRN
        mov     word [rs_ent],0
        mov     word [rs_free],0
        mov     byte [rs_max],0
rs_dl:  mov     al,[si]
        or      al,al
        jz      rs_df
        cmp     al,0xE5
        je      rs_df
        call    rs_ours                 ; AL = Datei, AH = Lauf; FEh alt, FFh fremd
        cmp     al,0xFF
        jne     rs_dm
        jmp     rs_fail
rs_dm:  cmp     al,0xFE
        jne     rs_dm1
        mov     bx,[si+26]              ; Dateien aus 0.12-0.15 loeschen
        call    fat_free
        mov     byte [si],0xE5
        jmp     short rs_df
rs_dm1: cmp     ah,[rs_max]
        jbe     rs_dm2
        mov     [rs_max],ah
rs_dm2: cmp     byte [rs_run],0
        je      rs_dx
        cmp     al,[rs_file]            ; derselbe Name in diesem Lauf: ersetzen
        jne     rs_dx
        cmp     ah,[rs_run]
        jne     rs_dx
        mov     [rs_ent],si
        jmp     short rs_dx
rs_df:  cmp     word [rs_free],0        ; ersten freien Eintrag merken
        jne     rs_dx
        mov     [rs_free],si
rs_dx:  add     si,32
        loop    rs_dl
        cmp     byte [rs_run],0         ; erster Schreibvorgang dieses Starts
        jne     rs_hr
        mov     byte [rs_step],9
        mov     al,[rs_max]
        cmp     al,RS_RMAX
        jae     rs_fail
        inc     al
        mov     [rs_run],al
rs_hr:  mov     word [rs_oldc],0
        mov     ax,[rs_ent]
        or      ax,ax
        jz      rs_nf
        mov     bx,ax
        mov     ax,[bx+26]
        mov     [rs_oldc],ax
        jmp     short rs_ok1
rs_nf:  mov     ax,[rs_free]
        mov     byte [rs_step],8        ; Verzeichnis voll
        or      ax,ax
        jz      rs_fail
        mov     [rs_ent],ax
rs_ok1:
        ; 4. Daten in freie Cluster, Kette in der FAT
        mov     byte [rs_step],4
        mov     ax,[rs_len]
        add     ax,1023
        mov     cl,10
        shr     ax,cl
        mov     [rs_ncl],ax             ; Cluster (1 KB)
        mov     di,CAP_BUF              ; Rest des letzten Clusters mit 1Ah
        add     di,[rs_len]
        mov     cl,10
        shl     ax,cl
        mov     cx,ax
        add     cx,CAP_BUF
        sub     cx,di
        mov     al,0x1A
        rep     stosb
        mov     word [rs_clus],0
        mov     word [rs_prev],0
        mov     bx,CAP_BUF
        mov     cx,[rs_ncl]
        jcxz    rs_dir
rs_al:  push    bx
        mov     bx,2                    ; freien Cluster suchen
rs_as:  push    bx
        call    fat_get
        pop     bx
        or      ax,ax
        jz      rs_ag
        inc     bx
        cmp     bx,RS_NCL+2
        jb      rs_as
        pop     bx
        mov     byte [rs_step],10       ; Diskette voll
        jmp     rs_fail
rs_ag:  mov     ax,0x0FFF               ; neues Kettenende
        call    fat_set
        mov     ax,[rs_prev]
        or      ax,ax
        jnz     rs_ach
        mov     [rs_clus],bx            ; erster Cluster
        jmp     short rs_ac1
rs_ach: push    bx
        mov     ax,bx
        mov     bx,[rs_prev]
        call    fat_set                 ; Vorgaenger -> dieser
        pop     bx
rs_ac1: mov     [rs_prev],bx
        mov     ax,bx                   ; Sektor = RS_DATA + 2 (n - 2)
        sub     ax,2
        shl     ax,1
        add     ax,RS_DATA
        pop     bx                      ; Puffer
        mov     dl,1
        call    fd_xfer
        jc      rs_fail
        inc     ax
        add     bx,512
        call    fd_xfer
        jc      rs_fail
        add     bx,512
        loop    rs_al

        ; 5. Verzeichniseintrag im Puffer fuellen
rs_dir: mov     al,[rs_file]
        call    rs_nameof
        mov     di,[rs_ent]
        mov     cx,8
        rep     movsb
        call    rs_ext                  ; AX/DL = Erweiterung
        stosw
        mov     al,dl
        stosb
        mov     al,0x20                 ; Archiv
        stosb
        xor     ax,ax
        mov     cx,5
        rep     stosw
        mov     ax,FATTIME
        stosw
        mov     ax,FATDATE
        stosw
        mov     ax,[rs_clus]
        stosw
        mov     ax,[rs_len]
        stosw
        xor     ax,ax
        stosw
        mov     bx,[rs_oldc]            ; alte Fassung freigeben
        call    fat_free

        ; 6. FAT 1 und FAT 2, je 2 Sektoren
        mov     byte [rs_step],5
        xor     cx,cx
rs_ff:  mov     ax,RS_FAT
        add     ax,cx
        mov     bx,FAT_BUF
        test    cl,1
        jz      rs_f0
        add     bx,512
rs_f0:  mov     dl,1
        call    fd_xfer
        jc      rs_fail
        inc     cx
        cmp     cx,4
        jb      rs_ff

        ; 7. Verzeichnis zurueck
        mov     byte [rs_step],6
        mov     ax,RS_ROOT
        mov     bx,SEC_BUF
        mov     cx,7
        mov     dl,1
rs_rw:  call    fd_xfer
        jc      rs_fail
        inc     ax
        add     bx,512
        loop    rs_rw
        mov     byte [rs_step],7
        clc
        ret
rs_fail:
        stc
        ret

rs_report:                              ; Fehlerbild nach misslungenem Schreiben
        call    cls
        mov     si,s_rs_go
        call    puts
        call    nl
        mov     si,s_rs_bad
        call    puts
        call    rs_pname
        mov     si,s_rs_bad1
        call    puts
        mov     al,[rs_step]
        xor     ah,ah
        call    dec16
        call    nl
        mov     si,s_rs_why
        call    puts
        mov     al,[rs_step]
        xor     ah,ah
        shl     ax,1
        mov     bx,ax
        mov     si,[rs_whytab+bx]
        call    puts
        call    nl
        mov     si,s_rs_res
        call    puts
        mov     si,fx_res
        mov     cx,7
rs_rl:  lodsb
        call    hex8
        call    space
        loop    rs_rl
        call    nl
        mov     si,s_rs_wp
        call    puts
        call    nl
        call    nl
        mov     si,s_rs_off
        call    puts
        call    nl
        call    nl
        mov     si,s_rs_key
        call    puts
        call    getkey
        ret

rs_nameof:                              ; AL = Dateinummer -> SI = 8 Zeichen
        push    ax
        mov     ah,8
        mul     ah
        add     ax,rs_names
        mov     si,ax
        pop     ax
        ret

rs_ext:                                 ; rs_run -> AL AH DL = 'L', Ziffer(n)
%ifdef INFO
        mov     ax,'TX'                 ; DMVINFO: immer .TXT
        mov     dl,'T'
        ret
%endif
        push    bx
        mov     al,[rs_run]
        xor     ah,ah
        mov     bl,10
        div     bl                      ; AL = Zehner, AH = Einer
        mov     dl,' '
        or      al,al
        jz      re_1
        mov     dl,ah
        add     dl,'0'
        mov     ah,al
re_1:   add     ah,'0'
        mov     al,'L'
        pop     bx
        ret

rs_pname:                               ; Name.Erweiterung der aktuellen Datei ausgeben
        push    ax
        push    cx
        push    dx
        push    si
        mov     al,[rs_file]
        call    rs_nameof
        mov     cx,8
rp_n:   lodsb
        cmp     al,' '
        je      rp_n1
        call    putc
rp_n1:  loop    rp_n
        mov     al,'.'
        call    putc
        cmp     byte [rs_run],0
        jne     rp_r
        mov     al,'L'
        call    putc
        mov     al,'?'
        call    putc
        jmp     short rp_e
rp_r:   call    rs_ext
        push    ax
        call    putc
        pop     ax
        mov     al,ah
        call    putc
        cmp     dl,' '
        je      rp_e
        mov     al,dl
        call    putc
rp_e:   pop     si
        pop     dx
        pop     cx
        pop     ax
        ret

rs_ours:                                ; SI = Eintrag -> AL = Datei, AH = Lauf;
        push    bx                      ; AL = FEh alt (0.12-0.15), FFh fremd
        push    cx
        push    si
        push    di
        test    byte [si+11],0x18       ; Verzeichnis / Datentraegername: fremd
        jnz     ro_no
        xor     bl,bl
ro_l:   push    si
        mov     al,bl
        push    si
        call    rs_nameof
        mov     di,si
        pop     si
        mov     cx,8
        repe    cmpsb
        pop     si
        je      ro_got
        inc     bl
        cmp     bl,RS_NF
        jb      ro_l
        push    si
        mov     di,s_rs_lab             ; DMVDIAG.TXT / DMVDIAGn.TXT aus 0.12-0.14
        mov     cx,7
        repe    cmpsb
        pop     si
        jne     ro_no
        jmp     short ro_old
ro_got:
%ifdef INFO
        push    si                      ; DMVINFO.TXT ist unsere Datei, Lauf 1
        add     si,8
        mov     di,s_rs_ext
        mov     cx,3
        repe    cmpsb
        pop     si
        jne     ro_no
        mov     ax,0x0100
        jmp     short ro_e
%endif
        cmp     byte [si+8],'L'         ; Erweiterung L1-L99, sonst TXT (alt)
        jne     ro_old
        mov     al,[si+9]
        sub     al,'0'
        cmp     al,9
        ja      ro_no
        mov     ah,al
        mov     al,[si+10]
        cmp     al,' '
        je      ro_1
        sub     al,'0'
        cmp     al,9
        ja      ro_no
        mov     cl,al                   ; Einer
        mov     al,ah
        mov     ch,10
        mul     ch                      ; AX = Zehner * 10
        add     al,cl
        mov     ah,al
ro_1:   or      ah,ah
        jz      ro_no
        mov     al,bl
        jmp     short ro_e
ro_old: push    si
        add     si,8
        mov     di,s_rs_ext
        mov     cx,3
        repe    cmpsb
        pop     si
        jne     ro_no
        mov     al,0xFE
        jmp     short ro_e
ro_no:  mov     al,0xFF
ro_e:   pop     di
        pop     si
        pop     cx
        pop     bx
        ret

fat_get:                                ; FAT12: AX := Eintrag BX
        push    bx
        push    cx
        mov     cx,bx
        shr     cx,1
        add     cx,bx
        xchg    bx,cx                   ; BX = n * 3 / 2, CX = n
        mov     ax,[FAT_BUF+bx]
        test    cl,1
        jz      fg_e
        mov     cl,4
        shr     ax,cl
fg_e:   and     ax,0x0FFF
        pop     cx
        pop     bx
        ret

fat_free:                               ; Kette ab Cluster BX freigeben (0 = keine)
        push    ax
        push    bx
        push    cx
        mov     cx,RS_NCL               ; Schutz gegen Ringe
ff_l:   cmp     bx,2
        jb      ff_e
        cmp     bx,RS_NCL+2
        jae     ff_e
        call    fat_get
        push    ax
        xor     ax,ax
        call    fat_set
        pop     bx
        loop    ff_l
ff_e:   pop     cx
        pop     bx
        pop     ax
        ret

fat_set:                                ; FAT12: Eintrag BX := AX
        push    bx
        push    cx
        push    dx
        mov     dx,bx
        mov     cx,bx
        shr     cx,1
        add     bx,cx                   ; BX = n * 3 / 2
        add     bx,FAT_BUF
        test    dl,1
        jnz     fs_odd
        mov     [bx],al
        mov     cl,[bx+1]
        and     cl,0xF0
        or      cl,ah
        mov     [bx+1],cl
        jmp     short fs_e
fs_odd: mov     cx,ax
        shl     cx,1
        shl     cx,1
        shl     cx,1
        shl     cx,1                    ; v << 4
        mov     al,[bx]
        and     al,0x0F
        or      al,cl
        mov     [bx],al
        mov     [bx+1],ch
fs_e:   pop     dx
        pop     cx
        pop     bx
        ret

;--- ein Sektor von/nach A: -------------------------------------------------
; AX = logischer Sektor, BX = Puffer (Bank 0), DL = 0 lesen / 1 schreiben
; CF = Fehler; fx_res = Ergebnisbytes des 8272. Ablauf wie der Bootsektor
; (der auf echter Hardware laeuft): SEEK, DMA Kanal 3, READ/WRITE DATA,
; auf RQM warten, TC. Bei "Sektor nicht gefunden" wird der Doppelschritt
; umgeschaltet (40-Spur-Diskette im 80-Spur-Werk).
fd_xfer:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     [fx_dir],dl
        mov     byte [fx_try],4
fx_again:
        push    ax
        xor     dx,dx
        mov     cx,9
        div     cx
        inc     dl
        mov     [fx_s],dl
        mov     dl,al
        and     dl,1
        mov     [fx_h],dl
        shr     ax,1
        mov     [fx_c],al
        ; SEEK
        call    fdc_ready
        mov     al,0x0F
        call    fdc_out
        mov     al,[fx_h]
        shl     al,1
        shl     al,1
        call    fdc_out
        mov     al,[fx_c]
        cmp     byte [fx_dbl],0
        je      fx_sk
        add     al,al
fx_sk:  call    fdc_out
        call    fd_cmdwait
        ; DMA Kanal 3
        mov     al,0x07
        out     0x2A,al
        out     0x2C,al                 ; Byte-Zeiger zuruecksetzen
        mov     al,0x47                 ; Speicher <- FDC
        cmp     byte [fx_dir],0
        je      fx_dm
        mov     al,0x4B                 ; Speicher -> FDC
fx_dm:  out     0x2B,al
        mov     ax,bx
        out     0x26,al
        mov     al,ah
        out     0x26,al
        mov     al,0xFF
        out     0x27,al
        mov     al,0x01
        out     0x27,al
        mov     al,0x03
        out     0x2A,al
        ; READ DATA C6h / WRITE DATA C5h (MT, MFM)
        call    fdc_ready
        mov     al,0xC6
        cmp     byte [fx_dir],0
        je      fx_cm
        mov     al,0xC5
fx_cm:  call    fdc_out
        mov     al,[fx_h]
        shl     al,1
        shl     al,1
        call    fdc_out
        mov     al,[fx_c]
        call    fdc_out
        mov     al,[fx_h]
        call    fdc_out
        mov     al,[fx_s]
        call    fdc_out
        mov     al,2
        call    fdc_out
        mov     al,[fx_s]               ; EOT = dieser Sektor
        call    fdc_out
        mov     al,0x1B
        call    fdc_out
        mov     al,0xFF
        call    fdc_out
        call    tk_start
fx_w:   in      al,P_FDCSTAT
        test    al,FDC_RQM
        jnz     fx_done
        mov     ax,500                  ; 1 s
        call    tk_over_ms2
        jnc     fx_w
        mov     al,0x07
        out     0x2A,al
        mov     byte [fx_res],0xFF
        pop     ax
        jmp     short fx_bad
fx_done:
        out     P_TC,al
        mov     al,0x07
        out     0x2A,al
        mov     di,fx_res
        mov     cx,7
fx_rl:  call    fdc_in
        mov     [di],al
        inc     di
        loop    fx_rl
        pop     ax
        test    byte [fx_res],0xC0
        jz      fx_ok
        test    byte [fx_res+1],0x05    ; ST1: kein Sektor / keine Adressmarke
        jz      fx_bad                  ; sonst (z. B. schreibgeschuetzt): Schluss
        xor     byte [fx_dbl],1
        dec     byte [fx_try]
        jnz     fx_again
fx_bad: stc
        jmp     short fx_e
fx_ok:  clc
fx_e:   pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

s_rs_go db      4,'ERGEBNISSE AUF DIE DISKETTE (A:)',0
s_rs_now db     ' sichere ',0
s_rs_now1 db    ' auf A: ...',0
s_rs_bad db     3,'nicht gespeichert: ',1,0
s_rs_bad1 db    ', Schritt ',0
s_rs_why db     2,'  ',0
s_rs_res db     2,'  8272 ST0 ST1 ST2 C H R N: ',1,0
s_rs_wp db      2,'  (ST1 Bit 1 = schreibgeschuetzt; ST0 C0 + ST1 05 = Sektor nicht gefunden)',0
s_rs_off db     3,'Automatisches Sichern ist jetzt aus. D schaltet es wieder ein.',0
s_rs_key db     2,'beliebige Taste: weiter',0
%ifdef INFO
s_rs_lab db     'DMVINFO'
%else
s_rs_lab db     'DMVDIAG'
%endif
s_rs_ext db     'TXT'
%ifdef INFO
rs_names db     'DMVINFO '
%else
rs_names db     '01SYSTEM','02CPU   ','03SPEICH','04MODULE','05PORTS '
        db      '06IRQ   ','07DISK  ','08LAUFW ','09COPRO ','10DEUTNG'
        db      '03K233  ','05PROBEN','06IRQTST','08MOTOR ','08SCHRIT'
        db      '08BSPUR ','09COPROT'
%endif
rs_whytab dw    s_rw0, s_rw1, s_rw2, s_rw3, s_rw4, s_rw5, s_rw6, s_rw7, s_rw8, s_rw9, s_rw10
s_rw0   db      'Laufwerk',0
s_rw1   db      'Verzeichnis nicht lesbar',0
s_rw2   db      'keine DMVDIAG-Diskette in A: (Datentraegername)',0
s_rw3   db      'auf der Diskette stehen andere Dateien - nichts geschrieben',0
s_rw4   db      'Daten schreiben',0
s_rw5   db      'FAT schreiben',0
s_rw6   db      'Verzeichnis schreiben',0
s_rw7   db      '-',0
s_rw8   db      'Verzeichnis voll',0
s_rw9   db      '99 Laeufe auf der Diskette - Dateien woanders sichern und loeschen',0
s_rw10  db      'Diskette voll',0
cap_on  db      0
cap_ptr dw      0
cap_line dw     0
rs_len  dw      0
rs_ncl  dw      0
rs_prev dw      0
rs_oldc dw      0
rs_max  db      0
rs_run  db      0                       ; Laufnummer dieses Starts, 0 = noch keine
rs_step db      0
rs_file db      0
rs_test db      0xFF
rs_off  db      0
rs_ent  dw      0
rs_free dw      0
rs_clus dw      0
rs_have times RS_NF db 0
rs_sum  times RS_NF dw 0
fx_dir  db      0
fx_try  db      0
fx_dbl  db      0
fx_c    db      0
fx_h    db      0
fx_s    db      0
fx_res  times 7 db 0

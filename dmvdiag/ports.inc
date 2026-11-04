;=====================================================================
;  Seite "Schnittstellen": aktive Bausteinproben auf Taste S
;=====================================================================
; Jede Haelfte einer IFSEL-Gruppe (x0-x7) wird der Reihe nach geprueft;
; was eine Probe sicher erkennt, sperrt die Haelfte (oder die ganze
; Gruppe) fuer die folgenden Proben.
;
;   1. belegt?   x0-x3 je dreimal lesen (Bus vorher 00/FF/5A). Mindestens
;                ein Anschluss stabil und nicht nur FF = belegt. x4-x7
;                werden nur gelesen, wenn x0-x3 frei sind oder dort kein
;                2651 sitzt: beim 2651 ist ein Lesen auf x4-x7 ein
;                Schreiben (BA2 = R/W).
;   2. K215      nur auf den Basen aus V1K215 (70 78 30 38 B0 B8), nur
;                wenn x1 dreimal gleich 00 oder 08 liest (8741: Bit 3 = F1
;                zeigt A0 des letzten Schreibens). Befehl 0Bh (Echo) an x1, 55h
;                an x0, Antwort an x0; ebenso AAh. Auf einem 2651 sind
;                Schreibzugriffe auf x0/x1 wirkungslos (BA2 = 0: Lesen).
;   3. 2651/2661 Befehlsregister: x3 lesen, 23h und 03h ueber x7
;                schreiben und ueber x3 zuruecklesen, alter Wert zurueck
;                (NCRs Verfahren, RS232D6). Antwortet xB dasselbe, ist die
;                Gruppe gespiegelt: K211/K212/K213. Sonst nur diese
;                Haelfte: K801. 23h/03h sind fuer einen 8255 auf
;                derselben Lage BSR-Befehle "PC1 setzen" (INIT aus).
;   4. 8255      nur IFSEL 0: Steuerwort AAh wie NCRs Treiber, PC0-PC2
;                wie vorher, dann INTE A (PC6) setzen und loeschen; das
;                Statusbit 6 von Port C muss folgen. Port A wird nicht
;                beschrieben (das waere ein Zeichen an den Drucker).
;   5. Z8530     je Viertel x0-x3 / x4-x7: WR12 (Zeitkonstante) mit 55h
;                und AAh schreiben, RR12 lesen, alter Wert zurueck, wie
;                V1K216. Nicht auf 2A (30h-37h): dort sitzt die K806, und
;                ihr 8741 bekaeme Datenbytes.
; IFSEL 4 (Platte, K803) wird nicht beschrieben.
; Jeder Lesewert, der etwas beweisen soll, wird gelesen, nachdem ein
; anderer Wert auf dem Bus lag (pt_rdf, BL = Fuellwert).

PH_SIZE equ     24                      ; Ergebnis je Haelfte
PH_OCC1 equ     0                       ; x0-x3: 0 nicht gelesen, 1 frei, 2 belegt
PH_OCC2 equ     1                       ; x4-x7 ebenso
PH_K5   equ     2                       ; K215: 0 nicht, 1 Vorpruefung nein, 2 nein, 3 ja
PH_K5S  equ     3                       ; Status x1
PH_K5E1 equ     4
PH_K5E2 equ     5
PH_SIO  equ     6                       ; 2651: 0 nicht, 1 nein, 2 ja gespiegelt, 3 ja einzeln
PH_SC   equ     7                       ; x3 vorher
PH_S1   equ     8                       ; x3 nach 23h
PH_S2   equ     9                       ; x3 nach 03h
PH_M1   equ     10                      ; xB nach 23h
PH_M2   equ     11                      ; xB nach 03h
PH_Z1   equ     12                      ; SCC x0-x3: 0 nicht, 1 nein, 2 ja
PH_Z1O  equ     13
PH_Z1A  equ     14
PH_Z1B  equ     15
PH_Z2   equ     16                      ; SCC x4-x7
PH_Z2O  equ     17
PH_Z2A  equ     18
PH_Z2B  equ     19
PH_CLM  equ     20                      ; 1 = Haelfte vergeben
PH_P2   equ     21                      ; 8255 (nur 0A): 0 nicht, 1 nein, 2 ja
PH_PC   equ     22                      ; Port C vorher
PH_PI   equ     23                      ; Port C nach INTE A an (hi) / aus (lo): Bit 6 je

pt_rdf:                                 ; DX = Anschluss, BL = Fuellwert -> AL
        mov     [buscell],bl
        in      al,dx
        ret

pt_out:                                 ; DX = Anschluss, AL = Wert
        out     dx,al
pt_dly: push    cx                      ; Erholzeit (Z8530: 6 PCLK)
        mov     cx,4
pd_l:   loop    pd_l
        pop     cx
        ret

pt_occ:                                 ; DL = Basis eines Viertels -> AL 1 frei / 2 belegt
        push    bx
        push    cx
        push    dx
        mov     cx,4
        xor     dh,dh
po_l:   push    dx
        call    in3
        pop     dx
        cmp     al,ah
        jne     po_n
        cmp     al,[in3c]
        jne     po_n
        cmp     al,0xFF
        je      po_n
        cmp     al,dl                   ; unteres Adressbyte = Echo des leeren Busses
        jne     po_y
po_n:   inc     dl
        loop    po_l
        mov     al,1
        jmp     short po_e
po_y:   mov     al,2
po_e:   pop     dx
        pop     cx
        pop     bx
        ret

pt_wait:                                ; DX = Statusanschluss, AH = Maske, AL = Sollwert
        push    cx                      ; CF = Zeit um (20 ms)
        mov     cl,al
        call    tk_start
pw_l:   in      al,dx
        and     al,ah
        cmp     al,cl
        je      pw_ok
        mov     ax,10
        call    tk_over_ms2
        mov     ah,[pw_mask]
        jnc     pw_l
        stc
        pop     cx
        ret
pw_ok:  clc
        pop     cx
        ret

pt_k215:                                ; DI = Ergebnis, DL = Basis
        mov     bp,k5_bases
pk_b:   mov     al,[bp]
        or      al,al
        jz      pk_x                    ; keine K215-Lage
        inc     bp
        cmp     al,dl
        jne     pk_b
        xor     dh,dh
        inc     dx                      ; Status x1, dreimal 00?
        push    dx
        call    in3
        pop     dx
        mov     [di+PH_K5S],al
        mov     byte [di+PH_K5],1
        test    al,0xF7                 ; Bit 3 = F1 des 8741 (A0 des letzten Schreibens)
        jnz     pk_x
        cmp     al,ah
        jne     pk_x
        cmp     al,[in3c]
        jne     pk_x
        mov     byte [di+PH_K5],2
        mov     al,0x55
        call    pk_echo
        mov     [di+PH_K5E1],al
        mov     al,0xAA
        call    pk_echo
        mov     [di+PH_K5E2],al
        cmp     byte [di+PH_K5E1],0x55
        jne     pk_x
        cmp     byte [di+PH_K5E2],0xAA
        jne     pk_x
        mov     byte [di+PH_K5],3
        mov     byte [di+PH_CLM],1
pk_x:   ret

pk_echo:                                ; DX = Status x1, AL = Wert -> AL Antwort (Fuellwert = NOT AL)
        push    ax
        mov     byte [pw_mask],2        ; Eingang frei?
        mov     ax,0x0200
        call    pt_wait
        mov     al,0x0B                 ; Echo
        call    pt_out
        mov     byte [pw_mask],2
        mov     ax,0x0200
        call    pt_wait
        pop     ax
        dec     dx
        push    ax
        call    pt_out                  ; Datenbyte an x0
        inc     dx
        mov     byte [pw_mask],1        ; Ausgang voll?
        mov     ax,0x0101
        call    pt_wait
        pop     ax
        dec     dx
        mov     bl,al
        not     bl
        call    pt_rdf
        inc     dx
        ret

pt_2651:                                ; DI = Ergebnis, DL = Basis, CL = Haelfte (0 = A)
        xor     dh,dh
        mov     byte [di+PH_SIO],1
        add     dx,3
        mov     bl,0x5A
        call    pt_rdf
        mov     [di+PH_SC],al
        add     dx,4                    ; x7
        mov     al,0x23
        call    pt_out
        sub     dx,4
        mov     bl,0xDC
        call    pt_rdf
        mov     [di+PH_S1],al
        add     dx,8                    ; xB
        call    pt_rdf
        mov     [di+PH_M1],al
        sub     dx,4                    ; x7
        mov     al,0x03
        call    pt_out
        sub     dx,4
        mov     bl,0xFC
        call    pt_rdf
        mov     [di+PH_S2],al
        add     dx,8
        call    pt_rdf
        mov     [di+PH_M2],al
        sub     dx,8                    ; x3
        cmp     byte [di+PH_S1],0x23
        jne     ps_x
        cmp     byte [di+PH_S2],0x03
        jne     ps_x
        add     dx,4
        mov     al,[di+PH_SC]           ; alten Wert zurueck
        call    pt_out
        mov     byte [di+PH_SIO],3
        mov     byte [di+PH_CLM],1
        or      cl,cl                   ; nur von A aus nach B schauen
        jnz     ps_x
        cmp     byte [di+PH_M1],0x23
        jne     ps_x
        cmp     byte [di+PH_M2],0x03
        jne     ps_x
        mov     byte [di+PH_SIO],2
        mov     byte [di+PH_SIZE+PH_CLM],1      ; B gehoert dazu
ps_x:   ret

pt_8255:                                ; DI = Ergebnis 0A
        mov     byte [di+PH_P2],1
        mov     dx,IF_0A+2
        mov     bl,0x5A
        call    pt_rdf
        mov     [di+PH_PC],al
        mov     ah,al
        inc     dx                      ; Steuerwort
        mov     al,0xAA
        call    pt_out
        xor     cx,cx                   ; PC0-PC2 wie vorher
p8_l:   mov     al,cl
        shl     al,1
        test    ah,1
        jz      p8_0
        or      al,1
p8_0:   call    pt_out
        shr     ah,1
        inc     cx
        cmp     cx,3
        jb      p8_l
        mov     al,0x0D                 ; PC6 setzen: INTE A an
        call    pt_out
        dec     dx
        mov     bl,0x00
        call    pt_rdf
        and     al,0x40
        mov     ah,al
        inc     dx
        mov     al,0x0C                 ; INTE A aus
        call    pt_out
        dec     dx
        mov     bl,0x40
        call    pt_rdf
        and     al,0x40
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
        or      al,ah                   ; 40h = an gesehen, 04h = aus nicht geloescht
        mov     [di+PH_PI],al
        cmp     al,0x40
        jne     p8_x
        mov     byte [di+PH_P2],2
        mov     byte [di+PH_CLM],1
        mov     byte [di+PH_SIZE+PH_CLM],1
p8_x:   ret

pt_scc:                                 ; DI = Ergebnis + Versatz (PH_Z1/PH_Z2), DL = Viertel
        xor     dh,dh
        mov     byte [di],1
        in      al,dx                   ; Zeiger auf 0 zurueck
        call    pt_dly
        in      al,dx
        call    pt_dly
        mov     al,0x0C
        call    pt_out
        mov     bl,0x00
        call    pt_rdf
        mov     [di+1],al
        call    pt_dly
        mov     al,0x55
        mov     bl,0xAA
        call    pz_wr
        mov     [di+2],al
        mov     al,0xAA
        mov     bl,0x55
        call    pz_wr
        mov     [di+3],al
        cmp     byte [di+2],0x55
        jne     pz_x
        cmp     byte [di+3],0xAA
        jne     pz_x
        mov     al,[di+1]
        mov     bl,0
        call    pz_wr
        mov     byte [di],2
pz_x:   ret

pz_wr:                                  ; WR12 := AL, RR12 lesen (Fuellwert BL) -> AL
        push    ax
        mov     al,0x0C
        call    pt_out
        pop     ax
        call    pt_out
        mov     al,0x0C
        call    pt_out
        call    pt_rdf
        call    pt_dly
        ret

ports_run:
        cli
        mov     di,pt_res
        mov     cx,PH_SIZE*8
        xor     al,al
        rep     stosb
        mov     byte [pt_ran],1
        xor     cx,cx                   ; CX = Haelfte 0-7
pr_l:   mov     ax,cx
        mov     bl,PH_SIZE
        mul     bl
        add     ax,pt_res
        mov     di,ax
        mov     bx,cx
        mov     dl,[pt_bases+bx]        ; Basis der Haelfte
        cmp     byte [di+PH_CLM],0
        jne     pr_n
        call    pt_occ
        mov     [di+PH_OCC1],al
        cmp     al,2
        jne     pr_a2
        push    cx
        push    dx
        cmp     cx,2                    ; IFSEL 0: keine K215 (Druckerplatz)
        jb      pr_nk
        call    pt_k215
pr_nk:  pop     dx
        pop     cx
        cmp     byte [di+PH_CLM],0
        jne     pr_n
        push    cx
        push    dx
        cmp     dl,IF_2A                ; 2A: nur K215
        je      pr_ns
        and     cl,1
        call    pt_2651
pr_ns:  pop     dx
        pop     cx
        cmp     byte [di+PH_CLM],0
        jne     pr_n
        or      cx,cx                   ; 8255 nur auf 0A
        jnz     pr_z1
        push    cx
        push    dx
        call    pt_8255
        pop     dx
        pop     cx
        cmp     byte [di+PH_CLM],0
        jne     pr_n
pr_z1:  cmp     dl,IF_2A
        je      pr_a2
        push    cx
        push    dx
        push    di
        add     di,PH_Z1
        call    pt_scc
        pop     di
        pop     dx
        pop     cx
pr_a2:  cmp     byte [di+PH_CLM],0      ; x4-x7 nur, wenn hier kein 2651 sitzt
        jne     pr_n
        cmp     dl,IF_2A
        je      pr_n
        add     dl,4
        call    pt_occ
        mov     [di+PH_OCC2],al
        cmp     al,2
        jne     pr_n
        push    cx
        push    di
        add     di,PH_Z2
        call    pt_scc
        pop     di
        pop     cx
pr_n:   inc     cx
        cmp     cx,8
        jb      pr_l
        ret

;--- Anzeige ---------------------------------------------------------------
pg_ports:
        mov     si,s_pp
        call    puts
        call    nl
        cmp     byte [pt_ran],1
        je      pp_r
        mov     si,s_pp0
        call    puts
        call    nl
        mov     si,s_pp1
        call    puts
        call    nl
        mov     si,s_pp2
        call    puts
        call    nl
        mov     si,s_pp3
        call    puts
        call    nl
        ret
pp_r:   mov     si,s_pph
        call    puts
        call    nl
        xor     cx,cx
pp_l:   mov     ax,cx
        mov     bl,PH_SIZE
        mul     bl
        add     ax,pt_res
        mov     di,ax
        call    pp_half
        inc     cx
        cmp     cx,8
        jb      pp_l
        mov     si,s_pp4
        call    puts
        call    nl
        ret

pp_half:                                ; CX = Haelfte, DI = Ergebnis
        push    cx
        mov     bx,cx
        shl     bx,1
        mov     si,pt_names
        add     si,bx
        lodsb
        call    putc
        lodsb
        call    putc
        call    space
        mov     bx,cx
        mov     al,[pt_bases+bx]
        call    hex8
        mov     al,'h'
        call    putc
        call    space
        ; belegt?
        mov     al,[di+PH_OCC1]
        call    pp_occ
        mov     al,[di+PH_OCC2]
        call    pp_occ
        ; K215
        cmp     byte [di+PH_K5],0
        je      ph_s
        mov     si,s_ppk
        call    puts
        mov     al,[di+PH_K5S]
        call    hex8
        cmp     byte [di+PH_K5],1
        je      ph_s
        mov     al,':'
        call    putc
        mov     al,[di+PH_K5E1]
        call    hex8
        mov     al,'/'
        call    putc
        mov     al,[di+PH_K5E2]
        call    hex8
ph_s:   cmp     byte [di+PH_SIO],0
        je      ph_p
        mov     si,s_pps
        call    puts
        mov     al,[di+PH_SC]
        call    hex8
        mov     al,':'
        call    putc
        mov     al,[di+PH_S1]
        call    hex8
        mov     al,'/'
        call    putc
        mov     al,[di+PH_S2]
        call    hex8
        mov     si,s_ppm
        call    puts
        mov     al,[di+PH_M1]
        call    hex8
        mov     al,'/'
        call    putc
        mov     al,[di+PH_M2]
        call    hex8
ph_p:   cmp     byte [di+PH_P2],0
        je      ph_z
        mov     si,s_ppp
        call    puts
        mov     al,[di+PH_PC]
        call    hex8
        mov     al,':'
        call    putc
        mov     al,[di+PH_PI]
        call    hex8
ph_z:   push    di
        add     di,PH_Z1
        mov     al,0
        call    pp_scc
        pop     di
        push    di
        add     di,PH_Z2
        mov     al,4
        call    pp_scc
        pop     di
        call    nl
        ; Deutung
        mov     si,s_cpj
        call    puts
        mov     si,s_ppj_k5
        cmp     byte [di+PH_K5],3
        je      ph_j
        mov     si,s_ppj_ms
        cmp     byte [di+PH_SIO],2
        je      ph_j
        mov     si,s_ppj_1s
        cmp     byte [di+PH_SIO],3
        je      ph_j
        mov     si,s_ppj_p
        cmp     byte [di+PH_P2],2
        je      ph_j
        mov     si,s_ppj_z
        cmp     byte [di+PH_Z1],2
        je      ph_j
        cmp     byte [di+PH_Z2],2
        je      ph_j
        mov     si,s_ppj_cl
        cmp     byte [di+PH_CLM],1
        je      ph_j
        mov     si,s_ppj_u
        cmp     byte [di+PH_OCC1],2
        je      ph_j
        cmp     byte [di+PH_OCC2],2
        je      ph_j
        mov     si,s_ppj_f
ph_j:   call    puts
        cmp     si,s_ppj_z
        jne     ph_e
        cmp     byte [di+PH_Z1],2
        mov     al,0
        je      ph_zq
        mov     al,4
ph_zq:  pop     cx
        push    cx
        mov     bx,cx
        add     al,[pt_bases+bx]
        call    hex8
        mov     al,'h'
        call    putc
ph_e:   call    nl
        pop     cx
        ret

pp_occ:                                 ; AL = 0/1/2 -> '-', 'f', 'b'
        push    si
        mov     si,s_ppo
        xor     ah,ah
        add     si,ax
        mov     al,[si]
        call    putc
        pop     si
        ret

pp_scc:                                 ; DI = SCC-Ergebnis, AL = Versatz
        cmp     byte [di],0
        je      pz_e
        mov     si,s_ppz
        call    puts
        call    hex8
        mov     al,' '
        call    putc
        mov     al,[di+1]
        call    hex8
        mov     al,':'
        call    putc
        mov     al,[di+2]
        call    hex8
        mov     al,'/'
        call    putc
        mov     al,[di+3]
        call    hex8
pz_e:   ret

pt_bases db     IF_0A, IF_0B, IF_1A, IF_1B, IF_2A, IF_2B, IF_3A, IF_3B
pt_names db     '0A0B1A1B2A2B3A3B'
k5_bases db     IF_1A, IF_1B, IF_2A, IF_2B, IF_3A, IF_3B, 0
s_ppo   db      '-fb'
s_pp    db      4,'SCHNITTSTELLEN - Bausteinproben (Taste S)',0
s_pp0   db      2,'Taste S: K215-Echo, 2651/2661, 8255 (nur 0A), Z8530 je Haelfte;',0
s_pp1   db      2,'was erkannt ist, sperrt die Haelfte fuer die weiteren Proben.',0
s_pp2   db      2,'Schreibt in Register der Karten und stellt sie zurueck. 8255: Drucker',0
s_pp3   db      2,'bekommt evtl. INIT. IFSEL 4 (Platte, Uhr) wird nicht beruehrt.',0
s_pp4   db      2,'b/f = x0-x3, x4-x7 belegt/frei. K215 Status:55/AA. 2651 x3:23/03 xB.',0
s_pph   db      2,'IF Basis x  Rohwerte',0
s_ppk   db      2,' K215 ',1,0
s_pps   db      2,' 2651 ',1,0
s_ppm   db      2,' xB ',1,0
s_ppp   db      2,' 8255 ',1,0
s_ppz   db      2,' SCC+',1,0
s_ppj_k5 db     'K215 (Echo 55/AA)',0
s_ppj_ms db     '2651, A=B gespiegelt: K211/K212/K213',0
s_ppj_1s db     '2651/2661 nur in dieser Haelfte: K801',0
s_ppj_p db      '8255: K210',0
s_ppj_z db      'Z8530: K216 an ',0
s_ppj_cl db     '(gehoert zur Haelfte darueber)',0
s_ppj_u db      'belegt, kein bekannter Baustein',0
s_ppj_f db      'frei',0
pt_ran  db      0
pw_mask db      0
pt_res  times PH_SIZE*8 db 0

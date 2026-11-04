;-----------------------------------------------------------------------
; GHW.INC - Hardware der NCR Decision Mate V erkennen und den
; Koprozessor laufen lassen, fuer MASM 5.1 und JWasm -Zm. Gemeinsame
; Fassung fuer DMVGRAF (Kern der DOS-Bibliothek, tools/gpicbin.py),
; DMVDEMO, DMVRACE und PITTEST; aus HWDET.INC und GINFO.C, nach "NCR
; Decision Mate V - Hardware erkennen" (2.11.2026).
;
; Ergebnisse in gh_tab (16 Worte, Reihenfolge wie ghinfo in DMVGRAF.H,
; Index GHI_ROM ... GHI_UHR wie GH_ROM ... GH_UHR in C - MASM unterscheidet
; Gross und Klein nicht -, also gh_tab[GHI_CPU*2]), Rohwerte in gh_roh.
; Fehler meldet CF. Alle Register bleiben, ausser den Ergebnissen in AX.
; DS muss auf die Daten zeigen (wie bei HWDET.INC). Wer gh_takt, gh_bild,
; gh_uhr oder gh_info braucht, bindet vorher GZEIT.INC ein.
;
;   gh_dmv    ist das eine DMV? nur lesend (Statusbits des 7220 wechseln,
;             nie FFh). AX 1 ja, CF = 1 nein
;   gh_rom    Kennbuchstabe aus 0FF9h -> AL 'C' Farbe, 'M' Mono, 0
;             unbekannt (Programm oder Stapel unter 2000h: CF = 1)
;   gh_romv   Fassung der Firmware -> AX = gh_tab[GHI_ROM]
;   gh_cpu    PUSH 0 und AAD 16 -> AX 1 8088, 2 V20, 3 uneins
;   gh_87     8087 -> AX 0 keiner, 1 8087 (CW 03FFh), 2 anderer
;   gh_k235   13h Bit 1 und 8259 auf 90h/91h -> AX 1 K235, 2 K230/K231,
;             0 keine 16-Bit-Karte
;   gh_cpuk   AX = CPU | FPU << 2 | Karte << 4 (ghcpu)
;   gh_schirm Bildschirmprobe mit Wiederherstellen (Schreibstelle EAD
;             gesichert); nur im Grafikbetrieb aufrufen. AX 1 Mono,
;             2 Farbe; gh_tab[GHI_SCHIRM] 1, 2 oder 3 (Probe und
;             Kennbuchstabe uneins)
;   gh_maus   K806 ueber F1 -> AX 30h oder 0
;   gh_baenke Erweiterungsbaenke -> AX
;   gh_spur   Spuren von Laufwerk A -> AX 40, 80 oder 0
;   gh_takt   CPU-Takt in 10 kHz -> AX (GZEIT.INC)
;   gh_bild   Bildfrequenz in 1/100 Hz -> AX (GZEIT.INC, 0,26 s)
;   gh_uhr    K803 auf 4B laeuft -> AX 1 (GZEIT.INC, 1,2 s)
;   gh_info   AX = GH_MITCOPRO | GH_MITUHR: gh_tab wie ghinfo fuellen
;             (GHI_SCHIRM bleibt, das setzt gh_schirm)
;   gh_copro  K234 oder K238 suchen (zweisprachige Probe aus DMVDIAG),
;             nur K235 -> AX = gh_tab[GHI_COPRO]: 1 keiner, 2 K234, 3 K238,
;             4 der Z80 kam nicht zurueck, 5 nicht probiert (Warteplatz
;             belegt), 0 keine K235
;   gh_corun  DS:SI Startzustand (CX Byte, hoechstens 40) nach 0:0000,
;             Koprozessor einmal laufen lassen. -> AX 0 gut, 1 kein Weg,
;             2 Z80 kam nicht zurueck (CF = 1); gh_res, gh_c0a, gh_c0b
;   gh_corz   eigener Z80-Code in Bank 0: AX:SI Code, CX Laenge, BX
;             Segment fuer CX+8 Byte Sicherung -> AX wie gh_corun
;   gh_vec    AX Segment P -> Startzustand in gh_stz (68008: SSP P+1000h,
;             PC P+100h; Z8001 bei gh_tab[GHI_COPRO] = 3: FCW C000h, PC
;             Segment und Versatz P+100h)
;   gh_coblk  BX Absaetze -> AX Segment eines DOS-Blocks, der nicht ueber
;             dem Warteplatz liegt (Z8001: in einem 64-KB-Segment); CF = 1
;   gh_zhit   liegt [AX*16, AX*16+CX) ueber dem Warteplatz? CF = 1 ja
;   gh_zeile  AL 0 deutsch, 1 englisch: "Erkannt: K235 mit V20, 8087,
;             K234 (68008)" nach gh_ztext ('$', dann 0); AH = 1: auch
;             ausgeben (INT 21h, 09h)
;
; Koprozessor (nur K235; auf K230/K231 ist jedes OUT D8h-DFh eine
; Prozessorumschaltung): Der Z80 wartet bei F10Eh (OUT (D0h),A / NOP).
; Ab F111h steht DI / OUT (DFh),A / LD A,5Ah / LD (F11Ch),A / JP F10Eh;
; DI zuerst, sonst saehe ein Z80-Interrupt (IM 1) bei 0038h die
; 8088-Vektoren als Code. Start immer mit OUT DFh, nie D8h (damit lief
; die echte K234 nicht an). Der 68008 gibt mit move.b d0,$FFDF.w zurueck
; und haelt mit stop, der Z8001 mit OUTB DFh und HALT. F10Eh-F11Ch und
; die ueberschriebenen Vektoren werden gesichert und zurueckgelegt; der
; 8088 steht waehrend des Laufs, Unterbrechungen gesperrt.
;
; Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT
;-----------------------------------------------------------------------
GHI_ROM    EQU   0               ; 1 M.07.00, 2 C.07.00, 3 M.05.00, 4 M.06.00, 5 C.06.00
GHI_SCHIRM EQU   1               ; 1 Mono, 2 Farbe, 3 widerspruechlich
GHI_KARTE  EQU   2               ; 1 K235, 2 K230/K231, 0 keine
GHI_CPU    EQU   3               ; 1 8088, 2 V20, 3 uneins
GHI_FPU    EQU   4               ; 0 keiner, 1 8087, 2 anderer
GHI_COPRO  EQU   5               ; siehe gh_copro
GHI_BAENKE EQU   6
GHI_SPUR_A EQU   7
GHI_TAKT   EQU   8               ; 10 kHz
GHI_BILD   EQU   9               ; 1/100 Hz
GHI_UHR    EQU   10
GH_MITCOPRO EQU 1
GH_MITUHR   EQU 2
GH_ZLO    EQU   0F10Eh          ; Warteplatz des Z80: F10Eh..F11Ch
GH_ZLEN   EQU   15
GH_ZMARK  EQU   0F11Ch

IFDEF GH_NURKERN
; ---- Kopf fuer die Bibliothek: der Kern liegt ab Versatz 0 eines
; eigenen Segments, Aufruf fern ueber gh_fern mit BP = Nummer * 2 ----
        dw      gh_fern, gh_tab, gh_roh, gh_stz, gh_ztext, gz_daten
gh_fern: push   ds
        push    cs
        pop     ds
        call    word ptr cs:gh_ptab[bp]
        pop     ds
        retf
gh_ptab dw      gh_dmv, gh_rom, gh_romv, gh_cpu, gh_87, gh_k235, gh_cpuk
        dw      gh_schirm, gh_maus, gh_baenke, gh_spur, gh_takt, gh_bild
        dw      gh_uhr, gh_info, gh_copro, gh_corun, gh_corz, gh_vec
        dw      gh_coblk, gh_zeile, gz_lern, gz_start, gz_us, gz_cal0
        dw      gz_calv, gz_c0rd
ENDIF

gh_tab  dw      16 dup (0)
gh_roh  LABEL   BYTE            ; Rohwerte
gh_spd  db      0               ; +0  SP-Differenz nach PUSH 0 (V20: 2)
gh_aad  db      0               ; +1  AL nach AAD 16 (8088 12h, V20 0Ch)
gh_sw   dw      0               ; +2  8087 Statuswort (vorbelegt 5A5Ah)
gh_cw   dw      0               ; +4  8087 Steuerwort
gh_res  db      0               ; +6  Byte 0:0000 nach dem Lauf
gh_zok  db      0               ; +7  1 = Z80 kam zurueck (Marke)
gh_romz db      0               ; +8  Kennbuchstabe 0FF9h
        db      0
gh_c0a  dw      0               ; +10 Kanal 0 vor dem Lauf
gh_c0b  dw      0               ; +12 Kanal 0 nach dem Lauf
gh_stz  db      40 dup (0)      ; Startzustand fuer gh_corun (gh_vec)
gh_ivt  db      40 dup (0)      ; gesicherte 8088-Vektoren
gh_ctxn dw      0
gh_zsav db      GH_ZLEN dup (0)
gh_zlauf db     0D3h,0D0h,000h  ; F10Eh: OUT (D0h),A / NOP
        db      0F3h,0D3h,0DFh,03Eh,05Ah,032h,01Ch,0F1h,0C3h,00Eh,0F1h
        db      000h            ; F11Ch: Marke
gh_zeig db      0D3h,0D0h,000h  ; fuer gh_corz: DI / JP Platz
        db      0F3h,0C3h,000h,000h,000h,000h,000h,000h,000h,000h,000h
        db      000h
gh_zende db     03Eh,05Ah,032h,01Ch,0F1h,0C3h,00Eh,0F1h ; LD A,5Ah / LD (F11Ch),A / JP F10Eh
; zweisprachige Probe (DMVDIAG, K238-Design-Stand 7.2), 40 Byte ab 0:0000:
; 68008: SSP 0000C000h, PC 8: BRA 0Eh; MOVE.B #36h,0000h; MOVE.B D0,FFDFh;
;        STOP #2700h
; Z8001: FCW C000h, PC 0:0008; LDB RH4,0000h; JR 1Ch; LDB RL0,#5Ah;
;        LDB 0000h,RL0; OUTB DFh,RL0; HALT
gh_probe db     000h,000h,0C0h,000h,000h,000h,000h,008h,060h,004h,000h,000h
        db      0E8h,007h,011h,0FCh,000h,036h,000h,000h,011h,0C0h,0FFh,0DFh
        db      04Eh,072h,027h,000h,0C8h,05Ah,06Eh,008h,000h,000h,03Ah,086h
        db      000h,0DFh,07Ah,000h
gh_ztext db     80 dup (0)
gh_zpl  dw      0               ; gh_corz: Platz in Bank 0
gh_zln  dw      0               ; gh_corz: Laenge samt Rueckweg
gh_zsg  dw      0               ; gh_corz: Sicherung
gh_zqs  dw      0               ; gh_corz: Quelle
gh_zqo  dw      0
gh_cbs  dw      4 dup (0)       ; gh_coblk: unpassende Bloecke
gh_crc0 dw      0
gh_crc1 dw      0

; ---- 7220 ----
gh_gw   PROC    NEAR            ; bis das FIFO nicht voll ist
        push    ax
        push    cx
        mov     cx,60000
ghgw1:  in      al,0A0h
        test    al,2
        loopnz  ghgw1
        pop     cx
        pop     ax
        ret
gh_gw   ENDP

gh_gbef PROC    NEAR            ; AL Befehl
        call    gh_gw
        out     0A1h,al
        ret
gh_gbef ENDP

gh_gpar PROC    NEAR            ; AL Parameter
        call    gh_gw
        out     0A0h,al
        ret
gh_gpar ENDP

gh_glies PROC   NEAR            ; ein Byte aus dem FIFO -> AL (0 nach Wartezeit)
        push    cx
        mov     cx,60000
ghgl1:  in      al,0A0h
        test    al,1
        jnz     ghgl2
        loop    ghgl1
        pop     cx
        xor     al,al
        ret
ghgl2:  in      al,0A1h
        pop     cx
        ret
gh_glies ENDP

gh_gleer PROC   NEAR            ; FIFO leeren (hoechstens 16 Byte)
        push    ax
        push    cx
        mov     cx,16
ghge1:  in      al,0A0h
        test    al,1
        jz      ghge2
        in      al,0A1h
        loop    ghge1
ghge2:  pop     cx
        pop     ax
        ret
gh_gleer ENDP

gh_gcurs PROC   NEAR            ; CURS BX, MASK FFFFh, FIGS AL (0 schreiben, 1 lesen)
        push    ax
        mov     al,49h
        call    gh_gbef
        mov     al,bl
        call    gh_gpar
        mov     al,bh
        call    gh_gpar
        xor     al,al
        call    gh_gpar
        mov     al,4Ah
        call    gh_gbef
        mov     al,0FFh
        call    gh_gpar
        call    gh_gpar
        mov     al,4Ch
        call    gh_gbef
        mov     al,02h
        call    gh_gpar
        pop     ax
        push    ax
        call    gh_gpar
        xor     al,al
        call    gh_gpar
        pop     ax
        ret
gh_gcurs ENDP

gh_wlies PROC   NEAR            ; Wort bei BX -> AX
        mov     al,1
        call    gh_gcurs
        mov     al,0A0h
        call    gh_gbef
        call    gh_glies
        mov     ah,al
        call    gh_glies
        xchg    al,ah
        ret
gh_wlies ENDP

gh_wschr PROC   NEAR            ; Wort AX nach BX
        push    ax
        xor     al,al
        call    gh_gcurs
        mov     al,20h
        call    gh_gbef
        pop     ax
        push    ax
        call    gh_gpar
        mov     al,ah
        call    gh_gpar
        pop     ax
        ret
gh_wschr ENDP

; ---- DMV ----
gh_dmv  PROC    NEAR
        push    bx
        push    cx
        xor     bl,bl                   ; gesehene Zustaende
        mov     cx,30000
ghd1:   in      al,0A0h
        cmp     al,0FFh
        je      ghd8
        mov     ah,1
        test    al,20h
        jnz     ghd2
        mov     ah,2
ghd2:   or      bl,ah
        mov     ah,4
        test    al,40h
        jnz     ghd3
        mov     ah,8
ghd3:   or      bl,ah
        test    al,04h
        jz      ghd4
        or      bl,10h
ghd4:   cmp     bl,1Fh
        je      ghd7
        loop    ghd1
ghd8:   xor     ax,ax
        stc
        jmp     short ghd9
ghd7:   mov     ax,1
        clc
ghd9:   pop     cx
        pop     bx
        ret
gh_dmv  ENDP

; CF = 1, wenn Kern oder Stapel unter 2000h liegen (dort laege dann die
; Firmware)
gh_lo2k PROC    NEAR
        push    ax
        push    cx
        mov     ax,cs
        cmp     ax,200h
        jb      ghk1
        mov     ax,ss
        mov     cx,sp
        shr     cx,1
        shr     cx,1
        shr     cx,1
        shr     cx,1
        add     ax,cx
        cmp     ax,200h+8
        jb      ghk1
        clc
        jmp     short ghk2
ghk1:   stc
ghk2:   pop     cx
        pop     ax
        ret
gh_lo2k ENDP

gh_rom  PROC    NEAR
        xor     ax,ax
        mov     gh_romz,0
        call    gh_lo2k
        jc      ghr9
        push    ds
        pushf
        cli
        xor     ax,ax
        mov     ds,ax
        out     11h,al                  ; Firmware ueber 0-1FFFh
        mov     ah,ds:[0FF9h]
        out     10h,al
        popf
        pop     ds
        mov     al,ah
        xor     ah,ah
        mov     gh_romz,al
        clc
ghr9:   ret
gh_rom  ENDP

; CRC-32 (wie zip) der Firmware, dann Kennung bei 0FF7h
gh_romv PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        xor     ax,ax
        mov     gh_tab[GHI_ROM*2],ax
        call    gh_lo2k
        jc      ghv9
        push    ds
        pushf
        cli
        mov     ax,0FFFFh
        mov     dx,ax                   ; DX:AX = CRC
        xor     si,si
        xor     bx,bx
        mov     ds,bx
        out     11h,al
ghv1:   xor     al,ds:[si]
        mov     cx,8
ghv2:   shr     dx,1
        rcr     ax,1
        jnc     ghv3
        xor     dx,0EDB8h
        xor     ax,8320h
ghv3:   loop    ghv2
        inc     si
        cmp     si,2000h
        jb      ghv1
        mov     bl,ds:[0FF7h]           ; Kennung: 08h, " M.05.00"
        mov     bh,ds:[0FF9h]
        mov     cl,ds:[0FFBh]
        mov     ch,ds:[0FFCh]
        mov     di,word ptr ds:[0FFDh]  ; Ziffer, '.'
        out     10h,al
        popf
        pop     ds
        not     ax
        not     dx
        mov     gh_crc0,ax
        mov     gh_crc1,dx
        mov     si,1
        cmp     dx,1209h
        jne     ghv4
        cmp     ax,51B6h
        je      ghv8                    ; M.07.00 (33609)
ghv4:   inc     si
        cmp     dx,0BF25h
        jne     ghv5
        cmp     ax,0F3F0h
        je      ghv8                    ; C.07.00 (33610)
ghv5:   xor     si,si
        cmp     bl,8
        jne     ghv8
        cmp     cx,'0' SHL 8 + '.'
        jne     ghv8
        mov     ax,di
        cmp     ah,'.'
        jne     ghv8
        mov     si,3
        cmp     bh,'M'
        jne     ghv6
        cmp     al,'5'
        je      ghv8
        inc     si
        cmp     al,'6'
        je      ghv8
        xor     si,si
        jmp     short ghv8
ghv6:   mov     si,5
        cmp     bh,'C'
        jne     ghv7
        cmp     al,'6'
        je      ghv8
ghv7:   xor     si,si
ghv8:   mov     gh_tab[GHI_ROM*2],si
        clc
ghv9:   mov     ax,gh_tab[GHI_ROM*2]
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
gh_romv ENDP

; ---- Prozessoren ----
gh_cpu  PROC    NEAR
        push    bx
        mov     bx,sp
        db      6Ah,00h                 ; V20: PUSH 0   8088: JPE $+2
        mov     ax,bx
        sub     ax,sp
        mov     sp,bx
        mov     gh_spd,al
        mov     ax,0102h
        db      0D5h,10h                ; AAD 16
        mov     gh_aad,al
        mov     ah,1                    ; 8088?
        cmp     gh_spd,0
        jne     ghc1
        cmp     al,12h
        je      ghc3
        jmp     short ghc2
ghc1:   mov     ah,2                    ; V20?
        cmp     gh_spd,2
        jne     ghc2
        cmp     al,0Ch
        je      ghc3
ghc2:   mov     ah,3
ghc3:   mov     al,ah
        xor     ah,ah
        mov     gh_tab[GHI_CPU*2],ax
        pop     bx
        ret
gh_cpu  ENDP

gh_87   PROC    NEAR
        push    cx
        mov     gh_sw,5A5Ah
        mov     gh_cw,5A5Ah
        db      0DBh,0E3h               ; FNINIT
        mov     cx,40
gh8a:   loop    gh8a
        db      0DDh,3Eh                ; FNSTSW gh_sw
        dw      gh_sw
        mov     cx,40
gh8b:   loop    gh8b
        db      0D9h,3Eh                ; FNSTCW gh_cw
        dw      gh_cw
        mov     cx,40
gh8c:   loop    gh8c
        xor     ax,ax
        cmp     gh_cw,5A5Ah
        je      gh8x
        inc     ax
        cmp     gh_cw,03FFh
        je      gh8x
        inc     ax
gh8x:   mov     gh_tab[GHI_FPU*2],ax
        pop     cx
        ret
gh_87   ENDP

gh_k235 PROC    NEAR
        push    bx
        xor     bx,bx
        in      al,13h
        test    al,02h                  ; negiert: 0 = 16-Bit-Karte da
        jnz     ghp9
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
        mov     bx,1
        je      ghp9
        inc     bx
ghp9:   mov     gh_tab[GHI_KARTE*2],bx
        mov     ax,bx
        pop     bx
        ret
gh_k235 ENDP

gh_cpuk PROC    NEAR
        push    bx
        push    cx
        call    gh_k235
        mov     bx,ax
        mov     cl,4
        shl     bx,cl
        call    gh_87
        shl     ax,1
        shl     ax,1
        or      bx,ax
        call    gh_cpu
        or      ax,bx
        pop     cx
        pop     bx
        ret
gh_cpuk ENDP

; ---- Bildschirm: Farbkarte drei Ebenen zu 16 K Worten, Monokarte eine
; (spiegelt 4000h auf 0000h). A5C3h nach 3FF0h, 3C5Ah nach 7FF0h, 7FF0h
; dann 3FF0h zuruecklesen, alte Worte zurueck; Schreibstelle (EAD) vorher
; mit CURD lesen, danach mit CURS zurueck ----
gh_schirm PROC  NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        call    gh_gleer
        mov     al,0E0h                 ; CURD
        call    gh_gbef
        call    gh_glies
        mov     dl,al
        call    gh_glies
        mov     dh,al
        call    gh_glies
        mov     cl,al
        push    cx
        push    dx
        call    gh_gleer                ; der echte 7220 schickt noch MASK
        mov     bx,3FF0h
        call    gh_wlies
        mov     si,ax
        mov     bx,7FF0h
        call    gh_wlies
        mov     di,ax
        mov     bx,3FF0h
        mov     ax,0A5C3h
        call    gh_wschr
        mov     bx,7FF0h
        mov     ax,3C5Ah
        call    gh_wschr
        mov     bx,7FF0h
        call    gh_wlies
        mov     cx,ax                   ; v7
        mov     bx,3FF0h
        call    gh_wlies
        mov     dx,ax                   ; v3
        mov     bx,7FF0h
        mov     ax,di
        call    gh_wschr
        mov     bx,3FF0h
        mov     ax,si
        call    gh_wschr
        xor     si,si                   ; Ergebnis der Probe
        cmp     cx,3C5Ah
        jne     ghs1
        mov     si,2
        cmp     dx,0A5C3h
        je      ghs1
        mov     si,1
        cmp     dx,3C5Ah
        je      ghs1
        xor     si,si
ghs1:   pop     dx                      ; Schreibstelle zurueck
        pop     cx
        mov     al,49h
        call    gh_gbef
        mov     al,dl
        call    gh_gpar
        mov     al,dh
        call    gh_gpar
        mov     al,cl
        and     al,03h
        call    gh_gpar
        mov     al,4Ah
        call    gh_gbef
        mov     al,0FFh
        call    gh_gpar
        call    gh_gpar
        call    gh_rom                  ; mit dem Kennbuchstaben vergleichen
        mov     bx,si
        or      si,si
        jnz     ghs3
        mov     bx,1                    ; Probe unklar: nach der Firmware
        cmp     al,'C'
        jne     ghs2
        inc     bx
ghs2:   mov     si,3
        jmp     short ghs5
ghs3:   cmp     al,'C'
        jne     ghs4
        cmp     bx,1
        jne     ghs5
        mov     si,3
        jmp     short ghs5
ghs4:   cmp     al,'M'
        jne     ghs5
        cmp     bx,2
        jne     ghs5
        mov     si,3
ghs5:   mov     gh_tab[GHI_SCHIRM*2],si
        mov     ax,bx
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
gh_schirm ENDP

; ---- K806 auf 2A (30h-33h): erst nur lesen (leerer Platz: unteres
; Adressbyte oder FFh), dann Befehl 00h an 31h: F1 (Bit 3) zeigt A0 des
; letzten Schreibens, und es kommen genau fuenf Datenbytes ----
gh_maus PROC    NEAR
        push    bx
        push    cx
        push    dx
        xor     bx,bx
        mov     dx,30h
ghm1:   in      al,dx
        cmp     al,dl
        je      ghm2
        cmp     al,0FFh
        jne     ghm3
ghm2:   inc     bx
ghm3:   inc     dx
        cmp     dx,34h
        jb      ghm1
        cmp     bx,4
        je      ghm0
        mov     cx,64                   ; alte Daten
ghm4:   in      al,31h
        test    al,1
        jz      ghm5
        in      al,30h
        loop    ghm4
ghm5:   mov     cx,20000
ghm6:   in      al,31h
        test    al,2
        jz      ghm7
        loop    ghm6
        jmp     short ghm0
ghm7:   xor     al,al
        out     31h,al
        in      al,31h
        test    al,8
        jz      ghm0
        xor     bx,bx                   ; Datenbytes
ghm8:   mov     cx,20000
        cmp     bx,5
        jb      ghm9
        mov     cx,2500
ghm9:   in      al,31h
        test    al,1
        jnz     ghma
        loop    ghm9
        jmp     short ghmb
ghma:   in      al,30h
        inc     bx
        cmp     bx,6
        jb      ghm8
ghmb:   cmp     bx,5
        jne     ghm0
        mov     ax,30h
        jmp     short ghmx
ghm0:   xor     ax,ax
ghmx:   pop     dx
        pop     cx
        pop     bx
        ret
gh_maus ENDP

; ---- Speicher, Laufwerk ----
; liegt die Stelle n:FFF0h (n in BL) im 64-KB-Fenster, das bei CS, DS oder
; SS beginnt (genaehert: Segment beginnt in Bank n)?
gh_eigen PROC   NEAR
        push    ax
        push    cx
        push    dx
        mov     cx,3
        mov     dx,cs
ghe1:   mov     ax,dx                   ; phys. Anfang in 64-KB-Einheiten:
        push    cx                      ; [Seg*16, Seg*16+10000h) trifft
        mov     cl,12                   ; n:FFF0h, wenn n = Seg>>12 oder
        shr     ax,cl                   ; (n-1) = Seg>>12 und Seg & 0FFFh > 0FFEh
        pop     cx
        cmp     al,bl
        je      ghe8
        cmp     cx,3
        jne     ghe2
        mov     dx,ds
        jmp     short ghe3
ghe2:   mov     dx,ss
ghe3:   loop    ghe1
        clc
        jmp     short ghe9
ghe8:   stc
ghe9:   pop     dx
        pop     cx
        pop     ax
        ret
gh_eigen ENDP

gh_baenke PROC  NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        pushf
        cli
        mov     bl,1                    ; alte Worte sichern
        mov     di,offset gh_stz
ghb1:   call    gh_bseg
        mov     ax,es:[0FFF0h]
        mov     [di],ax
        mov     ax,es:[0FFF2h]
        mov     [di+2],ax
        add     di,4
        inc     bl
        cmp     bl,8
        jb      ghb1
        mov     bl,1                    ; Muster schreiben
ghb2:   call    gh_eigen
        jc      ghb3
        call    gh_bseg
        mov     ah,0A5h
        mov     al,bl
        mov     es:[0FFF0h],ax
        not     ax
        mov     es:[0FFF2h],ax
ghb3:   inc     bl
        cmp     bl,8
        jb      ghb2
        xor     si,si                   ; zusammenhaengend ab 1
        mov     bl,1
ghb4:   call    gh_eigen
        jc      ghb5
        call    gh_bseg
        mov     ah,0A5h
        mov     al,bl
        cmp     es:[0FFF0h],ax
        jne     ghb6
        not     ax
        cmp     es:[0FFF2h],ax
        jne     ghb6
ghb5:   mov     si,bx
        and     si,0FFh
        inc     bl
        cmp     bl,8
        jb      ghb4
ghb6:   mov     bl,7                    ; zurueck, von oben
        mov     di,offset gh_stz+24
ghb7:   call    gh_eigen
        jc      ghb8
        call    gh_bseg
        mov     ax,[di]
        mov     es:[0FFF0h],ax
        mov     ax,[di+2]
        mov     es:[0FFF2h],ax
ghb8:   sub     di,4
        dec     bl
        jnz     ghb7
        popf
        mov     ax,si
        mov     gh_tab[GHI_BAENKE*2],ax
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
gh_baenke ENDP

gh_bseg PROC    NEAR            ; ES = BL * 1000h
        push    ax
        mov     ah,bl
        shl     ah,1
        shl     ah,1
        shl     ah,1
        shl     ah,1
        xor     al,al
        mov     es,ax
        pop     ax
        ret
gh_bseg ENDP

gh_spur PROC    NEAR
        push    es
        xor     ax,ax
        mov     es,ax
        mov     al,es:[0FE08h]
        pop     es
        cmp     al,0
        jne     ghsp1
        mov     ax,40
        jmp     short ghsp9
ghsp1:  cmp     al,1
        mov     ax,80
        je      ghsp9
        xor     ax,ax
ghsp9:  mov     gh_tab[GHI_SPUR_A*2],ax
        ret
gh_spur ENDP

IFDEF GZ_DA
; ---- Takt in 10 kHz: 1600 x AAM gegen Kanal 2, leere Schleife abziehen.
; AAM braucht auf dem V20 15 Takte, auf dem 8088 vermutlich 77 ----
gh_takt PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     ax,gh_tab[GHI_CPU*2]
        or      ax,ax
        jnz     ght0
        call    gh_cpu
ght0:   xor     di,di
        cmp     ax,1
        je      ght1
        cmp     ax,2
        jne     ght8
ght1:   pushf
        cli
        call    gz_us
        mov     si,ax                   ; t0 (unteres Wort reicht)
        mov     bx,100
ght2:   mov     cx,16
ght3:   aam
        loop    ght3
        call    gz_tick
        dec     bx
        jnz     ght2
        call    gz_us
        mov     di,ax                   ; t1
        mov     bx,100
ght4:   mov     cx,16
ght5:   nop
        loop    ght5
        call    gz_tick
        dec     bx
        jnz     ght4
        call    gz_us                   ; t2
        popf
        sub     ax,di                   ; (t1-t0) - (t2-t1)
        sub     di,si
        sub     di,ax
        jbe     ght7
        mov     ax,gh_tab[GHI_CPU*2]
        mov     bx,77
        cmp     ax,2
        jne     ght6
        mov     bx,15
ght6:   mov     ax,7100h                ; 160000 * Takte / us -> 10 kHz
        mul     bx                      ; (160000 = 27100h)
        shl     bx,1
        add     dx,bx
        cmp     dx,di
        jae     ght7
        div     di
        mov     di,ax
        jmp     short ght8
ght7:   xor     di,di
ght8:   mov     ax,di
        mov     gh_tab[GHI_TAKT*2],ax
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
gh_takt ENDP

gh_bild PROC    NEAR
        call    gz_calv
        call    gz_hz
        mov     gh_tab[GHI_BILD*2],ax
        ret
gh_bild ENDP

gh_bcd  PROC    NEAR            ; AL eine BCD-Zahl bis 59? CF = 1 nein
        push    ax
        mov     ah,al
        and     ah,0Fh
        cmp     ah,9
        ja      ghbc1
        cmp     al,59h
        ja      ghbc1
        pop     ax
        clc
        ret
ghbc1:  pop     ax
        stc
        ret
gh_bcd  ENDP

gh_uhr  PROC    NEAR
        push    bx
        push    dx
        push    si
        xor     bx,bx
        xor     al,al
        out     0C8h,al                 ; Registergruppe 0
        in      al,0CEh                 ; Sekunden (BCD)
        cmp     al,0CEh
        je      ghu9
        call    gh_bcd
        jc      ghu9
        mov     bl,al
        call    gz_us
        mov     si,ax
        mov     bh,dl
ghu1:   call    gz_us                   ; 1,2 s
        sub     ax,si
        sbb     dl,bh
        cmp     dl,18
        jb      ghu1
        jne     ghu2
        cmp     ax,20000
        jb      ghu1
ghu2:   in      al,0CEh
        call    gh_bcd
        mov     ah,bl
        mov     bx,0
        jc      ghu9
        cmp     al,ah
        je      ghu9
        inc     bx
ghu9:   mov     ax,bx
        mov     gh_tab[GHI_UHR*2],ax
        pop     si
        pop     dx
        pop     bx
        ret
gh_uhr  ENDP

gh_info PROC    NEAR
        push    bx
        push    cx
        push    di
        mov     bx,ax
        mov     cx,gh_tab[GHI_SCHIRM*2]
        mov     di,offset gh_tab
        xor     ax,ax
ghi1:   mov     [di],ax
        add     di,2
        cmp     di,offset gh_tab+32
        jb      ghi1
        mov     gh_tab[GHI_SCHIRM*2],cx
        call    gh_romv
        call    gh_k235
        call    gh_cpu
        call    gh_87
        test    bl,GH_MITCOPRO
        jz      ghi2
        cmp     gh_tab[GHI_KARTE*2],1
        jne     ghi2
        call    gh_copro
ghi2:   call    gh_baenke
        call    gh_spur
        call    gh_takt
        call    gh_bild
        test    bl,GH_MITUHR
        jz      ghi3
        call    gh_uhr
ghi3:   pop     di
        pop     cx
        pop     bx
        ret
gh_info ENDP
ENDIF

; ---- Koprozessor ----
; Liegt [AX*16, AX*16+CX) ueber dem Warteplatz? CF = 1: ja
gh_zhit PROC    NEAR
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
        jnz     ghz0                    ; ab 64 KB: nein
        mov     ax,bx
        add     ax,cx
        jc      ghz1                    ; reicht ueber 64 KB
        cmp     ax,GH_ZLO
        jbe     ghz0                    ; endet davor
ghz1:   cmp     bx,GH_ZLO+GH_ZLEN
        jae     ghz0                    ; beginnt dahinter
        stc
        jmp     short ghz9
ghz0:   clc
ghz9:   pop     dx
        pop     bx
        pop     ax
        ret
gh_zhit ENDP

; Laufen Kern (CS:0 bis gh_kende) oder Stapel ueber den Warteplatz? CF
gh_zfrei PROC   NEAR
        push    ax
        push    bx
        push    cx
        mov     ax,cs
        mov     cx,offset gh_kende
        call    gh_zhit
        jc      ghzf9
        mov     bx,sp                   ; SS:SP-96 .. SS:SP+32
        sub     bx,96
        jnc     ghzf1
        xor     bx,bx
ghzf1:  mov     ax,bx
        mov     cl,4
        shr     ax,cl
        mov     cx,ss
        add     ax,cx
        mov     cx,128+16
        call    gh_zhit
ghzf9:  pop     cx
        pop     bx
        pop     ax
        ret
gh_zfrei ENDP

; K235 da? sonst nachsehen. CF = 1: nein
gh_ist235 PROC  NEAR
        push    ax
        cmp     gh_tab[GHI_KARTE*2],1
        je      ghis1
        call    gh_k235
        cmp     ax,1
        je      ghis1
        pop     ax
        stc
        ret
ghis1:  pop     ax
        clc
        ret
gh_ist235 ENDP

gh_copro PROC   NEAR
        push    cx
        push    si
        xor     ax,ax
        call    gh_ist235
        jc      ghcp9
        mov     ax,5
        call    gh_zfrei
        jc      ghcp9
        mov     si,offset gh_probe
        mov     cx,40
        call    gh_lauf
        mov     ax,4
        cmp     gh_zok,1
        jne     ghcp9
        mov     ax,2
        cmp     gh_res,36h
        je      ghcp9
        inc     ax
        cmp     gh_res,5Ah
        je      ghcp9
        mov     ax,1
ghcp9:  mov     gh_tab[GHI_COPRO*2],ax
        pop     si
        pop     cx
        ret
gh_copro ENDP

gh_corun PROC   NEAR
        mov     ax,1
        call    gh_ist235
        jc      ghcr9
        call    gh_zfrei
        jc      ghcr9
        cmp     cx,40
        ja      ghcr9
        call    gh_lauf
        xor     ax,ax
        cmp     gh_zok,1
        je      ghcr8
        mov     ax,2
ghcr9:  stc
        ret
ghcr8:  clc
        ret
gh_corun ENDP

; DS:SI (CX Byte) nach 0:0000, Warteplatz mit dem Stummel BX, Z80 laufen
; lassen; danach alles zurueck. -> gh_res, gh_zok, gh_c0a, gh_c0b
gh_lauf PROC    NEAR
        push    bx
        mov     bx,offset gh_zlauf
        call    gh_laufb
        pop     bx
        ret
gh_lauf ENDP

gh_laufb PROC   NEAR
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
        mov     gh_ctxn,cx
        push    bx
        xor     bx,bx                   ; 8088-Vektoren <-> Startzustand
        jcxz    ghl1x
ghl1:   mov     al,es:[bx]
        mov     gh_ivt[bx],al
        mov     al,[si+bx]
        mov     es:[bx],al
        inc     bx
        cmp     bx,cx
        jb      ghl1
ghl1x:  pop     bx
        mov     si,GH_ZLO               ; Warteplatz sichern, Stummel hin
        mov     di,offset gh_zsav
        mov     cx,GH_ZLEN
ghl2:   mov     al,es:[si]
        mov     [di],al
        inc     si
        inc     di
        loop    ghl2
        mov     si,bx
        mov     di,GH_ZLO
        mov     cx,GH_ZLEN
        rep     movsb
IFDEF GZ_DA
        call    gz_c0rd
        mov     gh_c0a,ax
ENDIF
        out     0E0h,al                 ; Z80 auf Bank 0
        out     0D0h,al                 ; Bus an den Z80; der 8088 steht,
        nop                             ; bis der Z80 zurueckgibt
        nop
IFDEF GZ_DA
        call    gz_c0rd
        mov     gh_c0b,ax
ENDIF
        mov     al,es:[0]
        mov     gh_res,al
        mov     gh_zok,0
        cmp     byte ptr es:[GH_ZMARK],5Ah
        jne     ghl3
        mov     gh_zok,1
ghl3:   mov     si,offset gh_zsav       ; alles zuruecklegen
        mov     di,GH_ZLO
        mov     cx,GH_ZLEN
        rep     movsb
        xor     bx,bx
        cmp     gh_ctxn,0
        je      ghl5
ghl4:   mov     al,gh_ivt[bx]
        mov     es:[bx],al
        inc     bx
        cmp     bx,gh_ctxn
        jb      ghl4
ghl5:   popf
        pop     es
        pop     di
        pop     si
        pop     cx
        pop     bx
        pop     ax
        ret
gh_laufb ENDP

; eigener Z80-Code: AX:SI Code, CX Laenge, BX Sicherung (CX+8 Byte). Der
; Platz in Bank 0 liegt unter F100h und unter dem Programm (PSP bis zum
; hoechsten der Segmente CS, DS, SS + 64 KB), nicht unter 2000h. Der
; Z80-Code laeuft bis zu seinem Ende durch; den Rueckweg haengt gh_corz an.
gh_corz PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     gh_zqs,ax
        mov     gh_zqo,si
        mov     gh_zsg,bx
        mov     gh_zln,cx
        mov     ax,1
        call    gh_ist235
        jc      ghq8
        call    gh_zfrei
        jc      ghq8
        mov     cx,gh_zln
        or      cx,cx
        jz      ghq8
        add     cx,8
        jc      ghq8
        cmp     cx,0D000h
        ja      ghq8
        mov     gh_zln,cx               ; ab hier: Laenge samt Rueckweg
        mov     ah,51h                  ; PSP -> Anfang des Programms
        int     21h
        mov     ax,bx
        mov     dx,bx
        mov     cl,4
        shl     ax,cl
        mov     cl,12
        shr     dx,cl                   ; DX:AX
        mov     di,0F100h               ; erster Versuch: unter F100h
        sub     di,gh_zln
        and     di,0FF00h
        or      dx,dx
        jnz     ghq2                    ; Programm ab 64 KB: passt
        mov     cx,di
        add     cx,gh_zln
        cmp     cx,ax
        jbe     ghq2                    ; endet vor dem Programm
        cmp     ax,gh_zln               ; sonst unter das Programm
        jb      ghq8
        mov     di,ax
        sub     di,gh_zln
        and     di,0FF00h
ghq2:   cmp     di,2000h
        jb      ghq8
        mov     gh_zpl,di
        mov     ax,gh_zsg               ; Sicherung und Quelle duerfen den
        mov     cx,gh_zln               ; Platz nicht beruehren
        call    gh_plhit
        jc      ghq8
        mov     ax,gh_zqo
        mov     cl,4
        shr     ax,cl
        add     ax,gh_zqs
        mov     cx,gh_zln
        call    gh_plhit
        jc      ghq8
        cld
        mov     bx,gh_zpl
        mov     dx,gh_zln
        push    ds
        pushf
        cli
        mov     es,gh_zsg               ; Bank 0 sichern
        xor     di,di
        mov     si,bx
        mov     cx,dx
        xor     ax,ax
        mov     ds,ax
        rep     movsb
        popf
        pop     ds
        push    ds
        pushf
        cli
        xor     ax,ax                   ; Code nach 0:Platz
        mov     es,ax
        mov     di,bx
        mov     cx,dx
        sub     cx,8
        mov     si,gh_zqo
        mov     ds,gh_zqs
        rep     movsb
        popf
        pop     ds
        mov     si,offset gh_zende      ; Rueckweg dahinter
        mov     cx,8
        rep     movsb
        mov     word ptr gh_zeig[5],bx  ; F111h: DI / JP Platz
        mov     bx,offset gh_zeig
        xor     cx,cx                   ; keine Vektoren
        call    gh_laufb
        push    ds                      ; Bank 0 zurueck
        pushf
        cli
        xor     ax,ax
        mov     es,ax
        mov     di,gh_zpl
        mov     cx,gh_zln
        xor     si,si
        mov     ds,gh_zsg
        rep     movsb
        popf
        pop     ds
        xor     ax,ax
        cmp     gh_zok,1
        je      ghq9
        mov     ax,2
ghq8:   stc
        jmp     short ghqx
ghq9:   clc
ghqx:   pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
gh_corz ENDP

; beruehrt [AX*16, AX*16+CX) den Platz [gh_zpl, gh_zpl+gh_zln)? CF
gh_plhit PROC   NEAR
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
        jnz     ghph0
        mov     ax,bx
        add     ax,cx
        jc      ghph1
        cmp     ax,gh_zpl
        jbe     ghph0
ghph1:  mov     ax,gh_zpl
        add     ax,gh_zln
        cmp     bx,ax
        jae     ghph0
        stc
        jmp     short ghph9
ghph0:  clc
ghph9:  pop     dx
        pop     bx
        pop     ax
        ret
gh_plhit ENDP

; Startzustand fuer einen Block bei Segment AX nach gh_stz
gh_vec  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     dx,ax
        mov     cl,4
        shl     ax,cl
        mov     cl,12
        shr     dx,cl                   ; DX:AX = P
        cmp     gh_tab[GHI_COPRO*2],3
        je      ghvz8
        mov     bx,ax
        mov     cx,dx
        add     bx,1000h
        adc     cx,0
        mov     gh_stz[0],0
        mov     gh_stz[1],cl
        mov     gh_stz[2],bh
        mov     gh_stz[3],bl
        add     ax,100h
        adc     dx,0
        mov     gh_stz[4],0
        mov     gh_stz[5],dl
        mov     gh_stz[6],ah
        mov     gh_stz[7],al
        jmp     short ghvc9
ghvz8:  add     ax,100h
        mov     gh_stz[0],0
        mov     gh_stz[1],0
        mov     gh_stz[2],0C0h          ; FCW: segmentiert, System
        mov     gh_stz[3],0
        or      dl,80h
        mov     gh_stz[4],dl            ; Segment (Langform)
        mov     gh_stz[5],0
        mov     gh_stz[6],ah
        mov     gh_stz[7],al
ghvc9:  pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
gh_vec  ENDP

; Block fuer den Koprozessor: BX Absaetze -> AX Segment, CF = 1 keiner
; (hoechstens vier Versuche; unpassende Bloecke werden freigegeben)
gh_coblk PROC   NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        xor     di,di                   ; unpassende Bloecke (gh_cbs)
ghcb1:  push    bx
        mov     ah,48h
        int     21h
        pop     bx
        jc      ghcb7
        mov     dx,bx
        mov     cl,4
        shl     dx,cl
        mov     cx,dx                   ; Groesse in Bytes (Block < 64 KB)
        call    gh_zhit
        jc      ghcb2
        cmp     gh_tab[GHI_COPRO*2],3
        jne     ghcb8
        mov     dx,ax                   ; Z8001: Versatz + Groesse <= 10000h
        push    cx
        mov     cl,4
        shl     dx,cl
        pop     cx
        add     dx,cx
        jnc     ghcb8
        or      dx,dx
        jz      ghcb8
ghcb2:  mov     gh_cbs[di],ax
        add     di,2
        cmp     di,8
        jb      ghcb1
        stc
        jmp     short ghcb7
ghcb8:  clc
ghcb7:  pushf                           ; unpassende Bloecke freigeben
        push    ax
        push    es
ghcb5:  or      di,di
        jz      ghcb6
        sub     di,2
        mov     es,gh_cbs[di]
        mov     ah,49h
        int     21h
        jmp     ghcb5
ghcb6:  pop     es
        pop     ax
        popf
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
gh_coblk ENDP

; ---- Meldung ----
gh_tde  db      'Erkannt: $K235 mit $K230/K231 mit $8088$V20$CPU? (PUSH 0 und AAD 16 uneins)$'
        db      ', 8087$, K234 (68008)$, K238 (Z8001)$, keine K234/K238$'
        db      ', K234/K238 nicht geprueft$'
gh_ten  db      'Found: $K235 with $K230/K231 with $8088$V20$CPU? (PUSH 0 and AAD 16 disagree)$'
        db      ', 8087$, K234 (68008)$, K238 (Z8001)$, no K234/K238$'
        db      ', K234/K238 not tested$'

; Text Nummer BL aus der Liste ab SI an DI anhaengen
gh_zt   PROC    NEAR
        push    ax
        push    bx
        push    si
ghzt1:  or      bl,bl
        jz      ghzt3
ghzt2:  lodsb
        cmp     al,'$'
        jne     ghzt2
        dec     bl
        jmp     ghzt1
ghzt3:  lodsb
        cmp     al,'$'
        je      ghzt4
        mov     [di],al
        inc     di
        jmp     ghzt3
ghzt4:  pop     si
        pop     bx
        pop     ax
        ret
gh_zt   ENDP

gh_zeile PROC   NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        cld
        mov     cx,ax
        mov     si,offset gh_tde
        or      cl,cl
        jz      ghze1
        mov     si,offset gh_ten
ghze1:  mov     di,offset gh_ztext
        xor     bl,bl
        call    gh_zt
        mov     bl,2
        cmp     gh_tab[GHI_KARTE*2],1
        jne     ghze2
        mov     bl,1
ghze2:  call    gh_zt
        mov     bl,3
        cmp     gh_tab[GHI_CPU*2],1
        je      ghze3
        inc     bl
        cmp     gh_tab[GHI_CPU*2],2
        je      ghze3
        inc     bl
ghze3:  call    gh_zt
        cmp     gh_tab[GHI_FPU*2],0
        je      ghze4
        mov     bl,6
        call    gh_zt
ghze4:  mov     ax,gh_tab[GHI_COPRO*2]
        mov     bl,7
        cmp     ax,2
        je      ghze5
        inc     bl
        cmp     ax,3
        je      ghze5
        inc     bl
        cmp     ax,1
        je      ghze5
        cmp     ax,4
        je      ghze5
        inc     bl
        cmp     ax,5
        jne     ghze6
ghze5:  call    gh_zt
ghze6:  mov     word ptr [di],0A0Dh
        mov     word ptr [di+2],'$'
        cmp     ch,1
        jne     ghze7
        mov     dx,offset gh_ztext
        mov     ah,09h
        int     21h
ghze7:  mov     ax,offset gh_ztext
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
gh_zeile ENDP

gh_kende LABEL  BYTE

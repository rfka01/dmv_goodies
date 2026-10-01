; GDCLIB.INC - GDC-Grundroutinen, Zeichnen, Text, Tastatur, Grafik ein/aus
; gemeinsam fuer DMVDEMO und DMVRACE (per INCLUDE an derselben Stelle)

;=======================================================================
;  GDC-Grundroutinen
;=======================================================================

; Kommando AL an den GDC (wartet wie NCRGRAPH auf FIFO frei + nicht zeichnen)
gcmd    PROC    NEAR
        push    ax
gcmd1:  in      al,P_STAT
        test    al,0Ah
        jnz     gcmd1
        pop     ax
        out     P_CMD,al
        ret
gcmd    ENDP

; Parameter AL an den GDC
gpar    PROC    NEAR
        push    ax
gpar1:  in      al,P_STAT
        test    al,0Ah
        jnz     gpar1
        pop     ax
        out     P_PARM,al
        ret
gpar    ENDP

; ZOOM-Parameter AL ueber Port A2h (Boot-ROM C.07.00 macht es genauso:
; ZOOM 46h an A1h, Parameter an A2h). Der GDC sieht BA0=0, also einen
; Parameter; neuere Grafikkarten (Change Level > 35) merken sich den Wert
; zusaetzlich fuer den waagrechten Anzeige-Zoom.
gparz   PROC    NEAR
        push    ax
gparz1: in      al,P_STAT
        test    al,0Ah
        jnz     gparz1
        pop     ax
        out     P_ZOOM,al
        ret
gparz   ENDP

; Wort AX als zwei Parameter (low, high)
gpw     PROC    NEAR
        call    gpar
        xchg    al,ah
        call    gpar
        xchg    al,ah
        ret
gpw     ENDP

; CX=x, DX=y  ->  DI = Wortadresse (ohne Ebene, mit v_vbase), BL = Punkt 0..15
xyaddr  PROC    NEAR
        mov     ax,dx
        shl     ax,1
        shl     ax,1
        shl     ax,1
        mov     di,ax
        shl     ax,1
        shl     ax,1
        add     di,ax
        mov     ax,cx
        shr     ax,1
        shr     ax,1
        shr     ax,1
        shr     ax,1
        add     di,ax
        add     di,v_vbase
        mov     bl,cl
        and     bl,0Fh
        ret
xyaddr  ENDP

; CURS: DI = Wortadresse inkl. Ebene, BL = Punkt
curs    PROC    NEAR
        push    cx
        mov     al,49h
        call    gcmd
        mov     ax,di
        call    gpw
        mov     al,bl
        mov     cl,4
        shl     al,cl
        call    gpar
        pop     cx
        ret
curs    ENDP

; Zeichenmodus je Ebene: AL = Farbe, BL = Ebenenbit
; -> AL = WDAT-Kommando (20h..23h) oder 0 = Ebene auslassen
pmode   PROC    NEAR
        cmp     v_mono,0
        je      pmo1
        or      al,al
        jmp     short pmo2
pmo1:   and     al,bl
pmo2:   push    bx
        mov     bl,v_dmode
        mov     bh,0
        jz      pmo3
        mov     al,ONTAB[bx]
        pop     bx
        ret
pmo3:   mov     al,OFFTAB[bx]
        pop     bx
        ret
pmode   ENDP

; Rastermuster-Byte einer Farbe fuer eine Ebene
; AL = Farbe, BL = Ebenenbit, DL = Zeile 0..7  ->  AL = Muster
ppat    PROC    NEAR
        cmp     v_mono,0
        jne     ppa2
        and     al,bl
        jz      ppa1
        mov     al,0FFh
ppa1:   ret
ppa2:   push    bx
        and     al,7
        mov     bl,al
        mov     bh,0
        shl     bx,1
        shl     bx,1
        shl     bx,1
        push    dx
        and     dx,7
        add     bx,dx
        pop     dx
        mov     al,MONOPAT[bx]
        pop     bx
        ret
ppat    ENDP

; Figur auf allen Ebenen zeichnen
; v_fx,v_fy Startpunkt; FIGP.. Parameter (v_fn Stueck); v_fcmd 6Ch/68h;
; v_col Farbe, v_dmode Modus; v_pram = 8 Musterbytes (0 = Linienmuster v_lpat)
drawfig PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     cx,v_fx
        mov     dx,v_fy
        call    xyaddr
        mov     v_fdot,bl
        mov     v_faddr,di
        mov     si,offset PLTAB
        mov     cx,v_npl
dfg1:   push    cx
        mov     al,v_col
        mov     bl,[si+2]
        call    pmode
        or      al,al
        jz      dfg9
        call    gcmd                    ; Zeichenmodus setzen
        mov     di,v_faddr
        add     di,[si]
        mov     bl,v_fdot
        call    curs
        mov     al,78h                  ; PRAM ab Byte 8 (Muster)
        call    gcmd
        mov     bx,v_pram
        or      bx,bx
        jz      dfg2
        mov     cx,8
dfg3:   mov     al,[bx]
        call    gpar
        inc     bx
        loop    dfg3
        jmp     short dfg4
dfg2:   mov     ax,v_lpat
        call    gpw
dfg4:   mov     al,4Ch                  ; FIGS
        call    gcmd
        mov     bx,offset FIGP
        mov     cx,v_fn
dfg5:   mov     al,[bx]
        call    gpar
        inc     bx
        loop    dfg5
        mov     al,v_fcmd               ; FIGD / GCHRD
        call    gcmd
dfg9:   add     si,3
        pop     cx
        loop    dfg1
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
drawfig ENDP

; Linie von (v_x0,v_y0) nach (v_x1,v_y1), Farbe v_col, Modus v_dmode
line    PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     ax,v_x0
        mov     v_fx,ax
        mov     ax,v_y0
        mov     v_fy,ax
        mov     cx,v_x1
        sub     cx,v_x0                 ; dx
        mov     dx,v_y1
        sub     dx,v_y0                 ; dy
        mov     si,cx
        or      si,si
        jns     lin1
        neg     si                      ; |dx|
lin1:   mov     di,dx
        or      di,di
        jns     lin2
        neg     di                      ; |dy|
lin2:   or      dx,dx
        js      lin5
        or      cx,cx                   ; dy >= 0
        js      lin3
        cmp     di,si
        jb      lin2a
        mov     bl,0                    ; nach unten, Diagonale rechts
        jmp     short lin_y
lin2a:  mov     bl,1                    ; nach rechts, Diagonale unten
        jmp     short lin_x
lin3:   cmp     di,si
        jb      lin3a
        mov     bl,7
        jmp     short lin_y
lin3a:  mov     bl,6
        jmp     short lin_x
lin5:   or      cx,cx                   ; dy < 0
        jle     lin6
        cmp     si,di
        jb      lin5a
        mov     bl,2
        jmp     short lin_x
lin5a:  mov     bl,3
        jmp     short lin_y
lin6:   cmp     di,si
        jb      lin6a
        mov     bl,4
        jmp     short lin_y
lin6a:  mov     bl,5
        jmp     short lin_x
lin_y:  xchg    si,di                   ; Hauptachse y
lin_x:                                  ; SI = Haupt, DI = Neben
        mov     al,bl
        or      al,08h
        mov     FIGP,al
        mov     ax,si
        or      ax,4000h
        mov     FDC,ax
        mov     ax,di
        shl     ax,1
        mov     FD1,ax                  ; D1 = 2*neben
        sub     ax,si
        and     ax,3FFFh
        mov     FD,ax                   ; D = 2*neben - haupt
        mov     ax,di
        sub     ax,si
        shl     ax,1
        and     ax,3FFFh
        mov     FD2,ax                  ; D2 = 2*(neben-haupt)
        and     FD1,3FFFh
        mov     v_fn,9
        mov     v_fcmd,6Ch
        mov     v_pram,0
        call    drawfig
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
line    ENDP

; Rechteckrahmen (v_x0,v_y0)-(v_x1,v_y1)
rect    PROC    NEAR
        push    ax
        mov     ax,v_x0
        mov     v_fx,ax
        mov     ax,v_y0
        mov     v_fy,ax
        mov     FIGP,40h                ; Rechteck, Richtung 0 (abwaerts)
        mov     FDC,4003h
        mov     ax,v_y1
        sub     ax,v_y0
        mov     FD,ax
        mov     FDM,ax
        mov     ax,v_x1
        sub     ax,v_x0
        mov     FD2,ax
        mov     FD1,3FFFh
        mov     v_fn,11
        mov     v_fcmd,6Ch
        mov     v_pram,0
        call    drawfig
        pop     ax
        ret
rect    ENDP

; Kreis Mittelpunkt (v_cx,v_cy), Radius v_r - acht Oktanten in Hardware
circle  PROC    NEAR
        push    ax
        push    bx
        push    dx
        push    si
        mov     ax,v_r
        or      ax,ax
        jnz     cir0
        jmp     cir9
cir0:   mov     bx,181
        mul     bx
        mov     al,ah
        mov     ah,dl                   ; r*181/256 = r/wurzel2
        or      ax,4000h
        mov     FDC,ax
        mov     ax,v_r
        dec     ax
        mov     FD,ax
        shl     ax,1
        mov     FD2,ax
        mov     FD1,3FFFh
        mov     FDM,0
        mov     v_fn,11
        mov     v_fcmd,6Ch
        mov     v_pram,0
        mov     si,offset CIRTAB
cir1:   mov     al,[si]
        cmp     al,0FFh
        je      cir9
        or      al,20h
        mov     FIGP,al
        mov     al,[si+1]               ; x-Faktor -1/0/1
        cbw
        mul     v_r
        add     ax,v_cx
        mov     v_fx,ax
        mov     al,[si+2]
        cbw
        mul     v_r
        add     ax,v_cy
        mov     v_fy,ax
        mov     al,[si+3]
        mov     ah,0
        mov     FDM,ax
        call    drawfig
        add     si,4
        jmp     short cir1
cir9:   pop     si
        pop     dx
        pop     bx
        pop     ax
        ret
circle  ENDP

; Rechteck fuellen (Flaechenfuellung per GCHRD)
; v_x0,v_y0,v_w,v_h; v_ca/v_cb Farben, v_patp -> 8 Byte Mischmuster
; (Bit gesetzt = Farbe A, Zeile 0 oben, Bit 7 links); v_fmode 0=ersetzen 1=XOR(v_ca)
fill    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        cmp     v_w,0
        jne     fil0
        jmp     fil99
fil0:   cmp     v_h,0
        jne     fil0a
        jmp     fil99
fil0a:  mov     al,0
        call    setgz
        mov     cx,v_x0
        add     cx,v_w
        dec     cx
        mov     dx,v_y0
        call    xyaddr
        mov     v_fdot,bl
        mov     v_faddr,di
        mov     si,offset PLTAB
        mov     cx,v_npl
fil1:   push    cx
        mov     bl,[si+2]
        cmp     v_fmode,0
        je      fil2
        ; XOR: nur Ebenen mit Farbbit, Muster voll
        mov     al,v_ca
        push    ax
        mov     al,v_dmode
        mov     v_dsave,al
        mov     v_dmode,1
        pop     ax
        call    pmode
        push    ax
        mov     al,v_dsave
        mov     v_dmode,al
        pop     ax
        or      al,al
        jnz     fil1a
        jmp     fil9
fil1a:  mov     v_fcmdm,al
        mov     di,offset PBUF
        mov     cx,8
        mov     al,0FFh
        push    es
        push    ds
        pop     es
        rep     stosb
        pop     es
        jmp     short fil5
fil2:   mov     v_fcmdm,20h             ; ersetzen
        ; Muster je Ebene: (A and P) or (B and not P), Zeilenlage passend zu y
        mov     di,0
fil3:   mov     dx,v_y0
        add     dx,7
        sub     dx,di
        and     dx,7                    ; Zeile im Muster
        mov     al,v_ca
        call    ppat
        mov     ah,al                   ; AH = A
        mov     al,v_cb
        call    ppat                    ; AL = B
        push    bx
        mov     bx,v_patp
        add     bx,dx
        mov     bl,[bx]                 ; BL = P
        and     ah,bl
        not     bl
        and     al,bl
        or      al,ah
        pop     bx
        mov     PBUF[di],al
        inc     di
        cmp     di,8
        jb      fil3
fil5:   mov     al,v_fcmdm
        call    gcmd
        mov     di,v_faddr
        add     di,[si]
        mov     bl,v_fdot
        call    curs
        mov     al,78h
        call    gcmd
        mov     bx,offset PBUF
        mov     cx,8
fil6:   mov     al,[bx]
        call    gpar
        inc     bx
        loop    fil6
        mov     al,4Ch                  ; FIGS: Grafikzeichen, Richtung 6 (links)
        call    gcmd
        mov     al,16h
        call    gpar
        mov     ax,v_h
        dec     ax
        or      ax,4000h
        call    gpw
        mov     ax,v_w
        call    gpw
        mov     al,68h                  ; GCHRD
        call    gcmd
fil9:   add     si,3
        pop     cx
        dec     cx
        jz      fil99
        jmp     fil1
fil99:  pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
fill    ENDP

; einfarbig fuellen: v_x0,v_y0,v_w,v_h, AL = Farbe
fillc   PROC    NEAR
        mov     v_ca,al
        mov     v_cb,al
        mov     v_patp,offset PSOLID
        mov     v_fmode,0
        call    fill
        ret
fillc   ENDP

; Grafikzeichen AL an (v_tx,v_ty), Vergroesserung v_tz, Farbe v_col
gchar   PROC    NEAR
        push    ax
        push    bx
        push    cx
        cmp     al,20h
        jb      gch9
        cmp     al,7Fh
        ja      gch9
        sub     al,20h
        mov     ah,0
        shl     ax,1
        shl     ax,1
        shl     ax,1
        add     ax,offset FONT
        mov     v_pram,ax
        mov     al,byte ptr v_tz
        dec     al
        call    setgz
        mov     ax,v_tz
        mov     cl,3
        shl     ax,cl
        add     ax,v_tx
        dec     ax
        mov     v_fx,ax
        mov     ax,v_ty
        mov     v_fy,ax
        mov     FIGP,16h
        mov     FDC,4007h
        mov     FD,8
        mov     v_fn,5
        mov     v_fcmd,68h
        call    drawfig
        mov     v_pram,0
gch9:   pop     cx
        pop     bx
        pop     ax
        ret
gchar   ENDP

; Text SI (0-terminiert) an (v_tx,v_ty), Groesse v_tz, Farbe v_col (BUNT = Regenbogen)
text    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    si
        mov     bl,v_col
        mov     bh,0                    ; Regenbogenzaehler
        mov     cx,v_tx
        push    cx
txt1:   lodsb
        or      al,al
        jz      txt9
        cmp     bl,BUNT
        jne     txt2
        push    bx
        mov     bl,bh
        mov     bh,0
        mov     ah,RAINB[bx]
        pop     bx
        mov     v_col,ah
        cmp     al,' '
        je      txt2
        inc     bh
        cmp     bh,6
        jb      txt2
        mov     bh,0
txt2:   cmp     al,' '
        je      txt3
        call    gchar
txt3:   mov     ax,v_tz
        mov     cl,3
        shl     ax,cl
        add     v_tx,ax
        jmp     short txt1
txt9:   pop     cx
        mov     v_tx,cx
        mov     v_col,bl
        pop     si
        pop     cx
        pop     bx
        pop     ax
        ret
text    ENDP

; Text waagrecht zentriert: SI, v_ty, v_tz, v_col
ctext   PROC    NEAR
        push    ax
        push    cx
        push    dx
        push    si
        mov     cx,0
ctx1:   lodsb
        or      al,al
        jz      ctx2
        inc     cx
        jmp     short ctx1
ctx2:   mov     ax,v_tz
        mul     cx
        mov     cl,3
        shl     ax,cl                   ; Breite
        mov     cx,640
        sub     cx,ax
        shr     cx,1
        mov     v_tx,cx
        pop     si
        call    text
        pop     dx
        pop     cx
        pop     ax
        ret
ctext   ENDP

; Text mit Schatten: erst Farbe v_shc versetzt um v_sho, dann v_col
stext   PROC    NEAR
        push    ax
        mov     al,v_col
        push    ax
        mov     ax,v_sho
        add     v_tx,ax
        add     v_ty,ax
        mov     al,v_shc
        mov     v_col,al
        call    text
        mov     ax,v_sho
        sub     v_tx,ax
        sub     v_ty,ax
        pop     ax
        mov     v_col,al
        call    text
        pop     ax
        ret
stext   ENDP

; Worte fuellen: DI = Adresse inkl. Ebene, AX = Wort, CX = Anzahl (1..16384)
wfill   PROC    NEAR
        push    bx
        push    ax
        mov     bl,0
        call    curs
        mov     al,4Ah                  ; MASK FFFF (vor jedem Schreiben neu)
        call    gcmd
        mov     ax,0FFFFh
        call    gpw
        mov     al,4Ch
        call    gcmd
        mov     al,02h
        call    gpar
        mov     ax,cx
        dec     ax
        and     ax,3FFFh
        call    gpw
        mov     al,20h                  ; WDAT ersetzen
        call    gcmd
        pop     ax
        call    gpw
        pop     bx
        ret
wfill   ENDP

; Wortstrom beginnen: DI = Adresse inkl. Ebene (danach Worte mit gpw senden)
wbegin  PROC    NEAR
        push    bx
        mov     bl,0
        call    curs
        mov     al,4Ah
        call    gcmd
        mov     ax,0FFFFh
        call    gpw
        mov     al,4Ch
        call    gcmd
        mov     al,02h
        call    gpar
        mov     al,0
        call    gpar
        call    gpar
        mov     al,20h
        call    gcmd
        pop     bx
        ret
wbegin  ENDP

; ganze Zeilen einfarbig: DX = erste Zeile, CX = Zeilen, AL = Farbe
hlines  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     v_hcol,al
        push    dx
        mov     ax,cx
        mov     bx,40
        mul     bx
        mov     v_hcnt,ax
        pop     dx
        mov     cx,0
        call    xyaddr
        mov     v_faddr,di
        mov     si,offset PLTAB
        mov     cx,v_npl
hli1:   push    cx
        mov     al,v_hcol
        mov     bl,[si+2]
        mov     dl,0
        call    ppat
        mov     ah,al
        mov     di,v_faddr
        add     di,[si]
        mov     cx,v_hcnt
        call    wfill
        add     si,3
        pop     cx
        loop    hli1
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
hlines  ENDP

; Bildschirm loeschen (alle Ebenen komplett)
cls     PROC    NEAR
        push    ax
        push    cx
        push    si
        push    di
        mov     si,offset PLTAB
        mov     cx,v_npl
cls1:   push    cx
        mov     di,[si]
        mov     ax,0
        mov     cx,16384
        call    wfill
        add     si,3
        pop     cx
        loop    cls1
        pop     di
        pop     si
        pop     cx
        pop     ax
        ret
cls     ENDP

; ZOOM: Anzeige-Zoom v_zd, Zeichen-Zoom AL (0 = 1fach)
setgz   PROC    NEAR
        cmp     al,v_zg
        jne     sgz1
        ret
sgz1:   mov     v_zg,al
        jmp     short zsend
setgz   ENDP

setdz   PROC    NEAR
        mov     v_zd,al
zsend:  push    ax
        push    cx
        mov     al,46h
        call    gcmd
        mov     al,v_zd
        mov     cl,4
        shl     al,cl
        or      al,v_zg
        call    gparz                   ; wie das Boot-ROM C.07: ueber A2h
        push    ax                      ; dasselbe noch einmal ueber A0h, falls
        mov     al,46h                  ; A2h den GDC nicht erreicht (MAME ohne
        call    gcmd                    ; A2h-Anbindung); doppelt schadet nicht
        pop     ax
        call    gpar
        pop     cx
        pop     ax
        ret
setdz   ENDP

; Partition BX (0/1) eintragen: AX = Startadresse, CX = Zeilen, DL = 1 Grafik / 0 Text
mkarea  PROC    NEAR
        push    bx
        push    cx
        shl     bx,1
        shl     bx,1
        mov     AREA[bx],al
        mov     AREA[bx+1],ah
        mov     al,cl
        and     al,0Fh
        push    cx
        mov     cl,4
        shl     al,cl
        pop     cx
        mov     AREA[bx+2],al
        mov     ax,cx
        mov     cl,4
        shr     ax,cl
        and     al,3Fh
        or      dl,dl
        jz      mka1
        or      al,40h
mka1:   mov     AREA[bx+3],al
        pop     cx
        pop     bx
        ret
mkarea  ENDP

; Partitionen an den GDC (PRAM 0..7)
sendarea PROC   NEAR
        push    ax
        push    bx
        push    cx
        mov     al,70h
        call    gcmd
        mov     bx,offset AREA
        mov     cx,8
sar1:   mov     al,[bx]
        call    gpar
        inc     bx
        loop    sar1
        pop     cx
        pop     bx
        pop     ax
        ret
sendarea ENDP

; Standardanzeige: eine Grafikpartition ab 0, 400 Zeilen
stdarea PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bx,0
        mov     ax,0
        mov     cx,400
        mov     dl,1
        call    mkarea
        mov     bx,1
        call    mkarea
        call    sendarea
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
stdarea ENDP

; CCHAR (Zeilen je Textzeile/Cursor) AL
cchar   PROC    NEAR
        push    ax
        mov     al,4Bh
        call    gcmd
        pop     ax
        call    gpar
        ret
cchar   ENDP

; auf Bildwechsel (VSYNC, Status Bit 5) warten
vsync   PROC    NEAR
        push    ax
vsy1:   in      al,P_STAT
        test    al,20h
        jnz     vsy1
vsy2:   in      al,P_STAT
        test    al,20h
        jz      vsy2
        inc     v_frame
        pop     ax
        ret
vsync   ENDP

; Tastatur abfragen (DOS Fn 06h, ohne ^C-Pruefung) -> v_key 1 = weiter, 2 = ESC
poll_key PROC   NEAR
        push    ax
        push    dx
        cmp     v_key,0
        jne     pk9
        mov     ah,06h
        mov     dl,0FFh
        int     21h
        jz      pk9
        or      al,al
        jnz     pk1
        mov     ah,06h                  ; erweiterte Taste: Folgebyte holen
        mov     dl,0FFh
        int     21h
        mov     al,1
pk1:    mov     v_key,1
        cmp     al,1Bh
        jne     pk9
        mov     v_key,2
pk9:    pop     dx
        pop     ax
        ret
poll_key ENDP

; CX Bildwechsel warten, Abbruch bei Taste
wframes PROC    NEAR
        push    cx
        jcxz    wfr9
wfr1:   call    vsync
        call    poll_key
        cmp     v_key,0
        jne     wfr9
        loop    wfr1
wfr9:   pop     cx
        ret
wframes ENDP

; Zufallszahl -> AX
rnd     PROC    NEAR
        push    dx
        push    bx
        mov     ax,v_seed
        mov     bx,25173
        mul     bx
        add     ax,13849
        mov     v_seed,ax
        pop     bx
        pop     dx
        ret
rnd     ENDP

; Zufallszahl 0..BX-1 -> AX
rndn    PROC    NEAR
        push    dx
        call    rnd
        mul     bx
        mov     ax,dx
        pop     dx
        ret
rndn    ENDP

; AX * sin/cos-Wert BX (Q15) -> AX
mulq    PROC    NEAR
        push    dx
        imul    bx
        shl     ax,1
        rcl     dx,1
        mov     ax,dx
        pop     dx
        ret
mulq    ENDP

; sin(AL) -> BX,  cos(AL) -> CX   (256 Schritte je Vollkreis)
sincos  PROC    NEAR
        push    ax
        push    si
        mov     ah,0
        mov     si,ax
        shl     si,1
        mov     bx,SINTAB[si]
        add     al,64
        mov     si,ax
        shl     si,1
        mov     cx,SINTAB[si]
        pop     si
        pop     ax
        ret
sincos  ENDP

;=======================================================================
;  Grafik ein/aus, Erkennung
;=======================================================================

gr_on   PROC    NEAR
        mov     al,0Ch                  ; Anzeige dunkel
        call    gcmd
        mov     v_zd,0
        mov     v_zg,0
        mov     al,0
        call    setdz
        mov     al,0                    ; CCHAR wie NCRGRAPH
        call    cchar
        mov     al,70h                  ; PRAM: 2 Grafikpartitionen, Muster FF
        call    gcmd
        mov     si,offset PRGRAF
        mov     cx,16
gro1:   lodsb
        call    gpar
        loop    gro1
        call    stdarea
        call    cls
        mov     al,6Bh                  ; START
        call    gcmd
        mov     al,0Dh                  ; Anzeige ein (wie NCRGRAPH)
        call    gcmd
        ret
gr_on   ENDP

gr_off  PROC    NEAR
        mov     al,0
        mov     v_zg,al
        call    setdz
        mov     al,8Fh                  ; CCHAR wie NCRGRAPH GEXIT
        call    cchar
        mov     al,70h
        call    gcmd
        mov     si,offset PRTEXT
        mov     cx,16
gof1:   lodsb
        call    gpar
        loop    gof1
        mov     di,0                    ; Textspeicher leeren (Leerzeichen)
        mov     ax,0FC20h
        mov     cx,2000
        call    wfill
        mov     al,6Bh
        call    gcmd
        mov     al,0Dh
        call    gcmd
        mov     ah,02h                  ; Strg-Z: Konsole loescht und setzt Cursor
        mov     dl,1Ah
        int     21h
        ret
gr_off  ENDP

; ein Wort per RDAT lesen: DI = Adresse -> AX, CF=1 bei Zeitueberschreitung
rdword  PROC    NEAR
        push    bx
        push    cx
        mov     bl,0
        call    curs
        mov     al,4Ch
        call    gcmd
        mov     al,02h
        call    gpar
        mov     al,01h
        call    gpar
        mov     al,00h
        call    gpar
        mov     al,0A0h                 ; RDAT Wort
        call    gcmd
        call    rdfifo
        jc      rdw9
        mov     bl,al
        call    rdfifo
        jc      rdw9
        mov     ah,al
        mov     al,bl
        push    ax
rdw2:   mov     cx,200                  ; Rest leeren
rdw3:   in      al,P_STAT
        test    al,01h
        jnz     rdw4
        loop    rdw3
        pop     ax
        clc
        jmp     short rdw9
rdw4:   in      al,P_FIFO
        jmp     short rdw2
rdw9:   pop     cx
        pop     bx
        ret
rdword  ENDP

rdfifo  PROC    NEAR
        push    cx
        mov     cx,0
rdf1:   in      al,P_STAT
        test    al,01h
        jnz     rdf2
        loop    rdf1
        stc
        pop     cx
        ret
rdf2:   in      al,P_FIFO
        clc
        pop     cx
        ret
rdfifo  ENDP

; Farbe/Mono erkennen: gibt es eine eigene rote Ebene?
detect  PROC    NEAR
        mov     v_npl,3
        mov     di,0000h
        mov     ax,1234h
        mov     cx,1
        call    wfill
        mov     di,4000h
        mov     ax,0ABCDh
        call    wfill
        mov     di,4000h
        call    rdword
        jc      det_c
        cmp     ax,0ABCDh
        jne     det_m
        mov     di,0000h
        call    rdword
        jc      det_c
        cmp     ax,1234h
        jne     det_m
det_c:  mov     v_mono,0
        ret
det_m:  mov     v_mono,1
        ret
detect  ENDP

;=======================================================================
;  Hilfen fuer die Bilder
;=======================================================================

; Bild vorbereiten: Standardanzeige, alles loeschen
sreset  PROC    NEAR
        mov     v_vbase,0
        mov     v_lpat,0FFFFh
        mov     v_dmode,0
        mov     al,0
        mov     v_zg,al
        call    setdz
        mov     al,0
        call    cchar
        call    vsync
        call    stdarea
        call    cls
        ret
sreset  ENDP

; Jalousie zum Bildwechsel
wipe    PROC    NEAR
        push    cx
        push    dx
        mov     v_vbase,0
        mov     bx,0
wip1:   mov     dx,bx
wip2:   mov     cx,1
        mov     al,SCHWARZ
        call    hlines
        add     dx,8
        cmp     dx,400
        jb      wip2
        call    vsync
        inc     bx
        cmp     bx,8
        jb      wip1
        pop     dx
        pop     cx
        ret
wipe    ENDP

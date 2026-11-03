;=======================================================================
;  Bild 13: Verwobene Baender (BAENDER.INC, per INCLUDE in DMVDEMO)
;  Vier Farbbaender winden sich als Zopf um eine gedachte Achse, die
;  langsam hin und her schwenkt, und laufen nach unten aus dem Bild.
;  Hardware-Scrolling: der Bildspeicher ist ein Ring aus 408 Zeilen, zwei
;  GDC-Partitionen zeigen ihn ab Zeile b_T. Je Bildwechsel wird nur die
;  neue oberste Zeile gezeichnet (ausserhalb des sichtbaren Bereichs),
;  dann rueckt b_T eine Zeile nach oben. Hinten liegende Baender sind
;  dunkler gerastert, das vordere ueberdeckt - so entsteht das Verweben.
;  Am Ende laufen die Baender in vier Zeilen je Bildwechsel aus dem Bild.
;=======================================================================

RING    EQU     408             ; Zeilen im Ring (8 unsichtbar)
NRIB    EQU     4               ; Baender
DURB    EQU     900             ; neue Zeilen (~16 s bei einer je Bildwechsel)

; Baender: Farbe hell / dunkel
RCOL    db      GELB, ROT
        db      CYAN, BLAU
        db      MAGENTA, BLAU
        db      GRUEN, BLAU
PBIT    db      GRUEN, ROT, BLAU        ; Ebenen 0000h, 4000h, 8000h

b_fo    dw      0                       ; Auslaufen: Durchlaeufe
b_T     dw      0                       ; oberste sichtbare Ringzeile
b_w     dw      0                       ; Weltzeile (zaehlt die neuen Zeilen)
b_ph    dw      0                       ; Drehwinkel des Zopfs (8.8)
b_ps    dw      0                       ; Schwenk links/rechts
b_pr    dw      0                       ; Zopfradius
b_S     dw      0
b_R     dw      0
b_f0    dw      0
b_p50   db      0
b_pt    db      0
b_ca    db      0
b_cb    db      0
b_cnt   dw      0
b_b0    dw      0
b_m0    db      0
b_m1    db      0
b_val   db      0
b_pvok  db      0
        EVEN
b_pb    dw      0
        EVEN
b_lvl   dw      0
PV      db      32 dup (0)
b_wlo   dw      0                       ; Wortbereich dieser Zeile
b_whi   dw      0
LXMIN   db      RING dup (40)           ; je Ringzeile: zuletzt beschriebene Worte
LXMAX   db      RING dup (0)
ZDITH   dw      -1792,256,-768,1280,-1536,512,-512,1536
        EVEN
RX      dw      NRIB dup (0)
RZ      dw      NRIB dup (0)
RH      dw      NRIB dup (0)
ORD     dw      NRIB dup (0)
LBUF    db      3*80 dup (0)            ; Zeilenpuffer je Ebene, Bit 0 = links

;=======================================================================
;  Zeilenpuffer
;=======================================================================

; Ebenenbytes aller Farben fuer diese Zeile: PV[4*Farbe + Ebene]
; (Farbe: 0/FFh je Farbbit; Mono: Graustufenraster der Zeile b_w)
pvset   PROC    NEAR
        cmp     v_mono,0
        jne     pvs0
        cmp     b_pvok,0
        je      pvs0
        ret
pvs0:   mov     b_pvok,1
        push    ax
        push    bx
        push    cx
        push    si
        mov     bx,0
        mov     si,b_w
        and     si,7
pvs1:   cmp     v_mono,0
        jne     pvs3
        mov     cx,0
pvs2:   mov     ax,bx
        push    bx
        mov     bx,cx
        and     al,PBIT[bx]
        pop     bx
        jz      pvs2a
        mov     al,0FFh
pvs2a:  push    bx
        shl     bx,1
        shl     bx,1
        add     bx,cx
        mov     PV[bx],al
        pop     bx
        inc     cx
        cmp     cx,3
        jb      pvs2
        jmp     short pvs4
pvs3:   push    bx
        shl     bx,1
        shl     bx,1
        shl     bx,1
        mov     al,MONOPAT[bx+si]
        pop     bx
        push    bx
        shl     bx,1
        shl     bx,1
        mov     PV[bx],al
        pop     bx
pvs4:   inc     bx
        cmp     bx,8
        jb      pvs1
        pop     si
        pop     cx
        pop     bx
        pop     ax
        ret
pvset   ENDP

; Strecke CX..DX-1 (Punkte) in den Zeilenpuffer: Farbe b_ca wo das Muster
; b_pt 1 ist, sonst b_cb (Muster Bit n = Punkt x mod 8 = n)
span    PROC    NEAR
        cmp     cx,0
        jge     sp1
        xor     cx,cx
sp1:    cmp     dx,640
        jle     sp2
        mov     dx,640
sp2:    cmp     cx,dx
        jl      sp3
        ret
sp3:    push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        mov     si,cx                   ; erstes Byte und Maske
        shr     si,1
        shr     si,1
        shr     si,1
        and     cl,7
        mov     al,0FFh
        shl     al,cl
        mov     b_m0,al
        dec     dx                      ; letztes Byte und Maske
        mov     cl,dl
        shr     dx,1
        shr     dx,1
        shr     dx,1
        sub     dx,si
        mov     b_cnt,dx
        and     cl,7
        xor     cl,7
        mov     al,0FFh
        shr     al,cl
        mov     b_m1,al
        add     si,offset LBUF          ; SI = erstes Byte in Ebene 0
        mov     bl,b_ca                 ; BP -> Ebenenbytes Farbe A, DI -> B
        mov     bh,0
        shl     bx,1
        shl     bx,1
        lea     bp,PV[bx]
        mov     bl,b_cb
        mov     bh,0
        shl     bx,1
        shl     bx,1
        lea     di,PV[bx]
        mov     b_pb,di
        mov     cx,v_npl
        mov     dh,b_pt
spp:    mov     di,b_pb
        mov     ah,[di]                 ; B
        mov     al,ds:[bp]              ; A
        xor     al,ah                   ; B ^ ((A ^ B) and Muster)
        and     al,dh
        xor     al,ah
        mov     di,si
        mov     bx,b_cnt
        or      bx,bx
        jnz     sp_many
        mov     dl,b_m0                 ; nur ein Byte
        and     dl,b_m1
        mov     ah,al
        and     ah,dl
        not     dl
        and     dl,[di]
        or      dl,ah
        mov     [di],dl
        jmp     short spn
sp_many: mov    dl,b_m0                 ; erstes Byte
        mov     ah,al
        and     ah,dl
        not     dl
        and     dl,[di]
        or      dl,ah
        mov     [di],dl
        inc     di
        dec     bx
        jz      sp_last
spf:    mov     [di],al                 ; volle Bytes
        inc     di
        dec     bx
        jnz     spf
sp_last: mov    dl,b_m1                 ; letztes Byte
        mov     ah,al
        and     ah,dl
        not     dl
        and     dl,[di]
        or      dl,ah
        mov     [di],dl
spn:    add     si,80
        inc     bp
        inc     b_pb
        loop    spp
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
span    ENDP

; Strecke CX..DX-1 mit Farbe ca / cb und Muster pt (Register oder Wert)
SPANC   MACRO   ca,cb,pt
        push    ax
        mov     ah,cb
        mov     al,ca
        mov     b_ca,al
        mov     b_cb,ah
        mov     al,pt
        mov     b_pt,al
        pop     ax
        call    span
        ENDM

;=======================================================================
;  eine neue Zeile der Baender berechnen (Weltzeile b_w)
;=======================================================================

bline   PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        ; Puffer schwarz
        mov     di,offset LBUF
        mov     cx,120
        xor     ax,ax
        cld
        rep     stosw
        call    pvset
        ; Raster fuer diese Zeile
        mov     al,55h
        test    b_w,1
        jz      bl0
        mov     al,0AAh
bl0:    mov     b_p50,al
        ; Schwenk: S = 150 sin(ps)
        add     b_ps,45
        mov     al,byte ptr b_ps+1
        call    sincos
        mov     ax,140
        call    mulq
        mov     b_S,ax
        ; Radius: R = 90 + 40 sin(pr)
        add     b_pr,37
        mov     al,byte ptr b_pr+1
        call    sincos
        mov     ax,40
        call    mulq
        add     ax,90
        mov     b_R,ax
        ; Drehung des Zopfs
        add     b_ph,150
        ; vier Baender um 90 Grad versetzt
        mov     si,0
blr:    mov     ax,si
        mov     cl,5                    ; i*64 (si = 2i)
        shl     ax,cl
        add     al,byte ptr b_ph+1
        call    sincos                  ; BX = sin, CX = cos
        mov     RZ[si],cx
        mov     ax,b_R
        call    mulq
        add     ax,b_S
        add     ax,320
        mov     RX[si],ax
        mov     bx,cx
        mov     ax,8
        call    mulq
        add     ax,22
        mov     RH[si],ax
        mov     ORD[si],si
        add     si,2
        cmp     si,2*NRIB
        jb      blr
        ; Wortbereich der Zeile: alle Baender samt Rand
        mov     b_wlo,39
        mov     b_whi,0
        mov     si,0
blx:    mov     ax,RX[si]
        sub     ax,RH[si]
        call    xword
        cmp     ax,b_wlo
        jge     blx1
        mov     b_wlo,ax
blx1:   mov     ax,RX[si]
        add     ax,RH[si]
        call    xword
        cmp     ax,b_whi
        jle     blx2
        mov     b_whi,ax
blx2:   add     si,2
        cmp     si,2*NRIB
        jb      blx
        ; nach Tiefe sortieren (hinten zuerst)
        mov     cx,NRIB-1
bls1:   mov     si,0
        push    cx
bls2:   mov     bx,ORD[si]
        mov     di,ORD[si+2]
        mov     ax,RZ[bx]
        cmp     ax,RZ[di]
        jle     bls3
        mov     ORD[si],di
        mov     ORD[si+2],bx
bls3:   add     si,2
        loop    bls2
        pop     cx
        loop    bls1
        ; zeichnen
        mov     si,0
bld:    mov     bx,ORD[si]
        push    si
        call    ribbon
        pop     si
        add     si,2
        cmp     si,2*NRIB
        jb      bld
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
bline   ENDP

; Punkt AX -> Wort 0..39 (geklemmt)
xword   PROC    NEAR
        sar     ax,1
        sar     ax,1
        sar     ax,1
        sar     ax,1
        cmp     ax,0
        jge     xw1
        xor     ax,ax
xw1:    cmp     ax,39
        jle     xw2
        mov     ax,39
xw2:    ret
xword   ENDP

; Band BX (2i) in den Zeilenpuffer. Helligkeitsstufe 0..3 aus der Tiefe,
; zeilenweise gestreut (ZDITH), damit die Stufen weich ineinander laufen:
;   3 vorn:  Rand hell/dunkel gerastert, breite Mitte hell, Glanzlicht
;   2        Rand hell/dunkel, schmale Mitte hell
;   1        Rand dunkel/schwarz, Mitte hell/dunkel gerastert
;   0 hinten: Rand dunkel/schwarz, Mitte dunkel
ribbon  PROC    NEAR
        mov     ah,RCOL[bx]             ; hell
        mov     al,RCOL[bx+1]           ; dunkel
        mov     v_ca,ah
        mov     v_cb,al
        mov     di,RX[bx]               ; Mitte
        mov     si,RH[bx]               ; halbe Breite
        push    bx
        mov     bx,b_w
        and     bx,7
        shl     bx,1
        mov     ax,ZDITH[bx]
        pop     bx
        mov     cx,RZ[bx]
        sar     cx,1
        sar     cx,1
        add     ax,cx                   ; (beide durch 4 vorbereitet: kein Ueberlauf)
        add     ax,8192
        mov     cl,12
        sar     ax,cl                   ; Grenzen bei z = -16384, 0, 16384
        cmp     ax,0
        jge     rb0
        xor     ax,ax
rb0:    cmp     ax,3
        jle     rb1
        mov     ax,3
rb1:    mov     b_lvl,ax
        ; schwarzer Rand
        mov     cx,di
        sub     cx,si
        mov     dx,di
        add     dx,si
        SPANC   SCHWARZ,SCHWARZ,0FFh
        inc     cx
        dec     dx
        cmp     b_lvl,2
        jae     rb_front
        ; hinten
        mov     al,v_cb
        SPANC   al,SCHWARZ,b_p50
        mov     ax,si
        shr     ax,1
        mov     cx,di
        sub     cx,ax
        mov     dx,di
        add     dx,ax
        mov     al,v_ca
        mov     ah,v_cb
        cmp     b_lvl,0
        jne     rb_b1
        mov     al,ah
rb_b1:  SPANC   al,ah,b_p50
        ret
rb_front:
        mov     al,v_ca
        mov     ah,v_cb
        SPANC   al,ah,b_p50
        mov     ax,si                   ; Mitte: 2/3 h (Stufe 3) oder 1/2 h
        shr     ax,1
        cmp     b_lvl,3
        jne     rb_f1
        mov     ax,si
        shl     ax,1
        mov     bx,3
        cwd
        idiv    bx
rb_f1:  mov     cx,di
        sub     cx,ax
        mov     dx,di
        add     dx,ax
        mov     al,v_ca
        SPANC   al,al,0FFh
        cmp     b_lvl,3
        jne     rb_f9
        mov     ax,si                   ; Glanzlicht bei x - h/3
        mov     bx,3
        cwd
        idiv    bx
        mov     bx,di
        sub     bx,ax
        mov     cx,bx
        sub     cx,2
        mov     dx,bx
        add     dx,2
        mov     al,v_ca
        SPANC   WEISS,al,b_p50
        mov     cx,bx
        dec     cx
        mov     dx,bx
        inc     dx
        SPANC   WEISS,WEISS,0FFh
rb_f9:  ret
ribbon  ENDP

; Zeilenpuffer in Ringzeile AX schreiben (alle Ebenen). Geschrieben wird
; nur der Wortbereich, den die neue Zeile oder der alte Inhalt dieser
; Ringzeile belegt - der Rest ist schon schwarz.
wline   PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     si,ax
        mov     bl,LXMIN[si]            ; Vereinigung alt/neu
        mov     bh,0
        mov     cx,b_wlo
        mov     LXMIN[si],cl
        cmp     cx,bx
        jle     wl0
        mov     cx,bx
wl0:    mov     bl,LXMAX[si]
        mov     dx,b_whi
        mov     LXMAX[si],dl
        cmp     dx,bx
        jge     wl0a
        mov     dx,bx
wl0a:   sub     dx,cx
        jl      wl9                     ; nichts zu schreiben
        inc     dx
        shl     dx,1
        mov     b_cnt,dx                ; Bytes je Ebene
        mov     dx,40
        mul     dx
        add     ax,cx
        mov     dx,ax                   ; Wortadresse ohne Ebene
        mov     si,offset LBUF
        add     si,cx
        add     si,cx
        mov     bx,offset PLTAB
        mov     cx,v_npl
wl1:    push    cx
        push    si
        mov     di,dx
        add     di,[bx]
        call    wbegin
        mov     cx,b_cnt
        cld
wl2:    in      al,P_STAT               ; FIFO voll?
        test    al,02h
        jnz     wl2
        lodsb
        out     P_PARM,al
        loop    wl2
        pop     si
        add     si,80
        add     bx,3
        pop     cx
        loop    wl1
wl9:    pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
wline   ENDP

; Partitionen fuer den Ring ab Zeile b_T setzen (nach dem Bildwechsel)
ringar  PROC    NEAR
        mov     ax,RING
        sub     ax,b_T                  ; Zeilen bis zum Ringende
        cmp     ax,400
        jbe     ra1
        mov     ax,400
ra1:    mov     cx,ax
        push    cx
        mov     ax,b_T
        mov     dx,40
        mul     dx
        mov     bx,0
        mov     dl,1
        call    mkarea
        pop     ax
        mov     cx,400
        sub     cx,ax
        jnz     ra2
        mov     cx,400                  ; Partition 1 liegt dann unter dem Bild
ra2:    mov     ax,0
        mov     bx,1
        mov     dl,1
        call    mkarea
        call    vsync
        call    sendarea
        ret
ringar  ENDP

scrband PROC    NEAR
        mov     al,0Ch
        call    gcmd
        call    sreset
        mov     b_T,0
        mov     b_w,0
        mov     b_pvok,0
        mov     di,offset LXMIN         ; Ring ist schwarz
        mov     cx,RING
        mov     al,40
        cld
        rep     stosb
        mov     cx,RING
        mov     al,0
        rep     stosb
        call    ringar
        mov     al,0Dh
        call    gcmd
bd1:    mov     ax,b_T                  ; eine neue Zeile oben
        dec     ax
        jns     bd3
        mov     ax,RING-1
bd3:    mov     b_T,ax
        call    bline
        call    wline
        inc     b_w
        call    ringar
        test    b_w,7                   ; Tastatur nur jede 8. Zeile (DOS-Aufruf)
        jnz     bd4
        call    poll_key
        cmp     v_key,0
        jne     bd8
bd4:    cmp     b_w,DURB
        jb      bd1
        ; auslaufen: schwarze Zeilen, vier je Bildwechsel
        mov     di,offset LBUF
        mov     cx,120
        xor     ax,ax
        cld
        rep     stosw
        mov     b_wlo,39                ; leer
        mov     b_whi,0
        mov     b_fo,RING/4+1
bd5:    mov     cx,4
bd6:    mov     ax,b_T
        dec     ax
        jns     bd7
        mov     ax,RING-1
bd7:    mov     b_T,ax
        call    wline
        loop    bd6
        call    ringar              ; (veraendert AX..DX)
        dec     b_fo
        jnz     bd5
bd8:    mov     al,0Ch                  ; normale Partitionen (alles schwarz)
        call    gcmd
        call    sreset
        mov     al,0Dh
        call    gcmd
        ret
scrband ENDP

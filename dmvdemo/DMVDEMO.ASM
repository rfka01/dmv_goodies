;***********************************************************************
;*                                                                     *
;*  DMVDEMO  -  Grafikdemo fuer die NCR Decision Mate V                *
;*              MS-DOS 3.30, uPD7220 GDC, 640 x 400, 8 Farben          *
;*                                                                     *
;*  Bauen mit MASM 5.10:                                               *
;*      MASM DMVDEMO;                                                  *
;*      LINK DMVDEMO;          (Warnung "no stack segment" ist normal) *
;*      EXE2BIN DMVDEMO.EXE DMVDEMO.COM                                *
;*                                                                     *
;*  Aufruf:  DMVDEMO [n] [/M] [/C] [/6|/7|/8]                          *
;*      n   nur Bild n (1..13) zeigen                                  *
;*      /6  Bild 11: 68008 (K234), sonst 8087, sonst 8088 (experim.)   *
;*      /7  Bild 11: 8087, wenn vorhanden, sonst 8088 (Vorgabe)        *
;*      /8  Bild 11: nur 8088/V20                                      *
;*      /M  Mono erzwingen, /C Farbe erzwingen (sonst Erkennung)       *
;*  Taste = naechstes Bild, ESC = Ende                                 *
;*                                                                     *
;*  GDC-Grundlagen wie NCRs eigene Bibliothek NCRGRAPH (GINIT/GEXIT):  *
;*  Mischmodus, PRAM 00 00 00 59 = Grafik, 00 00 00 19 = Text,         *
;*  Ebenen gruen 0000h, rot 4000h, blau 8000h (je 40 Worte/Zeile),     *
;*  Farbbits wie NCR-GSX: rot=1 gruen=2 blau=4.                        *
;*  Mehrfarbig wird wie im NCR-Treiber DDDMV gezeichnet: jede Figur    *
;*  einmal je Ebene, mit SET/RESET/COMPLEMENT je nach Farbbit.         *
;*                                                                     *
;***********************************************************************

P_STAT   EQU     0A0h            ; GDC Status (lesen)
P_PARM   EQU     0A0h            ; GDC Parameter (schreiben)
P_CMD    EQU     0A1h            ; GDC Kommando (schreiben)
P_ZOOM   EQU     0A2h            ; ZOOM-Parameter: GDC (BA0=0) + Zoom-Latch der Karte
P_FIFO   EQU     0A1h            ; GDC FIFO (lesen)

SCHWARZ EQU     0
ROT     EQU     1
GRUEN   EQU     2
GELB    EQU     3
BLAU    EQU     4
MAGENTA EQU     5
CYAN    EQU     6
WEISS   EQU     7
BUNT    EQU     0FFh            ; Regenbogen je Zeichen

NSCREEN EQU     13
CALC87  EQU     1               ; 8087-Rechnung immer eingebaut
CALC68  EQU     1               ; 68008-Rechnung immer eingebaut (nur mit /6 benutzt)
MAXIT2  EQU     100             ; Spirale: Iterationen
GW      EQU     161             ; Plasma-Gitter
GH      EQU     101
OPT     EQU     16384           ; Tabellen hinter dem Programm (FREEMEM)
OCOLT   EQU     18432
OCV     EQU     19968
TRAIL   EQU     10              ; Schweiflaenge Mystify
MAXIT   EQU     32              ; Mandelbrot-Iterationen
NV      EQU     14              ; 3D-Punkte
NEDGE   EQU     24              ; 3D-Kanten

BAIL    MACRO   ziel
        LOCAL   weiter
        call    poll_key
        cmp     v_key,0
        je      weiter
        jmp     ziel
weiter:
        ENDM

        IFDEF   CALC87
        .8087
        ENDIF
CSEG    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  CS:CSEG,DS:CSEG,ES:CSEG,SS:CSEG
        ORG     100h

START:  jmp     main

;=======================================================================
;  Daten
;=======================================================================

SCRTAB  dw      scr1,scr2,scr3,scr4,scr5,scr6,scr7,scr8,scr9,scr10,scrsp,scrkitt,scrana

        INCLUDE GDCTAB.INC

T_NCR   db      'NCR',0
T_DMV   db      'DECISION MATE V',0
T_SPEC  db      '640 X 400 * 8 FARBEN * ',7Fh,'PD7220',0   ; 7Fh = mu
T_DEMO  db      'GRAFIK-DEMO 2026',0
T_BASED db      'EIN GRUSS AN DAS NCR GRAPHIC DEMO VON 1983',0
T_KEYS  db      'TASTE = WEITER    ESC = ENDE',0
T_TODO  db      'BILD IN ARBEIT',0


T2_TIT  db      'PLANNING CHART 83I',0
T2_SUB  db      'PRODUCTION SEQUENCE',0
T2_NOV  db      'NOVEMBER',0
T2_DEC  db      'DECEMBER',0
T2_JAN  db      'JANUARY',0
T2_DAYS db      '26',0,'27',0,'28',0,' 1',0,' 2',0,' 5',0,' 6',0,' 7',0
        db      ' 8',0,' 9',0,'12',0,'13',0,'14',0,'15',0,'16',0,'19',0
        db      '20',0,'21',0,' 2',0,' 3',0,' 4',0,' 5',0,' 6',0
T2_TASKS db     'PLANNING',0,'ORDERING',0,'FABRICATION',0,'SUB ASSEM.1',0
        db      'SUB ASSEM.2',0,'FINAL ASSEM',0,'TEST OUT',0
T2_CRED db      'NACH NCR DEMO5/DEMO7, 1983',0
T2_PART db      'PART.NR. 021-00102-99',0
T2_TODAY db     'HEUTE',0
; Spaltengrenzen der Tage: November 3 x 24 Punkte (Platz fuer NOVEMBER), sonst 17
DAYX    dw      200,224,248,272,289,306,323,340,357,374,391,408
        dw      425,442,459,476,493,510,527,544,561,578,595,612
BARCOL  db      GELB,CYAN,GRUEN,MAGENTA,ROT,BLAU,WEISS
BARS    db      0,4, 2,8, 5,13, 8,14, 10,16, 14,20, 18,23
T3_TXT  db      'HARDWARE-LINIEN IM XOR-MODUS',0
T4_TXT  db      'KREISE AUS DEM GDC - 8 OKTANTEN',0
T6_TIT  db      'DITHERING: AUS 8 WERDEN 36 FARBEN',0
T6_SUN  db      'SONNENUNTERGANG MIT 8 FARBEN UND RASTERN',0
SKY     db      BLAU,SCHWARZ
        dw      offset P25
        db      BLAU,SCHWARZ
        dw      offset P50
        db      BLAU,SCHWARZ
        dw      offset P75
        db      BLAU,BLAU
        dw      offset PSOLID
        db      MAGENTA,BLAU
        dw      offset P25
        db      MAGENTA,BLAU
        dw      offset P50
        db      MAGENTA,BLAU
        dw      offset P75
        db      MAGENTA,MAGENTA
        dw      offset PSOLID
        db      ROT,MAGENTA
        dw      offset P25
        db      ROT,MAGENTA
        dw      offset P50
        db      ROT,MAGENTA
        dw      offset P75
        db      ROT,ROT
        dw      offset PSOLID
        db      GELB,ROT
        dw      offset P25
        db      GELB,ROT
        dw      offset P50
        db      GELB,ROT
        dw      offset P75
        db      GELB,GELB
        dw      offset PSOLID

; Balkenprofil: Farbe A, Farbe B (0 schwarz, 1 Balken, 2 weiss), Muster, Hoehe
BANDS   db      1,0
        dw      offset P25
        db      3
        db      1,0
        dw      offset P50
        db      3
        db      1,0
        dw      offset P75
        db      4
        db      1,1
        dw      offset PSOLID
        db      6
        db      2,1
        dw      offset P25
        db      4
        db      2,1
        dw      offset P50
        db      4
        db      2,1
        dw      offset P75
        db      2
        db      2,1
        dw      offset P50
        db      4
        db      2,1
        dw      offset P25
        db      4
        db      1,1
        dw      offset PSOLID
        db      6
        db      1,0
        dw      offset P75
        db      4
        db      1,0
        dw      offset P50
        db      3
        db      1,0
        dw      offset P25
        db      3
        db      0FFh
T7_A    db      'HARDWARE-SCROLLING',0
T7_B    db      'NUR DIE STARTADRESSE WANDERT',0
T8_NEC  db      'NEC',0
T8_CHIP db      7Fh,'PD7220',0              ; 7Fh = mu
T8_GDC  db      'GRAPHIC DISPLAY CONTROLLER',0
T8_TXT  db      'HARDWARE-ZOOM + SCHWENK',0
; Zoomstufen: Anzeigezoom-1, Mitte Wort, Mitte Zeile, Schwenkradius
ZOOMS   db      1,10,100,40
        db      3,15,150,20
        db      7,17,175,10
        db      0FFh
T9_TXT  db      'MANDELBROT: DER 8088 RECHNET, DER GDC ZEICHNET',0
T9_T87  db      'MANDELBROT: DER 8087 RECHNET, DER GDC ZEICHNET',0
MPAL    db      BLAU,BLAU, BLAU,CYAN, CYAN,CYAN, CYAN,GRUEN
        db      GRUEN,GRUEN, GRUEN,GELB, GELB,GELB, GELB,ROT
        db      ROT,ROT, ROT,MAGENTA, MAGENTA,MAGENTA, MAGENTA,BLAU
MLUM    db      0,0,1,1,0,0,1,1
T10_A   db      'ANAGLYPHEN-3D',0
T10_B   db      'ROT/CYAN-BRILLE AUFSETZEN - DANN SCHWEBT DER WUERFEL',0
; Attribut (oberes Byte): Bit 0 halbe Helligkeit, Bit 2-4 Vordergrund RGB,
; Bit 5-7 Hintergrund RGB invertiert (E0h = schwarz). 6 Farben, volle
; Helligkeit (halbe Helligkeit wieder entfernt).
TATTR   db      0E0h+0Ch,0E0h+08h,0E0h+18h,0E0h+14h,0E0h+1Ch,0E0h+04h
NTATTR  EQU     6
TICK    db      '+++ NCR DECISION MATE V - GRAFIKDEMO 2026 +++ '
        db      'uPD7220: 640 X 400 PUNKTE, 8 FARBEN, LINIEN, KREISE, '
        db      'ZEICHEN UND FLAECHEN IN HARDWARE +++ DIESE LAUFSCHRIFT '
        db      'IST ECHTER TEXTMODUS IN EINER EIGENEN PARTITION, DER '
        db      'WUERFEL DARUEBER IST GRAFIK +++ ESC = ENDE, TASTE = '
        db      'NOCHMAL VON VORN +++ ',0
P3X     dw      -64, 64, 64,-64,-64, 64, 64,-64,  110,-110,   0,   0,   0,   0
P3Y     dw      -64,-64, 64, 64,-64,-64, 64, 64,    0,   0, 110,-110,   0,   0
P3Z     dw      -64,-64,-64,-64, 64, 64, 64, 64,    0,   0,   0,   0, 110,-110
EDG     db      0,1, 1,2, 2,3, 3,0, 4,5, 5,6, 6,7, 7,4, 0,4, 1,5, 2,6, 3,7
        db      8,10, 8,11, 8,12, 8,13, 9,10, 9,11, 9,12, 9,13
        db      10,12, 12,11, 11,13, 13,10

RING2   db      ROT,GELB,GRUEN,CYAN,BLAU,MAGENTA,ROT
B4      db      0,8,2,10, 12,4,14,6, 3,11,1,9, 15,7,13,5
B8      db      0,32,8,40,2,34,10,42, 48,16,56,24,50,18,58,26
        db      12,44,4,36,14,46,6,38, 60,28,52,20,62,30,54,22
        db      3,35,11,43,1,33,9,41, 51,19,59,27,49,17,57,25
        db      15,47,7,39,13,45,5,37, 63,31,55,23,61,29,53,21
T10_PL  db      'PLASMA: DER 8088 RECHNET, GLEICHVERTEILTER ZUFALL',0
T10_P87 db      'PLASMA: DER 8087 RECHNET, GAUSS-VERTEILTER ZUFALL',0

RINGM   db      SCHWARZ,BLAU,CYAN,WEISS,CYAN,BLAU,SCHWARZ
T12_TXT db      'SPIRALE BEI -0,7759+0,1362i - 16-BIT-FESTKOMMA, DER 8088 RECHNET',0

        IFDEF   CALC68
        INCLUDE M68.INC
        ENDIF
VEC68   db      8 dup (0)
IVTSAV  db      8 dup (0)
T_E88   db      'DER 8088 RECHNET MIT 16 BIT FESTKOMMA 3.13',0
T_E87   db      'DER 8087 RECHNET MIT 80 BIT GLEITKOMMA',0
T_E68   db      'DER 68008 (K234) RECHNET MIT 32 BIT FESTKOMMA 4.28',0
T_SECS  db      20 dup (0)              ; Zeit 'sss,hh SEKUNDEN'
T_LINE  db      90 dup (0)              ; Beschriftung oben
T_SEKU  db      ' SEKUNDEN',0
T_DASH  db      ' - ',0
v_lab   dw      0                       ; Beschriftung waehrend der Rechnung
v_hlo   dw      0                       ; Rechenzeit in 1/100 s
v_hhi   dw      0
T68_NO  db      'KEINE ANTWORT VOM 68008 (K234) - ERSATZRECHNUNG:',0
T_VAR87 db      '8087 gefunden - er rechnet Bild 9, 10 und 11',0Dh,0Ah,'$'
T_NO87  db      'Kein 8087 - Bild 9, 10 und 11 rechnet der 8088/V20',0Dh,0Ah,'$'
T_OFF87 db      '/8: Bild 9, 10 und 11 rechnet der 8088/V20',0Dh,0Ah,'$'
T_VAR68 db      '/6: Bild 11 rechnet der 68008 (K234), experimentell',0Dh,0Ah,'$'
T_R87   db      'Antwortet der 68008 nicht, rechnet der 8087',0Dh,0Ah,'$'
T_R88   db      'Antwortet der 68008 nicht, rechnet der 8088/V20',0Dh,0Ah,'$'
T68_MEM db      'Kein Speicher fuer den 68008-Block - Bild 11 rechnet der 8088',0Dh,0Ah,'$'
T68_RUN db      'Achtung: der 68008 rechnet noch - bitte die DMV neu starten!',0Dh,0Ah,'$'
        IFDEF   CALC87
FOUR87  dq      4.0
SCALE87 dq      0.0001220703125         ; 1/8192 (Bild 11, 3.13)
SCALE12 dq      0.000244140625          ; 1/4096 (Bild 9, 4.12)
ONE87   dq      1.0
MTWO87  dq      -2.0
GSC87   dq      0.000030517578125       ; 1/32768
HALF87  dq      0.5
v_g2    dq      0.0                     ; Plasma: zweite Normalzufallszahl
v_ghave db      0
v_gu    dw      0
v_gv    dw      0
v_gm    dw      0
v_gi    dw      0
v_sc87  dw      offset SCALE87          ; Skala fuer mandel87
        ENDIF

T_KFILE db      'KITT.PIC',0
T_KMISS db      'KITT.PIC FEHLT',0
T_KCRED db      'FOTO: POUDOU99, WIKIMEDIA COMMONS, CC BY-SA 4.0 (BEARBEITET)',0
KPLANE  dw      0000h,4000h,8000h
; Scanner: x0, y, Abstand, Breite, Hoehe (Punkte) - aus dem Foto abgelesen
KSCAN   dw      242,261,20,18,15
; Helligkeitsstufen 0..3: Farbe A, Farbe B, Muster
KSTUF   db      SCHWARZ,SCHWARZ
        dw      offset PSOLID
        db      ROT,SCHWARZ
        dw      offset P25
        db      ROT,SCHWARZ
        dw      offset P75
        db      WEISS,ROT
        dw      offset P25
KLEV    db      8 dup (0)

T_BANNER db     'DMVDEMO - Grafikdemo fuer die NCR Decision Mate V',0Dh,0Ah,'$'
T_FARBE db      'Farbbildschirm (8 Farben)',0Dh,0Ah,'$'
T_MONO  db      'Monochrom-Bildschirm',0Dh,0Ah,'$'
T_HELP  db      'DMVDEMO [n] [/M] [/C] [/6|/7|/8]   n = nur Bild 1..13, /M Mono, /C Farbe',0Dh,0Ah
        db      'Rechner fuer Bild 11: /6 68008 (K234) > 8087 > 8088, /7 8087 > 8088 (Vorgabe),',0Dh,0Ah
        db      '/8 nur 8088/V20',0Dh,0Ah
        db      'Taste = weiter, ESC = Ende',0Dh,0Ah,'$'

        INCLUDE GDCVAR.INC


        INCLUDE GDCLIB.INC

;=======================================================================
;  Bild 1: Titel mit Hardware-Zeichenzoom
;=======================================================================

scr1    PROC    NEAR
        call    sreset
        ; Rahmen aus sieben Rechtecken
        mov     v_dmode,0
        mov     cx,7
        mov     bx,2
s1r:    mov     v_x0,bx
        mov     v_y0,bx
        mov     ax,639
        sub     ax,bx
        mov     v_x1,ax
        mov     ax,399
        sub     ax,bx
        mov     v_y1,ax
        mov     al,cl
        mov     v_col,al
        call    rect
        inc     bx
        inc     bx
        loop    s1r
        BAIL    s1end
        ; "NCR" 12fach mit 3D-Extrusion
        mov     v_tz,12
        mov     cx,7
s1e:    mov     ax,cx
        shl     ax,1
        add     ax,168
        mov     v_tx,ax
        mov     ax,cx
        shl     ax,1
        add     ax,30
        mov     v_ty,ax
        mov     bx,cx
        mov     al,EXTCOL[bx]
        mov     v_col,al
        mov     si,offset T_NCR
        call    text
        BAIL    s1end
        loop    s1e
        mov     v_tx,168
        mov     v_ty,30
        mov     v_col,WEISS
        mov     si,offset T_NCR
        call    text
        ; DECISION MATE V, 4fach, Regenbogen mit Schatten
        mov     v_tz,4
        mov     v_ty,158
        mov     v_col,BUNT
        mov     v_shc,BLAU
        mov     v_sho,3
        mov     si,offset T_DMV
        call    ctext_s
        BAIL    s1end
        mov     v_tz,2
        mov     v_ty,222
        mov     v_col,CYAN
        mov     si,offset T_SPEC
        call    ctext
        mov     v_ty,262
        mov     v_col,GELB
        mov     v_shc,ROT
        mov     v_sho,2
        mov     si,offset T_DEMO
        call    ctext_s
        mov     v_tz,1
        mov     v_ty,300
        mov     v_col,WEISS
        mov     si,offset T_BASED
        call    ctext
        mov     v_ty,366
        mov     v_col,GRUEN
        mov     si,offset T_KEYS
        call    ctext
        BAIL    s1end
        ; Glanzlicht: weisser XOR-Streifen, dreimal hin (rechts) und zurueck (links)
        mov     v_x0,160
        mov     v_y0,28
        mov     v_w,16
        mov     v_h,112
        mov     v_ca,WEISS
        mov     v_kdir,4
        mov     cx,6                    ; 6 Wege = 3 x rechts + links
s1g0:   push    cx
s1g:    mov     v_fmode,1
        call    fill                    ; zeigen
        call    vsync
        call    fill                    ; wieder weg (XOR)
        mov     ax,v_x0
        add     ax,v_kdir
        mov     v_x0,ax
        cmp     ax,160
        jl      s1g1
        cmp     ax,464
        jle     s1g
s1g1:   mov     ax,v_kdir               ; Richtung umkehren, zurueck in den Bereich
        neg     ax
        mov     v_kdir,ax
        add     v_x0,ax
        call    poll_key
        pop     cx
        cmp     v_key,0
        jne     s1end
        loop    s1g0
        mov     v_fmode,0
        mov     cx,150
        call    wframes
s1end:  mov     v_fmode,0
        ret
scr1    ENDP

; zentrierter Text mit Schatten
ctext_s PROC    NEAR
        push    cx
        push    dx
        push    si
        mov     cx,0
cts1:   lodsb
        or      al,al
        jz      cts2
        inc     cx
        jmp     short cts1
cts2:   mov     ax,v_tz
        mul     cx
        mov     cl,3
        shl     ax,cl
        mov     cx,640
        sub     cx,ax
        shr     cx,1
        mov     v_tx,cx
        pop     si
        call    stext
        pop     dx
        pop     cx
        ret
ctext_s ENDP


;=======================================================================
;  Bild 2: Remake des NCR-Planungscharts von 1983 - jetzt in Farbe
;=======================================================================

scr2    PROC    NEAR
        call    sreset
        mov     v_tz,3
        mov     v_ty,8
        mov     v_col,GELB
        mov     v_shc,ROT
        mov     v_sho,2
        mov     si,offset T2_TIT
        call    ctext_s
        mov     v_tz,2
        mov     v_ty,42
        mov     v_col,CYAN
        mov     si,offset T2_SUB
        call    ctext
        BAIL    s2end
        ; Rahmen
        mov     v_dmode,0
        mov     v_col,WEISS
        mov     v_x0,196
        mov     v_y0,74
        mov     v_x1,626
        mov     v_y1,372
        call    rect
        ; punktierte Tageslinien (Linienmuster aus PRAM 8/9)
        mov     v_lpat,8888h
        mov     v_col,BLAU
        mov     bx,0
s2d:    mov     si,bx
        shl     si,1
        mov     ax,DAYX[si]
        mov     v_x0,ax
        mov     v_x1,ax
        mov     v_y0,110
        mov     v_y1,370
        call    line
        inc     bx
        cmp     bx,24
        jb      s2d
        mov     v_lpat,0FFFFh
        ; Monatsgrenzen
        mov     v_col,WEISS
        mov     v_y0,76
        mov     v_y1,370
        mov     ax,DAYX[3*2]            ; November/Dezember
        mov     v_x0,ax
        mov     v_x1,ax
        call    line
        mov     ax,DAYX[18*2]           ; Dezember/Januar
        mov     v_x0,ax
        mov     v_x1,ax
        call    line
        mov     v_x0,198
        mov     v_x1,624
        mov     v_y0,108
        mov     v_y1,108
        call    line
        ; Monate
        mov     v_tz,1
        mov     v_ty,80
        mov     v_col,GELB
        mov     v_tx,204
        mov     si,offset T2_NOV
        call    text
        mov     v_tx,368
        mov     si,offset T2_DEC
        call    text
        mov     v_tx,542
        mov     si,offset T2_JAN
        call    text
        ; Tage
        mov     v_col,WEISS
        mov     v_ty,96
        mov     si,offset T2_DAYS
        mov     bx,0
s2t:    push    si
        mov     si,bx
        shl     si,1
        mov     ax,DAYX[si+2]           ; Beschriftung mittig in der Spalte
        sub     ax,DAYX[si]
        sub     ax,16
        sar     ax,1
        add     ax,DAYX[si]
        mov     v_tx,ax
        pop     si
        call    text
s2t1:   lodsb
        or      al,al
        jnz     s2t1
        inc     bx
        cmp     bx,23
        jb      s2t
        BAIL    s2end
        ; Zeilen: Beschriftung und Trennlinien
        mov     bx,0
        mov     si,offset T2_TASKS
s2l:    mov     ax,bx
        mov     cl,36
        mul     cl
        add     ax,112
        mov     dx,ax
        mov     v_col,BLAU
        mov     v_x0,8
        mov     v_x1,624
        mov     v_y0,dx
        mov     v_y1,dx
        call    line
        mov     v_tz,2
        mov     v_tx,10
        add     dx,12
        mov     v_ty,dx
        mov     al,BARCOL[bx]
        mov     v_col,al
        call    text
s2l1:   lodsb
        or      al,al
        jnz     s2l1
        inc     bx
        cmp     bx,7
        jb      s2l
        mov     v_tz,1
        mov     v_col,GRUEN
        mov     v_tx,10
        mov     v_ty,384
        mov     si,offset T2_CRED
        call    text
        mov     v_col,WEISS
        mov     v_tx,450
        mov     si,offset T2_PART
        call    text
        BAIL    s2end
        ; Balken wachsen lassen (3D: hell oben, Farbe, Schatten unten)
        mov     bx,0
s2b:    mov     al,BARS[bx]
        mov     ah,0
        mov     si,ax
        shl     si,1
        mov     ax,DAYX[si]
        inc     ax
        mov     v_bx0,ax
        mov     al,BARS[bx+1]
        mov     ah,0
        mov     si,ax
        shl     si,1
        mov     ax,DAYX[si]
        dec     ax
        mov     v_bx1,ax
        push    bx
        shr     bx,1
        mov     al,BARCOL[bx]
        mov     v_bcol,al
        mov     ax,bx
        mov     cl,36
        mul     cl
        add     ax,120
        mov     v_by,ax
        pop     bx
        mov     dx,v_bx0
s2g:    mov     v_x0,dx
        mov     ax,v_bx1
        sub     ax,dx
        cmp     ax,6
        jbe     s2g1
        mov     ax,6
s2g1:   mov     v_w,ax
        mov     ax,v_by
        mov     v_y0,ax
        mov     v_h,5
        mov     al,WEISS
        mov     v_ca,al
        mov     al,v_bcol
        mov     v_cb,al
        mov     v_patp,offset P50
        call    fill
        add     v_y0,5
        mov     v_h,12
        mov     al,v_bcol
        call    fillc
        add     v_y0,12
        mov     v_h,5
        mov     al,v_bcol
        mov     v_ca,al
        mov     v_cb,SCHWARZ
        mov     v_patp,offset P50
        call    fill
        call    vsync
        BAIL    s2end
        add     dx,6
        cmp     dx,v_bx1
        jb      s2g
        add     bx,2
        cmp     bx,14
        jae     s2b9
        jmp     s2b
s2b9:   ; gestrichelte HEUTE-Linie (Muster F0F0h)
        mov     v_lpat,0F0F0h
        mov     v_col,ROT
        mov     ax,DAYX[12*2]           ; HEUTE
        mov     v_x0,ax
        mov     v_x1,ax
        mov     v_y0,110
        mov     v_y1,370
        call    line
        mov     v_lpat,0FFFFh
        mov     v_tz,1
        mov     v_col,ROT
        mov     v_tx,405
        mov     v_ty,62
        mov     si,offset T2_TODAY
        call    text
        mov     cx,300
        call    wframes
s2end:  mov     v_lpat,0FFFFh
        ret
scr2    ENDP

;=======================================================================
;  Bild 3: Linien-Moire - Hardwarelinien im XOR-Modus
;=======================================================================

; Faecher aus (v_fcx,v_fcy) zu allen Randpunkten im Abstand v_fst
fan     PROC    NEAR
        push    ax
        push    dx
        mov     v_fcnt,0
        mov     ax,0
fn1:    mov     dx,0
        call    fanl
        jc      fn9
        add     ax,v_fst
        cmp     ax,640
        jb      fn1
        mov     dx,0
fn2:    mov     ax,639
        call    fanl
        jc      fn9
        add     dx,v_fst
        cmp     dx,400
        jb      fn2
        mov     ax,639
fn3:    mov     dx,399
        call    fanl
        jc      fn9
        sub     ax,v_fst
        jns     fn3
        mov     dx,399
fn4:    mov     ax,0
        call    fanl
        jc      fn9
        sub     dx,v_fst
        jns     fn4
fn9:    pop     dx
        pop     ax
        ret
fan     ENDP

; eine Faecherlinie nach (AX,DX); CF=1 bei Tastendruck
fanl    PROC    NEAR
        push    ax
        push    bx
        mov     v_x1,ax
        mov     v_y1,dx
        mov     ax,v_fcx
        mov     v_x0,ax
        mov     ax,v_fcy
        mov     v_y0,ax
        mov     al,v_fcol
        cmp     al,BUNT
        jne     fnl1
        mov     ax,v_fcnt
        mov     bl,24
        div     bl
        mov     ah,0
        mov     bl,6
        div     bl
        mov     bl,ah
        mov     bh,0
        mov     al,RAINB[bx]
fnl1:   mov     v_col,al
        mov     v_dmode,1
        call    line
        inc     v_fcnt
        test    v_fcnt,7
        jnz     fnl2
        call    vsync
        call    poll_key
fnl2:   pop     bx
        pop     ax
        cmp     v_key,0
        je      fnl3
        stc
        ret
fnl3:   clc
        ret
fanl    ENDP

scr3    PROC    NEAR
        call    sreset
        mov     v_fcx,320
        mov     v_fcy,200
        mov     v_fst,4
        mov     v_fcol,BUNT
        call    fan
        BAIL    s3end
        mov     cx,50
        call    wframes
        mov     v_fcx,470
        mov     v_fcy,130
        mov     v_fst,5
        mov     v_fcol,WEISS
        call    fan
        BAIL    s3end
        mov     cx,50
        call    wframes
        mov     v_fcx,320
        mov     v_fcy,200
        mov     v_fst,4
        mov     v_fcol,BUNT
        call    fan                     ; erster Faecher verschwindet wieder
        BAIL    s3end
        mov     v_fcx,150
        mov     v_fcy,300
        mov     v_fst,3
        mov     v_fcol,GELB
        call    fan
        BAIL    s3end
        mov     cx,50
        call    wframes
        mov     v_dmode,0
        mov     v_tz,2
        mov     v_ty,376
        mov     v_col,WEISS
        mov     v_shc,SCHWARZ
        mov     v_sho,2
        mov     si,offset T3_TXT
        call    ctext_s
        mov     cx,200
        call    wframes
s3end:  mov     v_dmode,0
        ret
scr3    ENDP

;=======================================================================
;  Bild 4: Kreis-Interferenz - Kreisbogen-Hardware im XOR-Modus
;=======================================================================

scr4    PROC    NEAR
        call    sreset
        mov     v_dmode,1
        mov     bx,4
s4a:    mov     v_r,bx
        mov     v_cx,230
        mov     v_cy,200
        mov     v_col,GELB
        call    circle
        mov     v_cx,410
        mov     v_col,CYAN
        call    circle
        call    vsync
        BAIL    s4end
        add     bx,4
        cmp     bx,197
        jb      s4a
        mov     cx,40
        call    wframes
        mov     bx,6
s4b:    mov     v_r,bx
        mov     v_cx,320
        mov     v_cy,200
        mov     v_col,MAGENTA
        call    circle
        call    vsync
        BAIL    s4end
        add     bx,6
        cmp     bx,197
        jb      s4b
        mov     cx,40
        call    wframes
        ; gelbe Ringe von innen wieder entfernen
        mov     bx,4
s4c:    mov     v_r,bx
        mov     v_cx,230
        mov     v_cy,200
        mov     v_col,GELB
        call    circle
        call    vsync
        BAIL    s4end
        add     bx,4
        cmp     bx,197
        jb      s4c
        ; und in Blau zurueck, um 40 Punkte versetzt
        mov     bx,4
s4d:    mov     v_r,bx
        mov     v_cx,270
        mov     v_cy,200
        mov     v_col,BLAU
        call    circle
        call    vsync
        BAIL    s4end
        add     bx,4
        cmp     bx,197
        jb      s4d
        mov     v_dmode,0
        mov     v_tz,2
        mov     v_ty,380
        mov     v_col,WEISS
        mov     v_shc,SCHWARZ
        mov     v_sho,2
        mov     si,offset T4_TXT
        call    ctext_s
        mov     cx,250
        call    wframes
s4end:  mov     v_dmode,0
        ret
scr4    ENDP

;=======================================================================
;  Bild 5: Mystify - springende Linienzuege mit Schweif (XOR)
;=======================================================================


; Viereck aus Verlaufsplatz BX (0..TRAIL-1), Form DX (0/1)
quad    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    si
        push    di
        mov     ax,bx
        shl     ax,1
        add     ax,dx
        mov     si,ax
        mov     al,HC[si]
        mov     v_col,al
        mov     ax,bx                   ; Basis = (platz*8 + form*4) Worte
        shl     ax,1
        shl     ax,1
        shl     ax,1
        mov     si,dx
        shl     si,1
        shl     si,1
        add     si,ax
        shl     si,1                    ; Byteoffset
        mov     cx,0
q1:     mov     di,cx
        inc     di
        and     di,3
        shl     di,1
        add     di,si
        mov     bx,cx
        shl     bx,1
        add     bx,si
        mov     ax,HX[bx]
        mov     v_x0,ax
        mov     ax,HY[bx]
        mov     v_y0,ax
        mov     ax,HX[di]
        mov     v_x1,ax
        mov     ax,HY[di]
        mov     v_y1,ax
        call    line
        inc     cx
        cmp     cx,4
        jb      q1
        pop     di
        pop     si
        pop     cx
        pop     bx
        pop     ax
        ret
quad    ENDP

scr5    PROC    NEAR
        call    sreset
        mov     v_dmode,1
        ; Startpunkte und Geschwindigkeiten
        mov     si,0
s5i:    mov     bx,640
        call    rndn
        mov     MX[si],ax
        mov     bx,400
        call    rndn
        mov     MY[si],ax
        mov     bx,6
        call    rndn
        add     ax,3
        mov     MVX[si],ax
        call    rnd
        test    ax,1
        jz      s5i1
        neg     MVX[si]
s5i1:   mov     bx,5
        call    rndn
        add     ax,2
        mov     MVY[si],ax
        call    rnd
        test    ax,2
        jz      s5i2
        neg     MVY[si]
s5i2:   add     si,2
        cmp     si,16
        jb      s5i
        mov     v_mf,0
        mov     v_slot,0
s5f:    mov     bx,v_slot
        cmp     v_mf,TRAIL
        jb      s5f1
        mov     dx,0
        call    quad
        mov     dx,1
        call    quad
s5f1:   ; bewegen
        mov     si,0
s5m:    mov     ax,MX[si]
        add     ax,MVX[si]
        cmp     ax,0
        jl      s5m1
        cmp     ax,639
        jle     s5m2
s5m1:   neg     MVX[si]
        add     ax,MVX[si]
        add     ax,MVX[si]
s5m2:   mov     MX[si],ax
        mov     ax,MY[si]
        add     ax,MVY[si]
        cmp     ax,0
        jl      s5m3
        cmp     ax,399
        jle     s5m4
s5m3:   neg     MVY[si]
        add     ax,MVY[si]
        add     ax,MVY[si]
s5m4:   mov     MY[si],ax
        add     si,2
        cmp     si,16
        jb      s5m
        ; in Verlaufsplatz kopieren
        mov     ax,bx
        mov     cl,4
        shl     ax,cl                   ; platz*16 Bytes
        mov     di,ax
        mov     si,0
s5c:    mov     ax,MX[si]
        mov     HX[di],ax
        mov     ax,MY[si]
        mov     HY[di],ax
        add     si,2
        add     di,2
        cmp     si,16
        jb      s5c
        ; Farben
        mov     ax,v_mf
        mov     cl,5
        shr     ax,cl
        mov     dl,6
        div     dl
        mov     al,ah
        mov     ah,0
        mov     si,ax
        mov     al,RAINB[si]
        mov     di,bx
        shl     di,1
        mov     HC[di],al
        add     si,3
        cmp     si,6
        jb      s5c1
        sub     si,6
s5c1:   mov     al,RAINB[si]
        mov     HC[di+1],al
        mov     dx,0
        call    quad
        mov     dx,1
        call    quad
        call    vsync
        BAIL    s5end
        inc     bx
        cmp     bx,TRAIL
        jb      s5f2
        mov     bx,0
s5f2:   mov     v_slot,bx
        inc     v_mf
        cmp     v_mf,750
        jae     s5end
        jmp     s5f
s5end:  mov     v_dmode,0
        ret
scr5    ENDP

;=======================================================================
;  Bild 6: Dithering - aus 8 Farben werden 36, danach Sonnenuntergang
;=======================================================================

scr6    PROC    NEAR
        call    sreset
        mov     v_tz,2
        mov     v_ty,20
        mov     v_col,WEISS
        mov     si,offset T6_TIT
        call    ctext
        mov     v_patp,offset P50
        mov     v_fmode,0
        mov     bx,0                    ; Zeile j
s6r:    mov     dx,0                    ; Spalte i
s6c:    mov     ax,dx
        mov     cl,72
        mul     cl
        add     ax,32
        mov     v_x0,ax
        mov     ax,bx
        mov     cl,40
        mul     cl
        add     ax,64
        mov     v_y0,ax
        mov     v_w,64
        mov     v_h,36
        mov     v_ca,dl
        mov     v_cb,bl
        mov     v_patp,offset P50
        call    fill
        call    vsync
        BAIL    s6end
        inc     dx
        cmp     dx,8
        jb      s6c
        inc     bx
        cmp     bx,8
        jb      s6r
        mov     cx,250
        call    wframes
        cmp     v_key,0
        je      s6s
        jmp     s6end
s6s:    ; Sonnenuntergang
        call    cls
        mov     si,offset SKY
        mov     dx,0
s6k:    mov     v_x0,0
        mov     v_y0,dx
        mov     v_w,640
        mov     v_h,15
        mov     al,[si]
        mov     v_ca,al
        mov     al,[si+1]
        mov     v_cb,al
        mov     ax,[si+2]
        mov     v_patp,ax
        call    fill
        add     si,4
        add     dx,15
        cmp     dx,240
        jb      s6k
        BAIL    s6end
        ; Sterne
        mov     v_dmode,0
        mov     v_col,WEISS
        mov     cx,50
s6st:   mov     bx,640
        call    rndn
        mov     v_x0,ax
        mov     v_x1,ax
        mov     bx,70
        call    rndn
        mov     v_y0,ax
        mov     v_y1,ax
        call    line
        loop    s6st
        ; Sonne: Kreisflaeche aus waagrechten Streifen
        mov     v_ca,WEISS
        mov     v_cb,GELB
        mov     v_patp,offset P50
        mov     cx,0                    ; halbe Breite
        mov     bx,70                   ; dy
s6u:    mov     ax,bx
        mul     ax
        mov     di,4900
        sub     di,ax                   ; r^2 - dy^2
s6u1:   mov     ax,cx
        inc     ax
        mul     ax
        cmp     ax,di
        ja      s6u2
        inc     cx
        jmp     short s6u1
s6u2:   mov     ax,320
        sub     ax,cx
        mov     v_x0,ax
        mov     ax,cx
        shl     ax,1
        mov     v_w,ax
        mov     ax,236
        sub     ax,bx
        mov     v_y0,ax
        mov     v_h,1
        call    fill
        dec     bx
        jnz     s6u
        call    vsync
        BAIL    s6end
        ; Berge
        mov     v_ca,SCHWARZ
        mov     v_cb,BLAU
        mov     v_patp,offset P75
        mov     dx,30
        mov     cx,0
s6b:    mov     bx,13
        call    rndn
        sub     ax,6
        add     dx,ax
        cmp     dx,8
        jge     s6b1
        mov     dx,8
s6b1:   cmp     dx,80
        jle     s6b2
        mov     dx,80
s6b2:   mov     v_x0,cx
        mov     v_w,8
        mov     ax,240
        sub     ax,dx
        mov     v_y0,ax
        mov     v_h,dx
        call    fill
        add     cx,8
        cmp     cx,640
        jb      s6b
        ; Meer
        mov     v_x0,0
        mov     v_y0,240
        mov     v_w,640
        mov     v_h,160
        mov     v_ca,BLAU
        mov     v_cb,SCHWARZ
        mov     v_patp,offset P50
        call    fill
        call    vsync
        BAIL    s6end
        ; Spiegelung der Sonne
        mov     dx,243
s6w:    mov     ax,dx
        sub     ax,240
        shr     ax,1
        add     ax,40
        mov     cx,ax
        mov     bx,40
        call    rndn
        add     cx,ax                   ; Breite
        mov     v_w,cx
        mov     bx,21
        call    rndn
        sub     ax,10
        add     ax,320
        shr     cx,1
        sub     ax,cx
        mov     v_x0,ax
        mov     v_y0,dx
        mov     v_h,2
        mov     v_ca,GELB
        mov     v_cb,ROT
        mov     v_patp,offset P50
        cmp     dx,300
        jb      s6w1
        mov     v_ca,ROT
        mov     v_cb,BLAU
s6w1:   call    fill
        add     dx,6
        cmp     dx,396
        jb      s6w
        mov     v_tz,1
        mov     v_ty,388
        mov     v_col,WEISS
        mov     si,offset T6_SUN
        call    ctext
        mov     cx,300
        call    wframes
s6end:  mov     v_patp,offset PSOLID
        ret
scr6    ENDP

;=======================================================================
;  Bild 7: Endlos-Scroller ueber zwei Partitionen, Spiegel, Erdbeben
;=======================================================================

; AX = Zeile -> AX = Zeile*40
mul40   PROC    NEAR
        push    dx
        mov     dx,ax
        shl     ax,1
        shl     ax,1
        add     ax,dx
        shl     ax,1
        shl     ax,1
        shl     ax,1
        pop     dx
        ret
mul40   ENDP

; zwei Grafikpartitionen setzen: AX/CX = Start/Zeilen 0, BX/DX = Start/Zeilen 1
twoarea PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    bx
        push    dx
        mov     bx,0
        mov     dl,1
        call    mkarea
        pop     cx
        pop     ax
        mov     bx,1
        mov     dl,1
        call    mkarea
        call    vsync
        call    sendarea
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
twoarea ENDP

scr7    PROC    NEAR
        call    sreset
        ; acht Farbbalken zu je 50 Zeilen, mit Rastern schattiert
        mov     dx,0                    ; y
        mov     bx,0                    ; Balken
s7b:    push    bx
        mov     ax,bx
        mov     cl,6
        div     cl
        mov     bl,ah
        mov     bh,0
        mov     al,RAINB[bx]
        mov     v_bcol,al
        pop     bx
        mov     si,offset BANDS
s7n:    mov     al,[si]
        call    bandcol
        mov     v_ca,al
        mov     al,[si+1]
        call    bandcol
        mov     v_cb,al
        mov     ax,[si+2]
        mov     v_patp,ax
        mov     al,[si+4]
        mov     ah,0
        mov     v_h,ax
        mov     v_x0,0
        mov     v_w,640
        mov     v_y0,dx
        call    fill
        add     dx,v_h
        add     si,5
        cmp     byte ptr [si],0FFh
        jne     s7n
        BAIL    s7end
        inc     bx
        cmp     bx,8
        jb      s7b
        mov     v_tz,3
        mov     v_ty,96
        mov     v_col,SCHWARZ
        mov     v_shc,WEISS
        mov     v_sho,2
        mov     si,offset T7_A
        call    ctext_s
        mov     v_tz,2
        mov     v_ty,296
        mov     si,offset T7_B
        call    ctext_s
        mov     cx,50
        call    wframes
        cmp     v_key,0
        je      s7a0
        jmp     s7end
        ; Phase A: nahtloser Rundlauf (Partition 0 = Rest, Partition 1 = Anfang)
s7a0:   mov     v_sc,2
        mov     v_mf,0
s7a:    mov     ax,v_sc
        call    mul40
        mov     cx,400
        sub     cx,v_sc
        mov     bx,0
        mov     dx,v_sc
        call    twoarea
        call    poll_key
        cmp     v_key,0
        je      s7a1
        jmp     s7end
s7a1:   mov     ax,v_sc
        add     ax,2
        cmp     ax,400
        jb      s7a2
        mov     ax,2
s7a2:   mov     v_sc,ax
        inc     v_mf
        cmp     v_mf,400
        jb      s7a
        ; Phase B: oben aufwaerts, unten abwaerts - gleicher Speicher
        mov     v_sc,0
        mov     v_mf,0
s7s:    mov     ax,v_sc
        call    mul40
        mov     cx,200
        push    ax
        mov     ax,200
        sub     ax,v_sc
        call    mul40
        mov     bx,ax
        pop     ax
        mov     dx,200
        call    twoarea
        call    poll_key
        cmp     v_key,0
        je      s7s1
        jmp     s7end
s7s1:   mov     ax,v_sc
        add     ax,2
        cmp     ax,200
        jb      s7s2
        mov     ax,0
s7s2:   mov     v_sc,ax
        inc     v_mf
        cmp     v_mf,400
        jb      s7s
        ; Phase C: Erdbeben - Startadresse zittert
        mov     v_mf,150
s7q:    mov     ax,v_mf
        mov     cl,5
        shr     ax,cl
        inc     ax
        mov     bx,ax                   ; Staerke 1..5
        shl     bx,1
        inc     bx
        call    rndn                    ; 0..2*St
        sub     ax,v_mf
        add     ax,v_mf
        mov     cx,v_mf
        push    cx
        mov     cl,5
        shr     cx,cl
        inc     cx
        sub     ax,cx                   ; -St..St Zeilen
        pop     cx
        add     ax,8
        call    mul40
        push    ax
        mov     bx,3
        call    rndn
        pop     dx
        add     ax,dx
        dec     ax                      ; +-1 Wort (16 Punkte)
        mov     cx,400
        mov     bx,ax
        mov     dx,400
        call    twoarea
        call    poll_key
        cmp     v_key,0
        jne     s7end
        dec     v_mf
        jnz     s7q
        mov     ax,320
        mov     cx,400
        mov     bx,ax
        mov     dx,cx
        call    twoarea
        mov     cx,50
        call    wframes
s7end:  call    stdarea
        mov     v_patp,offset PSOLID
        ret
scr7    ENDP

; Bandfarbe: AL 0 = schwarz, 1 = Balkenfarbe, 2 = weiss
bandcol PROC    NEAR
        cmp     al,1
        jb      bc9
        je      bc1
        mov     al,WEISS
        ret
bc1:    mov     al,v_bcol
bc9:    ret
bandcol ENDP

;=======================================================================
;  Bild 8: Hardware-Lupe - Anzeige-Zoom und Schwenk ueber die Startadresse
;=======================================================================

scr8    PROC    NEAR
        call    sreset
        ; gepunktetes Raster
        mov     v_lpat,0AAAAh
        mov     v_col,BLAU
        mov     dx,16
s8h:    mov     v_x0,0
        mov     v_x1,639
        mov     v_y0,dx
        mov     v_y1,dx
        call    line
        add     dx,32
        cmp     dx,400
        jb      s8h
        mov     dx,16
s8v:    mov     v_x0,dx
        mov     v_x1,dx
        mov     v_y0,0
        mov     v_y1,399
        call    line
        add     dx,32
        cmp     dx,640
        jb      s8v
        mov     v_lpat,0FFFFh
        ; Kreise in den Ecken
        mov     bx,4
s8c:    mov     v_r,bx
        mov     ax,bx
        mov     cl,6
        div     cl
        mov     al,ah
        mov     ah,0
        mov     si,ax
        mov     al,RAINB[si]
        mov     v_col,al
        mov     v_cx,80
        mov     v_cy,80
        call    circle
        mov     v_cx,560
        mov     v_cy,320
        call    circle
        inc     bx
        inc     bx
        cmp     bx,70
        jb      s8c
        BAIL    s8end
        ; der Chip
        mov     v_x0,160
        mov     v_y0,120
        mov     v_w,320
        mov     v_h,160
        mov     v_ca,WEISS
        mov     v_cb,SCHWARZ
        mov     v_patp,offset P25
        call    fill
        mov     v_col,WEISS
        mov     v_x0,160
        mov     v_y0,120
        mov     v_x1,479
        mov     v_y1,279
        call    rect
        mov     bx,0
s8p:    mov     ax,bx
        mov     cl,26
        mul     cl
        add     ax,172
        mov     v_x0,ax
        mov     v_y0,102
        mov     v_w,10
        mov     v_h,18
        mov     al,GELB
        call    fillc
        mov     v_y0,280
        mov     al,GELB
        call    fillc
        inc     bx
        cmp     bx,12
        jb      s8p
        mov     v_cx,178
        mov     v_cy,200
        mov     v_r,10
        mov     v_col,WEISS
        call    circle
        mov     v_tz,3
        mov     v_tx,208
        mov     v_ty,134
        mov     v_col,WEISS
        mov     si,offset T8_NEC
        call    text
        mov     v_tz,4
        mov     v_ty,180
        mov     v_col,GELB
        mov     v_shc,ROT
        mov     v_sho,2
        mov     si,offset T8_CHIP
        call    ctext_s
        mov     v_tz,2
        mov     v_ty,236
        mov     v_col,CYAN
        mov     si,offset T8_GDC
        call    ctext
        mov     v_tz,2
        mov     v_ty,340
        mov     v_col,WEISS
        mov     v_shc,BLAU
        mov     si,offset T8_TXT
        call    ctext_s
        mov     cx,100
        call    wframes
        cmp     v_key,0
        je      s8z
        jmp     s8end
        ; Zoom 2, 4, 8 mit Kreisschwenk um die Chipmitte
s8z:    mov     si,offset ZOOMS
s8z1:   mov     al,[si]
        cmp     al,0FFh
        jne     s8z2
        jmp     s8z9
s8z2:   call    setdz
        mov     al,[si+1]
        mov     ah,0
        mov     v_zx,ax                 ; Mittelpunkt in Worten
        mov     al,[si+2]
        mov     v_zy,ax                 ; und Zeilen
        mov     al,[si+3]
        mov     v_zr,ax                 ; Schwenkradius (Zeilen)
        mov     v_mf,0
s8z3:   mov     ax,v_mf
        call    sincos                  ; BX = sin, CX = cos
        mov     ax,v_zr
        push    bx
        mov     bx,cx
        call    mulq
        pop     bx
        sar     ax,1
        sar     ax,1
        sar     ax,1
        add     ax,v_zx
        mov     di,ax                   ; Wortversatz
        mov     ax,v_zr
        call    mulq
        add     ax,v_zy
        call    mul40
        add     ax,di
        mov     cx,400
        mov     bx,ax
        mov     dx,400
        call    twoarea
        call    poll_key
        cmp     v_key,0
        jne     s8z8
        add     v_mf,2
        cmp     v_mf,512
        jb      s8z3
        add     si,4
        jmp     s8z1
s8z8:   jmp     s8end
s8z9:   mov     al,0
        call    setdz
        call    stdarea
        mov     cx,100
        call    wframes
s8end:  mov     al,0
        call    setdz
        call    stdarea
        mov     v_lpat,0FFFFh
        ret
scr8    ENDP

;=======================================================================
;  Bild 9: Mandelbrot 160 x 100 in 4x4-Zellen, 8088 rechnet, GDC zeichnet
;=======================================================================


; AX = AX*AX >> 12 (4.12 Festkomma), DX zerstoert
sq12    PROC    NEAR
        imul    ax
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        mov     ax,dx
        ret
sq12    ENDP

; Iterationen fuer (v_mr, v_mi) -> AX
mandel  PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        xor     si,si                   ; zr
        xor     di,di                   ; zi
        xor     cx,cx                   ; n
man1:   mov     ax,si
        or      ax,ax
        jns     man2
        neg     ax
man2:   cmp     ax,8192
        jae     man9
        mov     ax,di
        or      ax,ax
        jns     man3
        neg     ax
man3:   cmp     ax,8192
        jae     man9
        mov     ax,si
        call    sq12
        mov     bx,ax                   ; zr^2
        mov     ax,di
        call    sq12                    ; zi^2
        mov     dx,bx
        add     dx,ax
        cmp     dx,16384
        ja      man9
        sub     bx,ax                   ; zr^2 - zi^2
        mov     ax,si
        imul    di                      ; zr*zi, 2*..>>12 = >>11
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        add     dx,v_mi
        mov     di,dx
        add     bx,v_mr
        mov     si,bx
        inc     cx
        cmp     cx,MAXIT
        jb      man1
man9:   mov     ax,cx
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
mandel  ENDP

scr9    PROC    NEAR
        call    sreset
        IFDEF   CALC87
        mov     v_maxit,MAXIT
        mov     v_sc87,offset SCALE12
        ENDIF
        mov     si,offset T9_TXT        ; wer rechnet - gleich oben zeigen
        IFDEF   CALC87
        cmp     v_has87,0
        je      s9a
        mov     si,offset T9_T87
s9a:
        ENDIF
        mov     v_lab,si
        call    toplab0
        call    dostime
        mov     v_t0lo,ax
        mov     v_t0hi,dx
        mov     v_mj,0
        mov     ax,-4915
        mov     v_mi,ax
s9r:    ; Zeile rechnen: je Zelle Farbe A/B
        mov     ax,-9011
        mov     v_mr,ax
        mov     bx,0
s9c:
        IFDEF   CALC87
        cmp     v_has87,0
        je      s9c0
        call    mandel87
        jmp     short s9c0a
s9c0:
        ENDIF
        call    mandel
s9c0a:  cmp     ax,MAXIT
        jb      s9c1
        mov     ax,0
        mov     dx,0
        jmp     short s9c2
s9c1:   mov     cl,12
        div     cl
        mov     al,ah
        mov     ah,0
        shl     ax,1
        mov     si,ax
        mov     ax,word ptr MPAL[si]
        mov     dx,ax
s9c2:   mov     MCA[bx],al
        mov     MCB[bx],ah
        add     v_mr,82
        inc     bx
        cmp     bx,160
        jb      s9c
        ; vier Pixelzeilen je Ebene schreiben
        mov     si,offset PLTAB
        mov     cx,v_npl
s9p:    push    cx
        mov     ax,0                    ; Paritaet
s9q:    push    ax
        mov     v_mpar,al
        mov     bl,[si+2]
        call    mrow                    ; ROWB fuellen
        ; zwei Zeilen dieser Paritaet
        mov     ax,v_mj
        shl     ax,1
        shl     ax,1
        pop     cx
        push    cx
        add     ax,cx
        call    mrwrite
        add     ax,2
        call    mrwrite
        pop     ax
        inc     ax
        cmp     ax,2
        jb      s9q
        add     si,3
        pop     cx
        loop    s9p
        call    relab
        call    poll_key
        cmp     v_key,0
        je      s9n
        jmp     s9end
s9n:    add     v_mi,98
        inc     v_mj
        cmp     v_mj,100
        jae     s9d
        jmp     s9r
s9d:    call    elapsed
        mov     si,v_lab
        call    toplab
        mov     cx,300
        call    wframes
s9end:  mov     v_lab,0
        ret
scr9    ENDP

; ROWB (40 Worte) fuer Ebenenbit BL und Paritaet v_mpar aus MCA/MCB bauen
mrow    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     v_mpb,bl
        mov     di,0                    ; Zelle
        mov     si,0                    ; Wortindex*2
mro1:   mov     dx,0                    ; Wort
        mov     cl,0                    ; Schiebung
mro2:   mov     al,MCA[di]
        call    mon
        mov     ah,al                   ; A 0/1
        mov     al,MCB[di]
        call    mon                     ; B 0/1
        ; Nibble: gerade Paritaet A auf Bit 0,2 und B auf 1,3
        cmp     v_mpar,0
        je      mro3
        xchg    al,ah
mro3:   mov     bl,0
        or      ah,ah
        jz      mro4
        or      bl,5
mro4:   or      al,al
        jz      mro5
        or      bl,0Ah
mro5:   mov     bh,0
        shl     bx,cl
        or      dx,bx
        inc     di
        add     cl,4
        cmp     cl,16
        jb      mro2
        mov     ROWB[si],dx
        add     si,2
        cmp     si,80
        jb      mro1
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
mrow    ENDP

; Farbe AL an in Ebene v_mpb? -> AL 0/1  (Mono: helle Farben an)
mon     PROC    NEAR
        cmp     v_mono,0
        jne     mon2
        and     al,v_mpb
        jz      mon1
        mov     al,1
mon1:   ret
mon2:   push    bx
        mov     bl,al
        mov     bh,0
        mov     al,MLUM[bx]
        pop     bx
        ret
mon     ENDP

; ROWB als Pixelzeile AX schreiben (Ebene [SI])
mrwrite PROC    NEAR
        push    ax
        push    cx
        push    dx
        push    di
        push    bx
        call    mul40
        add     ax,[si]
        mov     di,ax
        call    wbegin
        mov     bx,0
mrw1:   mov     ax,ROWB[bx]
        call    gpw
        add     bx,2
        cmp     bx,80
        jb      mrw1
        pop     bx
        pop     di
        pop     dx
        pop     cx
        pop     ax
        ret
mrwrite ENDP

;=======================================================================
;  Bild 13 (immer zuletzt): Finale - Anaglyphen-3D ueber Hardware-Text-Laufschrift
;=======================================================================


; Punkte drehen und projizieren -> NLX, NRX, NY
proj    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     si,0
prj1:   mov     al,v_ang1
        call    sincos
        mov     v_sa,bx
        mov     v_ca_,cx
        ; x1 = x*cos + z*sin ; z1 = z*cos - x*sin
        mov     ax,P3X[si]
        mov     bx,v_ca_
        call    mulq
        mov     v_t1,ax
        mov     ax,P3Z[si]
        mov     bx,v_sa
        call    mulq
        add     ax,v_t1
        mov     v_x1r,ax
        mov     ax,P3Z[si]
        mov     bx,v_ca_
        call    mulq
        mov     v_t1,ax
        mov     ax,P3X[si]
        mov     bx,v_sa
        call    mulq
        sub     v_t1,ax
        ; y1 = y*cos b - z1*sin b ; z2 = y*sin b + z1*cos b
        mov     al,v_ang2
        call    sincos
        mov     v_sa,bx
        mov     v_ca_,cx
        mov     ax,P3Y[si]
        mov     bx,v_ca_
        call    mulq
        mov     v_t2,ax
        mov     ax,v_t1
        mov     bx,v_sa
        call    mulq
        sub     v_t2,ax                 ; y1
        mov     ax,P3Y[si]
        mov     bx,v_sa
        call    mulq
        mov     v_t3,ax
        mov     ax,v_t1
        mov     bx,v_ca_
        call    mulq
        add     ax,v_t3                 ; z2
        mov     v_t3,ax
        ; Projektion: Nenner z2+320
        add     ax,320
        mov     cx,ax
        mov     ax,v_x1r
        mov     bx,256
        imul    bx
        idiv    cx
        add     ax,320
        mov     dx,v_t3
        sar     dx,1
        sar     dx,1
        sar     dx,1
        sar     dx,1                    ; Parallaxe z2/16
        mov     bx,ax
        sub     bx,dx
        mov     NLX[si],bx
        add     ax,dx
        mov     NRX[si],ax
        mov     ax,v_t2
        mov     bx,213
        imul    bx
        idiv    cx
        add     ax,200
        mov     NY[si],ax
        add     si,2
        cmp     si,NV*2
        jae     prj9
        jmp     prj1
prj9:   pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
proj    ENDP

; alle Kanten zeichnen: BX -> x-Tabelle, DI -> y-Tabelle
edges   PROC    NEAR
        push    ax
        push    cx
        push    si
        push    bp
        mov     bp,di
        mov     si,0
edg1:   push    si
        mov     al,EDG[si]
        mov     ah,0
        shl     ax,1
        mov     si,ax
        mov     ax,[bx+si]
        mov     v_x0,ax
        mov     ax,[bp+si]
        mov     v_y0,ax
        pop     si
        push    si
        mov     al,EDG[si+1]
        mov     ah,0
        shl     ax,1
        mov     si,ax
        mov     ax,[bx+si]
        mov     v_x1,ax
        mov     ax,[bp+si]
        mov     v_y1,ax
        pop     si
        call    line
        add     si,2
        cmp     si,NEDGE*2
        jb      edg1
        pop     bp
        pop     si
        pop     cx
        pop     ax
        ret
edges   ENDP

scrana  PROC    NEAR
        call    sreset
        mov     al,0Fh                  ; 16 Rasterzeilen je Textzeile, kein Cursor
        call    cchar
        ; Laufschrift in die ersten 10 Zeilen der gruenen Ebene
        mov     di,0
        call    wbegin
        mov     si,offset TICK
        mov     bx,0
        mov     v_tlen,0
s10t:   lodsb
        or      al,al
        jz      s10t2
        call    tchar
        inc     v_tlen
        jmp     short s10t
s10t2:  mov     si,offset TICK          ; die ersten 80 Zeichen noch einmal
        mov     bx,0                    ; mit denselben Attributen
        mov     cx,80
s10t3:  lodsb
        call    tchar
        loop    s10t3
        ; Grafik ab Zeile 10
        mov     v_vbase,400
        mov     v_dmode,0
        mov     v_col,BLAU
        mov     v_x0,0
        mov     v_y0,0
        mov     v_x1,639
        mov     v_y1,383
        call    rect
        mov     v_tz,3
        mov     v_ty,8
        mov     v_col,WEISS
        mov     v_shc,BLAU
        mov     v_sho,2
        mov     si,offset T10_A
        call    ctext_s
        mov     v_tz,1
        mov     v_ty,366
        mov     v_col,GELB
        mov     si,offset T10_B
        call    ctext
        mov     v_tpos,0
        mov     v_mf,0
        mov     v_ang1,0
        mov     v_ang2,0
        call    proj
        call    s10ar
s10f:   ; neue Lage rechnen, alte Linien loeschen, neue zeichnen
        call    s10cp                   ; N -> O
        add     v_ang1,2
        add     v_ang2,1
        call    proj
        call    vsync
        call    s10ar
        mov     v_dmode,2
        mov     bx,offset OLX
        mov     di,offset OY
        mov     v_col,ROT
        call    edges
        cmp     v_mono,0
        jne     s10f1
        mov     bx,offset ORX
        mov     v_col,CYAN
        call    edges
s10f1:  mov     v_dmode,3
        mov     bx,offset NLX
        mov     di,offset NY
        mov     v_col,ROT
        call    edges
        cmp     v_mono,0
        jne     s10f2
        mov     bx,offset NRX
        mov     v_col,CYAN
        call    edges
s10f2:  call    poll_key
        cmp     v_key,0
        jne     s10end
        inc     v_mf
        test    v_mf,1
        jnz     s10f3
        inc     v_tpos
        mov     ax,v_tpos
        cmp     ax,v_tlen
        jb      s10f3
        mov     v_tpos,0
s10f3:  cmp     v_mf,1500
        jae     s10end
        jmp     s10f
s10end: mov     v_dmode,0
        mov     v_vbase,0
        mov     al,0
        call    cchar
        call    stdarea
        ret
scrana  ENDP

; Zeichen AL mit Regenbogenattribut als Textwort senden (BX = Zaehler):
tchar   PROC    NEAR
        push    bx
        push    dx
        mov     dl,al
        mov     ax,bx
        mov     bl,NTATTR
        div     bl                      ; AH = Zaehler mod NTATTR
        mov     bl,ah
        mov     bh,0
        mov     ah,TATTR[bx]
        mov     al,dl
        call    gpw
        pop     dx
        pop     bx
        inc     bx
        ret
tchar   ENDP

; Partitionen fuer Bild 10: Grafik ab Zeile 10 (384 Zeilen), Textzeile v_tpos
s10ar   PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bx,0
        mov     ax,400
        mov     cx,384
        mov     dl,1
        call    mkarea
        mov     bx,1
        mov     ax,v_tpos
        mov     cx,16
        mov     dl,0
        call    mkarea
        call    sendarea
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
s10ar   ENDP

; neue Punkte als alte merken
s10cp   PROC    NEAR
        push    cx
        push    si
        push    di
        mov     si,offset NLX
        mov     di,offset OLX
        mov     cx,NV*3
        rep     movsw
        pop     di
        pop     si
        pop     cx
        ret
s10cp   ENDP

;=======================================================================
;  Bild 10: Plasma wie FRACTINT - Mittelpunktverschiebung, Farbring
;           mit 4x4-Bayer-Rastern, Farbring dreht sich weiter
;=======================================================================

; CX = x, DX = y  ->  BX = Gitteroffset (y*161 + x)
gaddr   PROC    NEAR
        push    ax
        push    cx
        mov     bx,dx
        mov     ax,dx
        mov     cl,5
        shl     ax,cl                   ; y*32
        add     bx,ax
        shl     ax,1
        shl     ax,1                    ; y*128
        add     bx,ax
        pop     cx
        add     bx,cx
        pop     ax
        ret
gaddr   ENDP

; Randpunkt (P_MX,P_MY) aus (P_AX,P_AY) und (P_BX,P_BY) setzen, falls leer
adjust  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     cx,P_MX
        mov     dx,P_MY
        call    gaddr
        cmp     FREEMEM[bx],0
        jne     adj9
        mov     si,bx
        mov     cx,P_AX
        mov     dx,P_AY
        call    gaddr
        mov     al,FREEMEM[bx]
        mov     ah,0
        push    ax
        mov     cx,P_BX
        mov     dx,P_BY
        call    gaddr
        mov     al,FREEMEM[bx]
        mov     ah,0
        pop     dx
        add     ax,dx
        inc     ax
        shr     ax,1                    ; Mittelwert
        push    ax
        mov     ax,P_AX                 ; d = |ax-bx| + |ay-by|
        sub     ax,P_BX
        jns     adj1
        neg     ax
adj1:   mov     dx,P_AY
        sub     dx,P_BY
        jns     adj2
        neg     dx
adj2:   add     ax,dx
        IFDEF   CALC87
        cmp     v_has87,0
        je      adj3
        call    rdisp87
        jmp     short adj4
adj3:
        ENDIF
        call    rdisp
adj4:   pop     dx
        add     ax,dx
        call    clamp
        mov     FREEMEM[si],al
adj9:   pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
adjust  ENDP

; AX = Abstand d -> AX = Zufall -m..m mit m = d*5/8 (Koernung)
rdisp   PROC    NEAR
        push    bx
        push    cx
        push    dx
        mov     cx,ax
        shl     ax,1
        shl     ax,1
        add     ax,cx
        mov     cl,3
        shr     ax,cl                   ; m
        or      ax,ax
        jz      rds9
        mov     cx,ax
        shl     ax,1
        inc     ax
        mov     bx,ax
        call    rndn
        sub     ax,cx
rds9:   pop     dx
        pop     cx
        pop     bx
        ret
rdisp   ENDP

        IFDEF   CALC87
; wie rdisp, aber normalverteilt (8087, Polarmethode nach Marsaglia):
; AX = Abstand d -> AX = round(g * m/2), g ~ N(0,1), m = d*5/8
rdisp87 PROC    NEAR
        push    bx
        push    cx
        push    dx
        mov     cx,ax
        shl     ax,1
        shl     ax,1
        add     ax,cx
        mov     cl,3
        shr     ax,cl                   ; m
        or      ax,ax
        jnz     rg0
        jmp     rg9
rg0:    mov     v_gm,ax
        cmp     v_ghave,0
        je      rg1
        mov     v_ghave,0
        fld     v_g2                    ; zweite Zahl vom letzten Paar
        jmp     rg5
rg1:    call    rnd                     ; u, v gleichverteilt in [-1, 1)
        sub     ax,8000h
        mov     v_gu,ax
        call    rnd
        sub     ax,8000h
        mov     v_gv,ax
        fild    v_gu
        fmul    GSC87                   ; u
        fild    v_gv
        fmul    GSC87                   ; v u
        fld     st(1)
        fmul    st,st(0)                ; u^2 v u
        fld     st(1)
        fmul    st,st(0)                ; v^2 u^2 v u
        faddp   st(1),st                ; s v u
        fcom    ONE87
        fstsw   v_sw
        fwait
        mov     ah,byte ptr v_sw+1
        sahf
        jb      rg2
rg8:    fstp    st(0)                   ; s >= 1 oder s = 0: verwerfen
        fstp    st(0)
        fstp    st(0)
        jmp     rg1
rg2:    ftst
        fstsw   v_sw
        fwait
        mov     ah,byte ptr v_sw+1
        sahf
        je      rg8
        fld     st(0)                   ; s s v u
        fldln2                          ; ln2 s s v u
        fxch    st(1)                   ; s ln2 s v u
        fyl2x                           ; ln s, s, v, u
        fdiv    st,st(1)                ; ln s / s
        fstp    st(1)                   ; ln s / s, v, u
        fmul    MTWO87
        fsqrt                           ; f v u
        fmul    st(1),st                ; f  v*f  u
        fmulp   st(2),st                ; v*f  u*f
        fstp    v_g2                    ; u*f
        mov     v_ghave,1
rg5:    fild    v_gm
        fmulp   st(1),st                ; g*m
        fmul    HALF87
        fistp   v_gi
        fwait
        mov     ax,v_gi
rg9:    pop     dx
        pop     cx
        pop     bx
        ret
rdisp87 ENDP
        ENDIF

; AX auf 1..255 begrenzen -> AL
clamp   PROC    NEAR
        cmp     ax,1
        jge     clp1
        mov     ax,1
clp1:   cmp     ax,255
        jle     clp2
        mov     ax,255
clp2:   ret
clamp   ENDP

; rekursive Unterteilung: Stapel x1,y1,x2,y2 (x1 zuerst gepusht)
pdiv    PROC    NEAR
        push    bp
        mov     bp,sp
        sub     sp,4
        mov     ax,[bp+6]
        sub     ax,[bp+10]
        cmp     ax,2
        jae     pdv1
        mov     ax,[bp+4]
        sub     ax,[bp+8]
        cmp     ax,2
        jae     pdv1
        jmp     pdv9
pdv1:   mov     ax,[bp+10]
        add     ax,[bp+6]
        shr     ax,1
        mov     [bp-2],ax               ; x
        mov     ax,[bp+8]
        add     ax,[bp+4]
        shr     ax,1
        mov     [bp-4],ax               ; y
        ; oben: (x1,y1)-(x,y1)-(x2,y1)
        mov     ax,[bp+10]
        mov     P_AX,ax
        mov     ax,[bp+8]
        mov     P_AY,ax
        mov     P_MY,ax
        mov     P_BY,ax
        mov     ax,[bp-2]
        mov     P_MX,ax
        mov     ax,[bp+6]
        mov     P_BX,ax
        call    adjust
        ; rechts: (x2,y1)-(x2,y)-(x2,y2)
        mov     ax,[bp+6]
        mov     P_AX,ax
        mov     P_MX,ax
        mov     P_BX,ax
        mov     ax,[bp+8]
        mov     P_AY,ax
        mov     ax,[bp-4]
        mov     P_MY,ax
        mov     ax,[bp+4]
        mov     P_BY,ax
        call    adjust
        ; unten: (x1,y2)-(x,y2)-(x2,y2)
        mov     ax,[bp+10]
        mov     P_AX,ax
        mov     ax,[bp+4]
        mov     P_AY,ax
        mov     P_MY,ax
        mov     P_BY,ax
        mov     ax,[bp-2]
        mov     P_MX,ax
        mov     ax,[bp+6]
        mov     P_BX,ax
        call    adjust
        ; links: (x1,y1)-(x1,y)-(x1,y2)
        mov     ax,[bp+10]
        mov     P_AX,ax
        mov     P_MX,ax
        mov     P_BX,ax
        mov     ax,[bp+8]
        mov     P_AY,ax
        mov     ax,[bp-4]
        mov     P_MY,ax
        mov     ax,[bp+4]
        mov     P_BY,ax
        call    adjust
        ; Mitte = Mittel der vier Ecken
        mov     cx,[bp-2]
        mov     dx,[bp-4]
        call    gaddr
        cmp     FREEMEM[bx],0
        jne     pdv2
        mov     si,bx
        mov     di,2
        mov     cx,[bp+10]
        mov     dx,[bp+8]
        call    gaddr
        mov     al,FREEMEM[bx]
        mov     ah,0
        add     di,ax
        mov     cx,[bp+6]
        call    gaddr
        mov     al,FREEMEM[bx]
        add     di,ax
        mov     dx,[bp+4]
        call    gaddr
        mov     al,FREEMEM[bx]
        add     di,ax
        mov     cx,[bp+10]
        call    gaddr
        mov     al,FREEMEM[bx]
        add     di,ax
        shr     di,1
        shr     di,1
        mov     ax,di
        call    clamp
        mov     FREEMEM[si],al
pdv2:   ; vier Viertel
        push    word ptr [bp+10]
        push    word ptr [bp+8]
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        call    pdiv
        push    word ptr [bp-2]
        push    word ptr [bp+8]
        push    word ptr [bp+6]
        push    word ptr [bp-4]
        call    pdiv
        push    word ptr [bp-2]
        push    word ptr [bp-4]
        push    word ptr [bp+6]
        push    word ptr [bp+4]
        call    pdiv
        push    word ptr [bp+10]
        push    word ptr [bp-4]
        push    word ptr [bp-2]
        push    word ptr [bp+4]
        call    pdiv
pdv9:   mov     sp,bp
        pop     bp
        ret     8
pdiv    ENDP

; Farbtabellen fuer den Ring bauen (Drehung v_hoff 0..95)
plbuild PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        ; PT: Wert*4 (0..1020) -> Ringposition*16
        mov     di,0
plb1:   mov     ax,di
        mov     bx,18
        mul     bx
        mov     cl,6
        shr     ax,cl
        add     ax,v_hoff
        mov     bl,96
        div     bl
        mov     al,ah
        mov     ah,0
        mov     cl,4
        shl     ax,cl
        mov     bx,di
        shl     bx,1
        mov     word ptr FREEMEM[bx+OPT],ax
        inc     di
        cmp     di,1021
        jb      plb1
        ; COLT: Ringposition*16 + Schwelle -> Farbe
        mov     di,0
plb2:   mov     ax,di
        mov     cl,4
        shr     ax,cl                   ; p
        mov     dx,di
        and     dx,15                   ; t
        cmp     v_mono,0
        jne     plb4
        mov     bx,ax
        and     ax,15                   ; l
        mov     cl,4
        shr     bx,cl                   ; k
        cmp     ax,dx
        jbe     plb3
        inc     bx
plb3:   mov     al,RING2[bx]
        jmp     short plb6
plb4:   cmp     ax,48                   ; Mono: Helligkeitsdreieck
        jb      plb5
        neg     ax
        add     ax,95
plb5:   mov     bl,3
        div     bl
        mov     ah,0
        cmp     ax,dx
        mov     al,WEISS
        ja      plb6
        mov     al,SCHWARZ
plb6:   mov     FREEMEM[di+OCOLT],al
        inc     di
        cmp     di,96*16
        jb      plb2
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
plbuild ENDP

; Gitterwert AL -> v_ca, v_cb, Muster PBUF2 (8x8 Bayer) fuer grobe Bloecke
colpat  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     ah,0
        shl     ax,1
        shl     ax,1                    ; Wert*4
        mov     bx,ax
        shl     bx,1
        mov     ax,word ptr FREEMEM[bx+OPT]
        mov     cl,4
        shr     ax,cl                   ; p 0..95
        cmp     v_mono,0
        jne     cpt2
        mov     bx,ax
        and     ax,15
        shl     ax,1
        shl     ax,1                    ; Stufe 0..60
        mov     dx,ax
        mov     cl,4
        shr     bx,cl
        mov     al,RING2[bx]
        mov     v_cb,al
        mov     al,RING2[bx+1]
        mov     v_ca,al
        jmp     short cpt4
cpt2:   cmp     ax,48
        jb      cpt3
        neg     ax
        add     ax,95
cpt3:   mov     bl,3
        div     bl
        mov     ah,0
        shl     ax,1
        shl     ax,1
        mov     dx,ax
        mov     v_ca,WEISS
        mov     v_cb,SCHWARZ
cpt4:   ; Muster: Bit gesetzt (Farbe A), wo Bayer-Schwelle < Stufe
        mov     si,0
cpt5:   mov     ah,0
        mov     cx,8
cpt6:   shl     ah,1
        mov     al,B8[si]
        mov     bl,dl
        cmp     al,bl
        jae     cpt7
        or      ah,1
cpt7:   inc     si
        loop    cpt6
        mov     bx,si
        mov     cl,3
        shr     bx,cl
        dec     bx
        mov     PBUF2[bx],ah
        cmp     si,64
        jb      cpt5
        mov     v_patp,offset PBUF2
        mov     v_fmode,0
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
colpat  ENDP

; ein Pixel: AX = Wert*4, v_thr = Schwelle -> Bits in v_ar/v_ag/v_ab
ppix    PROC    NEAR
        mov     bx,ax
        shl     bx,1
        mov     bx,word ptr FREEMEM[bx+OPT]
        add     bx,v_thr
        mov     al,FREEMEM[bx+OCOLT]
        mov     dx,v_msk
        shr     al,1
        jnc     ppx1
        or      v_ar,dx
ppx1:   shr     al,1
        jnc     ppx2
        or      v_ag,dx
ppx2:   shr     al,1
        jnc     ppx3
        or      v_ab,dx
ppx3:   shl     v_msk,1
        ret
ppix    ENDP

; feine Darstellung: bilinear zwischen den Gitterpunkten, je Pixel geraster
plrender PROC   NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     v_pj,0
plr1:   mov     v_pfy,0
plr2:   ; Spaltenwerte CV[i] = g[j][i]*(4-fy) + g[j+1][i]*fy
        mov     cx,0
        mov     dx,v_pj
        call    gaddr
        mov     si,bx
        mov     di,0
plr3:   mov     al,FREEMEM[si]
        mov     ah,0
        mov     cx,4
        sub     cx,v_pfy
        mul     cx
        mov     cx,ax
        mov     al,FREEMEM[si+GW]
        mov     ah,0
        mul     v_pfy
        add     ax,cx
        mov     word ptr FREEMEM[di+OCV],ax
        inc     si
        add     di,2
        cmp     di,GW*2
        jb      plr3
        ; Schwellen dieser Zeile
        mov     bx,v_pfy
        shl     bx,1
        shl     bx,1
        mov     ax,word ptr B4[bx]
        mov     word ptr v_q0,ax
        mov     ax,word ptr B4[bx+2]
        mov     word ptr v_q2,ax
        ; 40 Worte bauen
        mov     si,0                    ; CV-Index*2
        mov     di,0                    ; Wort*2
plr4:   mov     v_ar,0
        mov     v_ag,0
        mov     v_ab,0
        mov     v_msk,1
        mov     cx,4
plr5:   push    cx
        mov     ax,word ptr FREEMEM[si+OCV]         ; a
        mov     v_pa,ax
        mov     ax,word ptr FREEMEM[si+OCV+2]       ; b
        mov     v_pb,ax
        mov     al,v_q0
        mov     ah,0
        mov     v_thr,ax
        mov     ax,v_pa
        call    ppix
        mov     al,v_q1
        mov     ah,0
        mov     v_thr,ax
        mov     ax,v_pa                 ; (3a+b)/4
        shl     ax,1
        add     ax,v_pa
        add     ax,v_pb
        shr     ax,1
        shr     ax,1
        call    ppix
        mov     al,v_q2
        mov     ah,0
        mov     v_thr,ax
        mov     ax,v_pa                 ; (a+b)/2
        add     ax,v_pb
        shr     ax,1
        call    ppix
        mov     al,v_q3
        mov     ah,0
        mov     v_thr,ax
        mov     ax,v_pb                 ; (a+3b)/4
        shl     ax,1
        add     ax,v_pb
        add     ax,v_pa
        shr     ax,1
        shr     ax,1
        call    ppix
        add     si,2
        pop     cx
        loop    plr5
        mov     ax,v_ag
        mov     RG[di],ax
        mov     ax,v_ar
        mov     RR[di],ax
        mov     ax,v_ab
        mov     RB[di],ax
        add     di,2
        cmp     di,80
        jae     plr4a
        jmp     plr4
plr4a:  ; Zeile y = 4j+fy schreiben
        mov     ax,v_pj
        shl     ax,1
        shl     ax,1
        add     ax,v_pfy
        call    mul40
        mov     v_prow,ax
        mov     di,ax
        mov     si,offset RG
        call    plwrite
        cmp     v_mono,0
        jne     plr6
        mov     di,v_prow
        add     di,4000h
        mov     si,offset RR
        call    plwrite
        mov     di,v_prow
        add     di,8000h
        mov     si,offset RB
        call    plwrite
plr6:   inc     v_pfy
        cmp     v_pfy,4
        jae     plr7
        jmp     plr2
plr7:   cmp     v_pj,3                  ; obere Zeilen: Beschriftung neu
        ja      plr7a
        push    si
        mov     si,v_lab
        or      si,si
        jz      plr7b
        call    toplab0
plr7b:  pop     si
plr7a:  call    poll_key
        cmp     v_key,0
        jne     plr9
        inc     v_pj
        cmp     v_pj,100
        jae     plr9
        jmp     plr1
plr9:   pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
plrender ENDP

; 40 Worte ab SI an Adresse DI schreiben
plwrite PROC    NEAR
        push    ax
        push    cx
        push    si
        call    wbegin
        mov     cx,40
plw1:   lodsw
        call    gpw
        loop    plw1
        pop     si
        pop     cx
        pop     ax
        ret
plwrite ENDP

; Beschriftung oben: Rechner und Rechenzeit der Unterteilung
pllabel PROC    NEAR
        mov     si,v_lab
        call    toplab
        ret
pllabel ENDP

scr10   PROC    NEAR
        call    sreset
        ; Gitter leeren
        mov     di,offset FREEMEM
        mov     cx,GW*GH
        mov     al,0
        cld
        rep     stosb
        mov     v_hoff,0
        call    plbuild
        mov     si,offset T10_PL        ; wer rechnet - gleich oben zeigen
        IFDEF   CALC87
        cmp     v_has87,0
        je      pl_a
        mov     si,offset T10_P87
pl_a:
        ENDIF
        mov     v_lab,si
        call    toplab0
        ; vier Ecken zufaellig, dann unterteilen
        mov     bx,255
        call    rndn
        inc     ax
        mov     FREEMEM[0],al
        call    rndn
        inc     ax
        mov     FREEMEM[GW-1],al
        call    rndn
        inc     ax
        mov     FREEMEM[GW*(GH-1)],al
        call    rndn
        inc     ax
        mov     FREEMEM[GW*GH-1],al
        call    dostime                 ; Rechenzeit der Unterteilung
        mov     v_t0lo,ax
        mov     v_t0hi,dx
        IFDEF   CALC87
        mov     v_ghave,0
        cmp     v_has87,0
        je      pl_0
        finit
pl_0:
        ENDIF
        mov     ax,0
        push    ax
        push    ax
        mov     ax,GW-1
        push    ax
        mov     ax,GH-1
        push    ax
        call    pdiv
        call    elapsed
        BAIL    pl_end
        ; grobe Durchgaenge wie bei FRACTINT: Bloecke 128, 64, 32, 16 Punkte
        mov     v_pstep,32
pl_p:   mov     dx,0                    ; Gitter-y
pl_q:   mov     cx,0                    ; Gitter-x
pl_r:   call    gaddr
        mov     al,FREEMEM[bx]
        call    colpat
        mov     ax,cx
        shl     ax,1
        shl     ax,1
        mov     v_x0,ax
        mov     ax,dx
        shl     ax,1
        shl     ax,1
        mov     v_y0,ax
        mov     ax,v_pstep
        shl     ax,1
        shl     ax,1
        mov     v_w,ax
        mov     v_h,ax
        mov     ax,640
        sub     ax,v_x0
        cmp     ax,v_w
        jae     pl_r1
        mov     v_w,ax
pl_r1:  mov     ax,400
        sub     ax,v_y0
        cmp     ax,v_h
        jae     pl_r2
        mov     v_h,ax
pl_r2:  call    fill
        add     cx,v_pstep
        cmp     cx,160
        jb      pl_r
        or      dx,dx                   ; oberste Blockreihe: Beschriftung neu
        jnz     pl_r0
        mov     si,v_lab
        call    toplab0
pl_r0:  call    poll_key
        cmp     v_key,0
        je      pl_r3
        jmp     pl_end
pl_r3:  add     dx,v_pstep
        cmp     dx,100
        jb      pl_q
        mov     si,v_lab                ; Bloecke haben die Beschriftung ueberdeckt
        call    toplab0
        mov     cx,25
        call    wframes
        shr     v_pstep,1
        cmp     v_pstep,4
        jb      pl_f
        jmp     pl_p
pl_f:
        ; feine Endfassung
        call    plrender
        cmp     v_key,0
        jne     pl_end
        call    pllabel
        mov     cx,300
        call    wframes
pl_end: mov     v_lab,0
        mov     v_patp,offset PSOLID
        ret
scr10   ENDP

;=======================================================================
;  Bild 11: Mandelbrot-Spirale bei -0,7759 + 0,1362i - je nach Variante
;           rechnet der 8088 (16 Bit 3.13), der 8087 oder der 68008 (K234);
;           Farbring schwarz-blau-cyan-weiss mit 4x4-Bayer je Zelle
;=======================================================================

; AX = AX*AX >> 13 (3.13 Festkomma), DX zerstoert
sq13    PROC    NEAR
        imul    ax
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        mov     ax,dx
        ret
sq13    ENDP

; Iterationen fuer (v_mr, v_mi) in 3.13 -> AX
mandel13 PROC   NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        xor     si,si
        xor     di,di
        xor     cx,cx
m13a:   mov     ax,si
        or      ax,ax
        jns     m13b
        neg     ax
m13b:   cmp     ax,16384
        jae     m13z
        mov     ax,di
        or      ax,ax
        jns     m13c
        neg     ax
m13c:   cmp     ax,16384
        jae     m13z
        mov     ax,si
        call    sq13
        mov     bx,ax                   ; zr^2
        mov     ax,di
        call    sq13                    ; zi^2
        mov     dx,bx
        add     dx,ax
        cmp     dx,32768
        ja      m13z
        sub     bx,ax
        mov     ax,si
        imul    di                      ; 2*zr*zi >> 13 = >> 12
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        shl     ax,1
        rcl     dx,1
        add     dx,v_mi
        mov     di,dx
        add     bx,v_mr
        mov     si,bx
        inc     cx
        cmp     cx,MAXIT2
        jb      m13a
m13z:   mov     ax,cx
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
mandel13 ENDP

; Farbe AL als Bits bei v_msk in v_ar/v_ag/v_ab eintragen
cbits   PROC    NEAR
        push    dx
        mov     dx,v_msk
        shr     al,1
        jnc     cbt1
        or      v_ar,dx
cbt1:   shr     al,1
        jnc     cbt2
        or      v_ag,dx
cbt2:   shr     al,1
        jnc     cbt3
        or      v_ab,dx
cbt3:   shl     v_msk,1
        pop     dx
        ret
cbits   ENDP

; Zelle BX mit Iterationszahl AX einfaerben -> MCA/MCB/MCL
; (innen = v_maxit -> schwarz; Ringposition p = n*v_pmul mod 96)
ncolor  PROC    NEAR
        push    ax
        push    cx
        push    si
        cmp     ax,v_maxit
        jb      ncl1
        mov     MCA[bx],SCHWARZ
        mov     MCB[bx],SCHWARZ
        mov     MCL[bx],0
        jmp     short ncl9
ncl1:   mul     v_pmul
        mov     cl,96
        div     cl
        mov     al,ah
        mov     ah,0                    ; p
        cmp     v_mono,0
        jne     ncl2
        mov     si,ax
        and     al,15
        mov     MCL[bx],al
        mov     cl,4
        shr     si,cl
        mov     al,RINGM[si]
        mov     MCA[bx],al
        mov     al,RINGM[si+1]
        mov     MCB[bx],al
        jmp     short ncl9
ncl2:   cmp     ax,48                   ; Mono: Helligkeitsdreieck
        jb      ncl3
        neg     ax
        add     ax,95
ncl3:   mov     cl,3
        div     cl
        mov     MCL[bx],al
        mov     MCA[bx],SCHWARZ
        mov     MCB[bx],WEISS
ncl9:   pop     si
        pop     cx
        pop     ax
        ret
ncolor  ENDP

; Zellenzeile v_mj (160 Zellen aus MCA/MCB/MCL, je 4x4 Bayer) zeichnen
crow    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     v_pfy,0
crw1:   mov     bx,v_pfy
        shl     bx,1
        shl     bx,1
        mov     ax,word ptr B4[bx]
        mov     word ptr v_q0,ax
        mov     ax,word ptr B4[bx+2]
        mov     word ptr v_q2,ax
        mov     si,0                    ; Zelle
        mov     di,0                    ; Wort*2
crw2:   mov     v_ar,0
        mov     v_ag,0
        mov     v_ab,0
        mov     v_msk,1
        mov     cx,4
crw3:   push    cx
        mov     bx,0
crw4:   mov     dl,MCL[si]
        mov     al,MCA[si]
        cmp     dl,v_q0[bx]
        jbe     crw5
        mov     al,MCB[si]
crw5:   call    cbits
        inc     bx
        cmp     bx,4
        jb      crw4
        inc     si
        pop     cx
        loop    crw3
        mov     ax,v_ag
        mov     RG[di],ax
        mov     ax,v_ar
        mov     RR[di],ax
        mov     ax,v_ab
        mov     RB[di],ax
        add     di,2
        cmp     di,80
        jb      crw2
        mov     ax,v_mj
        shl     ax,1
        shl     ax,1
        add     ax,v_pfy
        call    mul40
        mov     v_prow,ax
        mov     di,ax
        mov     si,offset RG
        call    plwrite
        cmp     v_mono,0
        jne     crw6
        mov     di,v_prow
        add     di,4000h
        mov     si,offset RR
        call    plwrite
        mov     di,v_prow
        add     di,8000h
        mov     si,offset RB
        call    plwrite
crw6:   inc     v_pfy
        cmp     v_pfy,4
        jae     crw7
        jmp     crw1
crw7:   call    relab
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
crow    ENDP

; Uhrzeit (DOS Fn 2Ch) in Hundertstelsekunden seit Mitternacht -> DX:AX
dostime PROC    NEAR
        push    bx
        push    cx
        mov     ah,2Ch
        int     21h
        mov     v_tsec,dx               ; DH = Sekunden, DL = Hundertstel
        mov     al,ch
        mov     ah,0
        mov     bl,60
        mul     bl                      ; Stunden*60
        mov     bl,cl
        mov     bh,0
        add     ax,bx                   ; Minuten des Tages
        mov     bx,6000
        mul     bx                      ; DX:AX = Minuten*6000
        push    dx
        push    ax
        mov     al,byte ptr v_tsec+1
        mov     ah,0
        mov     bl,100
        mul     bl                      ; Sekunden*100
        mov     bl,byte ptr v_tsec
        mov     bh,0
        add     ax,bx
        mov     bx,ax
        pop     ax
        pop     dx
        add     ax,bx
        adc     dx,0
        pop     cx
        pop     bx
        ret
dostime ENDP

; vergangene Sekunden seit v_t0 -> v_secs, v_tok = 1 wenn die Uhr lief
elapsed PROC    NEAR
        push    ax
        push    bx
        push    dx
        call    dostime
        sub     ax,v_t0lo
        sbb     dx,v_t0hi
        jns     ela1
        add     ax,0D600h               ; Mitternacht: + 8.640.000
        adc     dx,0083h
ela1:   mov     v_tok,0
        mov     v_hlo,ax
        mov     v_hhi,dx
        mov     bx,ax
        or      bx,dx
        jz      ela9
        cmp     dx,100
        jae     ela9                    ; unplausibel
        mov     bx,100
        div     bx
        mov     v_secs,ax
        mov     v_tok,1
ela9:   pop     dx
        pop     bx
        pop     ax
        ret
elapsed ENDP

; AX (0..999) dreistellig nach [DI]
dec3    PROC    NEAR
        push    ax
        push    bx
        push    dx
        cmp     ax,999
        jbe     dc1
        mov     ax,999
dc1:    mov     bx,100
        xor     dx,dx
        div     bx
        add     al,'0'
        mov     [di],al
        mov     ax,dx
        mov     bl,10
        div     bl
        add     al,'0'
        add     ah,'0'
        mov     [di+1],al
        mov     [di+2],ah
        pop     dx
        pop     bx
        pop     ax
        ret
dec3    ENDP

; Laenge des Textes SI -> CX
textlen PROC    NEAR
        push    ax
        push    si
        xor     cx,cx
tl1:    lodsb
        or      al,al
        jz      tl2
        inc     cx
        jmp     short tl1
tl2:    pop     si
        pop     ax
        ret
textlen ENDP

; Rechenzeit v_hhi:v_hlo (1/100 s) als 'sss,hh SEKUNDEN' nach T_SECS
; (v_tok = 0: '?')
fmth    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     ax,ds
        mov     es,ax
        cld
        mov     di,offset T_SECS
        cmp     v_tok,0
        jne     fh1
        mov     al,'?'
        stosb
        jmp     short fh5
fh1:    mov     ax,v_hlo
        mov     dx,v_hhi
        cmp     dx,100
        jb      fh1a
        mov     ax,0FFFFh               ; ueber 655 s: Anschlag
        mov     dx,99
fh1a:   mov     bx,100
        div     bx                      ; AX = Sekunden, DX = Hundertstel
        push    dx
        mov     cx,0                    ; Ziffern der Sekunden (ohne fuehrende Nullen)
        mov     bx,10
fh2:    xor     dx,dx
        div     bx
        push    dx
        inc     cx
        or      ax,ax
        jnz     fh2
fh3:    pop     ax
        add     al,'0'
        stosb
        loop    fh3
        mov     al,','
        stosb
        pop     ax
        mov     bl,10
        div     bl
        add     ax,'00'
        stosw                           ; Zehntel, Hundertstel
fh5:    mov     si,offset T_SEKU
fh6:    lodsb
        stosb
        or      al,al
        jnz     fh6
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
fmth    ENDP

; Beschriftung oben: Text SI + Rechenzeit (fmth), zentriert in Zeile 4
toplab  PROC    NEAR
        push    ax
        push    cx
        push    si
        push    di
        push    es
        call    fmth
        call    toplab0                 ; Text (steht schon da) ...
        call    textlen
        mov     ax,cx
        mov     cl,3
        shl     ax,cl
        add     ax,8
        mov     v_tx,ax                 ; ... und dahinter ' - Zeit'
        mov     ax,ds
        mov     es,ax
        cld
        mov     di,offset T_LINE
        mov     si,offset T_DASH
tlb2a:  lodsb
        or      al,al
        jz      tlb2b
        stosb
        jmp     short tlb2a
tlb2b:  mov     si,offset T_SECS
tlb3:   lodsb
        stosb
        or      al,al
        jnz     tlb3
        mov     si,offset T_LINE
        call    stext
        pop     es
        pop     di
        pop     si
        pop     cx
        pop     ax
        ret
toplab  ENDP

; Beschriftung oben links ohne Zeit: Text SI (Zeile 4, ab Punkt 8)
toplab0 PROC    NEAR
        push    ax
        push    si
        mov     v_tz,1
        mov     v_tx,8
        mov     v_ty,4
        mov     v_col,WEISS
        mov     v_shc,SCHWARZ
        mov     v_sho,1
        call    stext
        pop     si
        pop     ax
        ret
toplab0 ENDP

; waehrend der Rechnung: v_lab neu schreiben, solange die obersten
; Bildzeilen (Zelle 0..3) entstehen
relab   PROC    NEAR
        push    si
        cmp     v_lab,0
        je      rlb9
        cmp     v_mj,4
        jae     rlb9
        mov     si,v_lab
        call    toplab0
rlb9:   pop     si
        ret
relab   ENDP

; obere Textzeile loeschen (vor einer neuen Beschriftung)
clrtop  PROC    NEAR
        push    ax
        mov     v_x0,0
        mov     v_y0,0
        mov     v_w,640
        mov     v_h,14
        mov     al,SCHWARZ
        call    fillc
        pop     ax
        ret
clrtop  ENDP

;-----------------------------------------------------------------------
; Bild 11: die Spirale
;-----------------------------------------------------------------------
scrsp   PROC    NEAR
        call    sreset
        mov     v_maxit,MAXIT2
        IFDEF   CALC87
        mov     v_sc87,offset SCALE87
        ENDIF
        mov     v_pmul,5
        mov     v_eng,0
        mov     v_note,0
        call    dostime
        mov     v_t0lo,ax
        mov     v_t0hi,dx
        IFDEF   CALC68
        cmp     v_lvl,6
        jne     sp_cpu                  ; ohne /6: kein Versuch mit dem 68008
        call    sp68                    ; CF = 1: 68008 nicht da -> 8087/8088
        jc      sp_cpu
        cmp     v_key,0
        jne     sp_ke
        jmp     sp_fin
sp_ke:  jmp     sp_end
        ENDIF
sp_cpu: call    dostime                 ; Zeit neu starten (nach 68008-Versuch)
        mov     v_t0lo,ax
        mov     v_t0hi,dx
        IFDEF   CALC87
        cmp     v_has87,0
        jne     sp_87
        jmp     short sp_c0
sp_87:  mov     v_eng,1
sp_c0:
        ENDIF
        call    clrtop                  ; (68008-Versuch hatte evtl. schon beschriftet)
        mov     si,offset T_E88
        cmp     v_eng,1
        jne     sp_c0a
        mov     si,offset T_E87
sp_c0a: mov     v_lab,si
        call    toplab0
        mov     v_mj,0
        mov     v_mi,1048               ; Imaginaerteil der obersten Zeile
sp_r:   mov     v_mr,-6446
        mov     bx,0
sp_c:
        IFDEF   CALC87
        cmp     v_eng,1
        jne     sp_c1
        call    mandel87
        jmp     short sp_c2
sp_c1:
        ENDIF
        call    mandel13
sp_c2:  call    ncolor
        inc     v_mr
        inc     bx
        cmp     bx,160
        jb      sp_c
        call    crow
        call    poll_key
        cmp     v_key,0
        je      sp_c3
        jmp     sp_end
sp_c3:  inc     v_mi
        inc     v_mj
        cmp     v_mj,100
        jb      sp_r
sp_fin: ; Rechenzeit anzeigen
        call    elapsed
        IFDEF   CALC68
        cmp     v_eng,2
        jne     sp_f1
        cmp     v_tok,0
        jne     sp_f1
        mov     ax,v_mf                 ; DOS-Uhr steht: Bildwechsel zaehlen
        shl     ax,1                    ; 1/50 s -> 1/100 s
        mov     v_hlo,ax
        mov     v_hhi,0
        mov     v_tok,1
sp_f1:
        ENDIF
        mov     si,offset T_E88
        cmp     v_eng,1
        jne     sp_f3
        mov     si,offset T_E87
sp_f3:  cmp     v_eng,2
        jne     sp_f4
        mov     si,offset T_E68
sp_f4:  mov     v_lab,0
        call    toplab
        cmp     v_note,0
        je      sp_f5
        mov     v_tz,1
        mov     v_tx,8
        mov     v_ty,16
        mov     v_col,GELB
        mov     v_shc,SCHWARZ
        mov     v_sho,1
        mov     si,v_note
        call    stext
sp_f5:  mov     cx,400
        call    wframes
sp_end: mov     v_lab,0
        ret
scrsp   ENDP

        IFDEF   CALC87
;-----------------------------------------------------------------------
; 8087: Iterationen fuer (v_mr, v_mi)/8192 -> AX  (Stack: zr zi cr ci)
;-----------------------------------------------------------------------
mandel87 PROC   NEAR
        push    bx
        push    cx
        mov     bx,v_sc87               ; Skala: SCALE87 (Bild 11) oder SCALE12 (Bild 9)
        finit
        fild    v_mi
        fmul    qword ptr [bx]          ; ci
        fild    v_mr
        fmul    qword ptr [bx]          ; cr
        fldz                            ; zi
        fldz                            ; zr
        xor     cx,cx
m87a:   fld     st(0)
        fmul    st,st(0)                ; zr^2
        fld     st(2)
        fmul    st,st(0)                ; zi^2
        fld     st(1)
        fadd    st,st(1)                ; zr^2 + zi^2
        fcomp   FOUR87
        fstsw   v_sw                    ; 8087 kennt kein FSTSW AX
        fwait
        mov     ah,byte ptr v_sw+1
        sahf
        ja      m87z                    ; > 4: entkommen
        fsubp   st(1),st                ; zr^2 - zi^2
        fadd    st,st(3)                ; + cr = neues zr
        fxch    st(1)
        fmul    st,st(2)                ; zr*zi
        fadd    st,st(0)
        fadd    st,st(4)                ; 2 zr zi + ci = neues zi
        fstp    st(2)
        inc     cx
        cmp     cx,v_maxit
        jb      m87a
m87z:   mov     ax,cx
        pop     cx
        pop     bx
        ret
mandel87 ENDP

; 8087 suchen (nur FN-Befehle ohne WAIT - ohne 8087 wuerde WAIT haengen)
det87   PROC    NEAR
        mov     v_has87,0
        cmp     v_lvl,8
        je      d87x                    ; /8: nur der 8088/V20
        fninit
        mov     cx,40                   ; dem 8087 Zeit fuer FNINIT lassen
d87w:   loop    d87w
        mov     v_sw,5A5Ah
        fnstsw  v_sw
        cmp     byte ptr v_sw,0
        jne     d87x
        fnstcw  v_sw
        mov     ax,v_sw
        and     ax,103Fh
        cmp     ax,003Fh
        jne     d87x
        mov     v_has87,1
d87x:   ret
det87   ENDP
        ENDIF

        IFDEF   CALC68
;-----------------------------------------------------------------------
; 68008: rechnet im gemeinsamen Speicher, der 8088 zeichnet fertige Zeilen
; CF = 1: 68008 nicht verfuegbar (8088 soll rechnen)
;-----------------------------------------------------------------------
rd68    PROC    NEAR
        push    es
        mov     es,v_s68
        mov     al,es:[bx]
        pop     es
        ret
rd68    ENDP

sp68    PROC    NEAR
        cmp     v_s68,0
        jne     s68a0
        mov     v_note,offset T68_NO
        stc
        ret
s68a0:  ; Block vorbereiten: loeschen, Briefkasten, 68000-Code
        push    es
        mov     es,v_s68
        cld
        xor     di,di
        xor     ax,ax
        mov     cx,(4096+16000)/2
        rep     stosw
        mov     si,offset M68BOX
        xor     di,di
        mov     cx,32
        rep     movsb
        mov     si,offset M68CODE
        mov     di,100h
        mov     cx,M68LEN
        rep     movsb
        pop     es
        ; physische Adresse P = Segment*16 -> DX:AX
        mov     ax,v_s68
        mov     dx,ax
        mov     cl,4
        shl     ax,cl
        mov     cl,12
        shr     dx,cl
        ; Reset-Vektoren im 68000-Format: SSP = P+1000h, PC = P+100h
        mov     bx,ax
        mov     cx,dx
        add     bx,1000h
        adc     cx,0
        mov     VEC68[0],0
        mov     VEC68[1],cl
        mov     VEC68[2],bh
        mov     VEC68[3],bl
        mov     bx,ax
        mov     cx,dx
        add     bx,100h
        adc     cx,0
        mov     VEC68[4],0
        mov     VEC68[5],cl
        mov     VEC68[6],bh
        mov     VEC68[7],bl
        ; 8088-Vektoren 0 und 1 sichern, 68000-Vektoren eintragen, starten
        push    es
        xor     ax,ax
        mov     es,ax
        cli
        mov     bx,0
s68v1:  mov     al,es:[bx]
        mov     IVTSAV[bx],al
        mov     al,VEC68[bx]
        mov     es:[bx],al
        inc     bx
        cmp     bx,8
        jb      s68v1
        out     0D8h,al                 ; 68008: Reset und los
        sti
        pop     es
        ; warten, bis der 68008 "laeuft" meldet (hoechstens 3 s)
        mov     cx,150
s68w:   call    vsync
        mov     bx,0
        call    rd68
        or      al,al
        jnz     s68st
        loop    s68w
s68st:  push    ax
        push    es
        xor     ax,ax
        mov     es,ax
        cli
        mov     bx,0
s68v2:  mov     al,IVTSAV[bx]
        mov     es:[bx],al
        inc     bx
        cmp     bx,8
        jb      s68v2
        sti
        pop     es
        pop     ax
        or      al,al
        jnz     s68go
        mov     v_note,offset T68_NO
        stc
        ret
s68go:  mov     v_s68run,1
        mov     v_eng,2
        mov     v_lab,offset T_E68      ; der 68008 rechnet - oben zeigen
        push    si
        mov     si,offset T_E68
        call    toplab0
        pop     si
        mov     v_mj,0                  ; gezeichnete Zeilen
        mov     v_mf,0                  ; Bildwechsel seit Start
s68p:   mov     bx,1
        call    rd68                    ; fertige Zeilen
        mov     ah,0
        cmp     v_mj,ax
        jae     s68n
        ; Zeile v_mj aus den Ergebnissen zeichnen
        mov     ax,v_mj
        mov     cl,5
        shl     ax,cl                   ; *32
        mov     si,ax
        shl     ax,1
        shl     ax,1                    ; *128
        add     si,ax
        add     si,1000h
        push    es
        mov     es,v_s68
        mov     bx,0
s68c:   mov     al,es:[si]
        mov     ah,0
        call    ncolor
        inc     si
        inc     bx
        cmp     bx,160
        jb      s68c
        pop     es
        call    crow
        inc     v_mj
        jmp     short s68p
s68n:   mov     bx,0
        call    rd68
        cmp     al,2
        jb      s68n1
        cmp     v_mj,100
        jb      s68n1
        mov     v_s68run,0
        clc
        ret
s68n1:  call    vsync
        inc     v_mf
        call    poll_key
        cmp     v_key,0
        je      s68p
        ; Taste: 68008 bitten aufzuhoeren, hoechstens 10 s warten
        push    es
        mov     es,v_s68
        mov     byte ptr es:[bx+2],1
        pop     es
        mov     cx,500
s68a:   call    vsync
        mov     bx,0
        call    rd68
        cmp     al,2
        jae     s68a1
        loop    s68a
        clc                             ; laeuft noch: v_s68run bleibt 1
        ret
s68a1:  mov     v_s68run,0
        clc
        ret
sp68    ENDP
        ENDIF

;-----------------------------------------------------------------------
; Variante beim Start vorbereiten / beim Ende aufraeumen
;-----------------------------------------------------------------------
varinit PROC    NEAR
        call    det87
        cmp     v_lvl,6
        jne     vi5
        ; /6: 68008 versuchen, Ersatz ist der 8087 bzw. 8088
        mov     dx,offset T_VAR68
        mov     ah,09h
        int     21h
        mov     dx,offset T_R87
        cmp     v_has87,0
        jne     vi4
        mov     dx,offset T_R88
vi4:    mov     ah,09h
        int     21h
        mov     v_s68,0
        mov     bx,1000h                ; eigenen Block auf 64 KB kuerzen
        mov     ah,4Ah
        int     21h
        mov     bx,0500h                ; 20 KB fuer den 68008
        mov     ah,48h
        int     21h
        jc      vi2
        mov     v_s68,ax
        ret
vi2:    mov     dx,offset T68_MEM
        mov     ah,09h
        int     21h
        ret
vi5:    mov     dx,offset T_OFF87       ; /8
        cmp     v_lvl,8
        je      vi1
        mov     dx,offset T_VAR87       ; /7 (Vorgabe)
        cmp     v_has87,0
        jne     vi1
        mov     dx,offset T_NO87
vi1:    mov     ah,09h
        int     21h
        ret
varinit ENDP

varexit PROC    NEAR
        cmp     v_s68run,0
        je      vx1
        mov     dx,offset T68_RUN
        mov     ah,09h
        int     21h
vx1:    ret
varexit ENDP


;=======================================================================
;  Bild 12: Foto (KITT.PIC) mit Scanner-Lauflicht - auch auf den 8 LEDs
;  KITT.PIC: 4 Ebenen zu 32000 Bytes (gruen, rot, blau, mono), erzeugt
;  mit tools/pic2dmv.py; Lage des Scanners in der Tabelle KSCAN
;=======================================================================

; Foto laden: CF = 1, wenn die Datei fehlt oder zu kurz ist
kload   PROC    NEAR
        mov     dx,offset T_KFILE
        mov     ax,3D00h
        int     21h
        jc      kl9
        mov     v_fh,ax
        mov     v_kpl,0
kl1:    mov     bx,v_fh
        mov     cx,32000
        mov     dx,offset FREEMEM
        mov     ah,3Fh
        int     21h
        jc      kl8
        cmp     ax,32000
        jne     kl8
        ; Ziel: Farbe -> Ebene 0/1/2, Mono -> nur die vierte in die gruene
        mov     bx,v_kpl
        cmp     v_mono,0
        jne     kl2
        cmp     bx,3
        jae     kl5
        shl     bx,1
        mov     di,KPLANE[bx]
        jmp     short kl3
kl2:    cmp     bx,3
        jne     kl5
        mov     di,0
kl3:    call    wbegin
        mov     si,offset FREEMEM
        mov     cx,16000
kl4:    lodsw
        call    gpw
        loop    kl4
kl5:    inc     v_kpl
        cmp     v_kpl,4
        jb      kl1
        mov     bx,v_fh
        mov     ah,3Eh
        int     21h
        clc
        ret
kl8:    mov     bx,v_fh
        mov     ah,3Eh
        int     21h
kl9:    stc
        ret
kload   ENDP

; Scannersegment BX (0..7) in Helligkeit KLEV[bx] zeichnen
kseg    PROC    NEAR
        push    ax
        push    bx
        push    dx
        push    si
        mov     ax,bx
        mul     KSCAN+4                 ; Abstand
        add     ax,KSCAN                ; x0
        mov     v_x0,ax
        mov     ax,KSCAN+2
        mov     v_y0,ax
        mov     ax,KSCAN+6
        mov     v_w,ax
        mov     ax,KSCAN+8
        mov     v_h,ax
        mov     bl,KLEV[bx]
        mov     bh,0
        mov     si,bx
        shl     si,1
        shl     si,1                    ; 4 Bytes je Stufe
        mov     al,KSTUF[si]
        mov     v_ca,al
        mov     al,KSTUF[si+1]
        mov     v_cb,al
        mov     ax,word ptr KSTUF[si+2]
        mov     v_patp,ax
        mov     v_fmode,0
        call    fill
        pop     si
        pop     dx
        pop     bx
        pop     ax
        ret
kseg    ENDP

scrkitt PROC    NEAR
        call    sreset
        call    kload
        jnc     kt1
        mov     v_tz,2
        mov     v_ty,190
        mov     v_col,WEISS
        mov     si,offset T_KMISS
        call    ctext
        mov     cx,150
        call    wframes
        ret
kt1:    mov     v_tz,1
        mov     v_tx,4
        mov     v_ty,390
        mov     v_col,WEISS
        mov     v_shc,SCHWARZ
        mov     v_sho,1
        mov     si,offset T_KCRED
        call    stext
        ; Lauflicht: Punkt wandert hin und her, Schweif klingt ab
        mov     bx,0
kt2:    mov     KLEV[bx],0
        call    kseg
        inc     bx
        cmp     bx,8
        jb      kt2
        mov     v_kpos,0
        mov     v_kdir,1
        mov     v_mf,0
kt3:    mov     bx,0                    ; alle Stufen eine abwaerts
kt4:    cmp     KLEV[bx],0
        je      kt5
        dec     KLEV[bx]
kt5:    inc     bx
        cmp     bx,8
        jb      kt4
        mov     bx,v_kpos
        mov     KLEV[bx],3
        ; LEDs: Stufe >= 2 leuchtet (LED 1 = Bit 7, aktiv low)
        mov     ah,0
        mov     bx,0
kt6:    shl     ah,1
        cmp     KLEV[bx],2
        jb      kt7
        or      ah,1
kt7:    inc     bx
        cmp     bx,8
        jb      kt6
        mov     al,ah
        not     al
        out     0,al
        ; Segmente zeichnen
        call    vsync
        mov     bx,0
kt8:    call    kseg
        inc     bx
        cmp     bx,8
        jb      kt8
        mov     cx,4
        call    wframes
        cmp     v_key,0
        jne     kt9
        ; weiterlaufen, an den Enden umkehren
        mov     ax,v_kpos
        add     ax,v_kdir
        cmp     ax,0
        jge     kt10
        mov     ax,1
        mov     v_kdir,1
kt10:   cmp     ax,7
        jle     kt11
        mov     ax,6
        mov     v_kdir,-1
kt11:   mov     v_kpos,ax
        inc     v_mf
        cmp     v_mf,150
        jae     kt9
        jmp     kt3
kt9:    mov     al,0FFh                 ; LEDs aus
        out     0,al
        mov     v_patp,offset PSOLID
        ret
scrkitt ENDP
;=======================================================================
;  Hauptprogramm
;=======================================================================

main    PROC    NEAR
        mov     sp,offset STACKTOP
        ; Befehlszeile auswerten
        mov     si,81h
        mov     v_force,0
        mov     v_single,0
m_arg:  lodsb
        cmp     al,0Dh
        je      m_argend
        cmp     al,'/'
        jne     m_num
        lodsb
        cmp     al,'6'                  ; /6 /7 /8: hoechste Rechenstufe fuer Bild 11
        jb      m_a0
        cmp     al,'8'
        ja      m_a0
        sub     al,'0'
        mov     v_lvl,al
        jmp     short m_arg
m_a0:   and     al,0DFh
        cmp     al,'M'
        jne     m_a1
        mov     v_force,1
        jmp     short m_arg
m_a1:   cmp     al,'C'
        jne     m_a2
        mov     v_force,2
        jmp     short m_arg
m_a2:   mov     dx,offset T_HELP
        mov     ah,09h
        int     21h
        mov     ax,4C00h
        int     21h
m_num:  cmp     al,'0'
        jb      m_arg
        cmp     al,'9'
        ja      m_arg
        sub     al,'0'
        mov     bh,al
        mov     al,v_single
        mov     bl,10
        mul     bl
        add     al,bh
        mov     v_single,al
        jmp     short m_arg
m_argend:
        cmp     v_single,NSCREEN
        jbe     m_s1
        mov     v_single,0
m_s1:   ; INT 23h (Strg-C) auf eigenen Ausgang legen
        mov     dx,offset brk_h
        mov     ax,2523h
        int     21h
        ; Mono/Farbe
        call    detect
        cmp     v_force,1
        jne     m_f1
        mov     v_mono,1
m_f1:   cmp     v_force,2
        jne     m_f2
        mov     v_mono,0
m_f2:   mov     v_npl,3
        cmp     v_mono,0
        je      m_f3
        mov     v_npl,1
m_f3:   mov     dx,offset T_BANNER
        mov     ah,09h
        int     21h
        mov     dx,offset T_FARBE
        cmp     v_mono,0
        je      m_f4
        mov     dx,offset T_MONO
m_f4:   mov     ah,09h
        int     21h
        call    varinit                 ; 8087 suchen / 68008-Block holen
        mov     ax,v_frame
        mov     v_seed,ax
        call    gr_on
        ; Bilder zeigen
m_loop: mov     bl,v_single
        or      bl,bl
        jnz     m_one
        mov     bl,v_cur
        inc     bl
        cmp     bl,NSCREEN
        jbe     m_l1
        mov     bl,1
m_l1:   mov     v_cur,bl
m_one:  mov     v_key,0
        mov     bh,0
        dec     bx
        shl     bx,1
        call    SCRTAB[bx]
        cmp     v_key,2
        je      m_end
        cmp     v_single,0
        je      m_next
        ; Einzelbild: stehen lassen bis Taste
m_wait: cmp     v_key,0
        jne     m_end
        call    vsync
        call    poll_key
        jmp     short m_wait
m_next: call    wipe
        jmp     short m_loop
m_end:  call    gr_off
        call    varexit
        mov     ax,4C00h
        int     21h
main    ENDP

brk_h   PROC    FAR
        push    cs
        pop     ds
        call    gr_off
        call    varexit
        mov     ax,4C00h
        int     21h
brk_h   ENDP

; Platzhalter fuer noch nicht fertige Bilder
scrx    PROC    NEAR
        call    sreset
        mov     v_tz,3
        mov     v_ty,180
        mov     v_col,BUNT
        mov     si,offset T_TODO
        call    ctext
        mov     cx,100
        call    wframes
        ret
scrx    ENDP

        EVEN
        db      1024 dup (?)
STACKTOP LABEL  WORD
FREEMEM LABEL   BYTE                    ; freier Speicher: Plasma-Gitter und Tabellen

CSEG    ENDS
        END     START

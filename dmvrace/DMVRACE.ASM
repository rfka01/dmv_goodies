;***********************************************************************
;*                                                                     *
;*  DMVRACE  -  Prozessor-Rennen fuer die NCR Decision Mate V          *
;*              MS-DOS 3.30, uPD7220 GDC, 640 x 400, 8 Farben          *
;*                                                                     *
;*  Bauen mit MASM 5.10:                                               *
;*      MASM DMVRACE;                                                  *
;*      LINK DMVRACE;          (Warnung "no stack segment" ist normal) *
;*      EXE2BIN DMVRACE.EXE DMVRACE.COM                                *
;*  Fertig beigelegt: Z80.INC (aus RACEZ80.ASM, z80asm), R68.INC (aus  *
;*  RACE68.S, GNU as m68k), TRACK.INC, FLAG.INC, CARS.INC -          *
;*  siehe tools/.                                                      *
;*                                                                     *
;*  Aufruf:  DMVRACE [/Z] [/K] [/8] [/M] [/C]                          *
;*      /Z  ohne Z80   /K  ohne 68008 (K234)   /8  ohne 8087           *
;*      /M  Mono erzwingen, /C Farbe erzwingen                         *
;*                                                                     *
;*  Vier Wagen: Z80 (Hauptplatine), 8088 bzw. V20, derselbe mit 8087,  *
;*  68008 (K234). Die Strecke hat fuenf Abschnitte, jeder ist ein Test:*
;*    Wurzel-Kurve        Gitterpunkte im Kreis, Summe isqrt(R^2-x^2)  *
;*    Sierpinski-Gerade   Pascal-Dreieck mod 2: Zeile ^= Zeile << 1    *
;*    Divisions-Kehre     Primfaktoren durch Probedivision             *
;*    Eratosthenes-Kurve  Sieb des Eratosthenes (BYTE 1981)            *
;*    Mandelbrot-Schikane Mandelbrot 40 x 25, Tiefe 50                 *
;*  Runde 1: Abschnitt fuer Abschnitt rechnen erst alle Prozessoren    *
;*  den Test, dann fahren die Wagen, bis der Schnellste das Ende des   *
;*  Abschnitts erreicht (die anderen stehen, wo sie dann waeren).      *
;*  Danach zwei weitere Runden mit den gemessenen Zeiten. Das Rennen   *
;*  laeuft in Echtzeit: ein Bildwechsel = 1/50 s Rechenzeit. Stimmt    *
;*  eine Pruefsumme nicht, wird der Wagen disqualifiziert. Unten zeigt *
;*  jeder Prozessor sein Ergebnis zum aktuellen Abschnitt als Bild.    *
;*  Vor dem Start stehen die Wagen in der Startaufstellung, eine Taste *
;*  startet. Nach dem Ziel: J = nochmal (neu messen), ESC = Ende.      *
;*                                                                     *
;*  Zeitmessung: PIT-Kanal 0, den das Boot-ROM als 50-Hz-Zaehler       *
;*  programmiert (Modus 5, 838 = 16,8 s Motor-Auszeit). Er zaehlt frei *
;*  weiter, auch waehrend der 8088 steht und der Z80 rechnet.          *
;*                                                                     *
;*  Z80: Das Boot-ROM (C.07.00, 0580h) legt bei F10Ch OUT (10h),A /    *
;*  OUT (D0h),A / NOP / JP F111h ab und schaltet damit auf den 8088.   *
;*  Der Z80 steht hinter dem OUT (D0h) und macht beim Zurueckschalten  *
;*  bei F110h weiter. DMVRACE legt den Z80-Kern in einen freien Teil   *
;*  von Bank 0 (Inhalt vorher gesichert, danach zurueckgelegt), passt  *
;*  ihn dort an und schreibt bei F10Eh OUT (D0h),A / NOP / JP Kern.    *
;*  Dann schaltet es mit OUT D0h um und wartet, bis der Z80 "fertig"   *
;*  meldet und zurueckschaltet. Waehrenddessen sind die Interrupts aus.*
;*                                                                     *
;***********************************************************************

P_STAT   EQU     0A0h            ; GDC Status (lesen)
P_PARM   EQU     0A0h            ; GDC Parameter (schreiben)
P_CMD    EQU     0A1h            ; GDC Kommando (schreiben)
P_ZOOM   EQU     0A2h            ; ZOOM-Parameter: GDC (BA0=0) + Zoom-Latch der Karte
P_FIFO   EQU     0A1h            ; GDC FIFO (lesen)
P_PIT0   EQU     080h            ; 8253 Kanal 0 (50 Hz)
P_PITC   EQU     083h            ; 8253 Steuerwort
P_SW16   EQU     0D0h            ; Prozessorwechsel Z80 <-> 8088
P_BANK   EQU     0E0h            ; Z80-Speicherbank (E0h = Bank 0)

SCHWARZ EQU     0
ROT     EQU     1
GRUEN   EQU     2
GELB    EQU     3
BLAU    EQU     4
MAGENTA EQU     5
CYAN    EQU     6
WEISS   EQU     7
BUNT    EQU     0FFh

TRAIL   EQU     10              ; fuer GDCVAR.INC
NV      EQU     14

; ---- Tests (wie tools/benchref.py, RACEZ80.ASM, RACE68.S) ----
QW      EQU     40              ; Mandelbrot: Zellen waagrecht
QH      EQU     25              ; Zellen senkrecht
NCELL   EQU     QW*QH
QMAX    EQU     50              ; Tiefe
QX0     EQU     -17613          ; cr links (3.13) = -2,15
QY0     EQU     9600            ; ci oben (3.13)  =  1,17
QDX     EQU     560             ; Schritt waagrecht = 0,068
QDY     EQU     800             ; Schritt senkrecht = 0,098
SIER_ROWS EQU   200
SIER_REPS EQU   40
SIEVE_SIZE EQU  8190
SIEVE_ITER EQU  8
DIV_N0  EQU     50000
DIV_K   EQU     400
SQ_R    EQU     6000

; ---- Rennen ----
NSEC    EQU     5               ; Abschnitte
LAPS    EQU     3               ; Runden fuer den Schnellsten
TRKOFS  EQU     432             ; Start/Ziel bei Streckenpunkt 432 (oben rechts)
THY     EQU     300             ; Vorschaubilder: obere Zeile
TLY     EQU     289             ; Beschriftung darueber
TBX     EQU     148             ; Tabelle: Spalte Testname
TCX     EQU     244             ; erste Prozessorspalte (je 60 Punkte)
TY_ST   EQU     116             ; Statuszeile
TY_HD   EQU     128             ; Kopfzeile
TY_S0   EQU     140             ; erste Testzeile (je 10)
TY_TOT  EQU     192             ; Gesamtzeit
TY_LAP  EQU     204             ; Runden
BOXY    EQU     TY_LAP+4        ; Box: Wagen hinter "BOX" in seiner Rundenzelle

        .8087
CSEG    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  CS:CSEG,DS:CSEG,ES:CSEG,SS:CSEG
        ORG     100h

START:  jmp     main

;=======================================================================
;  Daten
;=======================================================================

        INCLUDE GDCTAB.INC

        INCLUDE TRACK.INC
        INCLUDE FLAG.INC
        INCLUDE CARS.INC
        INCLUDE ANG.INC
        INCLUDE Z80.INC
        INCLUDE R68.INC

; Wagen: 0 = Z80, 1 = 8088/V20, 2 = mit 8087, 3 = 68008
ENAME   dw      offset TN0,offset TN1,offset TN2,offset TN3
TN0     db      'Z80',0
TN1     db      '8088',0,0,0,0,0,0
TN2     db      '8088+8087',0
TN3     db      '68008',0
ESHORT  dw      offset TS0,offset TS1,offset TS2,offset TS3
TS0     db      'Z80',0
TS1     db      '8088',0
TS2     db      '+8087',0
TS3     db      '68008',0
ECOL    db      ROT,GRUEN,GELB,CYAN
ETRKX   dw      offset TRKX0,offset TRKX1,offset TRKX2,offset TRKX3
ETRKY   dw      offset TRKY0,offset TRKY1,offset TRKY2,offset TRKY3
EORDER  db      1,2,3,0                 ; Reihenfolge im Qualifying
BOXX    dw      TCX+40,TCX+100,TCX+160,TCX+220

; Abschnitte in Fahrtrichtung ab Start/Ziel
SNAME   dw      offset SN0,offset SN1,offset SN2,offset SN3,offset SN4
SN0     db      'Wurzel',0
SN1     db      'Sierpinski',0
SN2     db      'Division',0
SN3     db      'Eratosthenes',0
SN4     db      'Mandelbrot',0
STEST   db      5,2,4,3,1               ; Testnummer je Abschnitt
SCOL    db      GELB,GRUEN,MAGENTA,CYAN,WEISS ; Bildfarbe je Abschnitt (Mandelbrot bunt)
SBND    dw      0,144,280,360,440,512   ; Grenzen (Schritte ab Start/Ziel)
SLEN    dw      144,136,80,80,72        ; Laengen (Schritte)
SCHKLO  dw      6E78h,1020,1388,1899,0  ; Pruefsummen (Kreis: 01AF6E78h = 28274296)
SCHKHI  dw      01AFh,0,0,0,0
SCHKON  db      1,1,1,1,0               ; Mandelbrot: keine Pruefung (Genauigkeit)
; Beschriftung an der Strecke: x, y, Text (0 = Ende, 1 = naechste Zeile)
LABS    dw      494,146
        db      'WURZEL-',1,'KURVE',0
        dw      252,226
        db      'SIERPINSKI-GERADE',0
        dw      66,146
        db      'DIVISIONS-',1,'KEHRE',0
        dw      24,42
        db      'ERATOSTHENES-KURVE',0
        dw      260,28
        db      'MANDELBROT-SCHIKANE',0
        dw      0FFFFh

; 68008-Briefkasten (68000-Format): Status, Zeilen, Abbruch, Test,
; x0, y0, dx, dy (4.28), W, H, Tiefe (Worte), Arbeitsplatz, Ergebnis
M68BXR  db      0,0,0,0
        db      0DDh,099h,080h,000h     ; x0 = -17613 * 2^15
        db      012h,0C0h,000h,000h     ; y0 =   9600 * 2^15
        db      001h,018h,000h,000h     ; dx =    560 * 2^15
        db      0FEh,070h,000h,000h     ; dy =   -800 * 2^15 (68008 addiert)
        db      000h,QW,000h,QH,000h,QMAX
        db      0,0,0,0,0,0
        db      0,0,0,0                 ; +32 Ergebnis

RINGQ   db      BLAU,CYAN,GRUEN,GELB,ROT,MAGENTA

T_BANNER db     'DMVRACE - Prozessor-Rennen fuer die NCR Decision Mate V',0Dh,0Ah,'$'
T_HELP  db      'DMVRACE [/Z] [/K] [/8] [/M] [/C]',0Dh,0Ah
        db      '/Z ohne Z80, /K ohne 68008 (K234), /8 ohne 8087, /M Mono, /C Farbe',0Dh,0Ah,'$'
T_PITNO db      'PIT-Kanal 0 zaehlt nicht - Zeit ueber die DOS-Uhr, Z80 faehrt nicht mit',0Dh,0Ah,'$'
T_ZOVL  db      'Z80: kein freier Bereich in Bank 0 - Z80 faehrt nicht mit',0Dh,0Ah,'$'
T_MEM   db      'Kein Speicher fuer den 68008-Block',0Dh,0Ah,'$'
T_68RUN db      'Achtung: der 68008 rechnet noch - bitte die DMV neu starten!',0Dh,0Ah,'$'
T_TITLE db      'DMV GRAND PRIX',0
T_QUAL  db      'QUALIFYING',0
T_ZWAIT db      'Z80 RECHNET - DER 8088 WARTET',0
T_START db      'TASTE = START, ESC = ENDE',0
T_AGAIN db      'J = NOCHMAL, ESC = ENDE',0
T_RACEN db      'RENNEN',0
T_QSEC  db      ': ALLE RECHNEN',0
T_RACE  db      'RENNEN UEBER 3 RUNDEN',0
T_ZIEL  db      'ZIEL! SIEGER: ',0
T_KEY   db      'TASTE = ENDE',0
T_TOT   db      'GESAMT-',0
T_LAPR  db      'RUNDE',0
T_DASH  db      '  -',0
T_FALSE db      'FALSCH',0
T_BOX   db      ' BOX',0
T_LAPX  db      ' 1/3',0
W_NO87  db      'KEIN 8087',0
W_NO68  db      'KEIN K234',0
W_OFF   db      'AUS',0
W_ZMEM  db      'KEIN RAUM',0
W_NOT   db      'OHNE ZEIT',0
W_DQ    db      'FALSCH',0
T_NUM   db      '000,00',0
T_RUNX  db      'RECHNET',0
T_WIN   db      40 dup (0)

        EVEN
E_OK    db      4 dup (0)               ; 1 = faehrt mit
E_WHY   dw      4 dup (0)               ; Grund, wenn nicht
E_T     dw      4*NSEC dup (0)          ; Rechenzeit je Abschnitt (1/50 s)
E_TOT   dw      4 dup (0)               ; Summe
E_CUM   dw      4*(NSEC+1) dup (0)      ; Zeit bis Abschnittsanfang (in der Runde)
E_HAS   db      4 dup (0)               ; Bit Test-1: Bilddaten vorhanden
E_OX    dw      4 dup (0)               ; gezeichnete Lage
E_OY    dw      4 dup (0)
E_LAP   db      4 dup (0)
E_DRAW  db      4 dup (0)
E_DIR   db      4 dup (0)               ; gezeichnete Lage 0..7 (45 Grad)
v_spcol db      0                       ; csprite: Farbe
v_spp   dw      0                       ; csprite: Muster
v_noz   db      0
v_nok   db      0
v_pitok db      0
v_zok   db      0
v_v20   db      0
v_lead  db      0
v_done  db      0
v_car   db      0                       ; Wagen im Qualifying
v_sec   db      0                       ; Abschnitt im Qualifying
v_meas  db      0                       ; gemessene Abschnitte (Runde 1)
v_psec  db      0FFh                    ; Abschnitt der Bilder unten
v_ptest db      0
v_abort db      0
v_win   db      0
        EVEN
v_tmin  dw      0
v_t0    dw      0
v_d0lo  dw      0
v_d0hi  dw      0
v_cell  dw      0
v_row   dw      0
v_col_  dw      0
v_rep   dw      0
v_rowc  dw      0
v_n     dw      0
v_cnt   dw      0
v_x     dw      0
v_k     dw      0
v_slo   dw      0
v_shi   dw      0
v_q     dd      0
v_rlo   dw      0                       ; Ergebnis des letzten Tests
v_rhi   dw      0
v_zbase dw      0                       ; Z80-Kern: Basis in Bank 0
v_zseg  dw      0                       ; dieselbe als Segment
v_t     dw      0                       ; Rennzeit (1/50 s)
v_tgt   dw      0
v_lapn  dw      0
v_pp    dw      0                       ; Bildzeiger der Kerne
v_pend  dw      0
v_c40   dw      0
v_y     dw      0
v_ovs   dw      0                       ; ovpics: Abschnitt
v_ovc   db      0                       ; ovpics: Farbe
v_ovx   dw      0                       ; ovpics: linker Rand
v_ovw   dw      0                       ; ovpics: Wagen*Bildgroesse
v_srcseg dw     0
VEC68   db      8 dup (0)
IVTSAV  db      8 dup (0)
TRAMP   db      0D3h,0D0h,000h,0C3h,0,0 ; OUT (D0h),A / NOP / JP Kern
PLG     dw      40 dup (0)              ; eine Bildzeile ueber alle vier Bilder, gruen
PLR     dw      40 dup (0)              ; rot
PLB     dw      40 dup (0)              ; blau
TROW    dw      10 dup (0)              ; Bildzeile eines Wagens (160 Punkte)
v_fph   db      0                       ; Flagge: Wehphase 0/1
v_flc   db      0                       ; Flagge: Bildzaehler
FOUR87  dq      4.0
SCALE87 dq      0.0001220703125         ; 1/8192
R2DD    dd      36000000                ; R^2
CW87    dw      0F7Fh                   ; 64 Bit, abschneiden, alles maskiert

        INCLUDE GDCVAR.INC

;=======================================================================
;  Code
;=======================================================================

        INCLUDE GDCLIB.INC

; AX * 40
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

;-----------------------------------------------------------------------
; Zeit: PIT-Kanal 0 lesen (zaehlt mit 50 Hz abwaerts) -> AX
;-----------------------------------------------------------------------
pitrd   PROC    NEAR
        mov     al,00h                  ; Kanal 0 festhalten
        out     P_PITC,al
        in      al,P_PIT0
        mov     ah,al
        in      al,P_PIT0
        xchg    al,ah
        ret
pitrd   ENDP

; Zeit merken (PIT oder DOS-Uhr)
tstart  PROC    NEAR
        push    ax
        push    dx
        cmp     v_pitok,0
        je      tst1
        call    pitrd
        mov     v_t0,ax
        jmp     short tst9
tst1:   call    dostime
        mov     v_d0lo,ax
        mov     v_d0hi,dx
tst9:   pop     dx
        pop     ax
        ret
tstart  ENDP

; vergangene Zeit in 1/50 s -> AX (mindestens 1)
tstop   PROC    NEAR
        push    dx
        push    bx
        cmp     v_pitok,0
        je      tsp1
        call    pitrd
        mov     dx,v_t0
        sub     dx,ax                   ; Zaehler laeuft abwaerts
        mov     ax,dx
        jmp     short tsp8
tsp1:   call    dostime                 ; Hundertstel -> /2
        sub     ax,v_d0lo
        sbb     dx,v_d0hi
        jns     tsp2
        add     ax,0D600h
        adc     dx,0083h
tsp2:   cmp     dx,1
        jb      tsp3
        mov     ax,0FFFFh
        jmp     short tsp8
tsp3:   shr     ax,1
tsp8:   or      ax,ax
        jnz     tsp9
        inc     ax
tsp9:   pop     bx
        pop     dx
        ret
tstop   ENDP

; Uhrzeit (DOS Fn 2Ch) in Hundertstelsekunden seit Mitternacht -> DX:AX
dostime PROC    NEAR
        push    bx
        push    cx
        mov     ah,2Ch
        int     21h
        push    dx
        mov     al,ch
        mov     ah,0
        mov     bl,60
        mul     bl
        mov     bl,cl
        mov     bh,0
        add     ax,bx
        mov     bx,6000
        mul     bx
        pop     cx                      ; CH = Sekunden, CL = Hundertstel
        push    dx
        push    ax
        mov     al,ch
        mov     ah,0
        mov     bl,100
        mul     bl
        mov     bl,cl
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

; laeuft der PIT-Kanal 0? -> v_pitok
pitchk  PROC    NEAR
        call    pitrd
        mov     bx,ax
        mov     cx,20
pck1:   call    vsync
        loop    pck1
        call    pitrd
        sub     bx,ax
        mov     v_pitok,0
        cmp     bx,3                    ; 20 Bildwechsel = ca. 0,36 s = ca. 18 Takte
        jb      pck9
        cmp     bx,200
        ja      pck9
        mov     v_pitok,1
pck9:   ret
pitchk  ENDP

; Zeit AX (1/50 s) als "sss,hh" nach T_NUM (fuehrende Nullen als Leerzeichen)
fmtime  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    di
        xor     dx,dx
        mov     bx,50
        div     bx                      ; AX = Sekunden, DX = 1/50
        shl     dx,1                    ; Hundertstel
        push    dx
        cmp     ax,999
        jbe     fmt0
        mov     ax,999
fmt0:   mov     di,offset T_NUM+2
        mov     cx,3
        mov     bx,10
fmt1:   xor     dx,dx
        div     bx
        add     dl,'0'
        mov     [di],dl
        dec     di
        loop    fmt1
        mov     di,offset T_NUM         ; fuehrende Nullen weg
        mov     cx,2
fmt2:   cmp     byte ptr [di],'0'
        jne     fmt3
        mov     byte ptr [di],' '
        inc     di
        loop    fmt2
fmt3:   pop     ax
        mov     bl,10
        div     bl
        add     al,'0'
        add     ah,'0'
        mov     byte ptr T_NUM+4,al
        mov     byte ptr T_NUM+5,ah
        pop     di
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
fmtime  ENDP

;=======================================================================
;  Rechenkerne 8088 / 8087 (Ergebnis DX:AX)
;=======================================================================

; 3.13-Quadrat
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
        cmp     cx,QMAX
        jb      m13a
m13z:   mov     ax,cx
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
mandel13 ENDP

; 8087: Iterationen fuer (v_mr, v_mi)/8192 -> AX  (Stapel: zr zi cr ci)
mandel87 PROC   NEAR
        push    cx
        finit
        fild    v_mi
        fmul    SCALE87                 ; ci
        fild    v_mr
        fmul    SCALE87                 ; cr
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
        cmp     cx,QMAX
        jb      m87a
m87z:   mov     ax,cx
        pop     cx
        ret
mandel87 ENDP

; Mandelbrot-Raster fuer Wagen v_car (1 = 8088, 2 = 8087) -> RESBUF
t_mand  PROC    NEAR
        mov     al,v_car
        mov     ah,0
        mov     bx,NCELL
        mul     bx
        add     ax,offset RESBUF
        mov     di,ax
        mov     v_mi,QY0
        mov     v_row,QH
tm1:    mov     v_mr,QX0
        mov     v_col_,QW
tm2:    cmp     v_car,2
        jne     tm3
        call    mandel87
        jmp     short tm4
tm3:    call    mandel13
tm4:    mov     [di],al
        inc     di
        add     v_mr,QDX
        dec     v_col_
        jnz     tm2
        sub     v_mi,QDY
        dec     v_row
        jnz     tm1
        xor     ax,ax
        xor     dx,dx
        ret
t_mand  ENDP

; Sierpinski: Zeile (256 Bit, 16 Worte) ^= Zeile << 1
t_sier  PROC    NEAR
        cld
        mov     al,v_car                ; Bild: PSIER + Wagen*1300
        mov     ah,0
        mov     bx,1300
        mul     bx
        add     ax,offset PSIER
        mov     v_pp,ax
        add     ax,1300
        mov     v_pend,ax
        mov     v_rep,SIER_REPS
sr1:    mov     di,offset SROW
        mov     cx,16
        xor     ax,ax
        rep     stosw
        mov     word ptr SROW,1
        call    srcopy
        mov     v_rowc,SIER_ROWS-1
sr2:    mov     si,offset SROW          ; 1. verschobene Kopie, Uebertrag durch alle Worte
        mov     di,offset STMP
        mov     cx,16
        clc
sr3:    lodsw
        rcl     ax,1
        stosw
        loop    sr3
        mov     si,offset STMP          ; 2. Zeile XOR Kopie
        mov     di,offset SROW
        mov     cx,16
sr4:    lodsw
        xor     [di],ax
        inc     di
        inc     di
        loop    sr4
        call    srcopy
        dec     v_rowc
        jnz     sr2
        dec     v_rep
        jnz     sr1
        mov     si,offset SROW          ; Summe der 32 Bytes
        mov     cx,32
        xor     bx,bx
        mov     ah,0
sr5:    lodsb
        add     bx,ax
        loop    sr5
        mov     ax,bx
        xor     dx,dx
        ret
t_sier  ENDP

; im letzten Durchgang die ersten 13 Bytes der Zeile ins Bild
srcopy  PROC    NEAR
        cmp     v_rep,1
        jne     src9
        mov     di,v_pp
        cmp     di,v_pend
        jae     src9
        mov     si,offset SROW
        mov     cx,13
        rep     movsb
        mov     v_pp,di
src9:   ret
srcopy  ENDP

; Sieb des Eratosthenes (BYTE 1981)
t_sieve PROC    NEAR
        cld
        mov     v_rep,SIEVE_ITER
tv1:    mov     di,offset FLAGS
        mov     cx,SIEVE_SIZE+1
        mov     al,1
        rep     stosb
        xor     bx,bx                   ; Zaehler
        xor     si,si                   ; i
tv2:    cmp     FLAGS[si],0
        je      tv4
        mov     dx,si                   ; Primzahl = 2i+3
        add     dx,dx
        add     dx,3
        mov     di,si
        add     di,dx                   ; k
tv3:    cmp     di,SIEVE_SIZE
        ja      tv3e
        mov     FLAGS[di],0
        add     di,dx
        jmp     short tv3
tv3e:   inc     bx
tv4:    inc     si
        cmp     si,SIEVE_SIZE
        jbe     tv2
        dec     v_rep
        jnz     tv1
        mov     ax,bx
        xor     dx,dx
        ret
t_sieve ENDP

; Primfaktoren durch Probedivision (DIV)
t_div   PROC    NEAR
        mov     al,v_car                ; Bild: PDIV + Wagen*160
        mov     ah,0
        mov     bx,160
        mul     bx
        add     ax,offset PDIV
        mov     v_pp,ax
        add     ax,160
        mov     v_pend,ax
        mov     v_n,DIV_N0
        mov     v_cnt,DIV_K
        xor     bx,bx                   ; Summe
td1:    mov     v_k,bx                  ; Summe vorher
        mov     si,v_n                  ; m
        mov     cx,2                    ; d
td2:    mov     ax,si
        xor     dx,dx
        div     cx                      ; AX = q, DX = Rest
        cmp     ax,cx
        jb      td4
        or      dx,dx
        jnz     td3
        mov     si,ax                   ; teilbar: m = q
        inc     bx
        jmp     short td2
td3:    inc     cx
        jmp     short td2
td4:    inc     bx                      ; Rest ist prim
        mov     di,v_pp                 ; Bild: Zahl der Faktoren von n
        cmp     di,v_pend
        jae     td5
        mov     ax,bx
        sub     ax,v_k
        mov     [di],al
        inc     v_pp
td5:    inc     v_n
        dec     v_cnt
        jnz     td1
        mov     ax,bx
        xor     dx,dx
        ret
t_div   ENDP

; Gitterpunkte im Viertelkreis, Wurzel Bit fuer Bit (16-Bit-Register)
t_circ  PROC    NEAR
        push    bp
        mov     al,v_car                ; Bild: PCIRC + Wagen*300
        mov     ah,0
        mov     bx,300
        mul     bx
        add     ax,offset PCIRC
        mov     v_pp,ax
        mov     v_c40,0
        mov     v_x,0
        mov     v_slo,0
        mov     v_shi,0
tc1:    mov     ax,v_x
        mul     ax                      ; x^2
        mov     si,word ptr R2DD
        mov     bp,word ptr R2DD+2
        sub     si,ax
        sbb     bp,dx                   ; N = BP:SI
        xor     di,di                   ; Wurzel
        xor     bx,bx                   ; Rest DX:BX
        xor     dx,dx
        mov     v_k,16
tc2:    shl     si,1                    ; 2 Bits von N in den Rest
        rcl     bp,1
        rcl     bx,1
        rcl     dx,1
        shl     si,1
        rcl     bp,1
        rcl     bx,1
        rcl     dx,1
        shl     di,1                    ; Wurzel * 2
        mov     ax,di                   ; Probe = 2*Wurzel + 1 (17 Bit: CX:AX)
        shl     ax,1
        mov     cx,0
        rcl     cx,1
        or      ax,1
        sub     bx,ax
        sbb     dx,cx
        jb      tc3
        inc     di
        jmp     short tc4
tc3:    add     bx,ax
        adc     dx,cx
tc4:    dec     v_k
        jnz     tc2
        mov     ax,di
        call    c40st
        add     v_slo,di
        adc     v_shi,0
        inc     v_x
        cmp     v_x,SQ_R
        jb      tc1
        mov     ax,v_slo
        mov     dx,v_shi
        pop     bp
        ret
t_circ  ENDP

; dasselbe mit dem 8087: FSQRT, FISTP mit Abschneiden
t_circ87 PROC   NEAR
        mov     al,v_car                ; Bild: PCIRC + Wagen*300
        mov     ah,0
        mov     bx,300
        mul     bx
        add     ax,offset PCIRC
        mov     v_pp,ax
        mov     v_c40,0
        finit
        fldcw   CW87
        mov     v_x,0
        mov     v_slo,0
        mov     v_shi,0
tk1:    fild    v_x
        fmul    st,st(0)
        fisubr  R2DD                    ; R^2 - x^2
        fsqrt
        fistp   v_q
        fwait
        mov     ax,word ptr v_q
        call    c40st
        mov     ax,word ptr v_q
        add     v_slo,ax
        mov     ax,word ptr v_q+2
        adc     v_shi,ax
        inc     v_x
        cmp     v_x,SQ_R
        jb      tk1
        finit
        mov     ax,v_slo
        mov     dx,v_shi
        ret
t_circ87 ENDP

; Bild: jede 40. Wurzel (AX) ablegen
c40st   PROC    NEAR
        cmp     v_c40,0
        jne     c4s1
        push    di
        mov     di,v_pp
        mov     [di],ax
        add     v_pp,2
        pop     di
        mov     v_c40,40
c4s1:   dec     v_c40
        ret
c40st   ENDP

; Test AL (1..5) auf dem 8088 (v_car = 1) bzw. mit 8087 (v_car = 2) -> DX:AX
t_cpu   PROC    NEAR
        cmp     al,1
        jne     tcp2
        jmp     t_mand
tcp2:   cmp     al,2
        jne     tcp3
        jmp     t_sier
tcp3:   cmp     al,3
        jne     tcp4
        jmp     t_sieve
tcp4:   cmp     al,4
        jne     tcp5
        jmp     t_div
tcp5:   cmp     v_car,2
        jne     tcp6
        jmp     t_circ87
tcp6:   jmp     t_circ
t_cpu   ENDP

; 8087 suchen (nur FN-Befehle ohne WAIT - ohne 8087 wuerde WAIT haengen)
det87   PROC    NEAR
        mov     v_has87,0
        cmp     v_lvl,8
        je      d87x
        fninit
        mov     cx,40
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

; V20 erkennen: PUSH imm8 (6Ah) kennt der V20; der 8088 fuehrt 60h..6Fh wie
; 70h..7Fh aus, also hier JP +0 (wirkt nicht)
detv20  PROC    NEAR
        mov     v_v20,0
        mov     bx,sp
        db      6Ah,00h                 ; V20: PUSH 0   8088: JP $+2
        cmp     bx,sp
        mov     sp,bx
        je      dv9
        mov     v_v20,1
        mov     si,offset TV20          ; Namen anpassen
        mov     di,offset TN1
        mov     cx,4
        rep     movsb
        mov     si,offset TV20P
        mov     di,offset TN2
        mov     cx,9
        rep     movsb
        mov     si,offset TV20S
        mov     di,offset TS1
        mov     cx,4
        rep     movsb
dv9:    ret
detv20  ENDP
TV20    db      'V20',0
TV20P   db      'V20+8087',0
TV20S   db      'V20',0

;=======================================================================
;  68008 (K234): je Test ein Start; CF = 1 wenn er nicht antwortet
;=======================================================================
rd68    PROC    NEAR
        push    es
        mov     es,v_s68
        mov     al,es:[bx]
        pop     es
        ret
rd68    ENDP

; Block vorbereiten (einmal): leeren, Code ablegen, Reset-Vektoren
p68     PROC    NEAR
        push    es
        mov     es,v_s68
        cld
        xor     di,di
        xor     ax,ax
        mov     cx,4000h/2
        rep     stosw
        mov     si,offset R68CODE
        mov     di,100h
        mov     cx,R68LEN
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
        ret
p68     ENDP

; Test AL starten und abwarten; CF = 1: keine Antwort / abgebrochen
q68     PROC    NEAR
        push    es
        mov     es,v_s68
        cld
        mov     si,offset M68BXR
        xor     di,di
        mov     cx,36
        rep     movsb
        mov     byte ptr es:[3],al      ; Test
        ; 8088-Vektoren 0 und 1 sichern, 68000-Vektoren eintragen, starten
        xor     ax,ax
        mov     es,ax
        cli
        mov     bx,0
q68v1:  mov     al,es:[bx]
        mov     IVTSAV[bx],al
        mov     al,VEC68[bx]
        mov     es:[bx],al
        inc     bx
        cmp     bx,8
        jb      q68v1
        call    tstart
        out     0D8h,al                 ; 68008: Reset und los
        ; warten, bis der 68008 "laeuft" meldet (hoechstens ca. 3 s)
        mov     cx,0
q68w:   mov     bx,0
        call    rd68
        or      al,al
        jnz     q68st
        loop    q68w
q68st:  push    ax
        mov     bx,0
q68v2:  mov     al,IVTSAV[bx]
        mov     es:[bx],al
        inc     bx
        cmp     bx,8
        jb      q68v2
        sti
        pop     ax
        pop     es
        or      al,al
        jnz     q68go
        stc
        ret
q68go:  mov     v_s68run,1
q68p:   mov     bx,0                    ; auf "fertig" warten
        call    rd68
        cmp     al,2
        jae     q68f
        call    poll_key                ; ESC: aufgeben (68008 laeuft weiter)
        cmp     v_key,2
        jne     q68p
        mov     v_key,0
        stc
        ret
q68f:   call    tstop
        mov     cx,ax
        mov     v_s68run,0
        push    es
        mov     es,v_s68
        mov     ah,byte ptr es:[32]     ; Ergebnis (Big Endian) -> DX:AX
        mov     al,byte ptr es:[33]
        mov     dx,ax
        mov     ah,byte ptr es:[34]
        mov     al,byte ptr es:[35]
        pop     es
        clc
        ret
q68     ENDP

; Mandelbrot-Ergebnisse des 68008 holen
;=======================================================================
;  Ulam-Spirale aus den Sieb-Flags: DS:SI = Flags (Byte i <> 0: 2i+3 prim),
;  ES:DI = Bild 100 Zeilen x 13 Bytes (Bit 0 = links). Zahl 1 bei (49,50),
;  dann rechts 1, hoch 1, links 2, runter 2, rechts 3 ... bis 10000.
;=======================================================================
ulam    PROC    NEAR
        push    bp
        push    di
        mov     cx,650
        xor     ax,ax
        rep     stosw
        pop     di
        mov     dl,49                   ; x
        mov     dh,50                   ; y
        mov     bp,1                    ; n
        mov     cx,1                    ; Lauflaenge
        mov     bl,0                    ; Richtung 0 rechts, 1 hoch, 2 links, 3 runter
ul1:    push    cx
ul2:    call    uplot
        inc     bp
        cmp     bp,10001
        jae     ul9
        cmp     bl,0
        jne     ul3
        inc     dl
        jmp     short ul6
ul3:    cmp     bl,1
        jne     ul4
        dec     dh
        jmp     short ul6
ul4:    cmp     bl,2
        jne     ul5
        dec     dl
        jmp     short ul6
ul5:    inc     dh
ul6:    loop    ul2
        pop     cx
        inc     bl
        and     bl,3
        test    bl,1
        jnz     ul1
        inc     cx                      ; nach "hoch" und "runter" eins laenger
        jmp     short ul1
ul9:    pop     cx
        pop     bp
        ret
ulam    ENDP

; Punkt fuer Zahl BP bei (DL,DH) setzen, wenn BP prim ist
uplot   PROC    NEAR
        cmp     dl,100
        jae     up9
        cmp     dh,100
        jae     up9
        mov     ax,bp
        cmp     ax,2
        je      up1
        test    al,1
        jz      up9
        cmp     ax,3
        jb      up9
        sub     ax,3
        shr     ax,1
        push    bx
        mov     bx,ax
        cmp     byte ptr [si+bx],0
        pop     bx
        je      up9
up1:    push    bx
        push    cx
        mov     al,dh
        mov     ah,13
        mul     ah                      ; AX = y*13 (DX bleibt)
        mov     bl,dl
        mov     bh,0
        mov     cl,3
        shr     bx,cl
        add     bx,ax
        add     bx,di
        mov     cl,dl
        and     cl,7
        mov     al,1
        shl     al,cl
        or      es:[bx],al
        pop     cx
        pop     bx
up9:    ret
uplot   ENDP

; Bilddaten von Test AL fuer Wagen v_car holen (aus Z80-, 68008- oder eigenem Speicher)
getpic  PROC    NEAR
        push    bp
        push    ds
        push    es
        cld
        mov     v_ptest,al
        mov     bl,v_car
        mov     bh,0
        mov     dx,ds
        mov     es,dx                   ; ES = Programm
        mov     v_srcseg,dx             ; Quelle: Programm
        cmp     bl,0
        jne     gp1
        mov     dx,v_zseg
        mov     v_srcseg,dx
gp1:    cmp     bl,3
        jne     gp2
        mov     dx,v_s68
        mov     v_srcseg,dx
gp2:    mov     al,v_ptest
        cmp     al,1
        jne     gp_2
        ; Mandelbrot (Wagen 1/2 rechnen direkt nach RESBUF)
        mov     si,ZRESO
        cmp     bl,0
        je      gp1a
        mov     si,1000h
        cmp     bl,3
        je      jx1093
        jmp     gp_x
jx1093:
gp1a:   mov     ax,bx
        mov     cx,NCELL
        mul     cx
        add     ax,offset RESBUF
        mov     di,ax
        mov     cx,NCELL
        mov     ds,v_srcseg
        rep     movsb
        jmp     gp_x
gp_2:   cmp     al,2
        jne     gp_3
        ; Sierpinski
        cmp     bl,0
        je      gp2z
        cmp     bl,3
        je      jx1109
        jmp     gp_x
jx1109:
        mov     ax,1300                 ; 68008: 16 Bytes je Zeile, Langworte big endian
        mul     bx
        add     ax,offset PSIER
        mov     di,ax
        mov     si,3500h
        mov     ds,v_srcseg
        mov     dx,100
gp2a:   mov     cx,13
        mov     bx,0
gp2b:   mov     ax,bx                   ; Quelle: (j and not 3) + 3 - (j and 3)
        and     ax,0FFFCh
        add     ax,3
        mov     bp,bx
        and     bp,3
        sub     ax,bp
        push    si
        add     si,ax
        lodsb
        pop     si
        stosb
        inc     bx
        loop    gp2b
        add     si,16
        dec     dx
        jnz     gp2a
        jmp     gp_x
gp2z:   mov     di,offset PSIER         ; Z80: wie abgelegt
        mov     si,ZSIERO
        mov     cx,1300
        mov     ds,v_srcseg
        rep     movsb
        jmp     gp_x
gp_3:   cmp     al,3
        jne     gp_4
        ; Sieb: Ulam-Spirale
        mov     ax,1300
        mul     bx
        add     ax,offset PSIEV
        mov     di,ax
        mov     si,offset FLAGS
        cmp     bl,0
        jne     gp3a
        mov     si,ZFLAGSO
gp3a:   cmp     bl,3
        jne     gp3b
        mov     si,1400h
gp3b:   mov     ds,v_srcseg
        call    ulam
        jmp     short gp_x
gp_4:   cmp     al,4
        jne     gp_5
        ; Division (Wagen 1/2 direkt)
        mov     si,ZDIVO
        cmp     bl,0
        je      gp4a
        mov     si,3C00h
        cmp     bl,3
        jne     gp_x
gp4a:   mov     ax,160
        mul     bx
        add     ax,offset PDIV
        mov     di,ax
        mov     cx,160
        mov     ds,v_srcseg
        rep     movsb
        jmp     short gp_x
gp_5:   ; Kreis (Wagen 1/2 direkt; 68008 big endian)
        mov     ax,300
        mul     bx
        add     ax,offset PCIRC
        mov     di,ax
        mov     cx,150
        cmp     bl,0
        jne     gp5a
        mov     si,ZCIRCO
        mov     ds,v_srcseg
        rep     movsw
        jmp     short gp_x
gp5a:   cmp     bl,3
        jne     gp_x
        mov     si,3E00h
        mov     ds,v_srcseg
gp5b:   lodsw
        xchg    al,ah
        stosw
        loop    gp5b
gp_x:   pop     es
        pop     ds
        pop     bp
        ret
getpic  ENDP

;=======================================================================
;  Z80: Kern in einen freien Teil von Bank 0 legen
;=======================================================================
; Ueberlappt [AX, AX+CX) (physisch, unter 64 KB) das eigene Programm?
; eigenes Programm: CS*16 .. CS*16 + ENDPROG (Code, Daten, Stapel)  CF = 1: ja
zhit    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     bx,ax                   ; Bereich [BX, BX+SI)
        mov     si,cx
        mov     ax,cs                   ; Programmbeginn physisch DX:AX
        mov     dx,ax
        mov     cl,4
        shl     ax,cl
        mov     cl,12
        shr     dx,cl
        or      dx,dx
        jnz     zh_free                 ; Programm liegt ab 64 KB
        mov     cx,ax                   ; Ueberlappung, wenn BX < Ende und BX+SI > Anfang
        add     cx,offset ENDPROG
        jc      zh_c1                   ; Programm reicht ueber 64 KB
        cmp     bx,cx
        jae     zh_free
zh_c1:  mov     cx,bx
        add     cx,si
        jc      zh_hit
        cmp     cx,ax
        jbe     zh_free
zh_hit: stc
        jmp     short zh_9
zh_free: clc
zh_9:   pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
zhit    ENDP

; Basis waehlen: hoechste 256-Byte-Grenze zwischen 2000h und F000h-Z80SIZE,
; die weder das Programm noch das Sprungbrett F10Eh beruehrt -> v_zbase
zplace  PROC    NEAR
        mov     v_zok,0
        mov     ax,Z80ORG
        mov     cx,6
        call    zhit
        jc      zp9
        mov     ax,(0F000h-Z80SIZE) AND 0FF00h
zp1:    cmp     ax,2000h
        jb      zp9
        mov     cx,Z80SIZE
        call    zhit
        jc      zp2
        mov     v_zbase,ax
        mov     cl,4
        shr     ax,cl
        mov     v_zseg,ax
        mov     v_zok,1
        ret
zp2:    sub     ax,100h
        jmp     short zp1
zp9:    ret
zplace  ENDP
Z80ORG  EQU     0F10Eh                  ; Sprungbrett

; Liegt das Programm selbst ueber F10Eh oder ist kein Bereich frei, zieht es
; in einen neuen DOS-Block um (der liegt hinter dem alten, also hoeher) und
; laeuft dort weiter. Danach noch einmal zplace.
zmove   PROC    NEAR
        call    zplace
        cmp     v_zok,0
        jne     zm9
        mov     bx,offset ENDPROG+15
        mov     cl,4
        shr     bx,cl
        mov     ah,48h
        int     21h
        jc      zm9
        push    es
        mov     es,ax                   ; ganzes Segment samt PSP und Stapel kopieren
        xor     si,si
        xor     di,di
        mov     cx,offset ENDPROG
        cld
        rep     movsb
        pop     es
        push    ax                      ; weiter im neuen Segment
        mov     bx,offset zm1
        push    bx
        retf
zm1:    mov     ax,cs
        mov     ds,ax
        mov     es,ax
        cli
        mov     ss,ax
        sti
        call    zplace
zm9:    ret
zmove   ENDP

; Bereiche sichern, Kern ablegen und anpassen, Sprungbrett schreiben (CLI!)
zbegin  PROC    NEAR
        push    ds
        push    es
        cld
        mov     ax,ds
        mov     es,ax
        mov     di,offset ZSAVE
        mov     ds,v_zseg               ; Kernbereich sichern
        xor     si,si
        mov     cx,Z80SIZE
        rep     movsb
        xor     ax,ax                   ; Sprungbrett-Bytes sichern
        mov     ds,ax
        mov     si,Z80ORG
        mov     cx,6
        rep     movsb
        pop     es
        pop     ds
        push    es
        mov     es,v_zseg               ; Kern ablegen
        xor     di,di
        mov     si,offset Z80CODE
        mov     cx,Z80LEN
        rep     movsb
        mov     al,byte ptr v_zbase+1   ; anpassen: Seite addieren
        mov     si,offset Z80REL
        mov     cx,Z80NREL
zb1:    mov     bx,[si]
        add     es:[bx],al
        add     si,2
        loop    zb1
        mov     di,ZX0O                 ; Mandelbrot-Parameter
        mov     ax,QX0
        stosw
        mov     ax,QY0
        stosw
        mov     ax,QDX
        stosw
        mov     ax,QDY
        stosw
        mov     al,QW
        stosb
        mov     al,QH
        stosb
        mov     al,QMAX
        stosb
        xor     ax,ax                   ; Sprungbrett: JP Basis+start
        mov     es,ax
        mov     ax,v_zbase
        add     ax,ZSTARTO
        mov     word ptr TRAMP+4,ax
        mov     si,offset TRAMP
        mov     di,Z80ORG
        mov     cx,6
        rep     movsb
        pop     es
        ret
zbegin  ENDP

; Test AL auf dem Z80 -> DX:AX (CLI!), Zeit -> CX
zrun    PROC    NEAR
        push    es
        mov     es,v_zseg
        mov     byte ptr es:[ZTESTO],al
        mov     byte ptr es:[ZSTATO],0
        out     P_BANK,al               ; Z80 auf Bank 0 (auch DMA-Seite)
        call    tstart
        out     P_SW16,al               ; Z80 rechnet, der 8088 steht hier
zr1:    cmp     byte ptr es:[ZSTATO],2
        jne     zr1                     ; (falls der 8088 noch kurz weiterlief)
        call    tstop
        mov     cx,ax
        mov     ax,es:[ZRESULTO]
        mov     dx,es:[ZRESULTO+2]
        pop     es
        ret
zrun    ENDP

; Ergebnisse holen, Bereiche zuruecklegen
zend    PROC    NEAR
        push    ds
        push    es
        cld
        mov     ax,ds
        mov     es,ax
        mov     di,offset RESBUF        ; Mandelbrot (Wagen 0)
        mov     ds,v_zseg
        mov     si,ZRESO
        mov     cx,NCELL
        rep     movsb
        pop     es
        pop     ds
        push    es
        mov     es,v_zseg
        xor     di,di
        mov     si,offset ZSAVE
        mov     cx,Z80SIZE
        rep     movsb
        xor     ax,ax
        mov     es,ax
        mov     di,Z80ORG
        mov     cx,6
        rep     movsb
        pop     es
        ret
zend    ENDP

;=======================================================================
;  Bildaufbau
;=======================================================================
; Linienzug BX (NBRD Punkte x,y, geschlossen), Farbe v_col
polyl   PROC    NEAR
        push    cx
        push    si
        mov     si,bx
        mov     cx,NBRD
pl1:    mov     ax,[si]
        mov     v_x0,ax
        mov     ax,[si+2]
        mov     v_y0,ax
        add     si,4
        cmp     cx,1
        jne     pl2
        mov     si,bx                   ; letzter Punkt -> erster
pl2:    mov     ax,[si]
        mov     v_x1,ax
        mov     ax,[si+2]
        mov     v_y1,ax
        call    line
        loop    pl1
        pop     si
        pop     cx
        ret
polyl   ENDP

; Querstrich ueber die Strecke an Streckenpunkt AX (0..NTRK-1), Farbe v_col
xline   PROC    NEAR
        push    bx
        shr     ax,1                    ; Randlinien haben NTRK/4 Punkte
        shr     ax,1
        shl     ax,1
        shl     ax,1                    ; *4 Bytes
        mov     bx,ax
        mov     ax,BRD0[bx]
        mov     v_x0,ax
        mov     ax,BRD0[bx+2]
        mov     v_y0,ax
        mov     ax,BRD1[bx]
        mov     v_x1,ax
        mov     ax,BRD1[bx+2]
        mov     v_y1,ax
        call    line
        pop     bx
        ret
xline   ENDP

; Start/Ziel-Linie
sfline  PROC    NEAR
        mov     v_col,WEISS
        mov     v_lpat,0CCCCh
        mov     ax,TRKOFS
        call    xline
        mov     v_lpat,0FFFFh
        ret
sfline  ENDP

; Strecke mit Abschnittsgrenzen und Namen
track   PROC    NEAR
        mov     v_dmode,0
        mov     v_col,BLAU
        mov     v_lpat,0F0F0h           ; Spurlinien gestrichelt
        mov     bx,offset BRD2
        call    polyl
        mov     bx,offset BRD3
        call    polyl
        mov     bx,offset BRD4
        call    polyl
        mov     v_lpat,0FFFFh
        mov     v_col,WEISS
        mov     bx,offset BRD0
        call    polyl
        mov     bx,offset BRD1
        call    polyl
        mov     v_col,MAGENTA           ; Abschnittsgrenzen
        mov     si,2
trk1:   mov     ax,SBND[si]
        add     ax,TRKOFS
        and     ax,NTRK-1
        call    xline
        add     si,2
        cmp     si,2*NSEC
        jb      trk1
        call    sfline
        ; Namen
        mov     v_tz,1
        mov     v_col,MAGENTA
        mov     si,offset LABS
trk2:   lodsw
        cmp     ax,0FFFFh
        je      trk9
        mov     v_tx,ax
        mov     cx,ax
        lodsw
        mov     v_ty,ax
trk3:   mov     bx,offset T_WIN         ; eine Zeile nach T_WIN kopieren
trk4:   lodsb
        cmp     al,1
        jbe     trk5
        mov     [bx],al
        inc     bx
        jmp     short trk4
trk5:   mov     byte ptr [bx],0
        push    si
        push    ax
        mov     si,offset T_WIN
        call    text
        pop     ax
        pop     si
        cmp     al,0
        je      trk2
        add     v_ty,10                 ; naechste Zeile
        mov     v_tx,cx
        jmp     short trk3
trk9:   ret
track   ENDP

; Text SI an (AX, v_ty) in Farbe v_col, vorher BX Punkte breit loeschen
cellt   PROC    NEAR
        push    ax
        push    bx
        push    cx
        mov     v_tx,ax
        mov     v_x0,ax
        mov     ax,v_ty
        mov     v_y0,ax
        mov     v_w,bx
        mov     v_h,8
        mov     al,SCHWARZ
        call    fillc
        mov     v_tz,1
        call    text
        pop     cx
        pop     bx
        pop     ax
        ret
cellt   ENDP

; Tabellenzelle: Wagen AL (0..3), Zeile v_ty, Text SI, Farbe v_col
tcell   PROC    NEAR
        push    ax
        push    bx
        mov     ah,0
        mov     bl,60
        mul     bl
        add     ax,TCX
        mov     bx,56
        call    cellt
        pop     bx
        pop     ax
        ret
tcell   ENDP

; Statuszeile (zentriert): Text SI, Farbe v_col
status  PROC    NEAR
        push    ax
        mov     v_x0,TBX
        mov     v_y0,TY_ST
        mov     v_w,TCX+4*60-TBX
        mov     v_h,8
        mov     al,SCHWARZ
        call    fillc
        mov     v_ty,TY_ST
        mov     v_tz,1
        call    ctext
        pop     ax
        ret
status  ENDP

; Beschriftung ueber Vorschaubild AL: Name in Wagenfarbe
qlabel  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     bl,al
        mov     bh,0
        mov     al,ECOL[bx]
        mov     v_col,al
        mov     ax,bx
        mov     cx,160
        mul     cx
        add     ax,4
        mov     v_ty,TLY
        shl     bx,1
        mov     si,ENAME[bx]
        mov     bx,76
        call    cellt
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
qlabel  ENDP

; rechte Haelfte der Beschriftung AL: Text SI, Farbe v_col (hoechstens 9 Zeichen)
qinfo   PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     ah,0
        mov     cx,160
        mul     cx
        add     ax,84
        mov     v_ty,TLY
        mov     bx,74
        call    cellt
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
qinfo   ENDP

; Farbe einer Zelle (Iterationen AL) -> AL
ccol    PROC    NEAR
        cmp     al,QMAX
        jb      cco1
        mov     al,SCHWARZ
        ret
cco1:   push    bx
        mov     ah,0
        mov     bl,6
        div     bl
        mov     bl,ah
        mov     bh,0
        mov     al,RINGQ[bx]
        pop     bx
        ret
ccol    ENDP

; Vorschaubild fuer Wagen AL aus RESBUF zeichnen (40 x 25 Zellen zu 4 x 4)
; Bildzeile Y (v_y) fuer alle Wagen in PLG/PLR/PLB bauen (Test v_ptest)
buildrow PROC   NEAR
        push    es
        mov     ax,ds
        mov     es,ax
        cld
        mov     di,offset PLG
        mov     cx,120
        xor     ax,ax
        rep     stosw
        pop     es
        cmp     v_ptest,4
        jb      br0
        ret                             ; Balken: nur loeschen
br0:    mov     v_cell,0                ; Wagen
br1:    mov     bx,v_cell
        mov     cl,byte ptr v_ptest
        dec     cl
        mov     al,1
        shl     al,cl
        test    E_HAS[bx],al
        jnz     br2
        jmp     br9
br2:    cmp     v_ptest,1
        jne     br5
        ; Mandelbrot: Zellenzeile y/4, vier Zellen je Wort
        mov     ax,bx
        mov     cx,NCELL
        mul     cx
        mov     si,ax
        mov     ax,v_y
        shr     ax,1
        shr     ax,1
        call    mul40
        add     si,ax
        add     si,offset RESBUF
        mov     di,bx                   ; Wortindex Wagen*10
        mov     ax,10
        mul     di
        shl     ax,1
        mov     di,ax
        mov     v_rowc,10
br3:    mov     cx,0
br3a:   lodsb
        call    ccol
        cmp     v_mono,0
        je      br3b
        or      al,al
        jz      br3b
        mov     al,GRUEN
br3b:   mov     dx,000Fh
        push    cx
        shl     cx,1
        shl     cx,1
        shl     dx,cl
        pop     cx
        test    al,GRUEN
        jz      br3c
        or      PLG[di],dx
br3c:   test    al,ROT
        jz      br3d
        or      PLR[di],dx
br3d:   test    al,BLAU
        jz      br3e
        or      PLB[di],dx
br3e:   inc     cx
        cmp     cx,4
        jb      br3a
        add     di,2
        dec     v_rowc
        jnz     br3
        jmp     br9
br5:    ; einfarbige Bilder (Sierpinski, Ulam): 13 Bytes ab Punkt 32 nach TROW
        push    es
        mov     ax,ds
        mov     es,ax
        mov     di,offset TROW
        mov     cx,10
        xor     ax,ax
        rep     stosw
        pop     es
        mov     ax,1300
        mul     bx
        mov     si,offset PSIER
        cmp     v_ptest,2
        je      br6
        mov     si,offset PSIEV
br6:    add     si,ax
        mov     ax,v_y
        mov     cx,13
        mul     cx
        add     si,ax
        mov     di,offset TROW+4
br7:    push    es
        mov     ax,ds
        mov     es,ax
        rep     movsb
        pop     es
        mov     al,ECOL[bx]             ; Farbe des Wagens
        cmp     v_mono,0
        je      br7a
        mov     al,GRUEN
br7a:   mov     dl,al                   ; DL = Farbe (nicht MUL verwenden!)
        mov     di,bx                   ; Wortindex Wagen*10 -> Bytes *20
        shl     di,1
        shl     di,1
        mov     ax,di
        shl     di,1
        shl     di,1
        add     di,ax
        mov     si,0
br8:    mov     ax,TROW[si]
        test    dl,GRUEN
        jz      br8a
        mov     PLG[di],ax
br8a:   test    dl,ROT
        jz      br8b
        mov     PLR[di],ax
br8b:   test    dl,BLAU
        jz      br8c
        mov     PLB[di],ax
br8c:   add     si,2
        add     di,2
        cmp     si,20
        jb      br8
br9:    inc     v_cell
        cmp     v_cell,4
        jae     br99
        jmp     br1
br99:   ret
buildrow ENDP

;=======================================================================
;  Nach Runde 1: die fuenf Ergebnisbilder (Daten des Schnellsten v_win)
;  nebeneinander, je 128 Punkte breit, jedes in seiner Farbe, Mandelbrot bunt
;=======================================================================
ovpics  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     v_x0,0                  ; Kopfzeile und Trennlinie loeschen
        mov     v_y0,TLY-1
        mov     v_w,640
        mov     v_h,THY-TLY+1
        mov     al,SCHWARZ
        call    fillc
        mov     v_ovs,0
ov1:    mov     bx,v_ovs
        mov     al,SCOL[bx]
        mov     v_col,al
        mov     ax,bx
        mov     cl,7
        shl     ax,cl                   ; x0 = s*128
        mov     v_x0,ax
        add     ax,123
        mov     v_x1,ax
        mov     v_y0,THY-2
        mov     v_y1,THY-2
        call    line
        mov     ax,v_x0
        add     ax,4
        mov     v_ty,TLY
        shl     bx,1
        mov     si,SNAME[bx]
        mov     bx,116
        call    cellt
        inc     v_ovs
        cmp     v_ovs,NSEC
        jb      ov1
        mov     v_y,0
ov2:    push    es                      ; Zeile leeren
        mov     ax,ds
        mov     es,ax
        cld
        mov     di,offset PLG
        mov     cx,120
        xor     ax,ax
        rep     stosw
        pop     es
        mov     v_ovs,0
ov3:    call    ovslot
        inc     v_ovs
        cmp     v_ovs,NSEC
        jb      ov3
        call    writerow
        inc     v_y
        cmp     v_y,100
        jb      ov2
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
ovpics  ENDP

; Punkt AX (0..639) der Zeile in Farbe DL setzen (PLG/PLR/PLB)
opset   PROC    NEAR
        push    ax
        push    bx
        push    cx
        mov     cl,al
        and     cl,15
        mov     bx,ax
        shr     bx,1
        shr     bx,1
        shr     bx,1
        and     bx,0FFFEh               ; Byte des Wortes
        mov     ax,1
        shl     ax,cl
        cmp     v_mono,0
        je      ops1
        or      dl,dl
        jz      ops9
        or      PLG[bx],ax
        jmp     short ops9
ops1:   test    dl,GRUEN
        jz      ops2
        or      PLG[bx],ax
ops2:   test    dl,ROT
        jz      ops3
        or      PLR[bx],ax
ops3:   test    dl,BLAU
        jz      ops9
        or      PLB[bx],ax
ops9:   pop     cx
        pop     bx
        pop     ax
        ret
opset   ENDP

; 13 Bytes ab SI in alle Ebenen der Farbe DL, ab Byte DI der Zeile
obytes  PROC    NEAR
        push    ax
        push    cx
        push    si
        push    di
        mov     cx,13
ob1:    lodsb
        cmp     v_mono,0
        je      ob2
        or      byte ptr PLG[di],al
        jmp     short ob5
ob2:    test    dl,GRUEN
        jz      ob3
        or      byte ptr PLG[di],al
ob3:    test    dl,ROT
        jz      ob4
        or      byte ptr PLR[di],al
ob4:    test    dl,BLAU
        jz      ob5
        or      byte ptr PLB[di],al
ob5:    inc     di
        loop    ob1
        pop     di
        pop     si
        pop     cx
        pop     ax
        ret
obytes  ENDP

; Zeile v_y des Bildes zu Abschnitt v_ovs (Daten von Wagen v_win)
ovslot  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        cld
        mov     bx,v_ovs
        mov     dl,SCOL[bx]
        mov     v_ovc,dl
        mov     ax,bx
        mov     cl,7
        shl     ax,cl
        mov     v_ovx,ax
        mov     al,STEST[bx]
        mov     bl,v_win
        mov     bh,0
        cmp     al,1
        je      osm
        cmp     al,4
        jb      osb
        je      osd
        jmp     osw
osb:    ; Sierpinski (2) / Ulam (3): 13 Bytes ab Punkt 16 der Spalte
        mov     si,offset PSIER
        cmp     al,2
        je      osb1
        mov     si,offset PSIEV
osb1:   mov     ax,1300
        mul     bx
        add     si,ax
        mov     ax,v_y
        mov     cx,13
        mul     cx
        add     si,ax
        mov     di,v_ovx
        shr     di,1
        shr     di,1
        shr     di,1
        add     di,2
        mov     dl,v_ovc                ; (MUL hat DX zerstoert)
        call    obytes
        jmp     osx
osm:    ; Mandelbrot: 40 Zellen zu 3 Punkten, Zellenzeile v_y/4
        mov     ax,NCELL
        mul     bx
        mov     si,ax
        mov     ax,v_y
        shr     ax,1
        shr     ax,1
        call    mul40
        add     si,ax
        add     si,offset RESBUF
        mov     cx,0
osm1:   lodsb
        call    ccol
        mov     dl,al
        mov     ax,cx
        shl     ax,1
        add     ax,cx                   ; 3*Zelle
        add     ax,v_ovx
        add     ax,4
        call    opset
        inc     ax
        call    opset
        inc     ax
        call    opset
        inc     cx
        cmp     cx,QW
        jb      osm1
        jmp     osx
osd:    ; Division: Balken Faktoren*6 je n (128 Spalten)
        mov     ax,160
        mul     bx
        add     ax,offset PDIV
        mov     si,ax
        mov     dl,v_ovc
        mov     cx,0
osd1:   mov     al,[si]
        mov     ah,6
        mul     ah
        call    oslit
        inc     si
        inc     cx
        cmp     cx,128
        jb      osd1
        jmp     short osx
osw:    ; Wurzel: Viertelkreis, Spalte i -> Wert (i*150/128), Hoehe /60
        mov     ax,300
        mul     bx
        add     ax,offset PCIRC
        mov     di,ax
        mov     dl,v_ovc
        mov     cx,0
osw1:   push    dx
        mov     ax,cx
        mov     bx,150
        mul     bx                      ; i*150
        mov     bl,7
        xchg    cl,bl
        shr     ax,cl                   ; /128
        xchg    cl,bl
        shl     ax,1
        mov     si,ax
        add     si,di
        mov     ax,[si]
        xor     dx,dx
        mov     bx,60
        div     bx
        pop     dx
        call    oslit
        inc     cx
        cmp     cx,128
        jb      osw1
osx:    pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
ovslot  ENDP

; Balken: Hoehe AX in Spalte CX der Slot-Spalte; Punkt setzen, wenn Zeile v_y
; im Balken liegt (Balken wachsen von unten, Zeile 99 = unten)
oslit   PROC    NEAR
        push    ax
        push    bx
        mov     bx,99
        sub     bx,v_y
        cmp     bx,ax
        jae     osl9
        mov     ax,cx
        add     ax,v_ovx
        call    opset
osl9:   pop     bx
        pop     ax
        ret
oslit   ENDP

; PLG/PLR/PLB als Zeile THY+v_y schreiben
writerow PROC   NEAR
        mov     ax,v_y
        add     ax,THY
        call    mul40
        mov     di,ax
        mov     si,offset PLG
        call    wput40
        cmp     v_mono,0
        jne     wr9
        push    di
        add     di,4000h
        mov     si,offset PLR
        call    wput40
        pop     di
        add     di,8000h
        mov     si,offset PLB
        call    wput40
wr9:    ret
writerow ENDP

; Bilder und Zeiten zum Abschnitt AL
drawpics PROC   NEAR
        mov     v_psec,al
        mov     bl,al
        mov     bh,0
        mov     al,STEST[bx]
        mov     v_ptest,al
        ; Zeiten ueber den Bildern
        mov     v_cell,0
dp1:    mov     bx,v_cell
        mov     cl,v_ptest
        dec     cl
        mov     al,1
        shl     al,cl
        test    E_HAS[bx],al
        jz      dp2
        mov     al,bl                   ; Zeit T[e][s]
        mov     cl,NSEC
        mul     cl
        add     al,v_psec
        mov     si,ax
        shl     si,1
        mov     ax,E_T[si]
        call    fmtime
        mov     si,offset T_NUM
        mov     v_col,WEISS
        jmp     short dp3
dp2:    mov     si,offset T_DASH
        mov     v_col,WEISS
        cmp     E_OK[bx],0
        jne     dp3
        shl     bx,1
        mov     si,E_WHY[bx]
        shr     bx,1
        mov     v_col,ROT
dp3:    mov     al,bl
        call    qinfo
        inc     v_cell
        cmp     v_cell,4
        jb      dp1
        ; Bild zeilenweise
        mov     v_y,0
dp4:    call    buildrow
        call    writerow
        inc     v_y
        cmp     v_y,100
        jb      dp4
        cmp     v_ptest,4
        jb      dp9
        call    bars
dp9:    ret
drawpics ENDP

; Balken fuer Division (Test 4) und Kreis (Test 5)
bars    PROC    NEAR
        mov     v_dmode,0
        mov     v_lpat,0FFFFh
        mov     v_cell,0
ba1:    mov     bx,v_cell
        mov     cl,v_ptest
        dec     cl
        mov     al,1
        shl     al,cl
        test    E_HAS[bx],al
        jnz     ba1a
        jmp     ba9
ba1a:   mov     al,ECOL[bx]
        mov     v_col,al
        mov     ax,160
        mul     bx
        mov     v_x,ax                  ; linker Rand
        mov     v_k,0
ba2:    mov     bx,v_cell
        cmp     v_ptest,4
        jne     ba3
        mov     ax,160                  ; Division: Faktoren * 6
        mul     bx
        add     ax,v_k
        mov     si,ax
        mov     al,PDIV[si]
        mov     ah,6
        mul     ah
        mov     cx,160
        jmp     short ba4
ba3:    mov     ax,300                  ; Kreis: Wurzel / 60
        mul     bx
        mov     si,ax
        mov     ax,v_k
        shl     ax,1
        add     si,ax
        mov     ax,word ptr PCIRC[si]
        xor     dx,dx
        mov     cx,60
        div     cx
        mov     cx,150
ba4:    cmp     ax,99
        jbe     ba5
        mov     ax,99
ba5:    or      ax,ax
        jz      ba6
        mov     dx,v_x
        add     dx,v_k
        cmp     v_ptest,5
        jne     ba5a
        add     dx,5
ba5a:   mov     v_x0,dx
        mov     v_x1,dx
        mov     v_y0,399
        mov     dx,400
        sub     dx,ax
        mov     v_y1,dx
        push    cx
        call    line
        pop     cx
ba6:    inc     v_k
        cmp     v_k,cx
        jae     ba9
        jmp     ba2
ba9:    inc     v_cell
        cmp     v_cell,4
        jae     ba99
        jmp     ba1
ba99:   ret
bars    ENDP

; 10 Worte ab SI nach Wortadresse DI schreiben
; 40 Worte ab SI nach Wortadresse DI schreiben
wput40  PROC    NEAR
        push    ax
        push    cx
        push    si
        call    wbegin
        mov     cx,40
wp1:    lodsw
        call    gpw
        loop    wp1
        pop     si
        pop     cx
        pop     ax
        ret
wput40  ENDP

; Text zentriert mit Schatten (ctext + stext)
ctext_s PROC    NEAR
        push    ax
        push    cx
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
        pop     cx
        pop     ax
        ret
ctext_s ENDP

; Grundbild: Titel, Strecke, Tabelle, Rahmen der Vorschaubilder
scene   PROC    NEAR
        call    sreset
        mov     v_tz,2
        mov     v_ty,4
        mov     v_col,WEISS
        mov     v_shc,ROT
        mov     v_sho,1
        mov     si,offset T_TITLE
        call    ctext_s
        call    track
        call    drawflag
        ; Tabelle: Kopf und Zeilennamen
        mov     v_ty,TY_HD
        mov     bx,0
sc0:    mov     al,ECOL[bx]
        mov     v_col,al
        mov     al,bl
        push    bx
        shl     bx,1
        mov     si,ESHORT[bx]
        pop     bx
        call    tcell
        inc     bx
        cmp     bx,4
        jb      sc0
        mov     v_col,WEISS
        mov     bx,0
sc0a:   mov     ax,bx
        mov     cl,10
        mul     cl
        add     ax,TY_S0
        mov     v_ty,ax
        push    bx
        shl     bx,1
        mov     si,SNAME[bx]
        pop     bx
        mov     ax,TBX
        push    bx
        mov     bx,84
        call    cellt
        pop     bx
        inc     bx
        cmp     bx,NSEC
        jb      sc0a
        mov     v_col,CYAN              ; GESAMT-/RUNDE hellblau
        mov     v_ty,TY_TOT
        mov     si,offset T_TOT
        mov     ax,TBX
        mov     bx,84
        call    cellt
        mov     v_ty,TY_LAP
        mov     si,offset T_LAPR
        mov     ax,TBX
        mov     bx,84
        call    cellt
        ; Rahmen und Namen der Vorschaubilder
        mov     al,0
sc1:    call    qlabel
        push    ax
        mov     ah,0
        mov     bx,ax
        mov     cx,160
        mul     cx
        mov     v_x0,ax
        add     ax,159
        mov     v_x1,ax
        mov     v_y0,THY-2
        mov     v_y1,THY-2
        mov     al,ECOL[bx]
        mov     v_col,al
        call    line
        pop     ax
        inc     al
        cmp     al,4
        jb      sc1
        ret
scene   ENDP

;=======================================================================
;  Qualifying
;=======================================================================
; Zelle Wagen v_car / Abschnitt v_sec: Text SI, Farbe v_col
qcell   PROC    NEAR
        push    ax
        mov     al,v_sec
        mov     ah,0
        mov     cl,10
        mul     cl
        add     ax,TY_S0
        mov     v_ty,ax
        mov     al,v_car
        call    tcell
        pop     ax
        ret
qcell   ENDP

; Ergebnis eines Tests verbuchen: Zeit CX, Ergebnis DX:AX
; (Wagen v_car, Abschnitt v_sec); falsche Pruefsumme -> disqualifiziert
qbook   PROC    NEAR
        push    bx
        mov     bl,v_sec
        mov     bh,0
        cmp     SCHKON[bx],0
        je      qb1
        shl     bx,1
        cmp     ax,SCHKLO[bx]
        jne     qb_dq
        cmp     dx,SCHKHI[bx]
        jne     qb_dq
qb1:    mov     al,v_car                ; Zeit eintragen
        mov     ah,0
        mov     bl,NSEC
        mul     bl
        add     al,v_sec
        mov     bx,ax
        shl     bx,1
        mov     E_T[bx],cx
        mov     bl,v_sec                ; Bilddaten gueltig
        mov     bh,0
        mov     cl,STEST[bx]
        dec     cl
        mov     al,1
        shl     al,cl
        mov     bl,v_car
        or      E_HAS[bx],al
        mov     al,v_car
        mov     ah,0
        mov     cl,NSEC
        mul     cl
        add     al,v_sec
        mov     bx,ax
        shl     bx,1
        mov     ax,E_T[bx]
        call    fmtime
        mov     si,offset T_NUM
        mov     v_col,WEISS
        call    qcell
        clc
        pop     bx
        ret
qb_dq:  mov     bl,v_car
        mov     bh,0
        shl     bx,1
        mov     E_WHY[bx],offset W_DQ
        mov     si,offset T_FALSE
        mov     v_col,ROT
        call    qcell
        stc
        pop     bx
        ret
qbook   ENDP

; Wagen v_car: alle Abschnitte rechnen. CF = 1: faehrt nicht mit (E_WHY)
; Statuszeile: Abschnittsname + ": ALLE RECHNEN"
secstat PROC    NEAR
        push    es
        mov     ax,ds
        mov     es,ax
        cld
        mov     bl,v_sec
        mov     bh,0
        shl     bx,1
        mov     si,SNAME[bx]
        mov     di,offset T_WIN
ss1:    lodsb
        or      al,al
        jz      ss2
        stosb
        jmp     short ss1
ss2:    mov     si,offset T_QSEC
ss3:    lodsb
        stosb
        or      al,al
        jnz     ss3
        pop     es
        mov     v_col,WEISS
        mov     si,offset T_WIN
        call    status
        ret
secstat ENDP

; Wagen v_car disqualifizieren / abmelden: von der Strecke in die Box
dqcar   PROC    NEAR
        push    bx
        mov     bl,v_car
        mov     bh,0
        mov     E_OK[bx],0
        call    parkcar
        pop     bx
        ret
dqcar   ENDP

; Wagen BX in die Box (Strecke freiraeumen, Tabelle "BOX")
parkcar PROC    NEAR
        push    ax
        push    cx
        push    dx
        push    si
        mov     si,bx
        shl     si,1
        cmp     E_DRAW[bx],0
        je      pc1
        mov     cx,E_OX[si]
        mov     dx,E_OY[si]
        mov     al,ECOL[bx]
        mov     ah,E_DIR[bx]
        call    csprite                 ; XOR: von der Strecke nehmen
        mov     E_DRAW[bx],0
pc1:    mov     cx,BOXX[si]
        mov     v_ty,TY_LAP             ; erst "BOX" (loescht die Zelle) ...
        mov     v_col,WEISS
        mov     si,offset T_BOX
        mov     al,bl
        call    tcell
        mov     dx,BOXY                 ; ... dann der Wagen dahinter
        mov     al,ECOL[bx]
        mov     ah,0
        call    csprite
        pop     si
        pop     dx
        pop     cx
        pop     ax
        ret
parkcar ENDP

; Abschnitt AL: alle Wagen rechnen den Test, dann Bilder
qsec    PROC    NEAR
        mov     v_sec,al
        call    secstat
        mov     si,0
qs1:    push    si
        mov     al,EORDER[si]
        mov     v_car,al
        mov     bl,al
        mov     bh,0
        cmp     E_OK[bx],0
        jne     qs2
        jmp     qs9
qs2:    mov     si,offset T_RUNX
        mov     v_col,GELB
        call    qcell
        mov     bl,v_sec
        mov     bh,0
        mov     al,STEST[bx]
        mov     v_ptest,al
        cmp     v_car,0
        je      qs_z
        cmp     v_car,3
        je      qs_68
        call    tstart                  ; 8088 / 8087
        mov     al,v_ptest
        call    t_cpu
        push    ax
        call    tstop
        mov     cx,ax
        pop     ax
        jmp     short qs_bk
qs_68:  mov     al,v_ptest
        call    q68
        jnc     qs_bk
        mov     E_WHY+6,offset W_NO68
        mov     si,offset T_DASH
        mov     v_col,WEISS
        call    qcell
        jmp     short qs_dq
qs_z:   mov     v_col,ROT
        mov     si,offset T_ZWAIT
        call    status
        cli
        call    zbegin
        mov     al,v_ptest
        call    zrun
        push    ax
        push    dx
        push    cx
        mov     al,v_ptest
        call    getpic
        call    zend
        sti
        pop     cx
        pop     dx
        pop     ax
        push    ax
        push    cx
        push    dx
        call    secstat
        pop     dx
        pop     cx
        pop     ax
qs_bk:  call    qbook
        jc      qs_dq
        cmp     v_car,0
        je      qs9
        mov     bl,v_sec
        mov     bh,0
        mov     al,STEST[bx]
        call    getpic
        jmp     short qs9
qs_dq:  call    dqcar
qs9:    pop     si
        inc     si
        cmp     si,4
        jae     qs99
        jmp     qs1
qs99:   mov     al,v_sec
        inc     al
        mov     v_meas,al
        call    mkcum
        mov     al,v_sec
        call    drawpics
        ret
qsec    ENDP

; Zeiten bis zu den Abschnittsanfaengen (aus den gemessenen Abschnitten)
mkcum   PROC    NEAR
        mov     bx,0
mc1:    mov     ax,bx
        mov     cl,(NSEC+1)*2
        mul     cl
        mov     si,ax
        mov     ax,bx
        mov     cl,NSEC*2
        mul     cl
        mov     di,ax
        mov     ax,0
        mov     E_CUM[si],ax
        mov     cl,v_meas
        mov     ch,0
mc2:    add     ax,E_T[di]
        add     si,2
        add     di,2
        mov     E_CUM[si],ax
        loop    mc2
        inc     bx
        cmp     bx,4
        jb      mc1
        ret
mkcum   ENDP

; Wagen BX zur Zeit v_t: Schritte seit Start -> AX
carpos  PROC    NEAR
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     ax,bx
        mov     cl,(NSEC+1)*2
        mul     cl
        add     ax,offset E_CUM
        mov     si,ax                   ; SI -> CUM[e][0]
        mov     ax,bx
        mov     cl,NSEC*2
        mul     cl
        add     ax,offset E_T
        mov     di,ax                   ; DI -> T[e][0]
        mov     v_lapn,0
        mov     ax,v_t
        mov     cl,v_meas
        mov     ch,0
        cmp     cl,NSEC
        jb      cp1
        xor     dx,dx
        div     word ptr [si+NSEC*2]    ; AX = Runde, DX = Zeit in der Runde
        mov     v_lapn,ax
        mov     ax,dx
cp1:    mov     bx,0                    ; Abschnitt*2 suchen
cp2:    mov     dx,bx
        shr     dx,1
        inc     dx
        cmp     dx,cx
        jae     cp3
        cmp     ax,[bx+si+2]
        jb      cp3
        add     bx,2
        jmp     short cp2
cp3:    sub     ax,[bx+si]              ; Anteil am Abschnitt
        mov     cx,[bx+di]
        cmp     ax,cx
        jbe     cp4
        mov     ax,cx
cp4:    mul     word ptr SLEN[bx]
        div     cx
        add     ax,SBND[bx]
        mov     dx,v_lapn
        mov     cl,9
        shl     dx,cl
        add     ax,dx
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        ret
carpos  ENDP

; Abschnitt zur Lage AX (0..NTRK-1) -> AL
secof   PROC    NEAR
        push    bx
        mov     bx,0
so1:    cmp     ax,SBND[bx+2]
        jb      so2
        add     bx,2
        cmp     bx,(NSEC-1)*2
        jb      so1
so2:    mov     ax,bx
        shr     al,1
        pop     bx
        ret
secof   ENDP

; Wagen BX an Schritt AX zeichnen (nur wenn er sich bewegt hat)
cardraw PROC    NEAR
        push    ax
        push    cx
        push    dx
        push    si
        push    di
        mov     si,bx
        shl     si,1
        add     ax,TRKOFS
        and     ax,NTRK-1
        push    ax
        shl     ax,1
        mov     di,ax
        push    bx
        mov     bx,ETRKX[si]
        mov     cx,[bx+di]
        mov     bx,ETRKY[si]
        mov     dx,[bx+di]
        pop     bx
        pop     di                      ; Punkt (fuer alle Spuren gleich)
        mov     al,ANGTAB[di]           ; Fahrtrichtung dort
        call    cardir                  ; -> AH
        cmp     E_DRAW[bx],0
        je      cd2
        cmp     cx,E_OX[si]
        jne     cd1
        cmp     dx,E_OY[si]
        jne     cd1
        cmp     ah,E_DIR[bx]
        je      cd9
cd1:    push    ax
        push    cx
        push    dx
        mov     cx,E_OX[si]
        mov     dx,E_OY[si]
        mov     al,ECOL[bx]
        mov     ah,E_DIR[bx]
        call    csprite                 ; alte Lage loeschen (XOR)
        pop     dx
        pop     cx
        pop     ax
cd2:    mov     E_OX[si],cx
        mov     E_OY[si],dx
        mov     E_DIR[bx],ah
        mov     E_DRAW[bx],1
        mov     al,ECOL[bx]
        call    csprite
cd9:    pop     di
        pop     si
        pop     dx
        pop     cx
        pop     ax
        ret
cardraw ENDP

; Fahrtrichtung AL (256 = Vollkreis, aus ANG.INC) -> AH = Wagenlage in
; 45-Grad-Schritten (0 rechts, 1 rechts unten, 2 runter ... 7 rechts oben).
; Gedreht wird, wenn die Richtung die Mitte zwischen zwei Lagen (22,5 Grad)
; um CHYST ueberschreitet - auf allen Spuren am selben Punkt, denn die
; Spuren sind Parallelkurven mit gemeinsamer Tangente.
CHYST   EQU     2                       ; ca. 3 Grad gegen Flattern
cardir  PROC    NEAR
        push    cx
        push    dx
        mov     dl,al
        mov     ah,E_DIR[bx]
        and     ah,7
        mov     al,ah
        mov     cl,5
        shl     al,cl                   ; Richtung der jetzigen Lage
        mov     dh,dl
        sub     dh,al                   ; Abweichung (mit Vorzeichen)
        mov     al,dh
        or      al,al
        jns     cdr1
        neg     al
cdr1:   cmp     al,16+CHYST
        jbe     cdr9                    ; noch nicht ueber die Mitte
        mov     ah,dl
        add     ah,16
        mov     cl,5
        shr     ah,cl
cdr9:   pop     dx
        pop     cx
        ret
cardir  ENDP

; Wagen (CARS.INC) per XOR zeichnen - zweimal gezeichnet ist er wieder weg.
; CX,DX = Mitte, AL = Farbe, AH = Richtung. Ein GDC-Grafikzeichen (8x8) je
; Ebene: Muster = Reifen (weiss) + Karosserie, wenn die Farbe die Ebene hat.
csprite PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        mov     v_spcol,al
        push    cx
        mov     al,ah
        and     ax,7
        mov     cl,4
        shl     ax,cl
        add     ax,offset CARSPR
        mov     v_spp,ax
        pop     cx
        add     cx,3                    ; rechter Rand (Feld x-4 .. x+3)
        sub     dx,4                    ; oberste Zeile (y-4 .. y+3)
        mov     al,0
        call    setgz
        call    xyaddr
        mov     v_fdot,bl
        mov     v_faddr,di
        mov     si,offset PLTAB
        mov     cx,v_npl
cs1:    push    cx
        mov     bl,[si+2]               ; Ebenenbit
        mov     bh,1                    ; Karosserie in dieser Ebene?
        cmp     v_mono,0
        jne     cs2
        test    v_spcol,bl
        jnz     cs2
        mov     bh,0
cs2:    push    si
        mov     si,v_spp
        mov     di,offset PBUF+7        ; PBUF[k] = Zeile 7-k
        mov     cx,8
        xor     dl,dl
cs3:    mov     al,[si+8]               ; Reifen
        or      bh,bh
        jz      cs4
        or      al,[si]                 ; Karosserie
cs4:    mov     [di],al
        or      dl,al
        inc     si
        dec     di
        loop    cs3
        pop     si
        or      dl,dl
        jz      cs9
        mov     ah,v_dmode              ; Komplement-Modus dieser Ebene
        mov     v_dmode,1
        mov     al,WEISS
        call    pmode
        mov     v_dmode,ah
        call    gcmd
        mov     di,v_faddr
        add     di,[si]
        mov     bl,v_fdot
        call    curs
        mov     al,78h                  ; Muster laden
        call    gcmd
        mov     bx,offset PBUF
        mov     cx,8
cs5:    mov     al,[bx]
        call    gpar
        inc     bx
        loop    cs5
        mov     al,4Ch                  ; FIGS: Grafikzeichen 8x8, Richtung 6
        call    gcmd
        mov     al,16h
        call    gpar
        mov     ax,4007h
        call    gpw
        mov     ax,8
        call    gpw
        mov     al,68h                  ; GCHRD
        call    gcmd
cs9:    add     si,3
        pop     cx
        dec     cx
        jz      cs99
        jmp     cs1
cs99:   pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
csprite ENDP

; fahren bis Rennzeit AX (ESC: v_abort = 1)
anim    PROC    NEAR
        mov     v_tgt,ax
an1:    mov     ax,v_t
        cmp     ax,v_tgt
        jb      an2
        ret
an2:    call    vsync
        inc     v_t
        mov     bx,0
an3:    cmp     E_OK[bx],0
        je      an8
        call    carpos
        mov     dx,ax
        mov     cl,9
        shr     dx,cl
        cmp     dl,E_LAP[bx]
        je      an4
        mov     E_LAP[bx],dl
        call    lapinfo
an4:    call    cardraw
an8:    inc     bx
        cmp     bx,4
        jb      an3
an9:    call    poll_key
        cmp     v_key,0
        je      an1
        cmp     v_key,2
        je      an10
        mov     v_key,0                 ; andere Taste: weiter
        jmp     an1
an10:   mov     v_abort,1
        ret
anim    ENDP

;=======================================================================
;  Rennen
;=======================================================================

; Geschwindigkeiten: K[e][s] = SA[s] * Tmin / T[e][s] (16.16 Schritte je Bild)


; Rundenanzeige fuer Wagen BX
lapinfo PROC    NEAR
        push    ax
        push    bx
        push    si
        mov     al,E_LAP[bx]
        cmp     al,LAPS
        jb      li1
        mov     al,LAPS-1
li1:    add     al,'1'
        mov     byte ptr T_LAPX+1,al
        mov     v_ty,TY_LAP
        mov     al,ECOL[bx]
        mov     v_col,al
        mov     si,offset T_LAPX
        mov     al,bl
        call    tcell
        pop     si
        pop     bx
        pop     ax
        ret
lapinfo ENDP

race    PROC    NEAR
        mov     v_win,0FFh
        mov     v_t,0
        mov     v_meas,0
        mov     v_psec,0FFh
        mov     v_abort,0
        ; wer faehrt mit?
        mov     bx,0
rc0:    mov     E_LAP[bx],0
        mov     E_DRAW[bx],0
        mov     E_HAS[bx],0
        mov     E_OK[bx],1
        inc     bx
        cmp     bx,4
        jb      rc0
        mov     E_WHY[0],offset W_OFF
        cmp     v_noz,0
        jne     rc1
        mov     E_WHY[0],offset W_ZMEM
        cmp     v_zok,0
        je      rc1
        mov     E_WHY[0],offset W_NOT
        cmp     v_pitok,0
        jne     rc2
rc1:    mov     E_OK[0],0
rc2:    cmp     v_has87,0
        jne     rc3
        mov     E_OK[2],0
        mov     E_WHY[4],offset W_NO87
rc3:    mov     E_WHY[6],offset W_OFF
        cmp     v_nok,0
        jne     rc4
        mov     E_WHY[6],offset W_NO68
        cmp     v_s68,0
        jne     rc5
rc4:    mov     E_OK[3],0
rc5:    ; Startaufstellung
        mov     bx,0
rc6:    cmp     E_OK[bx],0
        je      rc7
        mov     ax,0
        call    cardraw
        call    lapinfo
        jmp     short rc8
rc7:    call    parkcar
rc8:    inc     bx
        cmp     bx,4
        jb      rc6
        mov     v_col,WEISS
        mov     si,offset T_START
        call    status
        call    waitkey
        cmp     al,1Bh
        jne     rc9
        mov     v_abort,1
        ret
rc9:    cmp     E_OK[3],0
        je      rc10
        call    p68
rc10:   ; Runde 1: Abschnitt fuer Abschnitt rechnen und fahren
        mov     al,0
rc11:   push    ax
        call    qsec
        call    tmin                    ; Zielzeit: Schnellster am Abschnittsende
        pop     cx
        jnc     jx2578
        jmp     rc_end
jx2578:
        push    cx
        push    ax
        mov     v_col,WEISS
        mov     si,offset T_RACEN
        call    status
        pop     ax
        call    anim
        pop     ax
        cmp     v_abort,0
        je      jx2588
        jmp     rc_end
jx2588:
        inc     al
        cmp     al,NSEC
        jb      rc11
        ; Gesamtzeiten, Sieger, Runden 2 und 3
        mov     bx,0
rc12:   cmp     E_OK[bx],0
        je      rc13
        mov     ax,bx
        mov     cl,(NSEC+1)*2
        mul     cl
        mov     si,ax
        mov     ax,E_CUM[si+NSEC*2]
        call    fmtime
        mov     v_ty,TY_TOT
        mov     v_col,WEISS
        mov     si,offset T_NUM
        mov     al,bl
        call    tcell
rc13:   inc     bx
        cmp     bx,4
        jb      rc12
        mov     v_sec,NSEC-1            ; Zielzeit: LAPS Runden des Schnellsten
        call    tmin
        jc      rc_end
        mov     v_win,bl
        push    ax
        call    ovpics                  ; alle fuenf Ergebnisbilder nebeneinander
        pop     ax
        mov     cx,LAPS
        mul     cx
        call    anim
        cmp     v_abort,0
        jne     rc_end
        ; Sieger
        push    es
        mov     ax,ds
        mov     es,ax
        cld
        mov     si,offset T_ZIEL
        mov     di,offset T_WIN
rc14:   lodsb
        or      al,al
        jz      rc15
        stosb
        jmp     short rc14
rc15:   mov     bl,v_win
        mov     bh,0
        shl     bx,1
        mov     si,ENAME[bx]
rc16:   lodsb
        stosb
        or      al,al
        jnz     rc16
        pop     es
        mov     bl,v_win
        mov     bh,0
        mov     al,ECOL[bx]
        mov     v_col,al
        mov     si,offset T_WIN
        call    status
rc_end: ret
race    ENDP

; kleinste Zeit bis zum Ende von Abschnitt v_sec (Runde 1) -> AX, Wagen -> BL
; CF = 1: kein Wagen mehr dabei
tmin    PROC    NEAR
        mov     dx,0FFFFh
        mov     bx,0
        mov     di,0FFh
tm_1:   cmp     E_OK[bx],0
        je      tm_2
        mov     ax,bx
        mov     cl,(NSEC+1)*2
        mul     cl
        mov     si,ax
        mov     al,v_sec
        mov     ah,0
        inc     ax
        shl     ax,1
        add     si,ax
        mov     ax,E_CUM[si]
        cmp     ax,dx
        jae     tm_2
        mov     dx,ax
        mov     di,bx
tm_2:   inc     bx
        cmp     bx,4
        jb      tm_1
        cmp     di,0FFh
        je      tm_9
        mov     ax,dx
        mov     bx,di
        clc
        ret
tm_9:   stc
        ret
tmin    ENDP

; auf Taste warten -> AL (Grossbuchstabe, ESC = 1Bh)
; Start/Ziel-Flagge (FLAG.INC) in Phase v_fph zeichnen
drawflag PROC   NEAR
        push    ax
        push    bx
        push    cx
        push    si
        push    di
        mov     si,offset FLAG0
        cmp     v_fph,0
        je      df1
        mov     si,offset FLAG1
df1:    mov     bx,FLAGY
df2:    mov     ax,bx
        call    mul40
        add     ax,FLAGWX
        mov     di,ax
        call    fput4
        cmp     v_mono,0
        jne     df3
        push    di
        add     di,4000h                ; rot
        call    fput4
        pop     di
        push    di
        add     di,8000h                ; blau
        call    fput4
        pop     di
df3:    add     si,8
        inc     bx
        cmp     bx,FLAGY+FLAGH
        jb      df2
        pop     di
        pop     si
        pop     cx
        pop     bx
        pop     ax
        ret
drawflag ENDP

; 4 Worte ab DS:SI an Wortadresse DI schreiben
fput4   PROC    NEAR
        push    ax
        push    cx
        push    si
        call    wbegin
        mov     cx,4
fp1:    lodsw
        call    gpw
        loop    fp1
        pop     si
        pop     cx
        pop     ax
        ret
fput4   ENDP

; Taste abwarten (Grossbuchstabe in AL); die Flagge weht dabei
waitkey PROC    NEAR
wk1:    call    vsync
        inc     v_flc
        cmp     v_flc,12
        jb      wk1a
        mov     v_flc,0
        xor     v_fph,1
        call    drawflag
wk1a:
        mov     ah,06h
        mov     dl,0FFh
        int     21h
        jz      wk1
        or      al,al
        jnz     wk2
        mov     ah,06h                  ; erweiterte Taste: Folgebyte
        mov     dl,0FFh
        int     21h
        mov     al,0
wk2:    cmp     al,'a'
        jb      wk3
        cmp     al,'z'
        ja      wk3
        sub     al,20h
wk3:    ret
waitkey ENDP

;=======================================================================
;  Hauptprogramm
;=======================================================================
main    PROC    NEAR
        mov     sp,offset STKTOP        ; eigener Stapel im Programm
        cld
        mov     dx,offset T_BANNER
        mov     ah,09h
        int     21h
        ; Argumente
        mov     si,81h
ma1:    lodsb
        cmp     al,0Dh
        je      ma5
        cmp     al,'/'
        jne     ma1
        lodsb
        and     al,0DFh
        cmp     al,'Z'
        jne     ma2
        mov     v_noz,1
ma2:    cmp     al,'K'
        jne     ma3
        mov     v_nok,1
ma3:    cmp     al,'M'
        jne     ma3a
        mov     v_force,1
ma3a:   cmp     al,'C'
        jne     ma3b
        mov     v_force,2
ma3b:   cmp     al,'8' AND 0DFh
        jne     ma3c
        mov     v_lvl,8
ma3c:   cmp     al,'?' AND 0DFh
        jne     ma1
        mov     dx,offset T_HELP
        mov     ah,09h
        int     21h
        mov     ax,4C00h
        int     21h
ma5:    ; Speicher: eigener Block nur so gross wie noetig
        mov     bx,offset ENDPROG+15
        mov     cl,4
        shr     bx,cl
        mov     ah,4Ah
        int     21h
        call    det87
        call    detv20
        ; Z80-Bereich
        mov     v_zok,0
        cmp     v_noz,0
        jne     ma7
        call    zmove
        cmp     v_zok,0
        jne     ma7
        mov     dx,offset T_ZOVL
        mov     ah,09h
        int     21h
ma7:    ; 68008-Block (16 KB)
        mov     v_s68,0
        cmp     v_nok,0
        jne     ma8
        mov     bx,0400h
        mov     ah,48h
        int     21h
        jc      ma7a
        mov     v_s68,ax
        jmp     short ma8
ma7a:   mov     dx,offset T_MEM
        mov     ah,09h
        int     21h
ma8:    call    pitchk
        cmp     v_pitok,0
        jne     ma9
        mov     dx,offset T_PITNO
        mov     ah,09h
        int     21h
ma9:    ; Grafik
        call    gr_on
        cmp     v_force,0
        je      ma10
        mov     al,v_force
        dec     al
        xor     al,1
        mov     v_mono,al               ; /M -> 1, /C -> 0
        jmp     short ma11
ma10:   call    detect
ma11:   mov     v_npl,3
        cmp     v_mono,0
        je      ma12
        mov     v_npl,1
ma12:   call    scene
        call    race
        cmp     v_abort,0
        jne     ma15                    ; ESC: Ende
        mov     v_ty,TY_LAP+12          ; Nochmal?
        mov     v_col,WEISS
        mov     si,offset T_AGAIN
        mov     ax,TBX
        mov     bx,TCX+4*60-TBX
        call    cellt
ma13:   call    waitkey
        cmp     al,'J'
        je      ma12
        cmp     al,1Bh
        je      ma15
        cmp     al,'N'
        jne     ma13
ma15:
        call    gr_off
        cmp     v_s68run,0
        je      ma14
        mov     dx,offset T_68RUN
        mov     ah,09h
        int     21h
ma14:   mov     ax,4C00h
        int     21h
main    ENDP

        EVEN
ENDFILE LABEL   BYTE
; Puffer hinter dem Programm (stehen nicht in der Datei)
RESBUF  EQU     ENDFILE                 ; Mandelbrot je Wagen (4 x 1000)
PSIER   EQU     RESBUF+4*NCELL          ; Sierpinski-Bild je Wagen (4 x 1300)
PSIEV   EQU     PSIER+4*1300            ; Ulam-Spirale je Wagen (4 x 1300, Bits)
PDIV    EQU     PSIEV+4*1300            ; Faktoren je n (4 x 160)
PCIRC   EQU     PDIV+4*160              ; Wurzeln (4 x 150 Worte)
SROW    EQU     PCIRC+4*300             ; Sierpinski-Zeile (8088, 32 Bytes)
STMP    EQU     SROW+32
ZSAVE   EQU     STMP+32                 ; gesicherter Z80-Bereich + Sprungbrett
FLAGS   EQU     ZSAVE                   ; Sieb des 8088 (ZSAVE ist dann frei)
STKTOP  EQU     ZSAVE+Z80SIZE+6+1024    ; Stapel darunter
ENDPROG EQU     STKTOP

CSEG    ENDS
        END     START

; RACEZ80.ASM - Z80-Rechenkerne fuer DMVRACE (Hauptplatine der DMV, 4 MHz)
; Assembler: z80asm (GNU z80asm 1.8). tools/mkz80inc.py uebersetzt zweimal
; (ORG 0000h und 0100h); die Bytes, die sich unterscheiden, sind die oberen
; Haelften absoluter Adressen. So kann DMVRACE den Kern an jede freie
; 256-Byte-Grenze in Bank 0 legen und dort anpassen (Z80.INC).
;
; Ablauf: Das Boot-ROM (C.07.00, 0580h) legt bei F10Ch OUT (10h),A /
; OUT (D0h),A / NOP / JP F111h ab und schaltet mit dem OUT (D0h) auf den
; 8088. Der Z80 steht hinter diesem OUT und macht beim Zurueckschalten bei
; F110h weiter. DMVRACE legt dort sechs Bytes ab: F10Eh OUT (D0h),A,
; F110h NOP, F111h JP start. Jeder Test endet mit JP F10Eh: der Z80 schaltet
; zurueck und steht wieder an derselben Stelle.
;
; Tests (ZTEST): 1 Mandelbrot  2 Sierpinski  3 Sieb  4 Division  5 Kreis
; Rechenvorschrift wie tools/benchref.py; Ergebnis (32 Bit) in ZRESULT.
; Fuer die Bilder bleiben liegen: ZRES (Mandelbrot), ZSIER, ZDIV, ZCIRC und
; die Sieb-Flags (ZFLAGS).

SIER_ROWS:  equ 200
SIER_REPS:  equ 40
SIEVE_SIZE: equ 8190
SIEVE_ITER: equ 8
DIV_N0:     equ 50000
DIV_K:      equ 400
SQ_R:       equ 6000
SQ_R2L:     equ 0x5100          ; R^2 = 36.000.000 = 0225 5100h
SQ_R2H:     equ 0x0225

        org     0

start:  di
        ld      sp,stacktop
        ld      a,1
        ld      (ZSTAT),a
        ld      hl,0
        ld      (ZRESULT),hl
        ld      (ZRESULT+2),hl
        ld      a,(ZTEST)
        cp      1
        jp      z,mandel
        cp      2
        jp      z,sier
        cp      3
        jp      z,sieve
        cp      4
        jp      z,divt
        cp      5
        jp      z,circ
fin:    ld      a,2
        ld      (ZSTAT),a
        jp      0xf10e          ; zurueck zum 8088 (Sprungbrett bei F10Eh)

; ---- Parameterblock (vom 8088 gelesen und beschrieben) ----
ZTEST:  defb    0
ZSTAT:  defb    0
ZRESULT: defw   0,0
ZX0:    defw    0
ZY0:    defw    0
ZDX:    defw    0
ZDY:    defw    0
ZW:     defb    0
ZH:     defb    0
ZMAX:   defb    0
        defb    0

; ---- Arbeitsvariablen ----
cr:     defw    0
ci:     defw    0
zr:     defw    0
zi:     defw    0
zr2:    defw    0
zi2:    defw    0
cols:   defb    0
rows:   defb    0
n:      defb    0
azr:    defw    0
azi:    defw    0
cnt:    defw    0
num:    defw    0
mm:     defw    0
remh:   defb    0
xx:     defw    0
sum:    defw    0,0
zsp:    defw    0
before: defw    0
c40:    defb    0

;------------------------------------------------------------------------
; 1: Mandelbrot (16 Bit Festkomma 3.13, wie mandel13 im 8088-Programm)
;------------------------------------------------------------------------
mandel: ld      ix,ZRES
        ld      hl,(ZY0)
        ld      (ci),hl
        ld      a,(ZH)
        ld      (rows),a
rowlp:  ld      hl,(ZX0)
        ld      (cr),hl
        ld      a,(ZW)
        ld      (cols),a
collp:  call    point
        ld      (ix+0),a
        inc     ix
        ld      hl,(cr)
        ld      de,(ZDX)
        add     hl,de
        ld      (cr),hl
        ld      hl,cols
        dec     (hl)
        jr      nz,collp
        ld      hl,(ci)
        ld      de,(ZDY)
        or      a
        sbc     hl,de
        ld      (ci),hl
        ld      hl,rows
        dec     (hl)
        jr      nz,rowlp
        jp      fin

; Iterationen fuer (cr, ci) -> A
point:  ld      hl,0
        ld      (zr),hl
        ld      (zi),hl
        xor     a
        ld      (n),a
itlp:   ld      hl,(zr)         ; |zr| < 2 ?
        call    absh
        ld      a,h
        cp      0x40
        jp      nc,done
        ld      (azr),hl
        ld      hl,(zi)         ; |zi| < 2 ?
        call    absh
        ld      a,h
        cp      0x40
        jp      nc,done
        ld      (azi),hl
        ld      bc,(azr)        ; zr^2 >> 13
        ld      d,b
        ld      e,c
        call    mul14
        call    shr13
        ld      (zr2),hl
        ld      bc,(azi)        ; zi^2 >> 13
        ld      d,b
        ld      e,c
        call    mul14
        call    shr13
        ld      (zi2),hl
        ld      de,(zr2)        ; Summe > 4 (32768) ?
        add     hl,de           ; beide < 8000h: kein Uebertrag
        ld      de,0x8001
        or      a
        sbc     hl,de
        jp      nc,done         ; >= 8001h: entkommen
        ld      bc,(azr)        ; 2*zr*zi >> 13 = |zr|*|zi| >> 12, abgerundet
        ld      de,(azi)
        call    mul14
        ld      a,(zr+1)
        ld      b,a
        ld      a,(zi+1)
        xor     b
        jp      p,pos
        ld      bc,4095         ; negativ: -((p + 4095) >> 12)
        add     hl,bc
        jr      nc,neg1
        inc     de
neg1:   call    shr12
        ex      de,hl
        ld      hl,0
        or      a
        sbc     hl,de
        jr      prod
pos:    call    shr12
prod:   ld      de,(ci)         ; zi = 2 zr zi + ci
        add     hl,de
        ld      (zi),hl
        ld      hl,(zr2)        ; zr = zr^2 - zi^2 + cr
        ld      de,(zi2)
        or      a
        sbc     hl,de
        ld      de,(cr)
        add     hl,de
        ld      (zr),hl
        ld      hl,n
        inc     (hl)
        ld      a,(ZMAX)
        cp      (hl)
        jp      nz,itlp
done:   ld      a,(n)
        ret

; HL = |HL|
absh:   bit     7,h
        ret     z
        ex      de,hl
        ld      hl,0
        or      a
        sbc     hl,de
        ret

; DEHL = BC * DE, beide < 4000h (14 Bit) - Schieben und Addieren
mul14:  ld      hl,0
        sla     e               ; die zwei oberen (leeren) Bits vorab weg
        rl      d
        sla     e
        rl      d
        ld      a,14
mul1:   add     hl,hl
        rl      e
        rl      d
        jr      nc,mul2
        add     hl,bc
        jr      nc,mul2
        inc     de
mul2:   dec     a
        jr      nz,mul1
        ret

; HL = DEHL >> 13  (DE*8 + H>>5), Ergebnis < 8000h
shr13:  ld      a,h
        rlca
        rlca
        rlca
        and     7
        ex      de,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        or      l
        ld      l,a
        ret

; HL = DEHL >> 12  (DE*16 + H>>4)
shr12:  ld      a,h
        rlca
        rlca
        rlca
        rlca
        and     15
        ex      de,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        or      l
        ld      l,a
        ret


;------------------------------------------------------------------------
; 2: Sierpinski - Pascal-Dreieck modulo 2 als 256-Bit-Zeilen:
;    Zeile = Zeile XOR (Zeile << 1), 200 Zeilen, 40 Durchgaenge
;    Ergebnis: Summe der 32 Bytes der letzten Zeile
;------------------------------------------------------------------------
sier:   ld      a,SIER_REPS
        ld      (n),a
srep:   ld      hl,sbuf
        ld      b,32
        xor     a
sclr:   ld      (hl),a
        inc     hl
        djnz    sclr
        ld      a,1
        ld      (sbuf),a
        ld      hl,ZSIER        ; Bild: Zeilen 0..99 des letzten Durchgangs
        ld      (zsp),hl
        ld      a,(n)
        cp      1
        call    z,scopy
        ld      a,SIER_ROWS-1
        ld      (rows),a
srow:   ld      hl,sbuf         ; 1. um ein Bit verschobene Kopie (Uebertrag durch alle Bytes)
        ld      de,tbuf
        ld      b,32
        or      a
s1:     ld      a,(hl)
        rla
        ld      (de),a
        inc     hl
        inc     de
        djnz    s1
        ld      hl,sbuf         ; 2. Zeile XOR Kopie
        ld      de,tbuf
        ld      b,32
s2:     ld      a,(de)
        xor     (hl)
        ld      (hl),a
        inc     hl
        inc     de
        djnz    s2
        ld      a,(n)
        cp      1
        call    z,scopy
        ld      hl,rows
        dec     (hl)
        jr      nz,srow
        ld      hl,n
        dec     (hl)
        jr      nz,srep
        ld      hl,sbuf         ; Pruefsumme
        ld      de,0
        ld      b,32
ssum:   ld      a,(hl)
        add     a,e
        ld      e,a
        jr      nc,ssum1
        inc     d
ssum1:  inc     hl
        djnz    ssum
        ld      (ZRESULT),de
        jp      fin

; eine Zeile (13 Bytes = 104 Bit) ins Bild, solange Platz ist
scopy:  ld      hl,(zsp)
        ld      de,ZSIER+1300
        or      a
        sbc     hl,de
        ret     nc
        ld      de,(zsp)
        ld      hl,sbuf
        ld      bc,13
        ldir
        ld      (zsp),de
        ret

;------------------------------------------------------------------------
; 3: Sieb des Eratosthenes wie im BYTE-Benchmark (1981): 8190 Flags,
;    8 Durchgaenge, Ergebnis: Zahl der Primzahlen (1899)
;------------------------------------------------------------------------
sieve:  ld      a,SIEVE_ITER
        ld      (n),a
sit:    ld      hl,flags
        ld      de,flags+1
        ld      bc,SIEVE_SIZE
        ld      (hl),1
        ldir
        ld      hl,0
        ld      (cnt),hl
        ld      hl,flags
sl:     ld      a,(hl)
        or      a
        jr      z,snext
        push    hl              ; Primzahl = 2*i+3
        ld      bc,flags
        or      a
        sbc     hl,bc
        add     hl,hl
        inc     hl
        inc     hl
        inc     hl
        ex      de,hl
        pop     hl
        push    hl
        ld      bc,flags+SIEVE_SIZE+1
        add     hl,de           ; k = i + Primzahl
        jr      c,sdone         ; (ueber 64 KB hinaus: fertig)
skl:    push    hl
        or      a
        sbc     hl,bc
        pop     hl
        jr      nc,sdone
        ld      (hl),0
        add     hl,de
        jr      nc,skl
sdone:  pop     hl
        ld      bc,(cnt)
        inc     bc
        ld      (cnt),bc
snext:  inc     hl
        push    hl
        ld      bc,flags+SIEVE_SIZE+1
        or      a
        sbc     hl,bc
        pop     hl
        jr      c,sl
        ld      hl,n
        dec     (hl)
        jr      nz,sit
        ld      hl,(cnt)
        ld      (ZRESULT),hl
        jp      fin

;------------------------------------------------------------------------
; 4: Primfaktoren durch Probedivision, n = 50000..50399
;    Ergebnis: Summe der Zahl der Primfaktoren (mit Vielfachheit)
;------------------------------------------------------------------------
divt:   ld      hl,DIV_N0
        ld      (num),hl
        ld      hl,DIV_K
        ld      (cnt),hl
        ld      hl,0
        ld      (sum),hl
        ld      hl,ZDIV
        ld      (zsp),hl
dn:     ld      hl,(sum)
        ld      (before),hl
        ld      hl,(num)
        ld      (mm),hl
        ld      de,2
dd:     ld      hl,(mm)
        ld      a,h
        ld      c,l
        call    div16           ; AC = m / d, HL = Rest
        push    hl
        ld      h,a
        ld      l,c
        or      a
        sbc     hl,de
        jr      c,dbrk          ; Quotient < d: fertig
        add     hl,de
        pop     bc              ; Rest
        ld      a,b
        or      c
        jr      nz,dnext
        ld      (mm),hl         ; teilbar: m = q, Faktor zaehlen
        ld      hl,(sum)
        inc     hl
        ld      (sum),hl
        jr      dd
dnext:  inc     de
        jr      dd
dbrk:   pop     hl
        ld      hl,(sum)        ; Rest m ist prim
        inc     hl
        ld      (sum),hl
        ld      de,(before)     ; Bild: Zahl der Faktoren von n
        or      a
        sbc     hl,de
        ld      a,l
        ld      hl,(zsp)
        ld      (hl),a
        inc     hl
        ld      (zsp),hl
        ld      hl,(num)
        inc     hl
        ld      (num),hl
        ld      hl,(cnt)
        dec     hl
        ld      (cnt),hl
        ld      a,h
        or      l
        jr      nz,dn
        ld      hl,(sum)
        ld      (ZRESULT),hl
        jp      fin

; AC = AC / DE, HL = Rest (16 Bit, ohne Vorzeichen, Schieben und Abziehen)
div16:  ld      hl,0
        ld      b,16
dv1:    sla     c
        inc     c               ; Quotientenbit vorab 1
        rla
        adc     hl,hl
        sbc     hl,de
        jr      nc,dv2
        add     hl,de
        dec     c               ; passt nicht: Bit wieder 0
dv2:    djnz    dv1
        ret

;------------------------------------------------------------------------
; 5: Gitterpunkte im Viertelkreis: Summe isqrt(R^2 - x^2), x = 0..R-1
;    (R = 6000), Wurzel Bit fuer Bit, Ergebnis 32 Bit
;------------------------------------------------------------------------
circ:   ld      hl,ZCIRC
        ld      (zsp),hl
        xor     a
        ld      (c40),a
        ld      hl,0
        ld      (xx),hl
        ld      (sum),hl
        ld      (sum+2),hl
cx1:    ld      bc,(xx)         ; x^2
        ld      d,b
        ld      e,c
        call    mul14           ; DEHL
        ld      bc,SQ_R2L       ; N = R^2 - x^2
        or      a
        push    hl
        ld      h,b
        ld      l,c
        pop     bc
        sbc     hl,bc           ; low
        push    hl
        ld      hl,SQ_R2H
        sbc     hl,de           ; high
        ex      de,hl
        pop     hl              ; N = DE:HL
        push    de
        push    hl
        exx
        pop     hl              ; N in DE':HL'
        pop     de
        exx
        call    isqrt           ; IX = Wurzel
        push    ix
        pop     de
        ld      a,(c40)         ; Bild: jede 40. Wurzel
        or      a
        jr      nz,cx1b
        ld      hl,(zsp)
        ld      (hl),e
        inc     hl
        ld      (hl),d
        inc     hl
        ld      (zsp),hl
        ld      a,40
cx1b:   dec     a
        ld      (c40),a
        ld      hl,(sum)
        add     hl,de
        ld      (sum),hl
        jr      nc,cx2
        ld      hl,(sum+2)
        inc     hl
        ld      (sum+2),hl
cx2:    ld      hl,(xx)
        inc     hl
        ld      (xx),hl
        ld      de,SQ_R
        or      a
        sbc     hl,de
        jr      c,cx1
        ld      hl,(sum)
        ld      (ZRESULT),hl
        ld      hl,(sum+2)
        ld      (ZRESULT+2),hl
        jp      fin

; IX = isqrt(DE':HL'), Rest in remh:HL (24 Bit), Wurzel IX, Probe C:DE
isqrt:  ld      ix,0
        ld      hl,0
        xor     a
        ld      (remh),a
        ld      b,16
iq1:    exx                     ; 2 Bits von N in den Rest schieben
        add     hl,hl
        rl      e
        rl      d
        exx
        adc     hl,hl
        ld      a,(remh)
        rla
        ld      (remh),a
        exx
        add     hl,hl
        rl      e
        rl      d
        exx
        adc     hl,hl
        ld      a,(remh)
        rla
        ld      (remh),a
        add     ix,ix           ; Wurzel * 2
        push    ix              ; Probe = 2*Wurzel + 1 (17 Bit: C:DE)
        pop     de
        ld      c,0
        sla     e
        rl      d
        rl      c
        set     0,e
        or      a               ; Rest - Probe
        sbc     hl,de
        ld      a,(remh)
        sbc     a,c
        jr      c,iq2
        ld      (remh),a        ; passt: abziehen, Wurzelbit setzen
        inc     ix
        jr      iq3
iq2:    add     hl,de           ; passt nicht: zuruecknehmen
iq3:    djnz    iq1
        ret

; ---- Speicher ----
sbuf:   defs    32
tbuf:   defs    32
        defs    64
stacktop:
ZRES:   defs    1000            ; Mandelbrot: Iterationen je Zelle
ZSIER:  defs    1300            ; Sierpinski: 100 Zeilen zu 13 Bytes
ZDIV:   defs    400             ; Division: Faktoren je n
ZCIRC:  defs    300             ; Kreis: 150 Wurzeln (Worte)
ZFLAGS:
flags:  defs    SIEVE_SIZE+1
zend:

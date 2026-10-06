;=====================================================================
; MOUSETST.COM  -  graphical test for the K806 mouse adapter of the
;                  NCR Decision Mate V
;
; Draws a coordinate cross with the origin in the middle of the screen
; and traces the absolute position of the K806 (command 00). Moving
; right and up gives positive values, as in the NCR diagnostic program.
;
;   MOUSETST            search the K806 on all IFSEL addresses
;                       (30h = IFSEL 2A first)
;   MOUSETST nn         use base address nn (hex), e.g. MOUSETST 38
;
; Keys:  ESC        quit
;        0          set the origin here (the trace stays)
;        C          clear the trace and set the origin
;        1 2 4 8    scale: counts per pixel
;        B          limits: screen edge (like a mouse driver) or free
;                   (-32768..32767)
; Ticks every 100 counts, long ticks every 500 counts. The K806 address
; and IFSEL are shown in the help line and on exit.
;
; Requires MS-DOS 2.0 or later on the DMV with 8088/V20 card
; (K230/K231/K235) and a K806 mouse adapter. Works with the colour and
; the monochrome graphics board (only the green plane is used).
;
; Hardware:
;   uPD7220 GDC: A0h status/parameter, A1h command, status bit 1 = FIFO
;   full. Graphics: 40 words per line, green plane from word 0, bit 0 of
;   a word is the leftmost pixel.
;   K806: base+0 data, base+1 command/status (8741 UPI), status bit 1 =
;   IBF (input buffer full), bit 0 = OBF (output buffer full).
;   Firmware commands used: 00 read position (X lo/hi, Y lo/hi, buttons),
;   02 interrupts off, 04 set position, 05 set XMAX/YMAX, 0A set
;   XMIN/YMIN. The counters stop at the limits, but only when they reach
;   them exactly. Buttons: bit 0 = S1 left, bit 1 = S2 right,
;   bit 2 = S3 middle.
;
; Pixels are written with WDAT in "set" mode (23h). MAME's uPD7220
; ignores the mask register for WDAT in "replace" mode (20h) and
; clears the other pixels of the word.
;
; Build with MASM 5.10:  MASM MOUSETST; LINK MOUSETST;
;                        EXE2BIN MOUSETST.EXE MOUSETST.COM
;=====================================================================

GDC_STAT EQU    0A0h
GDC_PAR EQU     0A0h
GDC_CMD EQU     0A1h

SCR_W   EQU     640
WPL     EQU     40              ; words per screen line
ORG_X   EQU     320             ; origin on the screen
ORG_Y   EQU     200
TOP_Y   EQU     14              ; first line of the drawing area
BOT_Y   EQU     385             ; last line of the drawing area
STAT_Y  EQU     3               ; status line
HELP_Y  EQU     390             ; help line
HISTMAX EQU     7000            ; trace memory (positions)

CSEG    SEGMENT PARA PUBLIC 'CODE'
        ASSUME  CS:CSEG,DS:CSEG,ES:CSEG,SS:CSEG
        ORG     100h

START:  jmp     main

;=====================================================================
;  Data
;=====================================================================

kport   dw      30h                     ; K806 base address
pgiven  db      0                       ; 1 = address on the command line
limmin  dw      8000h,8000h             ; XMIN, YMIN
limmax  dw      7FFFh,7FFFh             ; XMAX, YMAX
newpos  dw      0,0
zero4   db      0,0,0,0
bounded db      1                       ; 1 = limits at the screen edge
shift   db      1                       ; scale 2^shift counts per pixel
force   db      1                       ; redraw status and trace point
havelast db     0
wdatcmd db      20h
pramgfx db      00h,00h,0F0h,7Fh        ; SAD 0, LEN 1023, graphics (IM=1)
pramtxt db      00h,00h,0F0h,3Fh        ; text, as set by the Z80 ROM

mx      dw      0                       ; position from command 00
my      dw      0
mb      db      0,0                     ; buttons
oldmx   dw      0
oldmy   dw      0
oldmb   db      0,0
lastsx  dw      0
lastsy  dw      0
tick    dw      0
lx1     dw      0
ly1     dw      0
ldx     dw      0
ldy     dw      0
lsx     dw      0
lsy     dw      0
histhead dw     0
histcnt dw      0

; IFSEL addresses: port, unused, name (default 2A first)
ifseltab db     30h,0,'2A', 38h,0,'2B', 60h,0,'0A', 68h,0,'0B'
        db      70h,0,'1A', 78h,0,'1B', 0B0h,0,'3A', 0B8h,0,'3B'
        db      0C0h,0,'4A', 0C8h,0,'4B', 0,0

txtbild db      'Bild '
txtfrei db      'frei '
sttext  db      'X='
stx     db      '+00000   Y='
sty     db      '+00000   Tasten '
stb     db      '- - -   Zaehler/Punkt 1:'
stscale db      '2   Grenze '
stbound db      'Bild ',0
helptext db     'ESC Ende  0 Nullpunkt  C Neu  1/2/4/8 Massstab  B Grenze  K806 '
hxport1 db      '30h IFSEL '
ifs1    db      '2A',0

msgnone db      'K806 nicht gefunden (Adressen 30 38 60 68 70 78 B0 B8 C0 C8',13,10
        db      'geprueft). Adresse angeben, z.B. MOUSETST 30',13,10,'$'
msglost db      'Verbindung zum K806 verloren',13,10,'$'
msgfound db     'K806 gefunden an Adresse '
hxport2 db      '30h (IFSEL '
ifs2    db      '2A)',13,10,'$'

        INCLUDE FONT.INC

;=====================================================================
;  Main program
;=====================================================================

main:   cld
        call    parsearg
        call    findmouse               ; search and identify the K806
        jc      nomj
        call    setorig
        jc      nomj
        call    setlimits
        jnc     init
nomj:   jmp     nomouse
init:   call    gfxon
        call    redraw

mainlp: mov     ah,06h                  ; read a key without waiting
        mov     dl,0FFh
        int     21h
        jz      nokey
        cmp     al,1Bh
        jne     key0
        jmp     quit
key0:   cmp     al,'0'
        jne     keyc
        call    setorig
        jc      lostj
        jmp     short forced
keyc:   and     al,0DFh                 ; upper case (digits are checked
        cmp     al,'C'                  ; after restoring bit 5)
        jne     keyb
        call    setorig
        jc      lostj
        call    redraw
        jmp     short forced
keyb:   cmp     al,'B'                  ; limits on/off
        jne     keyn
        xor     byte ptr [bounded],1
        call    setlimits
        jc      lostj
        jmp     short forced
keyn:   or      al,20h                  ; digit again
        mov     cl,0
        cmp     al,'1'
        je      keysc
        inc     cl
        cmp     al,'2'
        je      keysc
        inc     cl
        cmp     al,'4'
        je      keysc
        inc     cl
        cmp     al,'8'
        jne     nokey
keysc:  mov     [shift],cl
        call    setlimits
        jc      lostj
        call    redraw
forced: mov     byte ptr [force],1
nokey:  call    readpos
        jc      lostj
        cmp     byte ptr [force],0
        jne     moved
        mov     ax,[mx]
        cmp     ax,[oldmx]
        jne     moved
        mov     ax,[my]
        cmp     ax,[oldmy]
        jne     moved
        mov     al,[mb]
        cmp     al,[oldmb]
        jne     moved
        jmp     mainlp
lostj:  jmp     lost
moved:  mov     byte ptr [force],0
        mov     ax,[mx]
        mov     [oldmx],ax
        mov     ax,[my]
        mov     [oldmy],ax
        mov     al,[mb]
        mov     [oldmb],al
        mov     ax,[mx]
        mov     bx,[my]
        call    histadd
        call    drawto
        call    status
        jmp     mainlp

lost:   call    gfxoff
        mov     dx,offset msglost
        jmp     short errexit
nomouse:
        mov     dx,offset msgnone
errexit:
        mov     ah,09h
        int     21h
        mov     ax,4C01h
        int     21h

quit:   call    gfxoff
        mov     dl,1Ah                  ; clear screen, cursor home
        mov     ah,02h
        int     21h
        mov     dx,offset msgfound
        mov     ah,09h
        int     21h
        mov     ax,4C00h
        int     21h

;=====================================================================
;  Command line and K806 search
;=====================================================================

; optional hex base address on the command line
parsearg PROC   NEAR
        mov     si,81h
        xor     bx,bx
        xor     cx,cx                   ; number of digits
pa1:    lodsb
        cmp     al,' '
        je      pa1
        cmp     al,'0'
        jb      pa3
        cmp     al,'9'
        jbe     pa2
        and     al,0DFh
        cmp     al,'A'
        jb      pa3
        cmp     al,'F'
        ja      pa3
        sub     al,7
pa2:    sub     al,'0'
        shl     bx,1
        shl     bx,1
        shl     bx,1
        shl     bx,1
        or      bl,al
        inc     cx
        jmp     pa1
pa3:    jcxz    pa4
        and     bx,0FFh
        mov     [kport],bx
        mov     byte ptr [pgiven],1
pa4:    ret
parsearg ENDP

; Search the K806 at the given address or at all IFSEL addresses.
; A K806 accepts command 02 and answers command 00 with exactly five
; bytes. Puts address and IFSEL name into the texts. CF=1: not found.
findmouse PROC  NEAR
        cmp     byte ptr [pgiven],0
        je      fm1
        call    probe
        jc      fm9
        jmp     short fm4
fm1:    mov     si,offset ifseltab
fm2:    lodsw                           ; AL = port, AH unused
        or      al,al
        jz      fm3
        xor     ah,ah
        mov     [kport],ax
        push    si
        call    probe
        pop     si
        jnc     fm4
        add     si,2
        jmp     fm2
fm3:    stc
fm9:    ret
fm4:    mov     ax,[kport]
        mov     di,offset hxport1
        call    hex2
        mov     di,offset hxport2
        call    hex2
        mov     si,offset ifseltab
fm5:    lodsw
        or      al,al
        jz      fm7
        cmp     al,byte ptr [kport]
        je      fm6
        add     si,2
        jmp     fm5
fm6:    mov     ax,[si]
        jmp     short fm8
fm7:    mov     ax,'??'
fm8:    mov     word ptr [ifs1],ax
        mov     word ptr [ifs2],ax
        clc
        ret
findmouse ENDP

probe   PROC    NEAR
        mov     al,02h                  ; interrupts off
        call    kcmd
        jc      pr1
        call    readpos
pr1:    ret
probe   ENDP

; AL as two hex digits to [DI]
hex2    PROC    NEAR
        push    ax
        mov     ah,al
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
        call    hexdig
        mov     al,ah
        and     al,0Fh
        call    hexdig
        pop     ax
        ret
hex2    ENDP

hexdig  PROC    NEAR
        add     al,'0'
        cmp     al,'9'
        jbe     hd1
        add     al,7
hd1:    mov     [di],al
        inc     di
        ret
hexdig  ENDP

;=====================================================================
;  K806 access, CF=1 on timeout
;=====================================================================

kwait   PROC    NEAR                    ; wait until IBF = 0
        push    cx
        push    dx
        mov     dx,[kport]
        inc     dx
        xor     cx,cx
kw1:    in      al,dx
        test    al,02h
        jz      kw2
        loop    kw1
        stc
        jmp     short kw3
kw2:    clc
kw3:    pop     dx
        pop     cx
        ret
kwait   ENDP

kcmd    PROC    NEAR                    ; AL = command
        push    ax
        call    kwait
        pop     ax
        jc      kc1
        push    dx
        mov     dx,[kport]
        inc     dx
        out     dx,al
        pop     dx
        clc
kc1:    ret
kcmd    ENDP

kdata   PROC    NEAR                    ; AL = data byte
        push    ax
        call    kwait
        pop     ax
        jc      kd1
        push    dx
        mov     dx,[kport]
        out     dx,al
        pop     dx
        clc
kd1:    ret
kdata   ENDP

ksend4  PROC    NEAR                    ; four data bytes from [SI]
        mov     cx,4
ks1:    lodsb
        push    cx
        call    kdata
        pop     cx
        jc      ks2
        loop    ks1
        clc
ks2:    ret
ksend4  ENDP

kget    PROC    NEAR                    ; AL = data byte from the K806
        push    cx
        push    dx
        mov     dx,[kport]
        inc     dx
        xor     cx,cx
kg1:    in      al,dx
        test    al,01h
        jnz     kg2
        loop    kg1
        stc
        jmp     short kg3
kg2:    dec     dx
        in      al,dx
        clc
kg3:    pop     dx
        pop     cx
        ret
kget    ENDP

readpos PROC    NEAR                    ; command 00: X, Y, buttons
        mov     al,00h
        call    kcmd
        jc      rp2
        mov     di,offset mx
        mov     cx,5
rp1:    call    kget
        jc      rp2
        mov     [di],al
        inc     di
        loop    rp1
        clc
rp2:    ret
readpos ENDP

; Limits: screen edge for the current scale (like a mouse driver) or
; free (-32768..32767). A position outside the new range is moved to
; the edge first, because the firmware only stops the counters when they
; reach a limit exactly.
setlimits PROC  NEAR
        cmp     byte ptr [bounded],0
        je      sl1
        mov     cl,[shift]
        mov     ax,-ORG_X
        sal     ax,cl
        mov     [limmin],ax
        mov     ax,SCR_W-1-ORG_X
        shl     ax,cl
        mov     [limmax],ax
        mov     ax,-(BOT_Y-ORG_Y)
        sal     ax,cl
        mov     [limmin+2],ax
        mov     ax,ORG_Y-TOP_Y
        shl     ax,cl
        mov     [limmax+2],ax
        mov     si,offset txtbild
        jmp     short sl2
sl1:    mov     word ptr [limmin],8000h
        mov     word ptr [limmin+2],8000h
        mov     word ptr [limmax],7FFFh
        mov     word ptr [limmax+2],7FFFh
        mov     si,offset txtfrei
sl2:    mov     di,offset stbound
        mov     cx,5
        rep     movsb
        mov     al,0Ah                  ; XMIN/YMIN
        call    kcmd
        jc      sl9
        mov     si,offset limmin
        call    ksend4
        jc      sl9
        mov     al,05h                  ; XMAX/YMAX
        call    kcmd
        jc      sl9
        mov     si,offset limmax
        call    ksend4
        jc      sl9
        call    readpos
        jc      sl9
        mov     ax,[mx]
        mov     bx,[limmin]
        mov     dx,[limmax]
        call    clampr
        mov     [newpos],ax
        mov     ax,[my]
        mov     bx,[limmin+2]
        mov     dx,[limmax+2]
        call    clampr
        mov     [newpos+2],ax
        mov     ax,[newpos]
        cmp     ax,[mx]
        jne     sl3
        mov     ax,[newpos+2]
        cmp     ax,[my]
        je      sl4
sl3:    mov     al,04h                  ; set position
        call    kcmd
        jc      sl9
        mov     si,offset newpos
        call    ksend4
        jc      sl9
sl4:    clc
sl9:    ret
setlimits ENDP

clampr  PROC    NEAR                    ; AX to BX..DX (signed)
        cmp     ax,bx
        jge     cp1
        mov     ax,bx
cp1:    cmp     ax,dx
        jle     cp2
        mov     ax,dx
cp2:    ret
clampr  ENDP

setorig PROC    NEAR                    ; command 04: position 0/0
        mov     al,04h
        call    kcmd
        jc      so1
        mov     si,offset zero4
        call    ksend4
        jc      so1
        mov     byte ptr [havelast],0
        mov     word ptr [histhead],0   ; empty the trace memory
        mov     word ptr [histcnt],0
        clc
so1:    ret
setorig ENDP

;=====================================================================
;  Trace memory (ring buffer of positions) and drawing
;=====================================================================

histadd PROC    NEAR                    ; AX = X, BX = Y
        push    ax
        push    di
        mov     di,[histhead]
        shl     di,1
        shl     di,1
        mov     word ptr [hist+di],ax
        mov     word ptr [hist+di+2],bx
        mov     di,[histhead]
        inc     di
        cmp     di,HISTMAX
        jb      ha1
        xor     di,di
ha1:    mov     [histhead],di
        cmp     word ptr [histcnt],HISTMAX
        jae     ha2
        inc     word ptr [histcnt]
ha2:    pop     di
        pop     ax
        ret
histadd ENDP

histdraw PROC   NEAR                    ; draw the stored trace again
        mov     byte ptr [havelast],0
        mov     cx,[histcnt]
        jcxz    hd9
        mov     si,[histhead]
        sub     si,cx
        jge     hdl
        add     si,HISTMAX
hdl:    push    cx
        push    si
        shl     si,1
        shl     si,1
        mov     ax,word ptr [hist+si]
        mov     bx,word ptr [hist+si+2]
        call    drawto
        pop     si
        pop     cx
        inc     si
        cmp     si,HISTMAX
        jb      hdn
        xor     si,si
hdn:    loop    hdl
hd9:    ret
histdraw ENDP

; line from the last position to the counter position AX/BX
drawto  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        call    toscreen                ; CX,DX = pixel
        cmp     byte ptr [havelast],0
        je      dt1
        mov     si,[lastsx]
        mov     di,[lastsy]
        call    line
        jmp     short dt2
dt1:    call    plotclip
        mov     byte ptr [havelast],1
dt2:    mov     [lastsx],cx
        mov     [lastsy],dx
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
drawto  ENDP

; counts AX/BX to pixel CX/DX (Y grows upwards)
toscreen PROC   NEAR
        mov     cl,[shift]
        sar     ax,cl
        call    clamp
        add     ax,ORG_X
        xchg    ax,bx
        sar     ax,cl
        call    clamp
        neg     ax
        add     ax,ORG_Y
        call    clamp
        mov     dx,ax
        mov     ax,bx
        call    clamp
        mov     cx,ax
        ret
toscreen ENDP

clamp   PROC    NEAR                    ; AX to -4000..4000 (line maths)
        cmp     ax,-4000
        jge     cl1
        mov     ax,-4000
cl1:    cmp     ax,4000
        jle     cl2
        mov     ax,4000
cl2:    ret
clamp   ENDP

;=====================================================================
;  GDC
;=====================================================================

gdcwait PROC    NEAR
        push    ax
gw1:    in      al,GDC_STAT
        test    al,02h                  ; FIFO full?
        jnz     gw1
        pop     ax
        ret
gdcwait ENDP

gcmd    PROC    NEAR                    ; AL = command
        call    gdcwait
        out     GDC_CMD,al
        ret
gcmd    ENDP

gpar    PROC    NEAR                    ; AL = parameter
        call    gdcwait
        out     GDC_PAR,al
        ret
gpar    ENDP

; write one word: AX = word address, BX = mask, DX = data,
; WDAT mode from [wdatcmd]
gword   PROC    NEAR
        push    ax
        push    ax
        mov     al,49h                  ; CURS
        call    gcmd
        pop     ax
        call    gpar                    ; EAD low
        mov     al,ah
        call    gpar                    ; EAD high
        xor     al,al
        call    gpar                    ; EAD bits 16-17, dAD 0
        mov     al,4Ah                  ; MASK
        call    gcmd
        mov     al,bl
        call    gpar
        mov     al,bh
        call    gpar
        mov     al,[wdatcmd]            ; WDAT word (20h replace, 23h set)
        call    gcmd
        mov     al,dl
        call    gpar
        mov     al,dh
        call    gpar
        pop     ax
        ret
gword   ENDP

gfxon   PROC    NEAR                    ; switch first, then clear
        mov     si,offset pramgfx
        call    gfxset
        xor     dx,dx                   ; clear all three planes
        jmp     fillall
gfxon   ENDP

gfxoff  PROC    NEAR                    ; text mode: blanks, white on black
        mov     dx,0FC20h
        call    fillall
        mov     si,offset pramtxt
        jmp     gfxset
gfxoff  ENDP

gfxset  PROC    NEAR                    ; PRAM from [SI]
        mov     al,0Ch                  ; STOP
        call    gcmd
        mov     al,70h                  ; PRAM
        call    gcmd
        mov     cx,4
gs1:    lodsb
        call    gpar
        loop    gs1
        mov     al,47h                  ; PITCH 80
        call    gcmd
        mov     al,50h
        call    gpar
        mov     al,6Bh                  ; START
        call    gcmd
        ret
gfxset  ENDP

; fill one plane (16384 words from word address AX) with DX:
; a single WDAT command with FIGS direction 2 (+1 word), DC = 16383
fillplane PROC  NEAR
        push    ax
        push    ax
        mov     al,49h                  ; CURS
        call    gcmd
        pop     ax
        call    gpar
        mov     al,ah
        call    gpar
        xor     al,al
        call    gpar
        mov     al,4Ah                  ; MASK FFFF
        call    gcmd
        mov     al,0FFh
        call    gpar
        call    gpar
        mov     al,4Ch                  ; FIGS
        call    gcmd
        mov     al,02h                  ; direction 2, no figure
        call    gpar
        mov     al,0FFh                 ; DC = 3FFFh
        call    gpar
        mov     al,3Fh
        call    gpar
        mov     al,20h                  ; WDAT word, replace
        call    gcmd
        mov     al,dl
        call    gpar
        mov     al,dh
        call    gpar
        pop     ax
        ret
fillplane ENDP

fillall PROC    NEAR                    ; all three planes with DX
        xor     ax,ax
        call    fillplane
        mov     ax,4000h
        call    fillplane
        mov     ax,8000h
        jmp     fillplane
fillall ENDP

; set pixel CX/DX (no clipping)
plot    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     ax,WPL
        push    dx
        mul     dx                      ; AX = y*40
        pop     dx
        mov     bx,cx
        shr     bx,1
        shr     bx,1
        shr     bx,1
        shr     bx,1
        add     ax,bx                   ; word address
        and     cl,0Fh
        mov     bx,1
        shl     bx,cl                   ; mask
        mov     dx,0FFFFh
        mov     byte ptr [wdatcmd],23h  ; "set" mode: only the masked bit
        call    gword
        mov     byte ptr [wdatcmd],20h
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
plot    ENDP

; set pixel CX/DX if it is inside the drawing area
plotclip PROC   NEAR
        cmp     cx,SCR_W
        jae     pc1                     ; also catches negative values
        cmp     dx,TOP_Y
        jl      pc1
        cmp     dx,BOT_Y
        jg      pc1
        call    plot
pc1:    ret
plotclip ENDP

; line from SI/DI to CX/DX (Bresenham)
line    PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        mov     [lx1],cx
        mov     [ly1],dx
        mov     ax,cx
        sub     ax,si                   ; dx = x1-x0
        mov     bx,1
        jge     ln1
        neg     ax
        neg     bx
ln1:    mov     [ldx],ax
        mov     [lsx],bx
        mov     ax,dx
        sub     ax,di
        mov     bx,1
        jge     ln2
        neg     ax
        neg     bx
ln2:    neg     ax                      ; dy = -|y1-y0|
        mov     [ldy],ax
        mov     [lsy],bx
        mov     bp,[ldx]
        add     bp,ax                   ; err = dx + dy
        mov     cx,si
        mov     dx,di
ln3:    call    plotclip
        cmp     cx,[lx1]
        jne     ln4
        cmp     dx,[ly1]
        je      ln6
ln4:    mov     ax,bp
        shl     ax,1                    ; e2 = 2*err
        cmp     ax,[ldy]
        jl      ln5
        add     bp,[ldy]
        add     cx,[lsx]
ln5:    cmp     ax,[ldx]
        jg      ln3
        add     bp,[ldx]
        add     dx,[lsy]
        jmp     ln3
ln6:    pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
line    ENDP

;=====================================================================
;  Text
;=====================================================================

; text at [SI] (zero-terminated) at column BX (even) and line DX
drawtext PROC   NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        shr     bx,1                    ; word column
        xor     di,di                   ; font row
dtrow:  mov     ax,dx
        add     ax,di
        push    dx
        mov     cx,WPL
        mul     cx
        pop     dx
        add     ax,bx
        mov     bp,ax                   ; word address of this row
        push    si
dtpair: mov     al,[si]
        or      al,al
        jz      dteol
        call    glyphrow
        mov     cl,al
        mov     al,[si+1]
        or      al,al
        jnz     dttwo
        mov     al,' '
        dec     si
dttwo:  call    glyphrow
        mov     ch,al
        push    bx
        push    dx
        mov     ax,bp
        mov     bx,0FFFFh
        mov     dx,cx
        call    gword
        pop     dx
        pop     bx
        inc     bp
        add     si,2
        jmp     dtpair
dteol:  pop     si
        inc     di
        cmp     di,FONT_H
        jb      dtrow
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
drawtext ENDP

glyphrow PROC   NEAR                    ; AL = character, DI = row -> AL
        push    bx
        cmp     al,20h
        jb      gr1
        cmp     al,7Eh
        jbe     gr2
gr1:    mov     al,20h
gr2:    sub     al,20h
        mov     ah,FONT_H
        mul     ah
        add     ax,di
        mov     bx,ax
        mov     al,[font+bx]
        pop     bx
        ret
glyphrow ENDP

; AX (signed) as +nnnnn to [DI]
fmtnum  PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     byte ptr [di],'+'
        or      ax,ax
        jns     fn1
        mov     byte ptr [di],'-'
        neg     ax
fn1:    mov     bx,10
        mov     cx,5
        add     di,5
fn2:    xor     dx,dx
        div     bx
        add     dl,'0'
        mov     [di],dl
        dec     di
        loop    fn2
        add     di,6
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
fmtnum  ENDP

status  PROC    NEAR
        mov     ax,[mx]
        mov     di,offset stx
        call    fmtnum
        mov     ax,[my]
        mov     di,offset sty
        call    fmtnum
        mov     al,[mb]
        mov     byte ptr [stb],'-'
        mov     byte ptr [stb+2],'-'
        mov     byte ptr [stb+4],'-'
        test    al,1
        jz      su1
        mov     byte ptr [stb],'L'
su1:    test    al,4
        jz      su2
        mov     byte ptr [stb+2],'M'
su2:    test    al,2
        jz      su3
        mov     byte ptr [stb+4],'R'
su3:    mov     si,offset sttext
        xor     bx,bx
        mov     dx,STAT_Y
        call    drawtext
        ret
status  ENDP

;=====================================================================
;  Screen: clear, axes, ticks, texts, stored trace
;=====================================================================

redraw  PROC    NEAR
        xor     dx,dx                   ; clear the green plane
        xor     ax,ax
        call    fillplane
        mov     byte ptr [havelast],0
        mov     dx,ORG_Y                ; horizontal axis
        xor     cx,cx
rd1:    call    plot
        inc     cx
        cmp     cx,SCR_W
        jb      rd1
        mov     cx,ORG_X                ; vertical axis
        mov     dx,TOP_Y
rd2:    call    plot
        inc     dx
        cmp     dx,BOT_Y+1
        jb      rd2
        mov     word ptr [tick],100     ; ticks every 100 counts
rd3:    mov     ax,[tick]
        mov     cl,[shift]
        shr     ax,cl                   ; distance in pixels
        cmp     ax,ORG_X
        ja      rd8
        mov     bx,ax
        xor     dx,dx
        mov     ax,[tick]
        mov     cx,500
        div     cx
        mov     si,2                    ; half tick length
        or      dx,dx
        jnz     rd4
        mov     si,6                    ; long tick every 500 counts
        jmp     short rd5
rd4:    cmp     bx,6                    ; short ticks only with enough room
        jb      rd7
rd5:    push    bx
        mov     cx,ORG_X
        add     cx,bx
        call    tickv
        mov     cx,ORG_X
        sub     cx,bx
        call    tickv
        pop     bx
        cmp     bx,ORG_Y-TOP_Y
        ja      rd7
        mov     dx,ORG_Y
        sub     dx,bx
        call    tickh
        mov     dx,ORG_Y
        add     dx,bx
        call    tickh
rd7:    add     word ptr [tick],100
        jmp     rd3
rd8:    mov     cl,[shift]              ; scale into the status line
        mov     al,1
        shl     al,cl
        add     al,'0'
        mov     [stscale],al
        mov     si,offset helptext
        xor     bx,bx
        mov     dx,HELP_Y
        call    drawtext
        call    histdraw
        mov     byte ptr [force],1
        ret
redraw  ENDP

tickv   PROC    NEAR                    ; vertical tick at x = CX
        mov     dx,ORG_Y
        sub     dx,si
        mov     di,si
        shl     di,1
        inc     di
tv1:    call    plotclip
        inc     dx
        dec     di
        jnz     tv1
        ret
tickv   ENDP

tickh   PROC    NEAR                    ; horizontal tick at y = DX
        cmp     dx,TOP_Y
        jl      th2
        cmp     dx,BOT_Y
        jg      th2
        mov     cx,ORG_X
        sub     cx,si
        mov     di,si
        shl     di,1
        inc     di
th1:    call    plotclip
        inc     cx
        dec     di
        jnz     th1
th2:    ret
tickh   ENDP


hist    LABEL   WORD                    ; trace memory, HISTMAX x 4 bytes,
                                        ; behind the program (not in the file)
CSEG    ENDS
        END     START

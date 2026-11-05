;=====================================================================
; MAUSTEST.COM  -  test for the K806 mouse adapter of the
;                  NCR Decision Mate V, graphical or as a text report
;
; This program is the former MOUSETST (graphics) and the former
; MAUSTEST of the MS-DOS 3.30 diskettes (text) in one file. They always
; shared their whole K806 access layer; keeping two programs meant
; keeping two copies of it.
;
;   MAUSTEST            graphical: coordinate cross and trace
;   MAUSTEST /T         text: interrupts, status byte, counters
;   MAUSTEST nn         use base address nn (hex), e.g. MAUSTEST 38
;   MAUSTEST /T 38      both together
;
; ---------------------------------------------------------------------
; Graphical mode
; ---------------------------------------------------------------------
; Draws a coordinate cross with the origin in the middle of the screen
; and traces the absolute position of the K806 (command 00). Moving
; right and up gives positive values, as in the NCR diagnostic program.
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
; ---------------------------------------------------------------------
; Text mode (/T)
; ---------------------------------------------------------------------
; Answers a different question: does the K806 interrupt reach the CPU
; at all? It counts the interrupts arriving on INT 0Ah (IRQ2) and
; INT 0Dh (IRQ5), shows the status byte with its INT bit, the 8259 mask
; and the position, and keeps the counter that matters: "stumm" - how
; often the main loop saw the INT bit set without an interrupt having
; arrived. If that one climbs, the adapter pulls its line but the CPU
; does not jump: mask, vector or missing interrupt controller.
;
; Why acknowledging can be switched off: the 8259 of the DMV runs
; EDGE TRIGGERED (NCR basinit.asm, ICW1 = 17h). The K806 pulls its line
; on every movement and every button change and leaves it active until
; command 03 acknowledges it. Without acknowledging there is exactly
; one interrupt and never another.
;
; Why the query can be locked (key S): the firmware checks after each
; of the five position bytes whether a command has arrived meanwhile,
; and gives up if one has (K806 ROM 33378, 055h and 05Ch). An
; acknowledge from the interrupt handler in the middle of a query
; therefore aborts it. Locked is the correct setting; S unlocks it to
; show the old failure. Both are counted: aborted queries and the
; leftover bytes drained afterwards.
;
; Keys:  Q acknowledge on/off   I adapter interrupts on/off
;        S lock the query       ESC quit
;
; The text mode uses nothing beyond DOS 2.0 (AH=09h, 02h, 06h, 25h,
; 35h, 30h) so that the same program can measure under MS-DOS 2.11 and
; 3.30 - the comparison it was written for.
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
; Build with MASM 5.10:  MASM MAUSTEST; LINK MAUSTEST;
;                        EXE2BIN MAUSTEST.EXE MAUSTEST.COM
;=====================================================================

ST_OBF  EQU     01h             ; a data byte is ready
ST_IBF  EQU     02h             ; input buffer full, do not write
ST_INT  EQU     40h             ; interrupt line active

K_POS   EQU     00h             ; read absolute position (five bytes)
K_INTA  EQU     02h             ; interrupts off
K_QUIT  EQU     03h             ; release line and INT bit
K_BEIDE EQU     07h             ; both interrupt sources

PIC_CMD EQU     90h
PIC_MSK EQU     91h

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
textmod db      0                       ; 1 = /T given: text report
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
        db      'geprueft). Adresse angeben, z.B. MAUSTEST 30',13,10
        db      'Aufruf: MAUSTEST [/T] [Anschluss in hex]',13,10,'$'
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
        cmp     byte ptr [textmod],0    ; /T: the text report instead
        je      graphic
        jmp     textmain
graphic:
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

; Command line: an optional hex base address and an optional /T.
; Both may appear in either order and with - instead of /. The whole
; tail is scanned (the old version stopped at the first non-digit,
; which would have swallowed a /T written after the address).
parsearg PROC   NEAR
        mov     si,80h
        lodsb                           ; length of the tail; SI -> 81h
        mov     cl,al                   ; (not "mov cl,[80h]": MASM reads
        xor     ch,ch                   ;  that as the constant 80h)
        xor     bx,bx
        xor     dx,dx                   ; DL = hex digits seen
pa1:    jcxz    pa4
        lodsb
        dec     cx
        cmp     al,'/'
        je      pasw
        cmp     al,'-'
        je      pasw
        cmp     al,'0'
        jb      pa1                     ; anything else separates
        cmp     al,'9'
        jbe     pa2
        and     al,0DFh
        cmp     al,'A'
        jb      pa1
        cmp     al,'F'
        ja      pa1
        sub     al,7
pa2:    sub     al,'0'
        shl     bx,1
        shl     bx,1
        shl     bx,1
        shl     bx,1
        or      bl,al
        inc     dl
        jmp     pa1
pasw:   jcxz    pa4                     ; the letter after / or -
        lodsb
        dec     cx
        and     al,0DFh
        cmp     al,'T'
        jne     pa1
        mov     byte ptr [textmod],1
        jmp     pa1
pa4:    or      dl,dl
        jz      pa5
        and     bx,0FFh
        mov     [kport],bx
        mov     byte ptr [pgiven],1
pa5:    ret
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


;=====================================================================
;  Text mode (/T)
;
;  Everything below uses the K806 layer above (kcmd, kget, kwait) and
;  nothing from the graphics part. Output goes through DOS only, so the
;  same program runs under MS-DOS 2.11 and 3.30.
;=====================================================================

quitt   db      1                       ; 1 = handler acknowledges
intein  db      0                       ; 1 = adapter interrupts on
sperre  db      1                       ; 1 = lock the query (see below)
inquery db      0                       ; 1 = a position query is running
quitoff db      0                       ; 1 = an acknowledge was deferred
tstat   db      0                       ; status byte of the last round
dosmaj  db      0
dosmin  db      0
pmask   db      0
n0a     dw      0                       ; interrupts on INT 0Ah (IRQ2)
n0d     dw      0                       ; interrupts on INT 0Dh (IRQ5)
nseen   dw      0
nmute   dw      0                       ; line active, no interrupt
nabort  dw      0                       ; queries given up
nrest   dw      0                       ; leftover bytes drained
old0a   dd      0
old0d   dd      0
tdigits db      8 dup(0)

tkopf   db      13,10,'MAUSTEST - K806 auf Anschluss $'
tkopf2  db      'h',13,10,'$'
tdos    db      'DOS $'
tmaske  db      '   8259-Maske $'
tnopic  db      ' (alle Bits gesetzt: kein Interruptcontroller, '
        db      'K230/K231)$'
tlegend db      13,10,'St: Bit0 OBF Bit1 IBF Bit2 F0 Bit3 F1 '
        db      'Bit4 ROM Bit5 RAM Bit6 INT',13,10,'$'
thelp   db      'Q Quittieren   I Interrupts   S Abfrage sperren   '
        db      'ESC Ende',13,10,10,'$'
tsum1   db      13,10,10,'INT 0Ah:$'
tsum2   db      '   INT 0Dh:$'
tsum3   db      '   stumm:$'
tsum4   db      13,10,'abgebrochene Abfragen:$'
tsum5   db      '   weggeraeumte Restbytes:$'
tende   db      13,10,'Vektoren zurueckgegeben, Modulinterrupts '
        db      'abgeschaltet.',13,10,'$'

;  Die laufende Zeile faengt mit CR an und bleibt unter 80 Zeichen, damit
;  sie sich selbst ueberschreibt statt zu rollen. Die ausfuehrlichen
;  Zaehler stehen im Abschluss.
tlflag  db      '[$'
tlzu    db      '] $'
tl0a    db      '0A=$'
tl0d    db      ' 0D=$'
tlst    db      ' St=$'
tlint   db      ' INT=$'
tlx     db      ' X=$'
tly     db      ' Y=$'
tlt     db      ' T=$'
tlmute  db      ' s=$'
tlab    db      ' a=$'

;---------------------------------------------------------------------
textmain:
        call    thead
        mov     al,K_BEIDE              ; both interrupt sources
        call    kcmd
        mov     byte ptr [intein],1
        call    hookvec

tloop:  call    tpos
        call    kstatus
        mov     [tstat],al

;  The counter that matters: line active, but no interrupt arrived.
;  Compared against the SUM of both counters - on whichever line it
;  came in, arrived is arrived.
        test    al,ST_INT
        jz      tl_not
        mov     ax,[n0a]
        add     ax,[n0d]
        cmp     ax,[nseen]
        jne     tl_not
        inc     word ptr [nmute]
        jmp     short tl_on
tl_not: mov     ax,[n0a]
        add     ax,[n0d]
        mov     [nseen],ax
tl_on:  call    tline

;  AH=06h with DL=0FFh does not wait - the very call Draw8086 spins in.
        mov     ah,06h
        mov     dl,0FFh
        int     21h
        jz      tloop
        cmp     al,1Bh
        je      tquit
        and     al,0DFh
        cmp     al,'Q'
        je      tkq
        cmp     al,'I'
        je      tki
        cmp     al,'S'
        je      tks
        jmp     short tloop
tkq:    xor     byte ptr [quitt],1
        jmp     short tloop
tks:    xor     byte ptr [sperre],1
        jmp     short tloop
tki:    xor     byte ptr [intein],1
        mov     al,K_INTA
        cmp     byte ptr [intein],0
        je      tki1
        mov     al,K_BEIDE
tki1:   call    kcmd
        jmp     short tloop

tquit:  mov     al,K_INTA               ; adapter interrupts off
        call    kcmd
        mov     al,K_QUIT               ; release the line
        call    kcmd
        call    unhookvec
        mov     dx,offset tsum1         ; die Zaehler noch einmal in Worten
        call    tputs
        mov     ax,[n0a]
        call    tdec5
        mov     dx,offset tsum2
        call    tputs
        mov     ax,[n0d]
        call    tdec5
        mov     dx,offset tsum3
        call    tputs
        mov     ax,[nmute]
        call    tdec5
        mov     dx,offset tsum4
        call    tputs
        mov     ax,[nabort]
        call    tdec5
        mov     dx,offset tsum5
        call    tputs
        mov     ax,[nrest]
        call    tdec5
        mov     dx,offset tende
        call    tputs
        mov     ax,4C00h
        int     21h

;---------------------------------------------------------------------
;  Header: version, address, DOS version, 8259 mask, legend, keys
;---------------------------------------------------------------------
thead   PROC    NEAR
        mov     dx,offset tkopf
        call    tputs
        mov     al,byte ptr [kport]
        call    thex
        mov     dx,offset tkopf2
        call    tputs

;  So that the screen photo says which system was measured. AH=30h
;  exists since DOS 2.0.
        mov     ah,30h
        int     21h
        mov     [dosmaj],al
        mov     [dosmin],ah
        mov     dx,offset tdos
        call    tputs
        mov     al,[dosmaj]
        call    tdec
        mov     al,'.'
        call    tchar
        mov     al,[dosmin]
        cmp     al,10
        jae     thd1
        mov     al,'0'
        call    tchar
        mov     al,[dosmin]
thd1:    call    tdec

;  Mask of the 8259. Without an interrupt controller (K230/K231) the
;  bus reads 0FFh - then it is already clear that nothing can arrive.
        mov     dx,offset tmaske
        call    tputs
        in      al,PIC_MSK
        mov     [pmask],al
        call    thex
        cmp     al,0FFh
        jne     thd2
        mov     dx,offset tnopic
        call    tputs
thd2:    mov     dx,offset tlegend
        call    tputs
        mov     dx,offset thelp
        call    tputs
        ret
thead   ENDP

;---------------------------------------------------------------------
;  Position query, locked against the interrupt handler
;
;  The firmware checks after each of the five bytes whether a command
;  has arrived and gives up if one has (ROM 33378, 055h/05Ch). An
;  acknowledge from the handler in the middle of a query therefore
;  breaks it. While inquery is set the handler only notes that an
;  acknowledge is due; it is sent here, afterwards. With S the lock is
;  switched off to show the old behaviour - then the aborts and the
;  drained leftover bytes start counting up.
;---------------------------------------------------------------------
tpos    PROC    NEAR
        mov     byte ptr [inquery],1
        mov     al,K_POS
        call    kcmd
        jc      tp_ab
        mov     di,offset mx
        mov     cx,5
tp1:    call    kget
        jc      tp_ab
        mov     [di],al
        inc     di
        loop    tp1
        mov     byte ptr [inquery],0
        call    tdue
        clc
        ret
tp_ab:  inc     word ptr [nabort]
        mov     byte ptr [inquery],0
        call    tdrain
        call    tdue
        stc
        ret
tpos    ENDP

;  Send a deferred acknowledge now that the query is over.
tdue    PROC    NEAR
        cmp     byte ptr [quitoff],0
        je      td9
        mov     byte ptr [quitoff],0
        cmp     byte ptr [quitt],0
        je      td9
        mov     al,K_QUIT
        call    kcmd
td9:    ret
tdue    ENDP

;  Read away what is left of an aborted answer, bounded.
tdrain  PROC    NEAR
        push    cx
        push    dx
        mov     cx,16
tdr1:   mov     dx,[kport]
        inc     dx
        in      al,dx
        test    al,ST_OBF
        jz      tdr2
        mov     dx,[kport]
        in      al,dx
        inc     word ptr [nrest]
        loop    tdr1
tdr2:   pop     dx
        pop     cx
        ret
tdrain  ENDP

;  AL = status byte (port 2)
kstatus PROC    NEAR
        push    dx
        mov     dx,[kport]
        inc     dx
        in      al,dx
        pop     dx
        ret
kstatus ENDP

;---------------------------------------------------------------------
;  The status line. Overwritten with a carriage return and no line
;  feed - every console can do that, under 2.11 as under 3.30, and it
;  needs no escape sequences.
;---------------------------------------------------------------------
tline   PROC    NEAR
        mov     al,13
        call    tchar
        mov     dx,offset tlflag
        call    tputs
        mov     al,[quitt]
        mov     ah,'Q'
        call    tflag
        mov     al,[intein]
        mov     ah,'I'
        call    tflag
        mov     al,[sperre]
        mov     ah,'S'
        call    tflag
        mov     dx,offset tlzu
        call    tputs

        mov     dx,offset tl0a
        call    tputs
        mov     ax,[n0a]
        call    tdec5
        mov     dx,offset tl0d
        call    tputs
        mov     ax,[n0d]
        call    tdec5

        mov     dx,offset tlst
        call    tputs
        mov     al,[tstat]
        call    thex
        mov     dx,offset tlint
        call    tputs
        mov     al,'n'
        test    byte ptr [tstat],ST_INT
        jz      tli1
        mov     al,'j'
tli1:   call    tchar

        mov     dx,offset tlx
        call    tputs
        mov     ax,[mx]
        call    tsigned
        mov     dx,offset tly
        call    tputs
        mov     ax,[my]
        call    tsigned

        mov     dx,offset tlt
        call    tputs
        mov     al,[mb]
        call    tbuttons

        mov     dx,offset tlmute
        call    tputs
        mov     ax,[nmute]
        call    tdec5
        mov     dx,offset tlab
        call    tputs
        mov     ax,[nabort]
        call    tdec5
        ret
tline   ENDP

;  AL = flag -> E or a
;  AL = switch, AH = its letter -> the letter when on, - when off.
tflag   PROC    NEAR
        or      al,al
        mov     al,ah
        jnz     tf1
        mov     al,'-'
tf1:    call    tchar
        ret
tflag   ENDP

;  AL = button byte -> three characters, a dot means released.
;  Bit 0 S1 (left), bit 1 S2 (right), bit 2 S3 (middle).
tbuttons PROC   NEAR
        push    bx
        mov     bl,al
        mov     al,'.'
        test    bl,01h
        jz      tb1
        mov     al,'L'
tb1:    call    tchar
        mov     al,'.'
        test    bl,02h
        jz      tb2
        mov     al,'R'
tb2:    call    tchar
        mov     al,'.'
        test    bl,04h
        jz      tb3
        mov     al,'M'
tb3:    call    tchar
        pop     bx
        ret
tbuttons ENDP

;=====================================================================
;  Interrupt vectors
;
;  Hooked through INT 21h AH=25h/35h, not by touching the table - 35h
;  exists since DOS 2.0.
;=====================================================================
hookvec PROC    NEAR
        push    ds
        mov     ax,350Ah
        int     21h
        mov     word ptr cs:[old0a],bx
        mov     word ptr cs:[old0a+2],es
        mov     ax,350Dh
        int     21h
        mov     word ptr cs:[old0d],bx
        mov     word ptr cs:[old0d+2],es
        push    cs
        pop     ds
        mov     ax,250Ah
        mov     dx,offset isr0a
        int     21h
        mov     ax,250Dh
        mov     dx,offset isr0d
        int     21h
        pop     ds
        ret
hookvec ENDP

unhookvec PROC  NEAR
        push    ds
        mov     ax,250Ah
        lds     dx,dword ptr cs:[old0a]
        int     21h
        pop     ds
        push    ds
        mov     ax,250Dh
        lds     dx,dword ptr cs:[old0d]
        int     21h
        pop     ds
        ret
unhookvec ENDP

;=====================================================================
;  The two interrupt handlers
;
;  They must be tiny and must not touch INT 21h. Everything they do is
;  count and - if switched on and not locked out - acknowledge. All
;  accesses carry a CS override: DS belongs to whoever was interrupted.
;
;  The EOI is sent because DDNCR.SYS sends one too. The 8259 of the DMV
;  runs in AEOI mode (ICW4 = 03h), so the bit in the in-service
;  register has already fallen and a further non-specific EOI finds
;  nothing there. Harmless, but closer to the original.
;=====================================================================
isr0a   PROC    FAR
        push    ax
        push    cx
        push    dx
        inc     word ptr cs:[n0a]
        call    isrq
        pop     dx
        pop     cx
        pop     ax
        iret
isr0a   ENDP

isr0d   PROC    FAR
        push    ax
        push    cx
        push    dx
        inc     word ptr cs:[n0d]
        call    isrq
        pop     dx
        pop     cx
        pop     ax
        iret
isr0d   ENDP

isrq    PROC    NEAR
        cmp     byte ptr cs:[quitt],0
        je      iq_eoi
        cmp     byte ptr cs:[sperre],0
        je      iq_now                  ; not locked: acknowledge here
        cmp     byte ptr cs:[inquery],0
        je      iq_now                  ; no query running: here as well
        mov     byte ptr cs:[quitoff],1 ; locked: let the main loop do it
        jmp     short iq_eoi
iq_now: mov     dx,cs:[kport]
        inc     dx
        mov     cx,200                  ; wait for IBF = 0, bounded
iq_w:   in      al,dx
        test    al,ST_IBF
        jz      iq_free
        loop    iq_w
        jmp     short iq_eoi            ; takes nothing: rather leave it
iq_free: mov    al,K_QUIT
        out     dx,al
iq_eoi: mov     al,20h                  ; non-specific EOI
        out     PIC_CMD,al
        ret
isrq    ENDP

;=====================================================================
;  Output through DOS
;=====================================================================
tputs   PROC    NEAR                    ; DX = text ending in '$'
        push    ax
        mov     ah,09h
        int     21h
        pop     ax
        ret
tputs   ENDP

tchar   PROC    NEAR                    ; AL
        push    ax
        push    dx
        mov     dl,al
        mov     ah,02h
        int     21h
        pop     dx
        pop     ax
        ret
tchar   ENDP

thex    PROC    NEAR                    ; AL as two hex digits
        push    ax
        push    bx
        mov     bl,al
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
        call    tnib
        mov     al,bl
        call    tnib
        pop     bx
        pop     ax
        ret
thex    ENDP

tnib    PROC    NEAR
        and     al,0Fh
        add     al,'0'
        cmp     al,'9'
        jbe     tn1
        add     al,7
tn1:    call    tchar
        ret
tnib    ENDP

tdec    PROC    NEAR                    ; AL decimal, no leading zeros
        push    ax
        push    bx
        push    cx
        push    dx
        xor     ah,ah
        xor     cx,cx
        mov     bx,10
tdc1:   xor     dx,dx
        div     bx
        push    dx
        inc     cx
        or      ax,ax
        jnz     tdc1
tdc2:   pop     ax
        add     al,'0'
        call    tchar
        loop    tdc2
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
tdec    ENDP

;  AX decimal, five places, right aligned with blanks. Fixed width so
;  the line does not flicker and no digits of the previous number stay.
tdec5   PROC    NEAR
        push    ax
        push    bx
        push    cx
        push    dx
        mov     cx,5
        mov     bx,offset tdigits
tw1:    mov     byte ptr [bx],' '
        inc     bx
        loop    tw1
        mov     bx,offset tdigits+4
        mov     cx,10
tw2:    xor     dx,dx
        div     cx
        add     dl,'0'
        mov     [bx],dl
        dec     bx
        or      ax,ax
        jnz     tw2
        mov     cx,5
        mov     bx,offset tdigits
tw3:    mov     al,[bx]
        call    tchar
        inc     bx
        loop    tw3
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
tdec5   ENDP

;  AX signed as sign and five digits, like the graphical status line.
tsigned PROC    NEAR
        push    ax
        or      ax,ax
        jns     ts1
        neg     ax
        push    ax
        mov     al,'-'
        call    tchar
        pop     ax
        jmp     short ts2
ts1:    push    ax
        mov     al,'+'
        call    tchar
        pop     ax
ts2:    push    bx
        push    cx
        push    dx
        mov     cx,5
        mov     bx,offset tdigits
tsa:    mov     byte ptr [bx],'0'
        inc     bx
        loop    tsa
        mov     bx,offset tdigits+4
        mov     cx,10
tsb:    xor     dx,dx
        div     cx
        add     dl,'0'
        mov     [bx],dl
        dec     bx
        or      ax,ax
        jnz     tsb
        mov     cx,5
        mov     bx,offset tdigits
tsc:    mov     al,[bx]
        call    tchar
        inc     bx
        loop    tsc
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
tsigned ENDP

hist    LABEL   WORD                    ; trace memory, HISTMAX x 4 bytes,
                                        ; behind the program (not in the file)
CSEG    ENDS
        END     START

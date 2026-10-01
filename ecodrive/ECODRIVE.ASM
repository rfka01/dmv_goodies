;=====================================================================
; ECODRIVE.COM  -  RAM disk in the red and blue planes of the
;                  NCR Decision Mate V colour graphics board
;
; The colour board has 96KB video RAM in three 32KB planes behind the
; uPD7220 (word addresses: green 0000h, red 4000h, blue 8000h). Text
; mode and green graphics only use the green plane, so red and blue
; (64KB) can hold a small RAM disk. "Eco" because the screen stays
; green.
;
; ECODRIVE is a TSR that hooks itself into DOS as a block device at
; run time (device chain, DPB chain, CDS) and can be removed again:
;
;   ECODRIVE            load, next free drive letter
;   ECODRIVE /X:NAME    load and add NAME to the program watch list
;                       (up to 8 names, NAME* matches a prefix)
;   ECODRIVE /F         set the drive up again (empty), also after damage
;   ECODRIVE /U         remove (must be the last drive and the last TSR);
;                       asks first, the files on the drive are lost
;   ECODRIVE /?         help
;
; Protection of the planes (the 8088 cannot trap port A0h/A1h):
;   1. INT 2Fh, AX=EC00h: installation check for cooperating programs
;        returns AL=FFh, BX=4543h ('EC'), CX=version (BCD 0150h),
;        DX=occupied planes (bit 1 red, bit 2 blue), ES=segment.
;      AX=EC01h returns AL=drive number (0=A:).
;   2. INT 21h, AX=4B00h: programs on the watch list are only started
;      after a J/N prompt; N returns error 5 (access denied). After a
;      watched program ends, all sectors are checked against their
;      checksums. If any is damaged, the drive is locked: DOS is told
;      the disk was changed (so it drops its buffers) and every access
;      reports "not ready" until ECODRIVE /F sets the drive up again.
;   3. A checksum per sector in main memory. Damaged sectors give a
;      DOS data error (CRC) instead of wrong data.
;
; Requires MS-DOS 3.1 - 3.3 (List of Lists layout of 3.x).
;
; Build:  MASM ECODRIVE;  LINK ECODRIVE;  EXE2BIN ECODRIVE ECODRIVE.COM
;=====================================================================

GDCSTA  EQU     0A0h            ; read: status, write: parameter
GDCCMD  EQU     0A1h            ; read: FIFO data, write: command

C_CSRW  EQU     49h             ; cursor position specify
C_MASK  EQU     4Ah             ; mask register load
C_FIGS  EQU     4Ch             ; figure drawing parameters
C_WDAT  EQU     20h             ; write data: word, low byte first, replace
C_RDAT  EQU     0A0h            ; read data: word, low byte first
C_CURD  EQU     0E0h            ; cursor address read

S_DRDY  EQU     01h             ; status: data ready
S_FULL  EQU     02h             ; status: FIFO full
S_EMPTY EQU     04h             ; status: FIFO empty
S_DRAW  EQU     08h             ; status: drawing in progress

PLANE0  EQU     40h             ; high byte of the first red word (4000h)
NSECT   EQU     128             ; 64KB / 512 bytes
MPXID   EQU     0ECh            ; INT 2Fh multiplex id
VERSION EQU     0150h
SIGN    EQU     4543h           ; 'EC'
NNAMES  EQU     13              ; watch list entries (5 built in + 8)

; German characters of the DMV console (DMVCON): the character
; generator 76161 has 128 glyphs and ignores bit 7, the accented
; characters sit at 00h-1Fh and are written as 80h-9Fh
; (NCR CRTTBL/GERMANY in basinit.asm of DOS 2.11)
AE      EQU     90h             ; ae (glyph 10h)
OE      EQU     96h             ; oe (glyph 16h)
UE      EQU     99h             ; ue (glyph 19h)
SZ      EQU     9Eh             ; sz (glyph 1Eh)

; error codes of the request header status word
E_NREADY EQU    02h
E_CRC   EQU     04h
E_SECT  EQU     08h
E_WRITE EQU     0Ah

CODE    SEGMENT
        ASSUME  CS:CODE, DS:CODE, ES:NOTHING, SS:CODE
        ORG     100h

START:  JMP     INSTALL

;=====================================================================
; Resident part
;=====================================================================

DEVHDR  DW      0FFFFh, 0FFFFh  ; next driver
        DW      2000h           ; block device, non-IBM format
        DW      OFFSET STRAT
        DW      OFFSET INTR
        DB      1               ; one unit
        DB      'ECODRV '

REQPTR  DD      0
OLD21   DD      0
OLD2F   DD      0
DRIVE   DB      0               ; drive number, 0 = A:
OPMODE  DB      0               ; 0 read, 1 write, 2 write + verify
GDCERR  DB      0               ; set by the GDC helpers on timeout
DAMAGED DB      0               ; 1 = sectors damaged, drive locked
MEDIACHG DB     0               ; 1 = report "disk changed" once
INEXEC  DB      0               ; 1 = a watched program is running
SAVSS   DW      0               ; caller stack during a watched EXEC
SAVSP   DW      0
EXAX    DW      0               ; EXEC result
EXFLG   DW      0
SAVEAD  DB      0, 0, 0         ; GDC cursor address (EAD) of the console
SAVMSK  DW      0               ; GDC mask register of the console

BPB     DW      512             ; bytes per sector
        DB      1               ; sectors per cluster
        DW      1               ; reserved sectors (boot sector)
        DB      1               ; number of FATs
        DW      32              ; root directory entries
        DW      NSECT           ; total sectors
        DB      0F8h            ; media descriptor
        DW      1               ; sectors per FAT
        DW      8               ; sectors per track
        DW      1               ; heads
        DW      0               ; hidden sectors
BPBLEN  EQU     $ - BPB

DPB     DB      21h DUP (0)     ; DOS 3.x drive parameter block

CSUMS   DW      NSECT DUP (0)   ; checksum per sector

; watch list: 8 characters + 0, a trailing '*' matches any rest
NAMES   DB      'DMVDEMO', 0, 0
        DB      'DMVDEM*', 0, 0
        DB      'DMVRACE', 0, 0
        DB      'GEM', 0, 0, 0, 0, 0, 0
        DB      'GEMVDI', 0, 0, 0
        DB      (NNAMES - 5) * 9 DUP (0)

ASKMSG  DB      13, 10, 'ECODRIVE: Dieses Programm kann Grafik in Rot/Blau zeichnen', 13, 10
        DB      'und dabei die RAM-Disk zerst', OE, 'ren.', 13, 10
        DB      'Trotzdem starten (J/N)? $'
CRLF    DB      13, 10, '$'
DMGMSG1 DB      13, 10, 'ECODRIVE: $'
DMGMSG2 DB      ' von 128 Sektoren besch', AE, 'digt. Laufwerk $'
DMGMSG3 DB      ': gesperrt.', 13, 10
        DB      'Neu einrichten mit ECODRIVE /F.', 13, 10, '$'

;---------------------------------------------------------------------
; Device driver entry points
;---------------------------------------------------------------------

STRAT   PROC    FAR
        MOV     WORD PTR CS:REQPTR, BX
        MOV     WORD PTR CS:REQPTR+2, ES
        RET
STRAT   ENDP

INTR    PROC    FAR
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    SI
        PUSH    DI
        PUSH    BP
        PUSH    DS
        PUSH    ES
        CLD
        LDS     BX, CS:REQPTR
        MOV     AL, [BX+2]              ; command code
        MOV     WORD PTR [BX+3], 0100h  ; done
        CMP     AL, 1
        JE      MEDIA
        CMP     AL, 2
        JE      BUILD
        CMP     AL, 4
        JE      DOREAD
        CMP     AL, 8
        JE      DOWRIT
        CMP     AL, 9
        JE      DOWRV
        OR      AL, AL                  ; init is never called
        JZ      IEX1
        MOV     WORD PTR [BX+3], 8103h  ; unknown command
IEX1:   JMP     IEXIT

MEDIA:  CMP     CS:DAMAGED, 0
        JNE     MCHG
        CMP     CS:MEDIACHG, 0
        JE      MSAME
        MOV     CS:MEDIACHG, 0
MCHG:   MOV     BYTE PTR [BX+0Eh], 0FFh ; changed: DOS drops its buffers
        JMP     IEXIT
MSAME:  MOV     BYTE PTR [BX+0Eh], 1    ; media not changed
        JMP     IEXIT

BUILD:  MOV     WORD PTR [BX+12h], OFFSET BPB
        MOV     WORD PTR [BX+14h], CS
        JMP     IEXIT

DOREAD: MOV     CS:OPMODE, 0
        JMP     SHORT XFER
DOWRIT: MOV     CS:OPMODE, 1
        JMP     SHORT XFER
DOWRV:  MOV     CS:OPMODE, 2

XFER:   MOV     CX, [BX+12h]            ; sector count
        MOV     DX, [BX+14h]            ; first sector
        LES     DI, [BX+0Eh]            ; transfer address
        MOV     WORD PTR [BX+12h], 0
        CMP     CS:DAMAGED, 0
        JE      XF1
        MOV     AL, E_NREADY            ; locked after damage
        JMP     SHORT XERR
XF1:
        MOV     AX, DX
        ADD     AX, CX
        JC      BADSEC
        CMP     AX, NSECT
        JA      BADSEC
        JCXZ    IEXIT

XLOOP:  PUSH    CX
        PUSH    BX
        PUSH    DS
        PUSH    DI
        PUSH    CS
        POP     DS
        ASSUME  DS:CODE
        MOV     AX, DX
        CMP     OPMODE, 0
        JNE     XWR
        CALL    RDSEC
        JMP     SHORT XCHK
XWR:    CALL    WRSEC
        OR      AL, AL
        JNZ     XCHK
        CMP     OPMODE, 2
        JNE     XCHK
        MOV     AX, DX
        CALL    VFYSEC
XCHK:   POP     DI
        POP     DS
        ASSUME  DS:NOTHING
        POP     BX
        POP     CX
        OR      AL, AL
        JNZ     XERR
        INC     WORD PTR [BX+12h]
        MOV     AX, ES                  ; next 512 bytes
        ADD     AX, 32
        MOV     ES, AX
        INC     DX
        LOOP    XLOOP
        JMP     SHORT IEXIT

BADSEC: MOV     AL, E_SECT
XERR:   MOV     AH, 81h                 ; error + done
        MOV     [BX+3], AX

IEXIT:  POP     ES
        POP     DS
        POP     BP
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        RET
INTR    ENDP
        ASSUME  DS:CODE

;---------------------------------------------------------------------
; Sector transfer (DS = CS). AX = sector, ES:DI = buffer.
; Returns AL = 0 or a request header error code.
;---------------------------------------------------------------------

; read a sector into ES:DI and check it against its checksum
RDSEC   PROC    NEAR
        MOV     BL, 1
        JMP     SHORT GETSEC
RDSEC   ENDP

; read a sector without storing it, only check it (write verify)
VFYSEC  PROC    NEAR
        MOV     BL, 0
        JMP     SHORT GETSEC
VFYSEC  ENDP

GETSEC  PROC    NEAR
        PUSH    DX
        MOV     BP, AX                  ; sector
        MOV     GDCERR, 0
        PUSHF
        CLI
        CALL    SAVECUR
        MOV     AX, BP
        MOV     AH, AL                  ; address = 4000h + sector * 256
        ADD     AH, PLANE0
        XOR     AL, AL
        CALL    CSRW
        MOV     AX, 256                 ; word count
        CALL    FIGSR
        MOV     AL, C_RDAT
        CALL    PUTCMD
        XOR     SI, SI                  ; checksum
        MOV     CX, 256
GS1:    CALL    GETB
        MOV     DL, AL
        CALL    GETB
        MOV     DH, AL
        ROL     SI, 1
        ADD     SI, DX
        OR      BL, BL
        JZ      GS2
        MOV     ES:[DI], DX
        ADD     DI, 2
GS2:    LOOP    GS1
        CALL    DRAIN
        CALL    RESTCUR
        POPF
        POP     DX
        MOV     AL, E_NREADY
        CMP     GDCERR, 0
        JNE     GS9
        MOV     BX, BP
        SHL     BX, 1
        MOV     AL, E_CRC
        CMP     CSUMS[BX], SI
        JNE     GS9
        XOR     AL, AL
GS9:    RET
GETSEC  ENDP

; write a sector from ES:DI and record its checksum
WRSEC   PROC    NEAR
        PUSH    DX
        MOV     BP, AX                  ; sector
        MOV     GDCERR, 0
        PUSHF
        CLI
        CALL    SAVECUR
        CALL    SETMASK
        MOV     AX, BP
        MOV     AH, AL
        ADD     AH, PLANE0
        XOR     AL, AL
        CALL    CSRW
        XOR     AX, AX                  ; one word per parameter pair
        CALL    FIGSR
        MOV     AL, C_WDAT
        CALL    PUTCMD
        XOR     SI, SI
        MOV     CX, 256
WS1:    MOV     DX, ES:[DI]
        ADD     DI, 2
        ROL     SI, 1
        ADD     SI, DX
        MOV     AL, DL
        CALL    PUTPAR
        MOV     AL, DH
        CALL    PUTPAR
        LOOP    WS1
        CALL    WAITIDLE
        CALL    RESTCUR
        POPF
        POP     DX
        MOV     BX, BP
        SHL     BX, 1
        MOV     CSUMS[BX], SI
        XOR     AL, AL
        CMP     GDCERR, 0
        JE      WS9
        NOT     WORD PTR CSUMS[BX]      ; contents unknown, reads will fail
        MOV     AL, E_WRITE
WS9:    RET
WRSEC   ENDP

;---------------------------------------------------------------------
; uPD7220 helpers. All of them give up after a timeout and set GDCERR
; instead of hanging; once GDCERR is set they return at once.
;---------------------------------------------------------------------

; wait until the FIFO can take a byte
WAITNF  PROC    NEAR
        PUSH    AX
        PUSH    CX
        XOR     CX, CX
WNF1:   IN      AL, GDCSTA
        TEST    AL, S_FULL
        JZ      WNF2
        LOOP    WNF1
        MOV     GDCERR, 1
WNF2:   POP     CX
        POP     AX
        RET
WAITNF  ENDP

; AL = command
PUTCMD  PROC    NEAR
        CMP     GDCERR, 0
        JNE     PC9
        CALL    WAITNF
        OUT     GDCCMD, AL
PC9:    RET
PUTCMD  ENDP

; AL = parameter
PUTPAR  PROC    NEAR
        CMP     GDCERR, 0
        JNE     PP9
        CALL    WAITNF
        OUT     GDCSTA, AL
PP9:    RET
PUTPAR  ENDP

; AL = next byte from the FIFO
GETB    PROC    NEAR
        PUSH    CX
        XOR     AL, AL
        CMP     GDCERR, 0
        JNE     GB9
        XOR     CX, CX
GB1:    IN      AL, GDCSTA
        TEST    AL, S_DRDY
        JNZ     GB2
        LOOP    GB1
        MOV     GDCERR, 1
        XOR     AL, AL
        JMP     SHORT GB9
GB2:    IN      AL, GDCCMD
GB9:    POP     CX
        RET
GETB    ENDP

; wait until the FIFO is empty and no drawing is in progress
WAITIDLE PROC   NEAR
        PUSH    AX
        PUSH    CX
        PUSH    DX
        CMP     GDCERR, 0
        JNE     WI9
        MOV     DX, 40                  ; about 40 x 64K polls
WI1:    XOR     CX, CX
WI2:    IN      AL, GDCSTA
        AND     AL, S_EMPTY OR S_DRAW
        CMP     AL, S_EMPTY
        JE      WI9
        LOOP    WI2
        DEC     DX
        JNZ     WI1
        MOV     GDCERR, 1
WI9:    POP     DX
        POP     CX
        POP     AX
        RET
WAITIDLE ENDP

; discard data the GDC still delivers after a read. The real chip may
; read DC+1 words where MAME reads DC, so up to one extra word arrives.
DRAIN   PROC    NEAR
        PUSH    AX
        PUSH    CX
        CMP     GDCERR, 0
        JNE     DRN9
        XOR     CX, CX
DRN1:    IN      AL, GDCSTA
        TEST    AL, S_DRDY
        JZ      DRN2
        IN      AL, GDCCMD
        JMP     SHORT DRN3
DRN2:    AND     AL, S_EMPTY OR S_DRAW
        CMP     AL, S_EMPTY
        JE      DRN9
DRN3:    LOOP    DRN1
DRN9:    POP     CX
        POP     AX
        RET
DRAIN   ENDP

; AX = word address below 10000h; the mask register is not changed
CSRW    PROC    NEAR
        PUSH    AX
        MOV     AL, C_CSRW
        CALL    PUTCMD
        POP     AX
        CALL    PUTPAR
        MOV     AL, AH
        CALL    PUTPAR
        RET
CSRW    ENDP

; direction "right" (address + 1), AX = DC
FIGSR   PROC    NEAR
        PUSH    AX
        MOV     AL, C_FIGS
        CALL    PUTCMD
        MOV     AL, 02h
        CALL    PUTPAR
        POP     AX
        CALL    PUTPAR
        MOV     AL, AH
        AND     AL, 3Fh
        CALL    PUTPAR
        RET
FIGSR   ENDP

; all 16 bits of a word are written
SETMASK PROC    NEAR
        MOV     AL, C_MASK
        CALL    PUTCMD
        MOV     AL, 0FFh
        CALL    PUTPAR
        CALL    PUTPAR
        RET
SETMASK ENDP

; save the console's cursor address and mask (CURD). Data a previous
; read of the console left in the FIFO is thrown away first.
SAVECUR PROC    NEAR
        CALL    DRAIN
        CALL    WAITIDLE
        MOV     AL, C_CURD
        CALL    PUTCMD
        CALL    GETB
        MOV     SAVEAD, AL
        CALL    GETB
        MOV     SAVEAD+1, AL
        CALL    GETB
        MOV     SAVEAD+2, AL
        CALL    GETB
        MOV     BYTE PTR SAVMSK, AL
        CALL    GETB
        MOV     BYTE PTR SAVMSK+1, AL
        RET
SAVECUR ENDP

; restore cursor address, mask and FIGS direction "right", DC = 0
RESTCUR PROC    NEAR
        CALL    WAITIDLE
        MOV     AL, C_CSRW
        CALL    PUTCMD
        MOV     AL, SAVEAD
        CALL    PUTPAR
        MOV     AL, SAVEAD+1
        CALL    PUTPAR
        MOV     AL, SAVEAD+2
        AND     AL, 03h
        JZ      RC1
        CALL    PUTPAR                  ; EAD bits 16/17 (sets the mask too)
RC1:    MOV     AL, C_MASK
        CALL    PUTCMD
        MOV     AL, BYTE PTR SAVMSK
        CALL    PUTPAR
        MOV     AL, BYTE PTR SAVMSK+1
        CALL    PUTPAR
        XOR     AX, AX
        CALL    FIGSR
        RET
RESTCUR ENDP

;---------------------------------------------------------------------
; INT 2Fh: installation check
;---------------------------------------------------------------------

I2F     PROC    FAR
        CMP     AH, MPXID
        JE      I2FME
        JMP     CS:OLD2F
I2FME:  CMP     AL, 0
        JNE     I2F1
        MOV     AL, 0FFh
        MOV     BX, SIGN
        MOV     CX, VERSION
        MOV     DX, 0006h               ; red and blue plane in use
        PUSH    CS
        POP     ES
        IRET
I2F1:   CMP     AL, 1
        JNE     I2F9
        MOV     AL, CS:DRIVE
I2F9:   IRET
I2F     ENDP

;---------------------------------------------------------------------
; INT 21h: EXEC watch
;---------------------------------------------------------------------

I21     PROC    FAR
        CMP     AX, 4B00h
        JE      I21X
I21GO:  JMP     CS:OLD21

I21X:   STI
        PUSH    AX
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    SI
        PUSH    DI
        PUSH    DS
        PUSH    ES
        CMP     CS:INEXEC, 0            ; already asked for the parent
        JNE     I21OK
        CALL    MATCH                   ; CF = program is on the list
        JNC     I21OK
        PUSH    CS
        POP     DS
        MOV     DX, OFFSET ASKMSG
        MOV     AH, 9
        INT     21h
I21K:   MOV     AH, 8
        INT     21h
        MOV     DL, AL
        AND     AL, 0DFh
        CMP     AL, 'J'
        JE      I21Y
        CMP     AL, 'Y'
        JE      I21Y
        CMP     AL, 'N'
        JNE     I21K
        CALL    ECHOKEY
        POP     ES
        POP     DS
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        MOV     AX, 5                   ; access denied
        STC
        RET     2
I21Y:   CALL    ECHOKEY
        POP     ES
        POP     DS
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        ; run the program and check the drive when it has ended
        MOV     CS:INEXEC, 1
        MOV     CS:SAVSS, SS
        MOV     CS:SAVSP, SP
        PUSHF
        CALL    CS:OLD21
        CLI
        MOV     SS, CS:SAVSS            ; DOS 2.x does not keep SS:SP
        MOV     SP, CS:SAVSP
        STI
        PUSHF
        POP     CS:EXFLG
        MOV     CS:EXAX, AX
        JC      I21R                    ; program was not started
        PUSH    BX
        PUSH    CX
        PUSH    DX
        PUSH    SI
        PUSH    DI
        PUSH    BP
        PUSH    DS
        PUSH    ES
        PUSH    CS
        POP     DS
        CALL    CHKALL
        POP     ES
        POP     DS
        POP     BP
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
I21R:   MOV     CS:INEXEC, 0
        MOV     AX, CS:EXAX
        PUSH    CS:EXFLG
        POPF
        RET     2

I21OK:  POP     ES
        POP     DS
        POP     DI
        POP     SI
        POP     DX
        POP     CX
        POP     BX
        POP     AX
        JMP     CS:OLD21
I21     ENDP

; check every sector against its checksum (DS = CS); lock the drive
; and report if any is damaged
CHKALL  PROC    NEAR
        CMP     DAMAGED, 0
        JNE     CK9
        XOR     CX, CX                  ; damaged sectors
        XOR     AX, AX
CK1:    PUSH    AX
        PUSH    CX
        CALL    VFYSEC
        POP     CX
        OR      AL, AL
        JZ      CK2
        INC     CX
CK2:    POP     AX
        INC     AX
        CMP     AX, NSECT
        JB      CK1
        JCXZ    CK9
        MOV     DAMAGED, 1
        MOV     MEDIACHG, 1
        PUSH    CX
        MOV     DX, OFFSET DMGMSG1
        MOV     AH, 9
        INT     21h
        POP     AX
        CALL    PRDEC
        MOV     DX, OFFSET DMGMSG2
        MOV     AH, 9
        INT     21h
        MOV     DL, DRIVE
        ADD     DL, 'A'
        MOV     AH, 2
        INT     21h
        MOV     DX, OFFSET DMGMSG3
        MOV     AH, 9
        INT     21h
CK9:    RET
CHKALL  ENDP

; print AX in decimal
PRDEC   PROC    NEAR
        MOV     BX, 10
        XOR     CX, CX
PD1:    XOR     DX, DX
        DIV     BX
        PUSH    DX
        INC     CX
        OR      AX, AX
        JNZ     PD1
PD2:    POP     DX
        ADD     DL, '0'
        MOV     AH, 2
        INT     21h
        LOOP    PD2
        RET
PRDEC   ENDP

; DL = key, echo it and a new line
ECHOKEY    PROC    NEAR
        MOV     AH, 2
        INT     21h
        MOV     DX, OFFSET CRLF
        MOV     AH, 9
        INT     21h
        RET
ECHOKEY    ENDP

; DS:DX = program path. CF = 1 if its name is on the watch list.
MATCH   PROC    NEAR
        MOV     SI, DX
        MOV     BX, DX                  ; start of the file name
MA1:    LODSB
        OR      AL, AL
        JZ      MA2
        CMP     AL, ':'
        JE      MA1A
        CMP     AL, '/'
        JE      MA1A
        CMP     AL, 5Ch                 ; backslash
        JNE     MA1
MA1A:   MOV     BX, SI
        JMP     SHORT MA1
MA2:    MOV     DI, OFFSET NAMES
        MOV     CX, NNAMES
MA3:    CMP     BYTE PTR CS:[DI], 0     ; empty entry
        JE      MA6
        MOV     SI, BX
        PUSH    DI
MA4:    MOV     AH, CS:[DI]
        LODSB
        CMP     AL, 'a'
        JB      MA4A
        CMP     AL, 'z'
        JA      MA4A
        SUB     AL, 20h
MA4A:   CMP     AH, '*'
        JE      MA5                     ; prefix matches
        OR      AH, AH
        JNZ     MA4B
        OR      AL, AL                  ; end of list name: the program
        JZ      MA5                     ; name must end here as well
        CMP     AL, '.'
        JE      MA5
        JMP     SHORT MA4C
MA4B:   CMP     AL, AH
        JNE     MA4C
        INC     DI
        JMP     SHORT MA4
MA4C:   POP     DI
MA6:    ADD     DI, 9
        LOOP    MA3
        CLC
        RET
MA5:    POP     DI
        STC
        RET
MATCH   ENDP

RESEND  LABEL   BYTE

;=====================================================================
; Transient part
;=====================================================================

LOLOFF  DW      0
LOLSEG  DW      0
RSEG    DW      0
FUNLOAD DB      0
FFORMAT DB      0
FHELP   DB      0
NEXTNM  DW      OFFSET NAMES + 5 * 9

MBANNER DB      'ECODRIVE 1.5 - RAM-Disk im Rot/Blau-Speicher der DMV-Farbgrafik'
        DB      13, 10, '$'
MHELP   DB      'Aufruf:  ECODRIVE           laden', 13, 10
        DB      '         ECODRIVE /F        Laufwerk neu einrichten (leer)', 13, 10
        DB      '         ECODRIVE /X:NAME   laden, NAME vor dem Start r', UE, 'ckfragen'
        DB      13, 10
        DB      '                            (bis zu 8 Namen, NAME* f', UE, 'r Anfang)'
        DB      13, 10
        DB      '         ECODRIVE /U        entfernen (Dateien gehen verloren)', 13, 10
        DB      13, 10
        DB      'Nur mit Textausgabe oder gr', UE, 'ner Grafik verwenden: Rot und Blau'
        DB      13, 10
        DB      'enthalten die RAM-Disk.', 13, 10, '$'
MOK     DB      'ECODRIVE als Laufwerk '
MOKDRV  DB      'X: geladen (62 KB).', 13, 10, '$'
MREMOK  DB      'ECODRIVE entfernt.', 13, 10, '$'
MBADVER DB      'ECODRIVE braucht MS-DOS 3.1 bis 3.3.', 13, 10, '$'
MDRDOS  DB      'ECODRIVE unterst', UE, 'tzt DR-DOS noch nicht.', 13, 10, '$'
MBADARG DB      'Ung', UE, 'ltiger Parameter. ECODRIVE /? zeigt die Hilfe.', 13, 10, '$'
MFULL   DB      'Zu viele Namen f', UE, 'r /X.', 13, 10, '$'
MLOADED DB      'ECODRIVE ist bereits geladen.', 13, 10, '$'
MNOTLD  DB      'ECODRIVE ist nicht geladen.', 13, 10, '$'
MNOLET  DB      'Kein freier Laufwerksbuchstabe. LASTDRIVE in CONFIG.SYS '
        DB      'erh', OE, 'hen.', 13, 10, '$'
MNOCOL  DB      'Keine Farbgrafikkarte gefunden (nur 32KB Bildspeicher vorhanden).', 13, 10, '$'
MGDCERR DB      'Der Grafikcontroller antwortet nicht.', 13, 10, '$'
MNOTTOP DB      'ECODRIVE kann nicht entfernt werden: ein sp', AE, 'ter geladenes '
        DB      'Programm', 13, 10, 'benutzt dieselben Interrupts.', 13, 10, '$'
MNOTLST DB      'ECODRIVE kann nicht entfernt werden: es ist nicht das letzte '
        DB      'Laufwerk.', 13, 10, '$'
MCURDRV DB      'ECODRIVE kann nicht entfernt werden: es ist das aktuelle '
        DB      'Laufwerk.', 13, 10, '$'
MNOTFND DB      'ECODRIVE nicht in den DOS-Tabellen gefunden.', 13, 10, '$'
MASKFMT DB      'Alle Dateien auf Laufwerk '
MASKDRV DB      'X: gehen verloren. Neu einrichten (J/N)? $'
MFMTOK  DB      'ECODRIVE: Laufwerk '
MFMTDRV DB      'X: neu eingerichtet.', 13, 10, '$'
MCANCEL DB      'Abgebrochen.', 13, 10, '$'
MASKREM DB      'Alle Dateien auf Laufwerk '
MASKRDR DB      'X: gehen beim Entfernen verloren. Entfernen (J/N)? $'

;---------------------------------------------------------------------

INSTALL:
        CLD
        MOV     AX, 3000h               ; DOS version
        INT     21h
        CMP     AL, 3
        JNE     BADVER
        CMP     AH, 10
        JAE     VEROK
BADVER: MOV     DX, OFFSET MBADVER
        JMP     ERREXIT
VEROK:  MOV     AX, 4452h               ; DR-DOS answers this one
        STC
        INT     21h
        JC      NODR
        MOV     DX, OFFSET MDRDOS
        JMP     ERREXIT
NODR:   CALL    PARSE
        JNC     ARGOK
        JMP     ERREXIT
ARGOK:  CMP     FHELP, 0
        JE      NOHELP
        MOV     DX, OFFSET MBANNER
        MOV     AH, 9
        INT     21h
        MOV     DX, OFFSET MHELP
        MOV     AH, 9
        INT     21h
        MOV     AX, 4C00h
        INT     21h
NOHELP: CMP     FFORMAT, 0
        JE      NOFMT
        JMP     REFORM
NOFMT:  CMP     FUNLOAD, 0
        JE      LOAD
        JMP     UNLOAD

;---------------------------------------------------------------------
; Load
;---------------------------------------------------------------------

LOAD:   MOV     AX, MPXID * 256         ; already there?
        XOR     BX, BX
        INT     2Fh
        CMP     AL, 0FFh
        JNE     LD1
        CMP     BX, SIGN
        JNE     LD1
        MOV     DX, OFFSET MLOADED
        JMP     ERREXIT

LD1:    MOV     AH, 52h                 ; List of Lists
        INT     21h
        MOV     LOLOFF, BX
        MOV     LOLSEG, ES
        MOV     AL, ES:[BX+20h]         ; number of block devices
        CMP     AL, ES:[BX+21h]         ; LASTDRIVE
        JAE     NOLET
        MOV     DRIVE, AL
        CALL    GETCDS                  ; ES:DI = CDS entry of the drive
        CMP     WORD PTR ES:[DI+43h], 0
        JE      LD2
NOLET:  MOV     DX, OFFSET MNOLET
        JMP     ERREXIT

LD2:    CALL    DETECT
        CMP     GDCERR, 0
        JE      LD3
        MOV     DX, OFFSET MGDCERR
        JMP     ERREXIT
LD3:    OR      AL, AL
        JZ      LD4
        MOV     DX, OFFSET MNOCOL
        JMP     ERREXIT

LD4:    CALL    FORMAT
        CMP     GDCERR, 0
        JE      LD5
        MOV     DX, OFFSET MGDCERR
        JMP     ERREXIT

        ; drive parameter block from our BPB
LD5:    PUSH    CS
        POP     ES
        MOV     SI, OFFSET BPB
        MOV     BP, OFFSET DPB
        MOV     AH, 53h
        INT     21h
        MOV     AL, DRIVE
        MOV     DPB[0], AL              ; drive
        MOV     DPB[1], 0               ; unit
        MOV     WORD PTR DPB[12h], OFFSET DEVHDR
        MOV     WORD PTR DPB[14h], CS
        MOV     DPB[17h], 0FFh          ; not accessed yet
        MOV     WORD PTR DPB[18h], 0FFFFh
        MOV     WORD PTR DPB[1Ah], 0FFFFh
        MOV     WORD PTR DPB[1Ch], 0
        MOV     WORD PTR DPB[1Eh], 0FFFFh

        ; append it to the DPB chain
        LES     BX, DWORD PTR LOLOFF
        LES     DI, ES:[BX]             ; first DPB
LD6:    CMP     WORD PTR ES:[DI+18h], 0FFFFh
        JE      LD7
        LES     DI, ES:[DI+18h]
        JMP     SHORT LD6
LD7:    MOV     WORD PTR ES:[DI+18h], OFFSET DPB
        MOV     ES:[DI+1Ah], CS

        ; device chain: insert after NUL
        LES     BX, DWORD PTR LOLOFF
        MOV     AX, ES:[BX+22h]
        MOV     DEVHDR, AX
        MOV     AX, ES:[BX+24h]
        MOV     DEVHDR+2, AX
        MOV     WORD PTR ES:[BX+22h], OFFSET DEVHDR
        MOV     ES:[BX+24h], CS

        ; current directory structure
        CALL    GETCDS
        MOV     AL, DRIVE
        ADD     AL, 'A'
        MOV     MOKDRV, AL
        MOV     ES:[DI], AL
        MOV     BYTE PTR ES:[DI+1], ':'
        MOV     BYTE PTR ES:[DI+2], 5Ch
        MOV     BYTE PTR ES:[DI+3], 0
        MOV     WORD PTR ES:[DI+43h], 4000h     ; physical drive
        MOV     WORD PTR ES:[DI+45h], OFFSET DPB
        MOV     ES:[DI+47h], CS
        MOV     WORD PTR ES:[DI+49h], 0FFFFh    ; never accessed
        MOV     WORD PTR ES:[DI+4Bh], 0FFFFh
        MOV     WORD PTR ES:[DI+4Dh], 0FFFFh
        MOV     WORD PTR ES:[DI+4Fh], 2         ; root backslash offset

        LES     BX, DWORD PTR LOLOFF
        INC     BYTE PTR ES:[BX+20h]

        ; interrupts
        MOV     AX, 3521h
        INT     21h
        MOV     WORD PTR OLD21, BX
        MOV     WORD PTR OLD21+2, ES
        MOV     AX, 352Fh
        INT     21h
        MOV     WORD PTR OLD2F, BX
        MOV     WORD PTR OLD2F+2, ES
        MOV     DX, OFFSET I21
        MOV     AX, 2521h
        INT     21h
        MOV     DX, OFFSET I2F
        MOV     AX, 252Fh
        INT     21h

        ; environment is not needed
        MOV     ES, DS:[2Ch]
        MOV     AH, 49h
        INT     21h
        MOV     WORD PTR DS:[2Ch], 0

        MOV     DX, OFFSET MBANNER
        MOV     AH, 9
        INT     21h
        MOV     DX, OFFSET MOK
        MOV     AH, 9
        INT     21h

        MOV     DX, OFFSET RESEND + 15
        MOV     CL, 4
        SHR     DX, CL
        MOV     AX, 3100h
        INT     21h

;---------------------------------------------------------------------
; Unload
;---------------------------------------------------------------------

UNLOAD: MOV     AX, MPXID * 256
        XOR     BX, BX
        INT     2Fh
        CMP     AL, 0FFh
        JNE     UL0
        CMP     BX, SIGN
        JE      UL1
UL0:    MOV     DX, OFFSET MNOTLD
        JMP     ERREXIT

UL1:    MOV     RSEG, ES
        MOV     AX, 3521h               ; still on top of both vectors?
        INT     21h
        CMP     BX, OFFSET I21
        JNE     ULTOP
        MOV     AX, ES
        CMP     AX, RSEG
        JNE     ULTOP
        MOV     AX, 352Fh
        INT     21h
        CMP     BX, OFFSET I2F
        JNE     ULTOP
        MOV     AX, ES
        CMP     AX, RSEG
        JE      UL2
ULTOP:  MOV     DX, OFFSET MNOTTOP
        JMP     ERREXIT

UL2:    MOV     ES, RSEG
        MOV     AL, ES:DRIVE
        MOV     DRIVE, AL
        MOV     AH, 19h                 ; current drive
        INT     21h
        CMP     AL, DRIVE
        JNE     UL3
        MOV     DX, OFFSET MCURDRV
        JMP     ERREXIT

UL3:    MOV     AH, 52h
        INT     21h
        MOV     LOLOFF, BX
        MOV     LOLSEG, ES
        MOV     AL, ES:[BX+20h]
        DEC     AL
        CMP     AL, DRIVE
        JE      UL4
        MOV     DX, OFFSET MNOTLST
        JMP     ERREXIT

        ; the files are lost: ask, unless the drive is locked anyway
UL4:    MOV     ES, RSEG
        CMP     ES:DAMAGED, 0
        JNE     UL4A
        MOV     AL, DRIVE
        ADD     AL, 'A'
        MOV     MASKRDR, AL
        MOV     DX, OFFSET MASKREM
        CALL    ASKJN
        JNC     UL4A
        MOV     DX, OFFSET MCANCEL
        JMP     ERREXIT

        ; find our device header and DPB before changing anything
UL4A:    LES     BX, DWORD PTR LOLOFF
        ADD     BX, 22h                 ; NUL device
UL5:    CMP     WORD PTR ES:[BX], OFFSET DEVHDR
        JNE     UL6
        MOV     AX, ES:[BX+2]
        CMP     AX, RSEG
        JE      UL7
UL6:    CMP     WORD PTR ES:[BX], 0FFFFh
        JE      ULNF
        LES     BX, ES:[BX]
        JMP     SHORT UL5
UL7:    PUSH    ES                      ; ES:BX = link to our header
        PUSH    BX
        LES     DI, DWORD PTR LOLOFF
        LES     DI, ES:[DI]             ; first DPB
UL8:    CMP     WORD PTR ES:[DI+18h], OFFSET DPB
        JNE     UL9
        MOV     AX, ES:[DI+1Ah]
        CMP     AX, RSEG
        JE      UL10
UL9:    CMP     WORD PTR ES:[DI+18h], 0FFFFh
        JE      ULNF2
        LES     DI, ES:[DI+18h]
        JMP     SHORT UL8
ULNF2:  POP     BX
        POP     ES
ULNF:   MOV     DX, OFFSET MNOTFND
        JMP     ERREXIT

        ; ES:DI = DPB before ours. From here on nothing can fail.
UL10:   MOV     WORD PTR ES:[DI+18h], 0FFFFh
        MOV     WORD PTR ES:[DI+1Ah], 0FFFFh
        POP     BX
        POP     ES
        PUSH    DS
        MOV     DS, RSEG
        MOV     AX, WORD PTR DS:DEVHDR
        MOV     ES:[BX], AX
        MOV     AX, WORD PTR DS:DEVHDR+2
        MOV     ES:[BX+2], AX
        POP     DS

        CALL    DROPBUF

        CALL    GETCDS
        MOV     WORD PTR ES:[DI+43h], 0
        LES     BX, DWORD PTR LOLOFF
        DEC     BYTE PTR ES:[BX+20h]

        PUSH    DS
        MOV     DS, RSEG
        LDS     DX, DS:OLD21
        MOV     AX, 2521h
        INT     21h
        POP     DS
        PUSH    DS
        MOV     DS, RSEG
        LDS     DX, DS:OLD2F
        MOV     AX, 252Fh
        INT     21h
        POP     DS

        MOV     ES, RSEG
        MOV     AH, 49h
        INT     21h

        MOV     DX, OFFSET MREMOK
        MOV     AH, 9
        INT     21h
        MOV     AX, 4C00h
        INT     21h

;---------------------------------------------------------------------
; Set the drive up again (/F)
;---------------------------------------------------------------------

REFORM: MOV     AX, MPXID * 256
        XOR     BX, BX
        INT     2Fh
        CMP     AL, 0FFh
        JNE     RF0
        CMP     BX, SIGN
        JE      RF1
RF0:    MOV     DX, OFFSET MNOTLD
        JMP     ERREXIT
RF1:    MOV     RSEG, ES
        MOV     AL, ES:DRIVE
        MOV     DRIVE, AL
        ADD     AL, 'A'
        MOV     MASKDRV, AL
        MOV     MFMTDRV, AL
        CMP     ES:DAMAGED, 0           ; nothing left to lose?
        JNE     RF3
        MOV     DX, OFFSET MASKFMT
        CALL    ASKJN
        JNC     RF3
        MOV     DX, OFFSET MCANCEL
        JMP     ERREXIT

RF3:    MOV     AH, 52h
        INT     21h
        MOV     LOLOFF, BX
        MOV     LOLSEG, ES
        CALL    DROPBUF
        CALL    FORMAT                  ; checksums go to our own copy
        CMP     GDCERR, 0
        JE      RF4
        MOV     DX, OFFSET MGDCERR
        JMP     ERREXIT
RF4:    MOV     ES, RSEG                ; hand them to the resident copy
        MOV     SI, OFFSET CSUMS
        MOV     DI, SI
        MOV     CX, NSECT
        PUSHF
        CLI
        REP     MOVSW
        MOV     ES:DAMAGED, 0
        MOV     ES:MEDIACHG, 1
        POPF
        MOV     DX, OFFSET MFMTOK
        MOV     AH, 9
        INT     21h
        MOV     AX, 4C00h
        INT     21h

ERREXIT:
        MOV     AH, 9
        INT     21h
        MOV     AX, 4C01h
        INT     21h

;---------------------------------------------------------------------
; Helpers of the transient part
;---------------------------------------------------------------------

; DX = question; waits for J/Y or N and echoes it. CF = 1 for N.
ASKJN   PROC    NEAR
        MOV     AH, 9
        INT     21h
AJ1:    MOV     AH, 8
        INT     21h
        MOV     DL, AL
        AND     AL, 0DFh
        CMP     AL, 'J'
        JE      AJ2
        CMP     AL, 'Y'
        JE      AJ2
        CMP     AL, 'N'
        JNE     AJ1
        STC
AJ2:    PUSHF
        MOV     AH, 2
        INT     21h
        MOV     DX, OFFSET CRLF
        MOV     AH, 9
        INT     21h
        POPF
        RET
ASKJN   ENDP

; write back DOS buffers and forget those of DRIVE (LOLOFF set)
DROPBUF PROC    NEAR
        MOV     AH, 0Dh
        INT     21h
        LES     BX, DWORD PTR LOLOFF
        LES     DI, ES:[BX+12h]
DB1:    MOV     AL, ES:[DI+4]
        CMP     AL, DRIVE
        JNE     DB2
        MOV     BYTE PTR ES:[DI+4], 0FFh
DB2:    CMP     WORD PTR ES:[DI], 0FFFFh
        JE      DB3
        LES     DI, ES:[DI]
        JMP     SHORT DB1
DB3:    RET
DROPBUF ENDP

; ES:DI = CDS entry of DRIVE (entries are 51h bytes in DOS 3.x)
GETCDS  PROC    NEAR
        LES     BX, DWORD PTR LOLOFF
        LES     DI, ES:[BX+16h]
        MOV     AL, DRIVE
        MOV     AH, 51h
        MUL     AH
        ADD     DI, AX
        RET
GETCDS  ENDP

; command line: /U, /?, /X:NAME. CF = 1 and DX = message on error.
PARSE   PROC    NEAR
        MOV     SI, 81h
PA1:    LODSB
        CMP     AL, ' '
        JE      PA1
        CMP     AL, 9
        JE      PA1
        CMP     AL, 13
        JE      PA9
        CMP     AL, '/'
        JNE     PABAD
        LODSB
        AND     AL, 0DFh
        CMP     AL, 'U'
        JNE     PA1F
        MOV     FUNLOAD, 1
        JMP     SHORT PA1
PA1F:   CMP     AL, 'F'
        JNE     PA2
        MOV     FFORMAT, 1
        JMP     SHORT PA1
PA2:    CMP     AL, '?' AND 0DFh
        JNE     PA3
        MOV     FHELP, 1
        JMP     SHORT PA1
PA3:    CMP     AL, 'X'
        JNE     PABAD
        LODSB
        CMP     AL, ':'
        JNE     PABAD
        MOV     DI, NEXTNM
        CMP     DI, OFFSET NAMES + NNAMES * 9
        JB      PA4
        MOV     DX, OFFSET MFULL
        STC
        RET
PA4:    MOV     CX, 8
PA5:    LODSB
        CMP     AL, ' '
        JE      PA7
        CMP     AL, 9
        JE      PA7
        CMP     AL, 13
        JE      PA7
        CMP     AL, '/'
        JE      PA7
        CMP     AL, 'a'
        JB      PA6
        CMP     AL, 'z'
        JA      PA6
        SUB     AL, 20h
PA6:    JCXZ    PA5                     ; longer names are cut at 8
        MOV     [DI], AL
        INC     DI
        DEC     CX
        JMP     SHORT PA5
PA7:    DEC     SI                      ; look at the delimiter again
        CMP     CX, 8
        JE      PABAD                   ; /X: without a name
        ADD     NEXTNM, 9
        JMP     SHORT PA1
PA9:    CLC
        RET
PABAD:  MOV     DX, OFFSET MBADARG
        STC
        RET
PARSE   ENDP

; read one word, AX = address -> AX = data
RDWORD  PROC    NEAR
        CALL    CSRW
        MOV     AX, 1
        CALL    FIGSR
        MOV     AL, C_RDAT
        CALL    PUTCMD
        CALL    GETB
        MOV     DL, AL
        CALL    GETB
        MOV     DH, AL
        CALL    DRAIN
        MOV     AX, DX
        RET
RDWORD  ENDP

; write one word, AX = address, DX = data
WRWORD  PROC    NEAR
        CALL    CSRW
        XOR     AX, AX
        CALL    FIGSR
        MOV     AL, C_WDAT
        CALL    PUTCMD
        MOV     AL, DL
        CALL    PUTPAR
        MOV     AL, DH
        CALL    PUTPAR
        CALL    WAITIDLE
        RET
WRWORD  ENDP

; Is the colour board fitted? AL = 0 yes, 1 no (GDCERR on timeout).
; A monochrome board mirrors its plane at 4000h and 8000h, so a write
; there changes word 0; the original word 0 is put back.
DETECT  PROC    NEAR
        MOV     GDCERR, 0
        PUSHF
        CLI
        CALL    SAVECUR
        CALL    SETMASK
        XOR     AX, AX
        CALL    RDWORD
        MOV     BX, AX                  ; BX = original word 0
        MOV     DX, AX
        XOR     DX, 5AA5h
        MOV     AX, 4000h
        CALL    WRWORD
        MOV     DX, BX
        XOR     DX, 0C33Ch
        MOV     AX, 8000h
        CALL    WRWORD
        XOR     AX, AX
        CALL    RDWORD
        CMP     AX, BX
        JNE     DT8                     ; mirrored: monochrome board
        MOV     AX, 4000h
        CALL    RDWORD
        XOR     AX, BX
        CMP     AX, 5AA5h
        JNE     DT9
        MOV     AX, 8000h
        CALL    RDWORD
        XOR     AX, BX
        CMP     AX, 0C33Ch
        JNE     DT9
        CALL    RESTCUR
        POPF
        XOR     AL, AL
        RET
DT8:    XOR     AX, AX                  ; put word 0 back
        MOV     DX, BX
        CALL    WRWORD
DT9:    CALL    RESTCUR
        POPF
        MOV     AL, 1
        RET
DETECT  ENDP

; clear red and blue and write boot sector, FAT and root directory
FORMAT  PROC    NEAR
        MOV     GDCERR, 0
        PUSHF
        CLI
        CALL    SAVECUR
        CALL    SETMASK
        MOV     AX, 4000h               ; red plane
        CALL    FILL0
        MOV     AX, 8000h               ; blue plane
        CALL    FILL0
        CALL    RESTCUR
        POPF
        ; all checksums are 0 now, which is the checksum of zeros

        ; boot sector
        CALL    CLRBUF
        MOV     DI, OFFSET SECBUF
        MOV     WORD PTR [DI], 0FEEBh   ; JMP $ (never booted)
        MOV     BYTE PTR [DI+2], 90h
        MOV     SI, OFFSET OEMNAME
        ADD     DI, 3
        MOV     CX, 8
        REP     MOVSB
        MOV     SI, OFFSET BPB
        MOV     CX, BPBLEN
        REP     MOVSB
        MOV     WORD PTR SECBUF+510, 0AA55h
        XOR     AX, AX
        CALL    PUTSEC

        ; FAT
        CALL    CLRBUF
        MOV     WORD PTR SECBUF, 0FFF8h
        MOV     BYTE PTR SECBUF+2, 0FFh
        MOV     AX, 1
        CALL    PUTSEC

        ; root directory with volume label
        CALL    CLRBUF
        MOV     SI, OFFSET OEMNAME
        MOV     DI, OFFSET SECBUF
        MOV     CX, 11
        REP     MOVSB
        MOV     BYTE PTR SECBUF+11, 08h
        CALL    DOSTIME                 ; date and time of the label
        MOV     WORD PTR SECBUF+22, AX
        MOV     WORD PTR SECBUF+24, DX
        MOV     AX, 2
        CALL    PUTSEC
        RET
FORMAT  ENDP

; AX = DOS time, DX = DOS date of now (directory entry format)
DOSTIME PROC    NEAR
        MOV     AH, 2Ch                 ; CH hour, CL minute, DH second
        INT     21h
        MOV     BL, DH                  ; BX = second / 2
        XOR     BH, BH
        SHR     BX, 1
        MOV     AL, CL                  ; minute << 5
        XOR     AH, AH
        MOV     CL, 5
        SHL     AX, CL
        OR      BX, AX
        MOV     AH, CH                  ; hour << 11
        XOR     AL, AL
        MOV     CL, 3
        SHL     AH, CL
        OR      BX, AX
        PUSH    BX
        MOV     AH, 2Ah                 ; CX year, DH month, DL day
        INT     21h
        SUB     CX, 1980
        JNC     DTI1
        XOR     CX, CX
DTI1:   MOV     AX, CX                  ; (year - 1980) << 9
        MOV     CL, 9
        SHL     AX, CL
        MOV     BL, DH                  ; month << 5
        XOR     BH, BH
        MOV     CL, 5
        SHL     BX, CL
        OR      AX, BX
        XOR     DH, DH                  ; day
        OR      AX, DX
        MOV     DX, AX
        POP     AX
        RET
DOSTIME ENDP

; AX = plane start; writes 4000h zero words
FILL0   PROC    NEAR
        CALL    CSRW
        MOV     AX, 3FFFh               ; DC + 1 = 16384 words
        CALL    FIGSR
        MOV     AL, C_WDAT
        CALL    PUTCMD
        XOR     AL, AL
        CALL    PUTPAR
        CALL    PUTPAR
        CALL    WAITIDLE
        RET
FILL0   ENDP

CLRBUF  PROC    NEAR
        PUSH    CS
        POP     ES
        MOV     DI, OFFSET SECBUF
        MOV     CX, 256
        XOR     AX, AX
        REP     STOSW
        RET
CLRBUF  ENDP

; AX = sector, writes SECBUF
PUTSEC  PROC    NEAR
        PUSH    CS
        POP     ES
        MOV     DI, OFFSET SECBUF
        CALL    WRSEC
        RET
PUTSEC  ENDP

OEMNAME DB      'ECODRIVE   '

SECBUF  DB      512 DUP (?)

CODE    ENDS
        END     START

cpu 8086                                ; NIE 386-Befehle erzeugen (8088/V20!)
;=====================================================================
; DIAG-BOOT - Bootsektor der selbststartenden Diagnosedisketten
;             (DMVDIAG, spaeter HWCONFIRM) fuer die NCR Decision Mate V
;
; Bootvertrag des Z80-ROMs (siehe docs/DMV-Hardware-Boot-und-Bildschirm):
;   ein Sektor nach 0000:2000, Kennung "16BIT" ab Offset 3 (7 Bytes),
;   Einsprung JMP FAR 0200:0000, CS=0200, DS=ES=SS=0, SP=0.
;
; Aufbau der Diskette: ein gewoehnliches 360-KB-Format (40 Zylinder,
; 2 Koepfe, 9 Sektoren zu 512 Byte), das Programm liegt in den
; RESERVIERTEN Sektoren 1..PROGSECS direkt hinter dem Bootsektor.
; DOS sieht eine leere, gueltige Diskette; kopiert wird sie als Image.
;
; Ablauf: die reservierten Sektoren der Reihe nach nach 0000:4000
; lesen (DMA Kanal 3, Ziel bleibt unter 64 KB), dann JMP 0000:4000
; mit DS=ES=SS=0 und SP=3FFEh. Fehler: Meldung ueber die Firmware,
; Taste, zurueck zum Z80.
;
; Die Leseroutine stammt aus unserem MS-DOS-3.3-Bootsektor
; (os/msdos33/src/SRC/BOOT/msboot_dmv.nasm), der auf beiden echten
; Maschinen startet - samt der Spurverdoppelung fuer 96-tpi-Laufwerke.
;=====================================================================

%include "dmvhw.nas"

%ifndef PROGSECS
%define PROGSECS 1
%endif

PROGADR equ 0x4000                      ; Ladeadresse des Programms (phys.)
VARS    equ 0x0300                      ; Variablen hinter dem Sektor (DS=0200)
count   equ VARS
rsec    equ VARS+2
rhead   equ VARS+3
rcyl    equ VARS+4
result  equ VARS+6

DMAADR  equ 0x26
DMACNT  equ 0x27
DMAMSK  equ 0x2A
DMAMOD  equ 0x2B

        org     0
        jmp     short begin
        nop
        db      '16BIT   '              ; 3..10, 7 Bytes werden geprueft
bps     dw      512                     ; 0Bh
spc     db      2
res     dw      1+PROGSECS              ; Bootsektor + Programm
nfats   db      2
rootent dw      112
totsec  dw      720
media   db      0xFD
spf     dw      2
spt     dw      9
heads   dw      2
hidden  dd      0
bigsec  dd      0
        db      6 dup 0                 ; Reserve bis 2Ah wie im DOS-Sektor
begin:
        cli
        cld
        push    cs
        pop     ds                      ; DS = 0200 fuer die Variablen
        mov     word [count],PROGSECS
        mov     ax,1                    ; erster Programmsektor
        mov     bx,PROGADR
        xor     si,si                   ; Spurverdoppelung zunaechst aus
loadlp:
        call    rdsec
        inc     ax
        add     bx,512
        dec     word [count]
        jnz     loadlp

        xor     ax,ax
        mov     ds,ax
        mov     es,ax
        mov     ss,ax
        mov     sp,PROGADR-2
        jmp     0x0000:PROGADR

;---------------------------------------------------------------------
; rdsec: AX = logischer Sektor, BX = physische Adresse (< 10000h)
; SI = Merker Spurverdoppelung (siehe msboot_dmv.nasm, Kasten bei seekc)
;---------------------------------------------------------------------
rdsec:
        mov     bp,6                    ; Versuche
rs_again:
        push    ax
        push    bx
        push    cx
        push    dx
        xor     dx,dx
        div     word [spt]
        inc     dl
        mov     [rsec],dl
        xor     dx,dx
        div     word [heads]
        mov     [rhead],dl
        mov     [rcyl],al

        mov     al,0x0F                 ; SEEK
        call    fdcout
        call    hdsbyte
        mov     al,[rcyl]
        or      si,si                   ; 48-tpi-Diskette im 96-tpi-Werk?
        jz      seekc
        add     al,al                   ; dann physisch doppelt so weit
seekc:
        call    fdcout
        call    wsense

        mov     al,0x07                 ; DMA Kanal 3 maskieren
        out     DMAMSK,al
        mov     al,0x47                 ; einzeln, schreiben, Kanal 3
        out     DMAMOD,al
        mov     ax,bx
        out     DMAADR,al
        mov     al,ah
        out     DMAADR,al
        mov     al,0xFF                 ; 512-1
        out     DMACNT,al
        mov     al,0x01
        out     DMACNT,al
        mov     al,0x03                 ; freigeben
        out     DMAMSK,al

        mov     al,0xC6                 ; READ DATA, MT + MFM
        call    fdcout
        call    hdsbyte
        mov     al,[rcyl]
        call    fdcout
        mov     al,[rhead]
        call    fdcout
        mov     al,[rsec]
        call    fdcout
        mov     al,2                    ; N = 512
        call    fdcout
        mov     al,9                    ; EOT
        call    fdcout
        mov     al,0x1B                 ; GPL
        call    fdcout
        mov     al,0xFF                 ; DTL
        call    fdcout
wex:
        in      al,P_SYSSTAT            ; Bit 2 = Laufwerk bereit/Transfer
        test    al,SS_READY
        jz      rd_err
        in      al,P_FDCSTAT            ; RQM = Ergebnisphase bereit
        test    al,FDC_RQM
        jz      wex
        out     P_TC,al                 ; Transferende an den Controller
        mov     al,0x07
        out     DMAMSK,al

        mov     cx,7                    ; Ergebnisphase
        mov     di,result
rd_res:
        call    fdcin
        mov     [di],al
        inc     di
        loop    rd_res
        test    byte [result],0xC0      ; ST0
        jnz     rd_err
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

rd_err:
        xor     si,1                    ; Spurverdoppelung umschalten
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        dec     bp
        jnz     rs_again
        jmp     short err

hdsbyte:                                ; Kopf- und Laufwerksbyte (A:)
        mov     al,[rhead]
        shl     al,1
        shl     al,1
        jmp     short fdcout

fdcout:
        push    cx
        xor     cx,cx
fo_w:
        push    ax
        in      al,P_FDCSTAT
        test    al,FDC_RQM
        pop     ax
        jnz     fo_go
        loop    fo_w
fo_go:
        out     P_FDCDATA,al
        pop     cx
        ret

fdcin:
        push    cx
        xor     cx,cx
fi_w:
        in      al,P_FDCSTAT
        test    al,FDC_RQM
        jnz     fi_go
        loop    fi_w
fi_go:
        in      al,P_FDCDATA
        pop     cx
        ret

wsense:                                 ; Ende der Kopfbewegung abwarten
        in      al,P_SYSSTAT
        test    al,SS_FDCINT
        jz      wsense
        mov     al,0x08                 ; SENSE INTERRUPT STATUS
        call    fdcout
        call    fdcin
        call    fdcin
        ret

err:
        xor     ax,ax
        mov     ds,ax
        mov     byte [0xF90B],0xE8      ; INVFLG: gruen auf schwarz
        mov     bx,0x2000+msg           ; Zaehlstring absolut
        call    0xFF00:0x0000           ; DISPLAY der Firmware
wkey:
        in      al,P_KBDCMD
        test    al,KBD_RDY
        jz      wkey
        in      al,P_KBDDATA
        out     P_ROMSEL,al             ; ROM einblenden
        out     0xD0,al                 ; zurueck zum Z80

msg     db      msge-msg-1,0x88,'DMVDIAG: Lesefehler beim Laden',0x88
msge    equ     $

        times   510-($-$$) db 0
        dw      0xAA55

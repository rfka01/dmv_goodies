cpu 8086                                ; NIE 386-Befehle erzeugen (8088/V20!)
;=====================================================================
; DMVDIAG - selbststartende Hardware-Erkennung fuer die NCR Decision
;           Mate V. Laeuft ohne DOS direkt nach dem Z80-ROM.
;
; Grundsaetze (docs/DMV-Hardwaretest.md):
;   1. Im Zweifel nichts behaupten - "unbekannt" ist ein Ergebnis.
;   2. Erst die Rohwerte, darunter die Deutung.
;   3. Eine Seite je Schirm, fotolesbar.
;   4. Ohne DOS, ohne eigene Treiber; Ausgabe nur ueber die Firmware
;      der 8088-Karte (FF00:0000, DISPLAY).
;
; Speicher (alles in Bank 0, unter C000h wegen K233):
;   0000:2000  Bootsektor
;   0000:3FFE  Stapel (waechst nach unten)
;   0000:4000  dieses Programm (org 4000h, CS=DS=ES=SS=0)
;   0000:F800  Variablen der Firmware, 0000:FE00 Postfach
;
; Steuerzeichen der Firmware-Ausgabe: nur 88h (neue Zeile) wirkt auf
; BEIDEN ROMs. 00h/01h/0Ch/8Bh kennt nur 33473 (K235); 32167 (K230/
; K231) gibt 00h-0Fh als Zeichen aus. Darum nur 88h benutzen und den
; Cursor ueber 0:F800/F801 setzen.
;=====================================================================

%include "dmvhw.nas"

%ifndef VERSION
%error VERSION fehlt - bauen mit mkimg.py (liest diag/VERSION)
%endif

DMA_MASK    equ 0x2A                    ; 8237: Einzelmaske (wie Bootsektor)

%ifndef PROGSECS
%error "PROGSECS fehlt - mit diag/mkimg.py bauen"
%endif
%ifndef FATDATE
%error "FATDATE fehlt - mit diag/mkimg.py bauen"
%endif

        org     0x4000

start:
        cli
        cld
        xor     ax,ax
        mov     ds,ax
        mov     es,ax
        mov     ss,ax
        mov     sp,0x3FFE
        mov     byte [FW_INVFLG],FW_ATTRNORM
        mov     al,0x07                 ; DMA Kanal 3 sperren (Lader fertig)
        out     DMA_MASK,al

        call    pit_init
        call    m_rom                   ; ROM-Pruefsummen fuer cls
        call    cls
        mov     si,s_hello
        call    puts

        call    measure_all
%ifdef FRAGE
        jmp     fr_main                 ; DMVFRAGE: ein Lauf, eine Seite (frage.inc)
%endif
%ifdef INFO
        jmp     inf_main                ; DMVINFO: Uebersicht (info.inc)
%endif

        mov     byte [page],0
pg_show:
        call    tk_start                ; Aufbauzeit messen
        call    cls
        call    cap_begin               ; Seite mitschreiben (results.inc)
        call    header
        mov     bl,[page]
        xor     bh,bh
        shl     bx,1
        call    [pagetab+bx]
        call    cap_end
        call    footer
pg_key:
        call    getkey
        or      al,0x20                 ; Kleinbuchstaben
        cmp     al,'z'
        je      pg_prev
        cmp     al,'w'
        je      pg_remeasure
        cmp     al,'x'
        je      pg_exit
        cmp     al,'d'                  ; Sichern wieder an, sofort schreiben
        jne     pg_kt
%ifdef INFO
        jmp     pg_show                 ; DMVINFO: D nur auf der Uebersicht
%else
        call    rs_force
        jmp     pg_show
%endif
pg_kt:  mov     si,testtab              ; Sondertests: Taste, Seite, Datei, Routine
pg_tl:  cmp     byte [si],0
        je      pg_next
        cmp     al,[si]
        jne     pg_tn
        mov     ah,[page]
        cmp     ah,[si+1]
        jne     pg_next                 ; Testtaste auf anderer Seite: weiter
        push    si
        call    rs_flush                ; Ergebnis des vorigen Tests sichern
        pop     si
        mov     al,[si+2]
        push    ax
        call    [si+3]
        pop     ax
        mov     [rs_test],al            ; nach dem Neuaufbau unter diesem Namen
        jmp     pg_show
pg_tn:  add     si,5
        jmp     short pg_tl
pg_next:
        mov     al,[page]               ; jede andere Taste: weiter
        inc     al
        cmp     al,NPAGES
        jb      pg_set
        xor     al,al
pg_set: push    ax
        call    rs_leave                ; Seite (und Test) sichern
        pop     ax
        mov     [page],al
        jmp     pg_show
pg_remeasure:
        call    rs_flush
        call    cls
        mov     si,s_hello
        call    puts
        mov     al,[page]
        push    ax
        call    measure_all
        pop     ax
        mov     [page],al
        jmp     pg_show
pg_prev:
        mov     al,[page]
        or      al,al
        jnz     pg_p1
        mov     al,NPAGES
pg_p1:  dec     al
        jmp     pg_set

pg_exit:
%ifdef INFO
        jmp     inf_show                ; DMVINFO: X fuehrt zur Uebersicht zurueck
%endif
        call    rs_leave
; Neustart: Der Z80 springt ueber seinen Warteplatz F111h in den
; Kaltstart seines ROMs. Der 8088 muss dabei in der Befehlsschleife
; seines eigenen ROMs stehen (K230 32167 und K235 33473: FF00:0018h
; OUT 11h, OUT D0h, dann Befehl aus FE04h), denn der Speichertest des
; Z80 ueberschreibt das RAM, auch Code, den der 8088 nach dem OUT D0h
; noch holen wuerde. Die Schleife gibt den Bus selbst an den Z80.
reboot:
        call    cls
        mov     si,s_bye
        call    puts
        cli
        mov     ax,K8_ROMSEG
        mov     es,ax
        cmp     word [es:K8_LOOP],0x11E6        ; OUT 11h
        jne     rb_old
        cmp     word [es:K8_LOOP+2],0xD0E6      ; OUT D0h
        jne     rb_old
        xor     ax,ax
        mov     es,ax
        mov     si,z80_boot             ; Warteplatz: DI, ROM ein, JP 0000h
        mov     di,Z80_RESUME
        mov     cx,Z80BOOTLEN
        rep     movsb
        mov     ss,ax                   ; wie nach FF00:0003h des ROMs
        mov     sp,ax
        jmp     K8_ROMSEG:K8_LOOP
rb_old: xor     ax,ax                   ; unbekanntes ROM: wie bisher zum Z80
        mov     es,ax
        out     P_ROMSEL,al
        out     P_SWITCH16,al
        jmp     $
z80_boot:
        db      0xF3                    ; DI
        db      0xD3,P_ROMSEL           ; OUT (11h),A   ROM ueber 0000h
        db      0xC3,0x00,0x00          ; JP 0000h      Kaltstart des Z80-ROMs
Z80BOOTLEN equ  $-z80_boot

measure_all:
        call    kbd_init                ; Landescode, zugleich Init
        call    m_rom                   ; ROM-Kennung, Pruefsummen
        call    m_cpu                   ; V20/8088, 8087, 8259
        call    m_clock                 ; Takt
        call    m_banks                 ; Speicherbaenke
        call    m_gdc                   ; Grafikkarte: Ebenenprobe
        call    judge                   ; Deutung der Grundwerte
        ret

pagetab dw      pg_system, pg_cpu, pg_mem, pg_ifsel, pg_ports, pg_irq, pg_disk, pg_drv7, pg_copro, pg_sum
PG_MEM  equ     2
PG_PORTS equ    4
PG_IRQ  equ     5
PG_DRIVES equ   7
PG_COPRO equ    8
NPAGES  equ     ($-pagetab)/2

testtab db      'k',PG_MEM,RF_K233
        dw      k233_run
        db      's',PG_PORTS,RF_PORT
        dw      ports_run
        db      'i',PG_IRQ,RF_IRQ
        dw      irq_run
        db      'm',PG_DRIVES,RF_MOT
        dw      mt_run
        db      't',PG_DRIVES,RF_STEP
        dw      drv_tprobe
        db      'b',PG_DRIVES,RF_BRD
        dw      drv_bprobe
        db      'p',PG_COPRO,RF_COP
        dw      copro_run
        db      0

;=====================================================================
;  AUSGABE
;=====================================================================

; Schirm loeschen. 0Ch versteht nur das ROM der K235 (33473); das ROM
; der K230/K231 (32167) gibt 00h-0Fh als Zeichen aus. Beide ROMs haben
; aber dieselbe Loeschroutine (CLSCR: Cursor 0, ein WDAT ueber 16 K
; Worte, Rollzeiger und Bildbereiche zuruecksetzen), nur an anderer
; Stelle und mit anderem Attribut (33473: E8h, 32167: 00h - auf einer
; Farbkarte waere das weisser Grund). Ablauf:
;   1. ROM-Routine aufrufen (nur bei bekannter ROM-Pruefsumme). Sie
;      endet mit einem nahen RET; wir legen davor die Adresse eines
;      RETF im selben ROM auf den Stapel, das RETF bringt uns zurueck.
;   2. Die 2000 Textworte selbst mit Leerzeichen und unserem
;      Normalattribut fuellen - ein einziges WDAT mit DC = 1999.
; Unbekanntes ROM: Leerzeichen ueber DISPLAY wie bisher (langsam).
; Vorher (0.02-0.04) liefen IMMER 2000 Einzelzeichen ueber DISPLAY.
cls:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        mov     al,A_VAL
        call    setattr
        mov     si,crc_33473
        call    crc88_is
        mov     bx,0x03FE               ; CLSCR in 33473
        mov     dx,0x0521               ; RETF in 33473 (Ende von DISPLAY)
        je      cl_rom
        mov     si,crc_32167
        call    crc88_is
        mov     bx,0x032D               ; CLSCR in 32167
        mov     dx,0x0458               ; RETF in 32167
        je      cl_rom
        mov     word [FW_CURX],0        ; unbekanntes ROM: langsam
        mov     cx,25
cl_l:   push    cx
        mov     bx,blankrow
        call    FW_DISPSEG:0x0000
        pop     cx
        loop    cl_l
        jmp     short cl_e
cl_rom: mov     [cl_far],bx
        mov     word [cl_far+2],FW_DISPSEG
        push    cs                      ; Rueckkehr fuer das RETF
        mov     ax,cl_back
        push    ax
        push    dx                      ; Ruecksprung fuer das nahe RET
        jmp     far [cl_far]
cl_back:
        xor     ax,ax                   ; das ROM hat Register benutzt
        mov     ds,ax
        mov     es,ax
        call    tk_update
        xor     bx,bx                   ; 2000 Textworte nachfuellen
        call    g_curs
        mov     ax,1999
        call    g_figs
        call    g_free
        mov     al,0x20                 ; WDAT, Wort, ersetzen
        out     P_GDCCMD,al
        mov     al,' '
        out     P_GDCPARAM,al
        mov     al,[FW_INVFLG]
        out     P_GDCPARAM,al
        call    g_free
        call    g_figs0
cl_e:   mov     word [FW_CURX],0
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

cl_far  dw      0,0
blankrow db     80
        times 80 db ' '

; Attribute je Rolle. In Texten stehen die Rollen als Steuerbytes
; 01h-05h (puts gibt sie nicht aus, sondern setzt das Attribut).
; Mono kennt kein "intensiv" (docs, Attributbyte): Kopf invers, Werte
; normal, Erlaeuterungen halbhell. Farbe: Kopf weiss auf blau, Titel
; gelb, Werte gruen, Deutung weiss, Erlaeuterungen tuerkis.
A_VAL   equ     1                       ; Rohwerte, Normalschrift
A_NOTE  equ     2                       ; Beschriftung und Erlaeuterung
A_JUDGE equ     3                       ; Deutung
A_TITLE equ     4                       ; Seitentitel
A_HEAD  equ     5                       ; Kopf- und Fusszeile
A_MOD   equ     6                       ; Modulnummer (Farbe: rot, Mono: normal)
attr_mono  db   0, 0xE8, 0xEC, 0xE8, 0xE9, 0xE9, 0xE8
attr_color db   0, 0xE8, 0xF8, 0xFC, 0xEC, 0x7C, 0xE4
attr_tab dw     attr_mono

setattr:                                ; AL = Rolle 1-5
        push    bx
        mov     bx,[attr_tab]
        xlatb
        mov     [FW_INVFLG],al
        pop     bx
        ret

putc:                                   ; AL = Zeichen
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    bp
        cmp     byte [cap_on],0
        je      pc_d
        call    cap_put
pc_d:   mov     [chbuf+1],al
        mov     byte [chbuf],1
        mov     bx,chbuf
        call    FW_DISPSEG:0x0000
        call    tk_update               ; Uhr fortschreiben (Umlauf 131 ms)
        pop     bp
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

puts:                                   ; SI = Text, 0-terminiert
        push    ax
        push    si
ps_l:   lodsb
        or      al,al
        jz      ps_e
        cmp     al,A_MOD
        ja      ps_c
        push    ax
        call    setattr
        pop     ax
        jmp     short ps_l
ps_c:   call    putc
        jmp     short ps_l
ps_e:   pop     si
        pop     ax
        ret

nl:     push    ax
        mov     al,0x88
        call    putc
        mov     al,A_VAL
        call    setattr
        pop     ax
        ret

space:  push    ax
        mov     al,' '
        call    putc
        pop     ax
        ret

spaces:                                 ; CX Leerzeichen
        jcxz    sp_e
sp_l:   call    space
        loop    sp_l
sp_e:   ret

gotox:                                  ; DL = Spalte (Zeile bleibt)
        mov     [FW_CURX],dl
        ret

hex4:   push    ax
        and     al,0x0F
        add     al,'0'
        cmp     al,'9'
        jbe     h4_p
        add     al,7
h4_p:   call    putc
        pop     ax
        ret

hex8:   push    ax                      ; AL
        push    cx
        mov     cl,4
        ror     al,cl
        call    hex4
        rol     al,cl
        call    hex4
        pop     cx
        pop     ax
        ret

hex16:  xchg    al,ah                   ; AX
        call    hex8
        xchg    al,ah
        call    hex8
        ret

dec16:                                  ; AX vorzeichenlos
        push    ax
        push    dx
        xor     dx,dx
        call    dec32
        pop     dx
        pop     ax
        ret

dec32:                                  ; DX:AX vorzeichenlos
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        xor     cx,cx
d32_l:  mov     bx,10                   ; DX:AX / 10, Rest -> Stapel
        mov     si,ax
        mov     ax,dx
        xor     dx,dx
        div     bx                      ; hohes Wort
        xchg    ax,si                   ; SI = Quotient hoch
        div     bx                      ; niedriges Wort, Rest in DX
        push    dx
        inc     cx
        mov     dx,si
        or      si,ax
        jnz     d32_l
d32_p:  pop     ax
        add     al,'0'
        call    putc
        loop    d32_p
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

dec2c:                                  ; AX = Hundertstel -> "n,nn"
        push    ax
        push    bx
        push    dx
        xor     dx,dx
        mov     bx,100
        div     bx
        call    dec16
        mov     al,','
        call    putc
        mov     ax,dx
        mov     bl,10
        div     bl                      ; AL Zehntel, AH Hundertstel
        add     ax,0x3030
        call    putc
        mov     al,ah
        call    putc
        pop     dx
        pop     bx
        pop     ax
        ret

bin8:                                   ; AL als 8 Bit
        push    ax
        push    cx
        mov     ah,al
        mov     cx,8
b8_l:   mov     al,'0'
        shl     ah,1
        adc     al,0
        call    putc
        loop    b8_l
        pop     cx
        pop     ax
        ret

hexdump16:                              ; SI = Adresse, 16 Bytes
        push    ax
        push    cx
        push    si
        mov     cx,16
hd_l:   lodsb
        call    hex8
        call    space
        loop    hd_l
        pop     si
        pop     cx
        pop     ax
        ret

;--- Kopf und Fuss ---------------------------------------------------
header:
        mov     al,A_HEAD
        call    setattr
        mov     si,s_head
        call    puts
%ifdef INFO
        cmp     byte [inf_raw],1        ; Seitenzahl nur auf den Rohwertseiten
        jne     hd_np
        mov     si,s_headp
        call    puts
%endif
%ifndef FRAGE
        mov     al,[page]
        inc     al
        cbw
        call    dec16
        mov     al,'/'
        call    putc
        mov     ax,NPAGES
        call    dec16
        call    space
        call    space
%endif
%ifdef INFO
hd_np:
%endif
        ; Kennzeile der Maschine: Firmware, CPU, 8087
        mov     si,fwtext
        call    puts
        call    space
        mov     si,[j_gfx]
        call    puts
        call    space
        mov     si,[j_cpu]
        call    puts
        call    space
        mov     si,[j_fpu]
        call    puts
        call    space
        mov     si,[j_card]
        call    puts
        call    pad80
        call    nl
        call    nl
        ret

pad80:                                  ; Zeile bis Spalte 80 mit Leerzeichen
pa_l:   cmp     byte [FW_CURX],80
        jae     pa_e
        call    space
        jmp     short pa_l
pa_e:   ret

footer:
        call    tk_since                ; Aufbauzeit der Seite
        mov     cx,5000                 ; Schritte zu 2 us -> 1/100 s
        cmp     dx,cx
        jb      ft_ok
        mov     ax,0xFFFF
        jmp     short ft_t
ft_ok:  div     cx
ft_t:   mov     [pg_time],ax
        mov     byte [FW_CURX],0
        mov     byte [FW_CURY],24
        mov     al,A_HEAD
        call    setattr
        mov     si,s_foot
        call    puts
%ifndef INFO
        mov     si,s_foot_a
        cmp     byte [rs_off],0
        je      ft_s
        mov     si,s_foot_d
ft_s:   call    puts
%endif
        mov     si,s_foot1
        call    puts
        mov     ax,[pg_time]
        call    dec2c
        mov     si,s_foot2
        call    puts
        call    pad80
        mov     al,A_VAL
        call    setattr
        ret

;=====================================================================
;  TASTATUR (8741)
;=====================================================================

kbd_init:
        mov     al,KBD_COUNTRY          ; zugleich Initialisierung
        out     P_KBDCMD,al
        mov     word [r_kbst],0xFFFF    ; FF = keine Antwort
        call    tk_start
ki_w:   in      al,P_KBDCMD
        test    al,KBD_RDY
        jnz     ki_got
ki_t:   mov     ax,250                  ; hoechstens 0,5 s
        call    tk_over_ms2             ; CF = Zeit um
        jnc     ki_w
        ret
ki_got: mov     ah,al
        in      al,P_KBDDATA
        test    ah,KBD_LANG             ; vorher gedrueckte Tasten verwerfen
        jz      ki_t
        mov     [r_kbst],ah
        mov     [r_kbst+1],al
        call    tk_since                ; Antwortzeit in 2-us-Schritten
        shl     ax,1
        rcl     dx,1
        mov     [r_kbt],ax              ; in us
        mov     [r_kbt+2],dx
        ret

; Statusbit 7 (Landescode) bleibt nach der Antwort auf 01h STEHEN - in
; MAME mit Status 89h bei jeder folgenden Taste gesehen (DMVDIAG 0.06).
; Es kennzeichnet also nur die erste Antwort; kbd_init hat sie schon
; abgeholt. Hier zaehlt allein Bit 0, wie im Konsolentreiber des
; DOS-Ports (dmvcon.inc).
getkey:                                 ; AL = Taste
gk_w:   in      al,P_KBDCMD
        test    al,KBD_RDY
        jz      gk_w
        in      al,P_KBDDATA
        ret

;=====================================================================
;  ZEIT: 8253 Kanal 2, Modus 2, voller Umfang, 500 kHz = 2 us je Schritt
;  Kanal 0 und 1 werden NICHT angefasst (Bildtakt, Motor-Auszeit).
;=====================================================================

pit_init:
        mov     al,0xB4                 ; Kanal 2, LSB+MSB, Modus 2, binaer
        out     P_PITCTL,al
        xor     al,al
        out     P_PIT2,al
        out     P_PIT2,al
        call    pit_read
        mov     [tk_last],ax
        ret

pit_read:                               ; AX = Zaehlerstand (faellt)
        mov     al,0x80                 ; Kanal 2 einfrieren
        out     P_PITCTL,al
        in      al,P_PIT2
        mov     ah,al
        in      al,P_PIT2
        xchg    al,ah
        ret

tk_update:                              ; Uhr fortschreiben -> DX:AX Schritte
        push    bx
        call    pit_read
        mov     bx,[tk_last]
        mov     [tk_last],ax
        sub     bx,ax                   ; alt - neu (mod 65536)
        add     [tk_lo],bx
        adc     word [tk_hi],0
        mov     ax,[tk_lo]
        mov     dx,[tk_hi]
        pop     bx
        ret

tk_start:                               ; Startpunkt merken
        push    ax
        push    dx
        call    tk_update
        mov     [tk_s0],ax
        mov     [tk_s1],dx
        pop     dx
        pop     ax
        ret

tk_since:                               ; DX:AX = Schritte seit tk_start
        call    tk_update
        sub     ax,[tk_s0]
        sbb     dx,[tk_s1]
        ret

tk_over_ms2:                            ; AX = Grenze in 2-ms-Einheiten
        push    ax                      ; CF = 1, wenn ueberschritten
        push    bx
        push    cx
        push    dx
        mov     bx,1000                 ; 2 ms = 1000 Schritte
        mul     bx
        mov     bx,ax
        push    dx
        call    tk_since
        pop     cx
        cmp     dx,cx
        jb      to_no
        ja      to_yes
        cmp     ax,bx
        jb      to_no
to_yes: stc
        jmp     short to_e
to_no:  clc
to_e:   pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

wait_ms2:                               ; AX = Wartezeit in 2-ms-Einheiten
        push    cx
        call    tk_start
wm_l:   call    tk_over_ms2
        jnc     wm_l
        pop     cx
        ret

;=====================================================================
;  MESSUNGEN
;=====================================================================

;--- Hauptplatinen-ROM und ROM der 8088-Karte --------------------------
m_rom:
        ; Hauptplatinen-ROM: nur sichtbar, solange OUT 11h es ueber
        ; 0000h-1FFFh legt. Dieser Code liegt ueber 2000h (Falle!).
        cli
        out     P_ROMSEL,al
        mov     si,0x0FF7               ; Laengenbyte + 8 Zeichen
        mov     di,r_fwraw
        mov     cx,9
        rep     movsb
        xor     si,si
        mov     cx,0x2000
        call    crc32
        out     P_RAMSEL,al
        mov     [r_crcmb],ax
        mov     [r_crcmb+2],dx
        ; ROM der 8088-Karte (4 KB, gespiegelt; FF00:0000 = Anfang)
        push    ds
        mov     ax,0xFF00
        mov     ds,ax
        xor     si,si
        mov     cx,0x1000
        call    crc32
        pop     ds
        mov     [r_crc88],ax
        mov     [r_crc88+2],dx
        ; Firmwaretext fuer die Kopfzeile (ohne fuehrende Leerzeichen)
        mov     si,r_fwraw+1
        mov     di,fwtext
        mov     cx,8
mr_sk:  cmp     byte [si],' '
        jne     mr_cp
        inc     si
        loop    mr_sk
mr_cp:  jcxz    mr_z
        rep     movsb
mr_z:   mov     byte [di],0
        ret

crc32:                                  ; DS:SI, CX Bytes -> DX:AX (CRC-32)
        push    bx
        push    bp
        mov     ax,0xFFFF
        mov     dx,ax
c32_b:  mov     bl,[si]
        inc     si
        xor     al,bl
        mov     bp,8
c32_k:  shr     dx,1
        rcr     ax,1
        jnc     c32_n
        xor     dx,0xEDB8
        xor     ax,0x8320
c32_n:  dec     bp
        jnz     c32_k
        loop    c32_b
        not     ax
        not     dx
        pop     bp
        pop     bx
        ret

;--- Prozessor, Koprozessor, 8259 ------------------------------------
m_cpu:
        ; PUSH imm8 (6Ah 00h): V20 legt ein Wort ab, der 8088 nimmt 6Ah
        ; als JPE $+2 - kein Stapelwechsel.
        mov     bx,sp
        db      0x6A,0x00
        mov     ax,bx
        sub     ax,sp
        add     sp,ax
        mov     [r_push],al
        ; AAD 16 (D5h 10h) auf AX = 0102h: 8088 -> 12h, V20 -> 0Ch
        mov     ax,0x0102
        db      0xD5,0x10
        mov     [r_aad],al

        ; 8087 nach AP-113, nur die wartefreien Formen
        mov     word [r_fsw],0x5A5A
        mov     word [r_fcw],0x5A5A
        db      0xDB,0xE3               ; FNINIT
        mov     cx,200
mc_d1:  loop    mc_d1
        db      0xDD,0x3E               ; FNSTSW [r_fsw]
        dw      r_fsw
        mov     cx,200
mc_d2:  loop    mc_d2
        db      0xD9,0x3E               ; FNSTCW [r_fcw]
        dw      r_fcw
        mov     cx,200
mc_d3:  loop    mc_d3

        ; 8259 (nur K235): Maske vorher roh, dann ICW, dann AAh/55h
        ; zurueck. Zwischen Schreiben und Lesen eine Speicherschreibung
        ; mit dem Gegenmuster, damit ein offener Bus nicht das gerade
        ; geschriebene Byte vortaeuscht.
        in      al,P_PICDATA
        mov     [r_pic0],al
        ; wie das ROM 33473: pegelgesteuert, normales EOI. So bleibt der
        ; 8259 im Zustand vor DOS, und das IRR zeigt die Eingaenge
        ; (Seite Interrupts).
        mov     al,PIC_FWICW1
        out     P_PICCMD,al
        mov     al,PIC_ICW2
        out     P_PICDATA,al
        mov     al,PIC_FWICW4
        out     P_PICDATA,al
        mov     al,0xAA
        out     P_PICDATA,al
        mov     byte [buscell],0x55
        in      al,P_PICDATA
        mov     [r_pic1],al
        mov     al,0x55
        out     P_PICDATA,al
        mov     byte [buscell],0xAA
        in      al,P_PICDATA
        mov     [r_pic2],al
        mov     al,0xFF                 ; alles gesperrt; wir brauchen keine
        out     P_PICDATA,al            ; Interrupts
        ret

;--- Takt --------------------------------------------------------------
; Eine Schleife aus AAM: reine Registerarbeit, feste Taktzahl
; (8088: 83, V20: 15). 50 Durchlaeufe zu 32 AAM, davon die leere
; Schleife abgezogen. Die CPU-Art muss vorher feststehen.
m_clock:
        cli
        call    tk_start
        mov     cx,50
mk_e:   loop    mk_e
        call    tk_since
        mov     [r_tempty],ax
        call    tk_start
        mov     cx,50
mk_a:
%rep 32
        aam
%endrep
        loop    mk_a
        call    tk_since
        mov     [r_taam],ax
        ret

;--- Speicherbaenke -----------------------------------------------------
; Bank n (1-7) beginnt beim 8088 bei n*10000h. Je Bank ein eigenes
; Muster an n:FFF0 und n:FFF2 schreiben, ERST danach alle lesen - so
; faellt auch eine Spiegelung auf. Dazwischen Gegenmuster in Bank 0.
m_banks:
        mov     bx,1
mb_w:   mov     ax,bx
        mov     cl,12
        shl     ax,cl
        push    ds
        mov     ds,ax
        mov     ax,0xA500
        or      al,bl
        mov     [0xFFF0],ax
        not     ax
        mov     [0xFFF2],ax
        pop     ds
        inc     bx
        cmp     bx,8
        jb      mb_w
        mov     bx,1
        mov     di,r_bank
mb_r:   mov     word [buscell],0x0000
        mov     ax,bx
        mov     cl,12
        shl     ax,cl
        push    ds
        mov     ds,ax
        mov     ax,[0xFFF0]
        mov     dx,[0xFFF2]
        pop     ds
        mov     [di],ax
        mov     [di+2],dx
        add     di,4
        inc     bx
        cmp     bx,8
        jb      mb_r
        ret

;--- Dreifachlesen gegen den offenen Bus ------------------------------
; DX = Anschluss -> AL, AH, [in3c]: dreimal gelesen (BH geht verloren), jedesmal mit anderem
; Wert zuletzt auf dem Bus (Speicherschreibung 00h/FFh/5Ah) und anderen
; Fuellbytes hinter dem IN. Ein echtes Register antwortet dreimal
; gleich; ein offener Bus folgt dem, was zuletzt darauf lag.
in3:
        mov     al,0x00
        mov     [buscell],al
        in      al,dx
        nop
        nop
        nop
        mov     bh,al
        mov     al,0xFF
        mov     [buscell],al
        in      al,dx
        stc
        stc
        stc
        mov     ah,al
        mov     al,0x5A
        mov     [buscell],al
        in      al,dx
        clc
        clc
        clc
        mov     [in3c],al
        mov     al,bh
        ret

;--- Grafikkarte: Mono oder Farbe, unabhaengig von der Firmware -------
; Die Farbkarte hat drei Ebenen zu 16 K Worten (gruen 0000h, rot 4000h,
; blau 8000h), die Monokarte eine (4 x TMS4416). Probe am Ende der
; Ebenen, ausserhalb des Textbereichs (die Firmware benutzt im
; Textbetrieb nur die Worte 0-1999, als Ringpuffer):
;   1. Wort 3FF0h und 7FF0h lesen und merken
;   2. 3FF0h := GA, dann 7FF0h := GB
;   3. beide zuruecklesen
;   Farbe:  3FF0h = GA, 7FF0h = GB
;   Mono mit Spiegelung (Adressbit 14 fehlt): beide = GB
;   Mono ohne: 3FF0h = GA, 7FF0h irgendetwas
;   4. die gemerkten Worte zurueckschreiben (7FF0h zuerst)
; Lesen und Schreiben wie DMVSNAP (auf echter Hardware erprobt): vor
; jedem Kommando auf leere FIFO warten, CURS laedt MASK -> MASK FFFFh,
; am Ende FIGS mit DC = 0, sonst schreibt die Firmware jedes Zeichen
; mehrfach. RDAT mit DC = 1; was danach noch an Bytes kommt, wird
; gezaehlt (Frage aus dem GEM-Thread: liefert RDAT n oder n+1 Worte?).
GA      equ     0xA5C3
GB      equ     0x3C5A
m_gdc:
        mov     byte [r_gok],0
        mov     bx,0x3FF0
        call    g_rd
        jc      mg_x
        mov     [r_g0],ax
        mov     al,[g_extra]
        mov     [r_gex],al
        mov     bx,0x7FF0
        call    g_rd
        jc      mg_x
        mov     [r_g1],ax
        mov     bx,0x3FF0
        mov     ax,GA
        call    g_wr
        mov     bx,0x7FF0
        mov     ax,GB
        call    g_wr
        mov     bx,0x3FF0
        call    g_rd
        jc      mg_x
        mov     [r_g2],ax
        mov     bx,0x7FF0
        call    g_rd
        jc      mg_x
        mov     [r_g3],ax
        mov     bx,0x7FF0               ; zurueck, 7FF0h zuerst (Spiegel!)
        mov     ax,[r_g1]
        call    g_wr
        mov     bx,0x3FF0
        mov     ax,[r_g0]
        call    g_wr
        mov     byte [r_gok],1
mg_x:   call    g_figs0
        in      al,P_GDCSTAT
        mov     [r_gst],al
        ret

g_free:                                 ; CF = FIFO nicht leer geworden
        push    ax
        push    cx
        xor     cx,cx
gf_w:   in      al,P_GDCSTAT
        test    al,GDC_FIEMPTY
        jnz     gf_ok
        loop    gf_w
        pop     cx
        pop     ax
        stc
        ret
gf_ok:  pop     cx
        pop     ax
        clc
        ret

g_curs:                                 ; BX = Wortadresse
        call    g_free
        mov     al,0x49                 ; CURS
        out     P_GDCCMD,al
        mov     al,bl
        out     P_GDCPARAM,al
        mov     al,bh
        out     P_GDCPARAM,al
        xor     al,al                   ; dAD 0, EAD-Bits 16-17 = 0
        out     P_GDCPARAM,al
        call    g_free
        mov     al,0x4A                 ; MASK FFFFh (CURS hat sie verstellt)
        out     P_GDCCMD,al
        mov     al,0xFF
        out     P_GDCPARAM,al
        out     P_GDCPARAM,al
        ret

g_figs:                                 ; AX = DC
        push    ax
        call    g_free
        mov     al,0x4C                 ; FIGS, Richtung 2 (nach rechts)
        out     P_GDCCMD,al
        mov     al,2
        out     P_GDCPARAM,al
        pop     ax
        out     P_GDCPARAM,al
        mov     al,ah
        out     P_GDCPARAM,al
        ret

g_figs0:
        push    ax
        xor     ax,ax
        call    g_figs
        pop     ax
        ret

g_wr:                                   ; Wort AX nach Adresse BX
        push    ax
        call    g_curs
        xor     ax,ax
        call    g_figs
        call    g_free
        mov     al,0x20                 ; WDAT, Wort, ersetzen
        out     P_GDCCMD,al
        pop     ax
        out     P_GDCPARAM,al
        mov     al,ah
        out     P_GDCPARAM,al
        call    g_free
        ret

g_rd:                                   ; AX = Wort an BX; CF = Zeit um
        call    g_curs                  ; [g_extra] = Bytes nach den zweien
        mov     ax,1
        call    g_figs
        call    g_free
        mov     al,0xA0                 ; RDAT, Wort
        out     P_GDCCMD,al
        call    g_byte
        jc      gr_x
        mov     dl,al
        call    g_byte
        jc      gr_x
        mov     dh,al
        mov     byte [g_extra],0        ; Rest abholen und zaehlen
gr_more:
        mov     cx,200
gr_w:   in      al,P_GDCSTAT
        test    al,1
        jnz     gr_got
        loop    gr_w
        mov     ax,dx
        clc
        ret
gr_got: in      al,P_GDCCMD             ; beim Lesen: A1h = Daten
        inc     byte [g_extra]
        cmp     byte [g_extra],16
        jb      gr_more
        mov     ax,dx
        clc
        ret
gr_x:   stc
        ret

g_byte:                                 ; AL = Datenbyte; CF = Zeit um
        push    cx
        xor     cx,cx
gb_w:   in      al,P_GDCSTAT
        test    al,1                    ; DATA READY
        jnz     gb_ok
        loop    gb_w
        pop     cx
        stc
        ret
gb_ok:  in      al,P_GDCCMD
        pop     cx
        clc
        ret

;=====================================================================
;  DEUTUNG DER GRUNDWERTE
;=====================================================================
judge:
        ; CPU: beide Proben muessen sich einig sein
        mov     word [j_cpu],s_unk_cpu
        mov     al,[r_push]
        mov     ah,[r_aad]
        cmp     al,0
        jne     jc_v
        cmp     ah,0x12
        jne     jc_x
        mov     word [j_cpu],s_8088
        mov     word [r_aamclk],83
        jmp     short jc_f
jc_v:   cmp     al,2
        jne     jc_x
        cmp     ah,0x0C
        jne     jc_x
        mov     word [j_cpu],s_v20
        mov     word [r_aamclk],15
        jmp     short jc_f
jc_x:   mov     word [r_aamclk],0
jc_f:
        ; 8087: SW niederes Byte 0 und (CW AND 103Fh) = 003Fh.
        ; In der DMV kann nur ein 8087 stecken (Sockel B1 neben 8088/V20).
        ; Ein echter 8087 liefert nach FNINIT CW 03FFh (Bit 7 = IEM), auf
        ; Maschine B gemessen. MAMEs i8087 setzt beim Zuruecksetzen 037Fh
        ; - den Wert des 80287 und spaeter. Beides erfuellt die Bedingung;
        ; Bit 7 wird nur als Rohwert gezeigt, nicht gedeutet.
        mov     word [j_fpu],s_nofpu
        mov     ax,[r_fsw]
        cmp     ax,0x5A5A
        je      jf_e                    ; nicht beschrieben -> keiner
        mov     word [j_fpu],s_unk_fpu
        or      al,al
        jnz     jf_e
        mov     ax,[r_fcw]
        and     ax,0x103F
        cmp     ax,0x003F
        jne     jf_e
        mov     word [j_fpu],s_8087
jf_e:
        ; CPU-Karte: 8259 und ROM-Pruefsumme muessen zusammenpassen
        mov     byte [j_pic],0
        cmp     byte [r_pic1],0xAA
        jne     jk_p
        cmp     byte [r_pic2],0x55
        jne     jk_p
        mov     byte [j_pic],1
jk_p:   mov     si,crc_33473
        call    crc88_is
        mov     word [j_card],s_card_unk
        jne     jk_n
        cmp     byte [j_pic],1
        jne     jk_e
        mov     word [j_card],s_k235
        jmp     short jk_e
jk_n:   mov     si,crc_32167
        call    crc88_is
        jne     jk_e
        cmp     byte [j_pic],0
        jne     jk_e
        mov     word [j_card],s_k230
jk_e:
        ; Hauptplatinen-ROM aus der Tabelle
        mov     word [j_rom],s_unk
        mov     si,romtab
jr_l:   mov     ax,[si]
        or      ax,[si+2]
        jz      jr_e
        mov     ax,[si]
        cmp     ax,[r_crcmb]
        jne     jr_n
        mov     ax,[si+2]
        cmp     ax,[r_crcmb+2]
        jne     jr_n
        mov     ax,[si+4]
        mov     [j_rom],ax
        jmp     short jr_e
jr_n:   add     si,6
        jmp     short jr_l
jr_e:
        ; Takt in Hundertstel MHz: 1600*clk*100 / (us netto)
        mov     word [j_mhz],0
        mov     ax,[r_taam]
        sub     ax,[r_tempty]
        jbe     jm_e
        shl     ax,1                    ; Schritte -> us
        jc      jm_e
        mov     [r_tus],ax
        mov     bx,ax
        mov     ax,[r_aamclk]
        or      ax,ax
        jz      jm_e
        mov     cx,16000                ; 1600*100 / 10 -> *10 unten
        mul     cx                      ; DX:AX = clk*16000
        mov     cx,10
        push    dx
        mul     cx                      ; niedriges Wort *10
        mov     si,ax
        mov     di,dx
        pop     ax
        mul     cx                      ; hohes Wort *10
        add     ax,di
        mov     dx,ax                   ; DX:SI = clk*160000
        mov     ax,si
        cmp     dx,bx
        jae     jm_e                    ; Ueberlauf: ungueltig
        div     bx
        mov     [j_mhz],ax
jm_e:
        ; Bildschirm: Firmware-Kennung und Ebenenprobe muessen passen
        mov     al,[r_fwraw+2]
        mov     ah,0                    ; 1 = Firmware Farbe, 2 = Mono
        cmp     al,ROM_COLOR
        jne     jg_1
        mov     ah,1
jg_1:   cmp     al,ROM_MONO
        jne     jg_2
        mov     ah,2
jg_2:   mov     al,0                    ; Probe: 1 Farbe, 2 Mono, 0 unklar
        cmp     byte [r_gok],1
        jne     jg_3
        cmp     word [r_g2],GA
        jne     jg_m
        cmp     word [r_g3],GB
        jne     jg_m
        mov     al,1
        jmp     short jg_3
jg_m:   cmp     word [r_g2],GB          ; gespiegelt
        jne     jg_n
        cmp     word [r_g3],GB
        jne     jg_3
        mov     al,2
        jmp     short jg_3
jg_n:   cmp     word [r_g2],GA          ; 4000h-Ebene fehlt
        jne     jg_3
        mov     al,2
jg_3:   mov     [j_gprobe],al
        mov     word [j_gfx],s_gfx_unk
        cmp     al,ah
        jne     jg_e
        mov     word [j_gfx],s_gfx_col
        cmp     al,1
        je      jg_e
        mov     word [j_gfx],s_gfx_mono
        cmp     al,2
        je      jg_e
        mov     word [j_gfx],s_gfx_unk
jg_e:
        ; Speicherbaenke
        mov     byte [j_banks],0
        mov     si,r_bank
        mov     bx,1
jb_l:   mov     ax,0xA500
        or      al,bl
        cmp     [si],ax
        jne     jb_n
        not     ax
        cmp     [si+2],ax
        jne     jb_n
        inc     byte [j_banks]
jb_n:   add     si,4
        inc     bx
        cmp     bx,8
        jb      jb_l
        ; Attribute: Farbe nur bei sicherer Farbkarte oder, wenn die
        ; Probe unklar ist, nach der Firmware
        mov     word [attr_tab],attr_mono
        cmp     word [j_gfx],s_gfx_col
        je      ja_c
        cmp     word [j_gfx],s_gfx_mono
        je      ja_e
        cmp     byte [r_fwraw+2],ROM_COLOR
        jne     ja_e
ja_c:   mov     word [attr_tab],attr_color
ja_e:   ret

crc88_is:                               ; SI -> erwartete CRC; ZF = gleich
        mov     ax,[si]
        cmp     ax,[r_crc88]
        jne     ci_e
        mov     ax,[si+2]
        cmp     ax,[r_crc88+2]
ci_e:   ret

;=====================================================================
;  SEITEN
;=====================================================================

;--- Seite 1: System ---------------------------------------------------
pg_system:
        mov     si,s_p1
        call    puts
        call    nl
        mov     si,s_fwraw
        call    puts
        mov     si,r_fwraw
        mov     cx,9
p1_f:   lodsb
        call    hex8
        call    space
        loop    p1_f
        mov     al,'"'
        call    putc
        mov     si,r_fwraw+1
        mov     cx,8
p1_t:   lodsb
        cmp     al,' '
        jb      p1_dot
        cmp     al,0x7E
        jbe     p1_c
p1_dot: mov     al,'.'
p1_c:   call    putc
        loop    p1_t
        mov     al,'"'
        call    putc
        call    nl

        mov     si,s_crcmb
        call    puts
        mov     ax,[r_crcmb+2]
        call    hex16
        mov     ax,[r_crcmb]
        call    hex16
        mov     si,s_arrow
        call    puts
        mov     si,[j_rom]
        call    puts
        call    nl

        mov     si,s_crc88
        call    puts
        mov     ax,[r_crc88+2]
        call    hex16
        mov     ax,[r_crc88]
        call    hex16
        mov     si,s_arrow
        call    puts
        mov     si,s_unk
        mov     di,crc_33473
        call    p1_crcname
        call    nl
        call    nl

        mov     si,s_mbox
        call    puts
        call    nl
        mov     bx,0xFE00
p1_mb:  mov     si,s_ind
        call    puts
        mov     ax,bx
        call    hex16
        mov     al,':'
        call    putc
        call    space
        mov     si,bx
        call    hexdump16
        call    nl
        add     bx,16
        cmp     bx,0xFE40
        jb      p1_mb
        mov     si,s_mbnote
        call    puts
        call    nl
        call    nl

        mov     si,s_kbd
        call    puts
        mov     al,[r_kbst]
        call    hex8
        mov     si,s_kbd2
        call    puts
        mov     al,[r_kbst+1]
        call    hex8
        mov     si,s_kbd3
        call    puts
        mov     ax,[r_kbt]
        mov     dx,[r_kbt+2]
        call    dec32
        mov     si,s_kbd4
        call    puts
        call    nl
        ; Deutung Landescode
        mov     si,s_kbdj
        call    puts
        mov     al,[r_kbst]
        test    al,KBD_LANG
        jz      p1_kn
        mov     al,[r_kbst+1]
        sub     al,KBD_LANGBAS
        cmp     al,8
        jae     p1_kn
        push    ax
        call    hex4
        call    space
        pop     ax
        cbw
        mov     bx,ax
        shl     bx,1
        mov     si,[countries+bx]
        call    puts
        mov     si,s_kbdv1
        call    puts
        jmp     short p1_ke
p1_kn:  mov     si,s_unk
        call    puts
p1_ke:  call    nl
        call    nl
        ; Grafikkarte
        mov     si,s_g1
        call    puts
        mov     ax,[r_g0]
        call    hex16
        mov     si,s_g1b
        call    puts
        mov     ax,[r_g1]
        call    hex16
        mov     si,s_g1c
        call    puts
        mov     al,[r_gex]
        call    hex8
        mov     si,s_g1d
        call    puts
        mov     al,[r_gst]
        call    hex8
        call    nl
        mov     si,s_g2
        call    puts
        mov     ax,[r_g2]
        call    hex16
        mov     si,s_g2b
        call    puts
        mov     ax,[r_g3]
        call    hex16
        cmp     byte [r_gok],1
        je      p1_g3
        mov     si,s_gto
        call    puts
p1_g3:  call    nl
        mov     si,s_g3
        call    puts
        mov     al,[j_gprobe]
        mov     si,s_gp_col
        cmp     al,1
        je      p1_g4
        mov     si,s_gp_mono
        cmp     al,2
        je      p1_g4
        mov     si,s_unk
p1_g4:  call    puts
        mov     si,s_g3b
        call    puts
        mov     si,[j_gfx]
        call    puts
        call    nl
        ret

p1_crcname:                             ; Name der 8088-Karten-ROM-CRC
        mov     si,crc_33473
        call    crc88_is
        jne     pn_2
        mov     si,s_rom33473
        jmp     short pn_p
pn_2:   mov     si,crc_32167
        call    crc88_is
        jne     pn_3
        mov     si,s_rom32167
        jmp     short pn_p
pn_3:   mov     si,s_unk
pn_p:   call    puts
        ret

;--- Seite 2: Prozessor ------------------------------------------------
pg_cpu:
        mov     si,s_p2
        call    puts
        call    nl
        mov     si,s_push
        call    puts
        mov     al,[r_push]
        call    hex8
        mov     si,s_push2
        call    puts
        call    nl
        mov     si,s_aad
        call    puts
        mov     al,[r_aad]
        call    hex8
        mov     si,s_aad2
        call    puts
        call    nl
        mov     si,s_cpuj
        call    puts
        mov     si,[j_cpu]
        call    puts
        call    nl
        call    nl

        mov     si,s_fpu1
        call    puts
        mov     ax,[r_fsw]
        call    hex16
        mov     si,s_fpu2
        call    puts
        mov     ax,[r_fcw]
        call    hex16
        call    nl
        mov     si,s_fpu3
        call    puts
        call    nl
        mov     si,s_fpuj
        call    puts
        mov     si,[j_fpu]
        call    puts
        call    nl
        call    nl

        mov     si,s_pic1
        call    puts
        mov     al,[r_pic0]
        call    hex8
        mov     si,s_pic2
        call    puts
        mov     al,[r_pic1]
        call    hex8
        mov     si,s_pic3
        call    puts
        mov     al,[r_pic2]
        call    hex8
        call    nl
        mov     si,s_crc88b
        call    puts
        call    p1_crcname
        call    nl
        mov     si,s_cardj
        call    puts
        mov     si,[j_card]
        call    puts
        call    nl
        mov     si,s_cardn
        call    puts
        call    nl
        call    nl

        mov     si,s_clk1
        call    puts
        mov     ax,[r_tempty]
        call    dec16
        mov     si,s_clk2
        call    puts
        mov     ax,[r_taam]
        call    dec16
        mov     si,s_clk3
        call    puts
        mov     ax,[r_tus]
        call    dec16
        mov     si,s_clk4
        call    puts
        call    nl
        mov     si,s_clkj
        call    puts
        mov     ax,[j_mhz]
        or      ax,ax
        jz      p2_cu
        call    dec2c
        mov     si,s_mhz
        call    puts
        mov     si,s_mhz2
        call    puts
        mov     ax,[r_aamclk]
        call    dec16
        mov     si,s_mhz3
        call    puts
        jmp     short p2_ce
p2_cu:  mov     si,s_unk
        call    puts
p2_ce:  call    nl
        ret

;--- Seite 3: Speicher ---------------------------------------------------
pg_mem:
        mov     si,s_p3
        call    puts
        call    nl
        mov     si,s_fe06
        call    puts
        mov     al,[MB_MEM16]
        call    hex8
        mov     si,s_fe06b
        call    puts
        call    nl
        call    nl
        mov     si,s_bankh
        call    puts
        call    nl
        mov     si,r_bank
        mov     bx,1
p3_l:   mov     si,s_ind
        call    puts
        mov     ax,bx
        call    hex4
        mov     al,'0'
        call    putc
        mov     si,s_bank2
        call    puts
        mov     ax,0xA500
        or      al,bl
        call    hex16
        call    space
        not     ax
        call    hex16
        mov     si,s_bank3
        call    puts
        mov     di,bx
        dec     di
        shl     di,1
        shl     di,1
        add     di,r_bank
        mov     ax,[di]
        call    hex16
        call    space
        mov     ax,[di+2]
        call    hex16
        call    space
        call    space
        ; Deutung je Bank
        mov     ax,0xA500
        or      al,bl
        cmp     [di],ax
        jne     p3_x
        not     ax
        cmp     [di+2],ax
        jne     p3_x
        mov     si,s_da
        jmp     short p3_p
p3_x:   mov     ax,[di]
        cmp     ah,0xA5                 ; Muster einer anderen Bank?
        jne     p3_f
        mov     si,s_mirror
        call    puts
        call    hex4
        jmp     short p3_n
p3_f:   mov     si,s_fehlt
p3_p:   call    puts
p3_n:   call    nl
        inc     bx
        cmp     bx,8
        jb      p3_l
        call    nl
        mov     si,s_memj
        call    puts
        mov     al,[j_banks]
        cbw
        inc     ax
        mov     cl,6
        shl     ax,cl
        call    dec16
        mov     si,s_kb
        call    puts
        mov     al,[j_banks]
        mov     si,s_k200
        cmp     al,1
        je      p3_c
        mov     si,s_k202
        cmp     al,3
        je      p3_c
        mov     si,s_k208
        cmp     al,7
        je      p3_c
        mov     si,s_nok
        cmp     al,0
        je      p3_c
        mov     si,s_unk
p3_c:   call    puts
        call    nl
        mov     si,s_memn
        call    puts
        call    nl
        call    nl
        ; K233
        cmp     byte [k_ran],1
        je      pm_k
        mov     si,s_kk0
        call    puts
        call    nl
        ret
pm_k:   mov     si,s_kk1
        call    puts
        mov     al,[k_m0]
        call    hex8
        mov     si,s_kk2
        call    puts
        mov     al,[k_r1]
        call    hex8
        mov     si,s_kk3
        call    puts
        mov     al,[k_r2]
        call    hex8
        mov     si,s_kk4
        call    puts
        mov     al,[k_wt]
        call    hex8
        mov     si,s_kk5
        call    puts
        mov     al,[k_7b]
        call    hex8
        call    nl
        mov     si,s_cpj
        call    puts
        mov     si,s_kno
        cmp     byte [k_r2],0x5A        ; im "ein"-Zustand die Kartenkopie?
        jne     pm_kj
        cmp     byte [k_r1],0xA5        ; im "aus"-Zustand die Hauptplatine?
        jne     pm_kj
        mov     si,s_kyes
pm_kj:  call    puts
        cmp     si,s_kyes
        jne     pm_ke
        mov     si,s_kwt1
        cmp     byte [k_wt],0x3C
        je      pm_kw
        mov     si,s_kwt0
        cmp     byte [k_wt],0xA5
        je      pm_kw
        mov     si,s_kwtu
pm_kw:  call    puts
pm_ke:  call    nl
        ret

;--- K233: 16 KB gemeinsames RAM bei C000h-FFFFh ------------------------
; Schaltbild der K233: eine Kippstufe, getaktet
; von Schreibzugriffen auf 7Eh/7Fh; nach RESET aus. TRAMD/ der Karte
; sperrt auf der Hauptplatine nur die AUSGABE des RAM (Schaltplan
; Hauptplatine: TRAMD/ -> B12 -> F2 -> OE/ von F4), das Schreiben (WE/
; des 8203) nicht: im eingeschalteten Zustand landen Schreibzugriffe
; vermutlich in Karte UND Hauptplatine. Dazu passt NCRs V1K233, das
; C000h-FFFFh vor dem Test nach 8000h rettet und danach zurueckkopiert.
; MAME schreibt bisher nur in die Karte.
;
; Probe (nach RESET aus; die Probe kommt mit beiden Annahmen zurecht):
;   m0 := [C000]                         ; Hauptplatine
;   kipp, [C000] := 5Ah                  ; Karte (und ggf. Hauptplatine)
;   kipp, [C000] := A5h, r1 := [C000]    ; Hauptplatine
;   kipp, r2 := [C000]                   ; Karte: 5Ah, wenn sie da ist
;   [C000] := 3Ch, kipp, wt := [C000]    ; 3Ch = Schreiben geht durch
;   [C000] := m0                         ; Hauptplatine wiederherstellen
; Gekippt wird mit OUT 7Fh. Auf 7Fh liegt aber auch das Befehlsregister
; einer K211 (ihr BA3 ist nicht angeschlossen): darum vorher ihr
; Befehlsregister ueber 7Bh lesen und genau diesen Wert schreiben.
; Ohne Karte gibt es nur die Hauptplatine, alle Lesungen = letzter
; Schreibwert. Solange die Karte an ist, liegen Postfach, Firmware-
; Variablen (F800h) und der Stapel der Firmware darunter: CLI, kein
; DISPLAY, unser Stapel liegt unter 4000h.
k233_kipp:
        in      al,0x7B                 ; Befehlsregister einer K211 (x3)
        mov     [k_7b],al
        out     0x7F,al                 ; dieselben Bits zurueck, K233 kippt
        ret

k233_run:
        cli
        mov     al,[0xC000]
        mov     [k_m0],al
        call    k233_kipp
        mov     byte [0xC000],0x5A
        call    k233_kipp
        mov     byte [0xC000],0xA5
        mov     al,[0xC000]
        mov     [k_r1],al
        call    k233_kipp
        mov     al,[0xC000]
        mov     [k_r2],al
        mov     byte [0xC000],0x3C
        call    k233_kipp               ; vier Mal gekippt: wieder aus
        mov     al,[0xC000]
        mov     [k_wt],al
        mov     al,[k_m0]
        mov     [0xC000],al
        mov     byte [k_ran],1
        ret

;--- Seite 4: IFSEL roh ----------------------------------------------------
pg_ifsel:
        mov     si,s_p4
        call    puts
        call    nl
        mov     si,s_p4a
        call    puts
        call    nl
        mov     si,s_p4h
        call    puts
        call    nl
        mov     si,iftab
p4_l:   lodsb                           ; Basis
        or      al,al
        jz      p4_e
        mov     bl,al
        push    si
        mov     si,s_ind
        call    puts
        pop     si
        lodsw                           ; Name der Haelfte (2 Zeichen)
        call    putc
        mov     al,ah
        call    putc
        call    space
        mov     al,bl
        call    hex8
        mov     al,'h'
        call    putc
        mov     cx,4
        call    spaces
        ; vier Anschluesse x0..x3, je drei Lesungen
        mov     byte [p4_stable],1
        mov     byte [p4_allff],1
        mov     byte [p4_echo],1
        xor     cx,cx
p4_p:   mov     dl,bl
        add     dl,cl
        xor     dh,dh
        push    bx
        call    in3                     ; AL, AH, [in3c]
        pop     bx
        mov     bh,[in3c]
        cmp     al,dl                   ; Adressecho des leeren Busses?
        jne     p4_ne
        cmp     ah,dl
        jne     p4_ne
        cmp     bh,dl
        je      p4_ec
p4_ne:  mov     byte [p4_echo],0
p4_ec:
        ; Merker. Stabilitaet nur an x1 und x3 beurteilen: beim 2651
        ; schaltet jedes Lesen von x2 zwischen MR1 und MR2 um, und x0
        ; ist ein Datenregister. Das Lesen von x3 setzt den Zeiger
        ; wieder auf MR1 zurueck.
        test    cl,1
        jz      p4_s
        cmp     al,ah
        jne     p4_ns
        cmp     al,bh
        je      p4_s
p4_ns:  mov     byte [p4_stable],0
p4_s:   cmp     al,0xFF
        jne     p4_nf
        cmp     ah,0xFF
        jne     p4_nf
        cmp     bh,0xFF
        je      p4_f
p4_nf:  mov     byte [p4_allff],0
p4_f:   call    hex8
        push    ax
        mov     al,'/'
        call    putc
        pop     ax
        mov     al,ah
        call    hex8
        mov     al,'/'
        call    putc
        mov     al,bh
        call    hex8
        call    space
        inc     cx
        cmp     cx,4
        jb      p4_p
        ; Deutung der Haelfte (nur die Art der Antwort, keine Karte)
        push    si
        mov     si,s_ff
        cmp     byte [p4_allff],1
        je      p4_j
        mov     si,s_echo
        cmp     byte [p4_echo],1
        je      p4_j
        mov     si,s_float
        cmp     byte [p4_stable],0
        je      p4_j
        mov     si,s_stable
p4_j:   call    puts
        pop     si
        call    nl
        jmp     p4_l
p4_e:   call    nl
        mov     si,s_p4n1
        call    puts
        call    nl
        mov     si,s_p4n2
        call    puts
        call    nl
        mov     si,s_p4n3
        call    puts
        call    nl
        ret

iftab:  db      IF_0A,'0A', IF_0B,'0B', IF_1A,'1A', IF_1B,'1B'
        db      IF_2A,'2A', IF_2B,'2B', IF_3A,'3A', IF_3B,'3B'
        db      IF_4B,'4B', 0

;--- Seite 5: Laufwerke, Platte, Uhr --------------------------------------
pg_disk:
        mov     si,s_p5
        call    puts
        call    nl
        ; FDC: Hauptstatus, offene Interrupts abholen
        mov     si,s_fdcms
        call    puts
        in      al,P_FDCSTAT
        call    hex8
        mov     si,s_sys13
        call    puts
        in      al,P_SYSSTAT
        call    hex8
        call    space
        call    bin8
        call    nl
        mov     si,s_fdcsi
        call    puts
        mov     cx,4
p5_si:  call    fdc_ready
        jc      p5_sx
        mov     al,0x08                 ; SENSE INTERRUPT STATUS
        call    fdc_out
        call    fdc_in
        call    hex8
        call    space
        cmp     al,0x80                 ; ungueltig = nichts mehr offen
        je      p5_sx
        call    fdc_in
        call    hex8
        call    space
        loop    p5_si
p5_sx:  call    nl
        ; SENSE DRIVE STATUS fuer Laufwerk 0-3
        mov     si,s_st3h
        call    puts
        call    nl
        xor     bx,bx
p5_ds:  mov     si,s_ind
        call    puts
        mov     al,bl
        add     al,'0'
        call    putc
        mov     al,':'
        call    putc
        call    space
        call    fdc_ready
        jc      p5_dx
        mov     al,0x04
        call    fdc_out
        mov     al,bl
        call    fdc_out
        call    fdc_in
        call    hex8
        call    space
        call    bin8
        call    space
        call    space
        push    ax
        test    al,0x10
        mov     si,s_t0y
        jnz     p5_t0
        mov     si,s_t0n
p5_t0:  call    puts
        pop     ax
        test    al,0x20
        mov     si,s_ryy
        jnz     p5_ry
        mov     si,s_ryn
p5_ry:  call    puts
        test    al,0x40
        mov     si,s_wpy
        jnz     p5_wp
        mov     si,s_wpn
p5_wp:  call    puts
        jmp     short p5_dn
p5_dx:  mov     si,s_fdcbusy
        call    puts
p5_dn:  call    nl
        inc     bx
        cmp     bx,4
        jb      p5_ds
        mov     si,s_st3n
        call    puts
        call    nl
        ; was das Z80-ROM beim Laden ueber A: festgestellt hat
        mov     si,s_fe08
        call    puts
        mov     al,[MB_Z80A]
        call    hex8
        mov     si,s_fe09
        call    puts
        mov     al,[MB_Z80B]
        call    hex8
        mov     si,s_fe0x
        call    puts
        mov     si,s_unk                ; nur gueltig, wenn die Kennung
        cmp     byte [MB_Z80B],2        ; unserer 512-Byte-Diskette kam
        jne     p5_a
        mov     al,[MB_Z80A]
        mov     si,s_a40
        cmp     al,0
        je      p5_a
        mov     si,s_a80
        cmp     al,1
        je      p5_a
        mov     si,s_unk
p5_a:   call    puts
        call    nl
        call    nl

        ; Festplattencontroller: Rueckleseprobe
        mov     si,s_hd1
        call    puts
        mov     al,0x55
        out     HD_COUNT,al
        mov     al,0xAA
        out     HD_SECTOR,al
        mov     byte [buscell],0x00
        in      al,HD_COUNT
        call    hex8
        call    space
        mov     ah,al
        mov     byte [buscell],0xFF
        in      al,HD_SECTOR
        push    ax
        call    hex8
        mov     si,s_hd1b
        call    puts
        pop     ax
        mov     si,s_hdno
        cmp     ax,0x55AA               ; AH = C2h zurueck, AL = C3h zurueck
        jne     p5_hj
        mov     si,s_hdyes
p5_hj:  call    puts
        call    nl
        ; Einheiten 0-2: Status sofort und nach bis zu 0,5 s
        xor     bx,bx
p5_hu:  mov     si,s_hdu
        call    puts
        mov     al,bl
        add     al,'0'
        call    putc
        mov     si,s_hdu2
        call    puts
        mov     al,bl
        mov     cl,3
        shl     al,cl
        or      al,HD_SDHBASE
        out     HD_SDH,al
        in      al,HD_CMDSTAT           ; sofort, wie B_Hard
        call    hex8
        mov     si,s_hdu3
        call    puts
        call    tk_start
p5_hw:  in      al,HD_CMDSTAT
        test    al,HD_BUSY
        jnz     p5_hb
        test    al,HD_READY
        jnz     p5_hr
p5_hb:  push    ax
        mov     ax,250                  ; 0,5 s
        call    tk_over_ms2
        pop     ax
        jnc     p5_hw
p5_hr:  call    hex8
        mov     si,s_hdu4
        call    puts
        push    ax
        call    tk_since
        mov     cx,500                  ; Schritte -> ms
        div     cx
        call    dec16
        pop     ax
        mov     si,s_ms
        call    puts
        test    al,HD_READY
        jz      p5_hn
        test    al,HD_BUSY
        jnz     p5_hn
        mov     si,s_hdrdy
        call    puts
p5_hn:  call    nl
        inc     bx
        cmp     bx,3
        jb      p5_hu
        mov     al,HD_DESEL
        out     HD_SDH,al
        call    nl

        ; K803: Sekundenregister zweimal im Abstand von 1,2 s
        mov     si,s_rtc
        call    puts
        xor     al,al
        out     RTC_LATCH,al
        call    rtc_sec
        mov     bl,al
        call    hex8
        mov     ax,600                  ; 1,2 s
        call    wait_ms2
        call    rtc_sec
        mov     bh,al
        mov     si,s_rtc2
        call    puts
        mov     al,bh
        call    hex8
        mov     si,s_rtc3
        call    puts
        ; laeuft = beide gueltiges BCD 00-59 und verschieden
        mov     al,bl
        call    bcd59
        jc      p5_ru
        mov     al,bh
        call    bcd59
        jc      p5_ru
        cmp     bl,bh
        je      p5_rs
        mov     si,s_rtcok
        jmp     short p5_rp
p5_rs:  mov     si,s_rtcst
        jmp     short p5_rp
p5_ru:  mov     si,s_unk
p5_rp:  call    puts
        call    nl
        ret

rtc_sec:                                ; zweimal gleich lesen
        push    cx
        mov     cx,10
rs_l:   in      al,RTC_SEC
        mov     ah,al
        in      al,RTC_SEC
        cmp     al,ah
        je      rs_e
        loop    rs_l
rs_e:   pop     cx
        ret

bcd59:                                  ; CF = kein BCD 00-59
        push    ax
        mov     ah,al
        and     ah,0x0F
        cmp     ah,9
        ja      b59_x
        shr     al,1
        shr     al,1
        shr     al,1
        shr     al,1
        cmp     al,5
        ja      b59_x
        pop     ax
        clc
        ret
b59_x:  pop     ax
        stc
        ret

fdc_ready:                              ; CF = Controller nicht bereit
        push    ax
        push    cx
        xor     cx,cx
fr_l:   in      al,P_FDCSTAT
        and     al,0xC0
        cmp     al,0x80                 ; RQM und Richtung "zum FDC"
        je      fr_ok
        loop    fr_l
        pop     cx
        pop     ax
        stc
        ret
fr_ok:  pop     cx
        pop     ax
        clc
        ret

fdc_out:
        push    cx
        push    ax
        xor     cx,cx
fo_l:   in      al,P_FDCSTAT
        test    al,FDC_RQM
        jnz     fo_g
        loop    fo_l
fo_g:   pop     ax
        out     P_FDCDATA,al
        pop     cx
        ret

fdc_in:
        push    cx
        xor     cx,cx
fi_l:   in      al,P_FDCSTAT
        test    al,FDC_RQM
        jnz     fi_g
        loop    fi_l
fi_g:   in      al,P_FDCDATA
        pop     cx
        ret

;--- Seite: Laufwerke aktiv (Tasten T und B) ----------------------------
; Schaltplan (System Technical Manual, Flex Disk Contr.): RDY des 8272
; haengt ueber D6 am READY/ des Laufwerks (J110-34). Ein 5,25-Zoll-
; Laufwerk meldet READY nur mit eingelegter, drehender Diskette. MAMEs
; upd765 fuehrt SEEK/RECALIBRATE ohne READY gar nicht aus (ST0: NR,
; abgebrochen). Ohne Diskette ist also weder ein Schritt noch eine
; Spurzahl zu erwarten - die Probe T belegt das am Geraet.
;   T: je Laufwerk RECALIBRATE, SEEK 2, RECALIBRATE; ST0/PCN/ST3 roh.
;      Hoechstens zwei Schritte, kein Anschlag.
;   B: mit eingelegter DMVDIAG-Diskette (40 Spuren) in B: auf den
;      physischen Zylinder 2, READ ID: C=2 -> 40-Spur-Werk, C=1 -> 80.
FDT_MAX equ     1000                    ; Wartegrenze 2 s (2-ms-Einheiten)

fd_cmdwait:                             ; auf Interrupt (13h Bit 3) warten,
        mov     ax,FDT_MAX              ; dann SENSE INTERRUPT
        call    tk_start                ; -> AL = ST0, AH = PCN, CF = Zeit um
fw_w:   in      al,P_SYSSTAT
        test    al,SS_FDCINT
        jnz     fw_si
        mov     ax,FDT_MAX
        call    tk_over_ms2
        jnc     fw_w
        mov     ax,0xFFFF
        stc
        ret
fw_si:  call    fdc_ready
        mov     al,0x08
        call    fdc_out
        call    fdc_in
        cmp     al,0x80                 ; nichts offen: weiter warten
        je      fw_w
        mov     ah,al                   ; ST0
        call    fdc_in                  ; PCN
        xchg    al,ah
        ; IC = 11: Meldung "READY hat sich geaendert" - kommt beim 8272
        ; ungefragt, sobald ein Laufwerk nach dem Motorstart bereit wird
        ; (MAME-Bild zu 0.08: ST0 C1 beim ersten RECALIBRATE von A:, das
        ; war die Meldung von B:). Verwerfen und auf die eigene warten.
        push    ax
        and     al,0xC0
        cmp     al,0xC0
        pop     ax
        jne     fw_ok
        mov     ax,FDT_MAX
        call    tk_over_ms2
        jnc     fw_w
        mov     ax,0xFFFF
        stc
        ret
fw_ok:  clc
        ret

fd_st3:                                 ; BL = Laufwerk -> AL = ST3
        call    fdc_ready
        mov     al,0x04
        call    fdc_out
        mov     al,bl
        call    fdc_out
        call    fdc_in
        ret

fd_motor:                               ; Motor an, bis 1 s auf "bereit"
        out     P_FDMOTOR,al
        call    tk_start
fm_w:   in      al,P_SYSSTAT
        test    al,SS_READY
        jnz     fm_e
        mov     ax,500                  ; 1 s (Datenblatt: READY bis 800 ms)
        call    tk_over_ms2
        jnc     fm_w
fm_e:   ret

drv_tprobe:
        call    cls
        mov     si,s_dt_go
        call    puts
        call    fd_motor
        xor     bx,bx
        mov     di,d_res                ; je Laufwerk 9 Byte
dt_l:   ; RECALIBRATE
        call    fdc_ready
        mov     al,0x07
        call    fdc_out
        mov     al,bl
        call    fdc_out
        call    fd_cmdwait
        mov     [di],ax
        call    fd_st3
        mov     [di+2],al
        ; SEEK 2
        call    fdc_ready
        mov     al,0x0F
        call    fdc_out
        mov     al,bl
        call    fdc_out
        mov     al,2
        call    fdc_out
        call    fd_cmdwait
        mov     [di+3],ax
        call    fd_st3
        mov     [di+5],al
        ; RECALIBRATE
        call    fdc_ready
        mov     al,0x07
        call    fdc_out
        mov     al,bl
        call    fdc_out
        call    fd_cmdwait
        mov     [di+6],ax
        call    fd_st3
        mov     [di+8],al
        add     di,9
        inc     bx
        cmp     bx,2
        jb      dt_l
        mov     byte [d_tran],1
        ret

drv_bprobe:
        call    cls
        mov     si,s_db_go
        call    puts
        call    fd_motor
        mov     byte [d_bok],0
        mov     word [d_bst0],0
        mov     word [d_bst0+2],0
        mov     bl,1                    ; Laufwerk B:
        call    fdc_ready
        mov     al,0x07                 ; RECALIBRATE
        call    fdc_out
        mov     al,bl
        call    fdc_out
        call    fd_cmdwait
        mov     [d_bst0],ax
        jc      db_e
        test    al,0xC8                 ; abgebrochen oder nicht bereit
        jnz     db_e
        call    fdc_ready
        mov     al,0x0F                 ; SEEK auf physisch 2
        call    fdc_out
        mov     al,bl
        call    fdc_out
        mov     al,2
        call    fdc_out
        call    fd_cmdwait
        mov     [d_bst0+2],ax
        jc      db_e
        test    al,0xC8
        jnz     db_e
        call    fdc_ready
        mov     al,0x4A                 ; READ ID, MFM, Kopf 0
        call    fdc_out
        mov     al,bl
        call    fdc_out
        call    tk_start                ; Ergebnisphase abwarten (<= 1 s)
db_w:   in      al,P_FDCSTAT
        and     al,0xC0
        cmp     al,0xC0                 ; RQM und Richtung "zum Rechner"
        je      db_r
        mov     ax,500
        call    tk_over_ms2
        jnc     db_w
        jmp     short db_e
db_r:   mov     cx,7
        mov     di,d_bid
db_rl:  call    fdc_in
        mov     [di],al
        inc     di
        loop    db_rl
        mov     byte [d_bok],1
        call    fdc_ready               ; Kopf zurueck auf Spur 0
        mov     al,0x07
        call    fdc_out
        mov     al,bl
        call    fdc_out
        call    fd_cmdwait
db_e:   mov     byte [d_bran],1
        ret

pg_drives:                              ; Titel und M: pg_drv7 (motor.inc)
        mov     si,s_dv2
        call    puts
        call    nl
        mov     si,s_dv3
        call    puts
        call    nl
        cmp     byte [d_tran],1
        je      pd_t
        mov     si,s_dvt
        call    puts
        call    nl
        jmp     pd_b
pd_t:   mov     si,s_dvh
        call    puts
        call    nl
        xor     bx,bx
        mov     si,d_res
pd_tl:  push    si
        mov     si,s_ind
        call    puts
        mov     al,bl
        add     al,'A'
        call    putc
        mov     al,':'
        call    putc
        pop     si
        mov     cx,3
pd_tc:  call    space
        call    space
        lodsw                           ; AL ST0, AH PCN
        call    hex8
        mov     al,'/'
        call    putc
        mov     al,ah
        call    hex8
        mov     al,'/'
        call    putc
        lodsb                           ; ST3
        call    hex8
        loop    pd_tc
        call    nl
        ; Deutung: NR in einem ST0? / Spur0 wechselt?
        push    si
        mov     si,s_cpj
        call    puts
        pop     si
        mov     al,[si-9]               ; ST0 erstes RECAL
        or      al,[si-6]
        or      al,[si-3]
        test    al,0x08
        jz      pd_nr
        push    si
        mov     si,s_dvnr
        call    puts
        pop     si
        jmp     short pd_n
pd_nr:  mov     al,[si-7]               ; ST3 nach RECAL: Spur 0?
        mov     ah,[si-4]               ; ST3 nach SEEK 2
        mov     dl,[si-1]               ; ST3 nach RECAL
        test    al,0x10
        jz      pd_nt
        test    ah,0x10
        jnz     pd_nt
        test    dl,0x10
        jz      pd_nt
        push    si
        mov     si,s_dvok
        call    puts
        pop     si
        jmp     short pd_n
pd_nt:  push    si
        mov     si,s_unk
        call    puts
        pop     si
pd_n:   call    nl
        inc     bx
        cmp     bx,2
        jb      pd_tl
pd_b:   call    nl
        mov     si,s_dv4
        call    puts
        call    nl
        cmp     byte [d_bran],1
        je      pd_bb
        mov     si,s_dvb
        call    puts
        call    nl
        ret
pd_bb:  mov     si,s_dbh
        call    puts
        mov     ax,[d_bst0]
        call    hex8
        mov     al,'/'
        call    putc
        mov     al,ah
        call    hex8
        call    space
        mov     ax,[d_bst0+2]
        call    hex8
        mov     al,'/'
        call    putc
        mov     al,ah
        call    hex8
        call    nl
        cmp     byte [d_bok],1
        je      pd_bi
        mov     si,s_dbno
        call    puts
        call    nl
        ret
pd_bi:  mov     si,s_dbid
        call    puts
        mov     si,d_bid
        mov     cx,7
pd_bl:  lodsb
        call    hex8
        call    space
        loop    pd_bl
        call    nl
        mov     si,s_cpj
        call    puts
        test    byte [d_bid],0xC0       ; ST0 normal?
        jnz     pd_bu
        mov     al,[d_bid+3]            ; C
        mov     ah,[d_bid+6]            ; N
        cmp     ah,3
        je      pd_b80
        cmp     al,2
        je      pd_b40
        cmp     al,1
        je      pd_b80
pd_bu:  mov     si,s_unk
        jmp     short pd_bp
pd_b40: mov     si,s_db40
        jmp     short pd_bp
pd_b80: mov     si,s_db80
pd_bp:  call    puts
        call    nl
        ret

;--- Seite: Koprozessor K234 / K238 (aktiv, nur auf Taste P) ----------
; Ablauf (Belege in docs/DMV-Hardware-Boot-und-Bildschirm.md, Abschnitt
; Uebergabe, und K238-Design-Stand 7.2):
;   - Die K234 startet nur vom Z80 aus: NCRs CP/M-68K macht OUT DFh auf
;     dem Z80, der 68008 gibt mit einem Schreibzugriff auf FFFDFh zurueck.
;     DMVDEMO /6 laeutete vom 8088 aus - auf echter Hardware ohne Wirkung.
;   - Den Z80 erreichen wir ueber seinen Warteplatz F111h im RAM (das
;     Z80-ROM laesst ihn dort kreisen, solange der 8088 laeuft).
;   - Sonde an 0:0000, zweisprachig: 68008 schreibt '6', Z8001 'Z' nach
;     0:0000 und laeutet DFh zurueck (68008: FFFDFh, wie NCRs
;     move.b #$7f,$fffdf; Z8001: OUTB DFh).
;   - Gestartet wird wie in NCRs CP/M-68K mit OUT DFh. 0.08-0.15 nahmen
;     D8h: in MAME gleichwertig, auf echter Hardware (Maschine A, K234)
;     lief der 68008 damit nicht an.
;   - Nur mit K235: die K230/K231 dekodiert D8h ohne BA3 als SWITCH16.
; Der Z80-Teil setzt danach eine Marke, so sieht man, ob er
; zurueckkam. Kommt ein Koprozessor an und laeutet nicht zurueck, steht
; die Maschine - das ist ebenfalls ein Befund.
Z80M    equ     0xF1F0                  ; Marke des Z80-Teils

copro_probe:                            ; 40 Byte, aus K238-Design-Stand 7.2
        db      0x00,0x00,0xC0,0x00,0x00,0x00,0x00,0x08,0x60,0x04,0x00,0x00
        db      0xE8,0x07,0x11,0xFC,0x00,0x36,0x00,0x00,0x11,0xC0,0xFF,0xDF
        db      0x4E,0x72,0x27,0x00,0xC8,0x5A,0x6E,0x08,0x00,0x00,0x3A,0x86
        db      0x00,0xDF,0x7A,0x00
z80_stub:                               ; laeuft auf dem Z80 ab F111h
        db      0xD3,0xDF               ; OUT (DFh),A   Koprozessor starten (wie NCR)
        db      0x3E,0x5A               ; LD A,5Ah
        db      0x32                    ; LD (Z80M),A   Marke: Z80 war da
        dw      Z80M
        db      0xD3,0xD0               ; OUT (D0h),A   Bus an den 8088
        db      0xC3                    ; JP F111h      wieder kreisen
        dw      Z80_RESUME
Z80STUBLEN equ  $-z80_stub
z80_idle:                               ; Urzustand: JP F111h
        db      0xC3
        dw      Z80_RESUME

copro_run:
        mov     byte [c_ran],0
        cmp     word [j_card],s_k235    ; nur mit K235 (8259 und ROM 33473)
        jne     cr_e
        call    cls
        mov     si,s_cp_go
        call    puts
        cli
        ; Vektoren 0-9 (0:0000-0:0027) und den Z80-Warteplatz retten
        mov     si,0
        mov     di,c_save
        mov     cx,40
        rep     movsb
        mov     si,copro_probe
        xor     di,di
        mov     cx,40
        rep     movsb
        mov     byte [Z80M],0
        mov     si,z80_stub
        mov     di,Z80_RESUME
        mov     cx,Z80STUBLEN
        rep     movsb
        out     P_SWITCH16,al           ; Bus an den Z80; der 8088 steht
        nop                             ; hier, bis der Z80 zurueckgibt
        nop
        mov     al,[0]                  ; Ergebnis der Sonde
        mov     [c_res],al
        mov     al,[Z80M]
        mov     [c_z80],al
        mov     si,z80_idle             ; Warteplatz wieder herstellen
        mov     di,Z80_RESUME
        mov     cx,3
        rep     movsb
        mov     si,c_save
        xor     di,di
        mov     cx,40
        rep     movsb
        mov     byte [c_ran],1
cr_e:   ret

pg_copro:
        mov     si,s_cp1
        call    puts
        call    nl
        call    nl
        mov     si,s_cp2
        call    puts
        call    nl
        mov     si,s_cp3
        call    puts
        call    nl
        mov     si,s_cp4
        call    puts
        call    nl
        call    nl
        mov     si,s_cpk
        call    puts
        mov     si,[j_card]
        call    puts
        cmp     word [j_card],s_k235
        je      pc_ok
        mov     si,s_cpno
        call    puts
        call    nl
        ret
pc_ok:  call    nl
        call    nl
        cmp     byte [c_ran],1
        je      pc_res
        mov     si,s_cpp
        call    puts
        call    nl
        mov     si,s_cpw
        call    puts
        call    nl
        ret
pc_res: mov     si,s_cpr
        call    puts
        mov     al,[c_res]
        call    hex8
        mov     si,s_cpz
        call    puts
        mov     al,[c_z80]
        call    hex8
        call    nl
        mov     si,s_cpj
        call    puts
        mov     al,[c_res]
        mov     si,s_cp68
        cmp     al,'6'
        je      pc_j
        mov     si,s_cpz8
        cmp     al,'Z'
        je      pc_j
        mov     si,s_cpnone
        cmp     al,0
        je      pc_j
        mov     si,s_unk
pc_j:   call    puts
        call    nl
        cmp     byte [c_z80],0x5A
        je      pc_z
        mov     si,s_cpnz
        call    puts
        call    nl
pc_z:   ret

;--- Seite 6: Deutung ------------------------------------------------------
pg_sum:
        mov     si,s_p6
        call    puts
        call    nl
        call    nl
        mov     si,s_sr
        call    puts
        mov     si,[j_rom]
        call    puts
        call    nl
        mov     si,s_ss
        call    puts
        mov     si,[j_gfx]
        call    puts
        mov     si,s_ssn
        call    puts
        call    nl
        mov     si,s_sc
        call    puts
        mov     si,s_z80
        call    puts
        mov     si,[j_cpu]
        call    puts
        call    space
        mov     si,[j_fpu]
        call    puts
        call    nl
        mov     si,s_sk
        call    puts
        mov     si,[j_card]
        call    puts
        call    nl
        mov     si,s_st
        call    puts
        mov     ax,[j_mhz]
        or      ax,ax
        jz      p6_tu
        call    dec2c
        mov     si,s_mhz
        jmp     short p6_tp
p6_tu:  mov     si,s_unk
p6_tp:  call    puts
        call    nl
        mov     si,s_sm
        call    puts
        mov     al,[j_banks]
        cbw
        inc     ax
        mov     cl,6
        shl     ax,cl
        call    dec16
        mov     si,s_kb
        call    puts
        call    nl
        mov     si,s_s68
        call    puts
        cmp     byte [c_ran],1
        jne     p6_68n
        mov     al,[c_res]
        mov     si,s_cp68
        cmp     al,'6'
        je      p6_68p
        mov     si,s_cpz8
        cmp     al,'Z'
        je      p6_68p
        mov     si,s_cpnone
        cmp     al,0
        je      p6_68p
        mov     si,s_unk
        jmp     short p6_68p
p6_68n: mov     si,s_s68b
p6_68p: call    puts
        call    nl
        call    nl
        mov     si,s_sopen
        call    puts
        call    nl
        mov     si,s_sopen2
        call    puts
        call    nl
        mov     si,s_sopen3
        call    puts
        call    nl
        call    nl
        mov     si,s_foto
        call    puts
        call    nl
        ret

;=====================================================================
;  TABELLEN UND TEXTE
;=====================================================================

; CRC-32 der Hauptplatinen-ROMs (8 KB) -> Name
romtab  dd      0x6624610E
        dw      s_r32664
        dd      0x7796519E
        dw      s_r32676
        dd      0xD5CEB559
        dw      s_r32838
        dd      0x120951B6
        dw      s_r33609
        dd      0xBF25F3F0
        dw      s_r33610
        dd      0
crc_33473 dd    0x104195DC
crc_32167 dd    0xF4A58880

s_r32664 db     'M.05.00 (32664)',0
s_r32676 db     'M.06.00 (32676)',0
s_r32838 db     'C.06.00 (32838)',0
s_r33609 db     'M.07.00 (33609)',0
s_r33610 db     'C.07.00 (33610)',0
s_rom33473 db   '33473 (K235)',0
s_rom32167 db   '32167 (K230/K231)',0

countries dw    c_us,c_uk,c_dk,c_de,c_se,c_no,c_es,c_it
c_us    db      'US',0
c_uk    db      'UK/Int.',0
c_dk    db      'Daenisch',0
c_de    db      'Deutsch',0
c_se    db      'Schwedisch/Finnisch',0
c_no    db      'Norwegisch',0
c_es    db      'Spanisch',0
c_it    db      'Italienisch',0

%ifdef INFO
s_hello db      'DMVINFO ',VERSION,' - messe ...',0
s_bye   db      'DMVINFO beendet - Neustart ...',0x88,0
s_head  db      'DMVINFO ',VERSION,'  ',0
s_headp db      'Seite ',0
%elifdef FRAGE
s_hello db      'DMVFRAGE ',VERSION,' - messe ...',0
s_bye   db      'DMVFRAGE beendet - Neustart ...',0x88,0
s_head  db      'DMVFRAGE ',VERSION,'  ',0
%else
s_hello db      'DMVDIAG ',VERSION,' - messe ...',0
s_bye   db      'DMVDIAG beendet - Neustart ...',0x88,0
s_head  db      'DMVDIAG ',VERSION,'  Seite ',0
%endif
s_rule  db      '--------------------------------------------------------------------------------',0
%ifdef INFO
s_foot  db      ' Taste: weiter  Z: zurueck  W: neu messen  X: Uebersicht ',0
%else
s_foot  db      ' Taste: weiter  Z: zurueck  W: neu messen  X: Ende  ',0
%endif
s_foot_a db     'Sichern auto',0
s_foot_d db     'D: Sichern an',0
s_foot1 db      ' Aufbau ',0
s_foot2 db      ' s',0
s_ind   db      '  ',0
s_arrow db      3,'  -> ',0
s_unk   db      'unbekannt',0
s_da    db      'da',0
s_fehlt db      'fehlt',0
s_mirror db     'Muster von Bank ',0
s_ms    db      ' ms',0
s_kb    db      ' KB',0

s_8088  db      '8088',0
s_v20   db      'V20',0
s_unk_cpu db    'CPU?',0
s_8087  db      '8087',0
s_nofpu db      'ohne 8087',0
s_unk_fpu db    '8087?',0
s_k235  db      'K235',0
s_k230  db      'K230/K231',0
s_card_unk db   'Karte?',0

s_p1    db      4,'SYSTEM - Rohwerte',0
s_fwraw db      2,'ROM 0FF7h-0FFFh:   ',1,0
s_crcmb db      2,'CRC32 Hauptplatinen-ROM (8 KB): ',1,0
s_crc88 db      2,'CRC32 ROM der 8088-Karte (4 KB): ',1,0
s_mbox  db      2,'Postfach 0:FE00-FE3F (FE06 Baenke, FE07 Start, FE08/09 siehe Seite 7):',0
s_mbnote db     2,'  FE20-FE3F ist nur nach Start von Platte gueltig.',0
s_kbd   db      2,'Tastatur, Antwort auf 01h:  Status ',1,0
s_kbd2  db      2,'  Daten ',1,0
s_kbd3  db      2,'  nach etwa ',1,0
s_kbd4  db      ' us',0
s_kbdj  db      3,'  -> Landescode ',0
s_kbdv1 db      2,' (Version-1-Belegung)',0
s_p2    db      4,'PROZESSOR - Rohwerte',0
s_push  db      2,'PUSH imm8: SP-Differenz ',1,0
s_push2 db      2,'h   (8088: 00, V20: 02)',0
s_aad   db      2,'AAD 16 auf 0102h: AL = ',1,0
s_aad2  db      2,'h   (8088: 12, V20: 0C)',0
s_cpuj  db      3,'  -> ',0
s_fpu1  db      2,'8087: Statuswort ',1,0
s_fpu2  db      2,'  Steuerwort ',1,0
s_fpu3  db      2,'  (vorbesetzt 5A5A; echter 8087: SW 0000, CW 03FF; MAME: CW 037F)',0
s_fpuj  db      3,'  -> ',0
s_pic1  db      2,'8259 auf 91h: vorher ',1,0
s_pic2  db      2,'  AA zurueck ',1,0
s_pic3  db      2,'  55 zurueck ',1,0
s_crc88b db     2,'ROM der 8088-Karte: ',1,0
s_cardj db      3,'  -> ',0
s_cardn db      2,'  (K230 und K231 sind elektrisch gleich; K234/K238 hier nicht sichtbar)',0
s_clk1  db      2,'Takt: leere Schleife ',1,0
s_clk2  db      2,', 1600 x AAM ',1,0
s_clk3  db      2,' Schritte zu 2 us, netto ',1,0
s_clk4  db      ' us',0
s_clkj  db      3,'  -> ',0
s_mhz   db      ' MHz',0
s_mhz2  db      2,'   (gerechnet mit ',0
s_mhz3  db      2,' Takten je AAM)',0
s_p3    db      4,'SPEICHER - Rohwerte',0
s_fe06  db      2,'Postfach FE06 (Erweiterungsbaenke laut Z80-ROM): ',1,0
s_fe06b db      'h',0
s_bankh db      2,'Bank  geschrieben  gelesen      bei n:FFF0',0
s_bank2 db      '000h  ',0
s_bank3 db      '  ',0
s_memj  db      3,'  -> zusammen ',0
s_k200  db      ', Karte K200',0
s_k202  db      ', Karte K202',0
s_k208  db      ', Karte K208',0
s_nok   db      ', keine Speicherkarte',0
s_memn  db      2,'  (nur zusammenhaengende Baenke ab 1 zaehlen)',0
s_kk0   db      2,'K233 (Shared RAM C000h): Taste K startet die Probe (kippt 7Fh)',0
s_kk1   db      2,'K233: C000 vorher ',1,0
s_kk2   db      2,'  aus ',1,0
s_kk3   db      2,'  ein ',1,0
s_kk4   db      2,'  durch ',1,0
s_kk5   db      2,'  7Bh ',1,0
s_kyes  db      'K233 da',0
s_kno   db      'keine K233',0
s_kwt1  db      ', Schreiben geht auch in die Hauptplatine',0
s_kwt0  db      ', Schreiben nur in die Karte (wie bisher in MAME)',0
s_kwtu  db      ', Schreibweg unklar',0
s_p4    db      4,'MODULE - Rohwerte der IFSEL-Anschluesse x0-x3 (nur gelesen)',0
s_p4a   db      2,'je Anschluss drei Lesungen: Bus vorher 00 / FF / 5A',0
s_p4h   db      2,'  IF Basis  x0       x1       x2       x3       Antwort',0
s_ff    db      'nur FF',0
s_float db      'schwebt',0
s_stable db     'stabil',0
s_echo  db      'Echo = frei',0
s_p4n1  db      2,'stabil = x1 und x3 je dreimal gleich, nicht nur FF. schwebt = sonst.',0
s_p4n2  db      2,'Ein Kartenname folgt erst aus Messungen mit bekannter Bestueckung.',0
s_p4n3  db      2,'4A (Platte) siehe Seite 7. x4-x7 werden nie gelesen (2651!).',0
s_p5    db      4,'LAUFWERKE, PLATTE, UHR - Rohwerte',0
s_fdcms db      2,'FDC Hauptstatus 50h: ',1,0
s_sys13 db      2,'   Anschluss 13h: ',1,0
s_fdcsi db      2,'SENSE INTERRUPT (offene Meldungen): ',1,0
s_st3h  db      2,'SENSE DRIVE STATUS, ST3 je Laufwerk:',0
s_t0y   db      'Spur0 ',0
s_t0n   db      '      ',0
s_ryy   db      'bereit ',0
s_ryn   db      '       ',0
s_wpy   db      'schreibgesch.',0
s_wpn   db      '',0
s_fdcbusy db    'FDC nicht bereit',0
s_st3n  db      2,'  (ohne Diskette ist "bereit" aus - Anwesenheit folgt daraus nicht)',0
s_fe08  db      2,'Z80-ROM, ID auf phys. Zyl. 2 von A: FE08 ',1,0
s_fe09  db      2,'  FE09 ',1,0
s_fe0x  db      3,'  -> A: ',0
s_a40   db      '40 Spuren',0
s_a80   db      '80 Spuren (Doppelschritt)',0
s_hd1   db      2,'Platte C2h/C3h: 55/AA geschrieben, zurueck ',1,0
s_hd1b  db      2,'  ',0
s_hdyes db      3,'-> Controller da',0
s_hdno  db      3,'-> kein Controller (C2 C3 = Echo, FF = MAME)',0
s_hdu   db      2,'  Einheit ',1,0
s_hdu2  db      2,': C7h sofort ',1,0
s_hdu3  db      2,'  nach Warten ',1,0
s_hdu4  db      2,'  nach ',1,0
s_hdrdy db      '  bereit',0
s_rtc   db      2,'Uhr K803, Sekunden (CEh): ',1,0
s_rtc2  db      2,'  1,2 s spaeter ',1,0
s_rtc3  db      3,'  -> ',0
s_rtcok db      'laeuft',0
s_rtcst db      'steht',0

s_p6    db      4,'DEUTUNG (nur was die Rohwerte sicher tragen)',0
s_sr    db      2,'Hauptplatinen-ROM:  ',3,0
s_ss    db      2,'Bildschirm:         ',3,0
s_dv1   db      4,'LAUFWERKE - aktive Proben (Tasten M, T, B in dieser Reihenfolge)',0
s_dv2   db      2,'T: A: und B: je RECALIBRATE, SEEK 2, RECALIBRATE - hoechstens zwei',0
s_dv3   db      2,'   leise Schritte. Erwartet ohne Diskette: nicht bereit (RDY = READY).',0
s_dv4   db      2,'B: DMVDIAG-Diskette in B: einlegen, READ ID auf physischem Zylinder 2.',0
s_dvt   db      2,'Taste T: Schrittprobe starten',0
s_dvb   db      2,'Taste B: Spurzahl von B: mit eingelegter DMVDIAG-Diskette',0
s_dvh   db      2,'       RECAL ST0/PCN/ST3   SEEK 2 ST0/PCN/ST3  RECAL ST0/PCN/ST3',0
s_dvnr  db      'nicht bereit - ohne Diskette kein Schritt (wie erwartet)',0
s_dvok  db      'steppt, Spur 0 aus und wieder an: Laufwerk da',0
s_dt_go db      'Schrittprobe laeuft ...',0
s_db_go db      'READ ID in B: ...',0
s_dbh   db      2,'B: RECAL und SEEK 2, ST0/PCN: ',1,0
s_dbno  db      3,'  -> keine Kennung gelesen (keine Diskette oder nicht bereit)',0
s_dbid  db      2,'READ ID: ST0 ST1 ST2 C H R N  ',1,0
s_db40  db      '40-Spur-Werk (Kennung C=2 auf physischem Zylinder 2)',0
s_db80  db      '80-Spur-Werk (C=1 bei 40-Spur-Diskette oder N=3)',0
s_cp1   db      4,'KOPROZESSOR K234 (68008) / K238 (Z8001) - aktive Probe',0
s_cp2   db      2,'Nur auf Taste P. Der Z80 startet die Karte mit OUT DFh, eine',0
s_cp3   db      2,'zweisprachige Sonde an 0:0000 schreibt 6 (68008) oder Z (Z8001)',0
s_cp4   db      2,'und laeutet zurueck. Steht danach alles: Foto, das ist ein Befund.',0
s_cpk   db      2,'CPU-Karte: ',1,0
s_cpno  db      0x88,3,'  -> gesperrt: braucht K235 (K230/K231 nimmt D8h als D0h)',0
s_cpp   db      2,'Taste P: Probe starten',0
s_cpw   db      2,'(Vektoren 0-9 und der Z80-Warteplatz werden gerettet und zurueckgeschrieben)',0
s_cp_go db      'Koprozessor-Probe laeuft ...',0
s_cpr   db      2,'Sonde 0:0000 danach: ',1,0
s_cpz   db      2,'   Marke des Z80 (5A = zurueckgekommen): ',1,0
s_cpj   db      3,'  -> ',0
s_cp68  db      '68008 antwortet (K234)',0
s_cpz8  db      'Z8001 antwortet (K238)',0
s_cpnone db     'nichts angelaufen',0
s_cpnz  db      '  Der Z80-Teil ist nicht zurueckgekommen - Ablauf pruefen!',0
s_ssn   db      2,'  (Firmware und Ebenenprobe, Seite 1)',0
s_gfx_col db    'Farbe',0
s_gfx_mono db   'Mono',0
s_gfx_unk db    'Schirm?',0
s_g1    db      2,'GDC Wort 3FF0h vorher ',1,0
s_g1b   db      2,'  7FF0h vorher ',1,0
s_g1c   db      2,'  RDAT-Rest ',1,0
s_g1d   db      2,'  Status ',1,0
s_g2    db      2,'  nach A5C3->3FF0h, 3C5A->7FF0h: 3FF0h ',1,0
s_g2b   db      2,'  7FF0h ',1,0
s_gto   db      '  ZEIT UM',0
s_g3    db      3,'  -> Ebenenprobe ',0
s_g3b   db      2,', mit Firmware: ',3,0
s_gp_col db     'drei Ebenen',0
s_gp_mono db    'eine Ebene',0
s_sc    db      2,'Prozessoren:        ',3,0
s_z80   db      'Z80 ',0
s_sk    db      2,'CPU-Karte:          ',3,0
s_st    db      2,'Takt 8088/V20:      ',3,0
s_sm    db      2,'Speicher:           ',3,0
s_s68   db      2,'K234/K238:          ',3,0
s_s68b  db      'nicht geprueft (Seite 9, Taste P)',0
s_sopen db      2,'Module, Laufwerke, Platte: Regeln werden erst aus den Fotos',0
s_sopen2 db     2,'mit bekannter Bestueckung abgeleitet. Bitte jedes Foto mit',0
s_sopen3 db     2,'Maschine und Steckplatzbelegung schicken.',0
s_foto  db      4,'>>> FOTO: diese Seite fotografieren (Maschine und Belegung notieren) <<<',0
;=====================================================================
;  VARIABLEN
;=====================================================================
align 2
%include "ports.inc"
%include "results.inc"
%include "motor.inc"
%include "irqtest.inc"
%ifdef FRAGE
%include "frage.inc"
%endif
%ifdef INFO
%include "info.inc"
%endif

page    db      0
pg_time dw      0
chbuf   db      0,0
buscell dw      0
fwtext  times 10 db 0
r_fwraw times 10 db 0
r_crcmb dd      0
r_crc88 dd      0
r_kbst  dw      0
r_kbt   dd      0
r_push  db      0
r_aad   db      0
r_fsw   dw      0
r_fcw   dw      0
r_pic0  db      0
r_pic1  db      0
r_pic2  db      0
r_tempty dw     0
r_taam  dw      0
r_tus   dw      0
r_aamclk dw     0
r_bank  times 28 db 0
in3c    db      0
p4_stable db    0
p4_allff db     0
p4_echo db      0
j_cpu   dw      0
j_fpu   dw      0
j_card  dw      0
j_rom   dw      0
j_pic   db      0
j_banks db      0
j_gprobe db     0
c_ran   db      0
k_ran   db      0
k_m0    db      0
k_r1    db      0
k_r2    db      0
k_wt    db      0
k_7b    db      0
d_tran  db      0
d_bran  db      0
d_bok   db      0
d_res   times 18 db 0
d_bst0  dw      0,0
d_bid   times 7 db 0
c_res   db      0
c_z80   db      0
c_save  times 40 db 0
j_gfx   dw      s_gfx_unk
r_gok   db      0
r_gex   db      0
r_gst   db      0
g_extra db      0
r_g0    dw      0
r_g1    dw      0
r_g2    dw      0
r_g3    dw      0
j_mhz   dw      0
tk_last dw      0
tk_lo   dw      0
tk_hi   dw      0
tk_s0   dw      0
tk_s1   dw      0

prog_end:
; Grenze: das Programm muss unter BF00h enden (K233-Fenster ab C000h).
; mkimg.py prueft das beim Bau.

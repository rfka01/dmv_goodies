# GCRTDOS.S - Startstueck und Hilfen fuer C-Programme mit DMVGRAF unter
# MS-DOS (8088/V20), gcc-ia16, Speichermodell small (CS eigenes Segment,
# DS = SS = ES = Daten, 64 KB). Ohne fremde Bibliothek. Mit -DTINY
# entsteht GCRTCOM.O fuer das Speichermodell tiny (.COM, alles in einem
# Segment).
#
#   start                 Einsprung (CS:IP aus dem EXE-Kopf, SS:SP auf
#                         das Ende des Datensegments): DS/ES setzen, BSS
#                         loeschen, Kommandozeile aus dem PSP wie eine
#                         CP/M-Basisseite (Laenge 80h, Zeichen ab 81h) an
#                         gcmain (GARG68.C), Ende ueber INT 21h/4Ch
#   gdos(r)               INT 21h mit AX, BX, CX, DX aus/nach r[0..3];
#                         Ergebnis die Flaggen (Bit 0 CF, Bit 6 ZF)
#   inb(p), outb(p, w)    Anschluss lesen/schreiben
#   gromz()               Kennbuchstabe der Firmware (0FF9h: 'C' Farbe,
#                         'M' Mono): OUT 11h legt das ROM ueber 0-1FFFh,
#                         OUT 10h zurueck, dazwischen keine Unterbrechung
#   gfput(seg, off, q, n) n Byte von DS:q nach seg:off
#   gfget(z, seg, off, n) n Byte von seg:off nach DS:z
#   gfmov(seg, z, q, n)   innerhalb seg, z < q
#   gdlst(liste)          GDC-Liste wie BIOS 66 an den 7220 (A1h Befehl,
#                         A0h Parameter, vor jedem Byte warten, bis das
#                         FIFO nicht voll ist)
#   memcpy, memmove, memset
#   gcli(), gfl(f)        Unterbrechungen sperren (liefert die alten
#                         Flaggen), Flaggen zurueck
#   gpit(k)               8253-Kanal k (0 oder 2) gelatcht lesen
#   gpit2()               Kanal 2 auf Modus 2, voller Umfang (ohne DOS)
#   gcpu()                Bit 0: PUSH 0 (6Ah 00h) senkt SP (V20),
#                         Bit 1: AAD 16 auf 0102h gibt 0Ch (V20)
#   gfpu(w)               w[0] Status-, w[1] Steuerwort nach FNINIT,
#                         beide vorher 5A5Ah
#   gaam(n), gaam0(n)     n-mal AAM, die leere Schleife (Taktmessung)
#   grom(puffer, v, n)    n Byte Firmware ab v (OUT 11h) nach DS:puffer
#   gdsg()                Datensegment
#   gblit(p)              Bild direkt an den 7220 (wie BlitAsm)
#
# Aufruf aus gcc-ia16: Argumente auf dem Stapel ab [sp+2], Ergebnis AX;
# SI, DI, BP, DS bleiben erhalten (ES hier auch).
#
# Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT

        .arch   i8086
        .code16
        .intel_syntax noprefix

        .section .text.start,"ax"
        .global start
#ifdef TINY
# Speichermodell tiny (.COM): CS = DS = ES = SS = PSP, Anfang bei 100h.
# Der Speicher wird auf 64 KB gekuerzt, damit DMVGRAF mit INT 21h/48h
# Bildspeicher bekommt; die Kommandozeile steht schon bei 80h.
start:  cld
        mov     di, offset __bss_start  # BSS loeschen
        mov     cx, offset _end
        sub     cx, di
        xor     al, al
        rep     stosb
        mov     ax, cs
        mov     gpsp, ax
        mov     ah, 0x4A                # auf 64 KB kuerzen (ES = PSP)
        mov     bx, 0x1000
        int     0x21
        xor     ax, ax                  # Basisseite = PSP
        push    ax
        call    gcmain
        add     sp, 2
        mov     ah, 0x4C
        int     0x21
        jmp     start
#else
start:  mov     bx, es                  # PSP
        mov     ax, ss
        mov     ds, ax
        mov     es, ax
        cld
        mov     di, offset __bss_start  # BSS loeschen
        mov     cx, offset _end
        sub     cx, di
        xor     al, al
        rep     stosb
        mov     gpsp, bx
        push    ds                      # Kommandozeile PSP:80h -> gbase+80h
        mov     si, 0x80
        mov     di, offset gbase+0x80
        mov     cx, 128
        mov     ds, bx
        rep     movsb
        pop     ds
        mov     ax, offset gbase
        push    ax
        call    gcmain
        add     sp, 2
        mov     ah, 0x4C
        int     0x21
        jmp     start

#endif

        .text
        .global gdos
gdos:   push    bp
        mov     bp, sp
        push    si
        push    di
        push    es
        mov     si, [bp+4]
        mov     ax, [si]
        mov     bx, [si+2]
        mov     cx, [si+4]
        mov     dx, [si+6]
        push    si
        int     0x21
        pop     si
        mov     [si], ax
        mov     [si+2], bx
        mov     [si+4], cx
        mov     [si+6], dx
        pushf
        pop     ax
        cld
        pop     es
        pop     di
        pop     si
        pop     bp
        ret

        .global inb
inb:    mov     bx, sp
        mov     dx, [bx+2]
        in      al, dx
        xor     ah, ah
        ret

        .global gromz
gromz:  pushf
        cli
        push    ds
        xor     ax, ax
        mov     ds, ax
        out     0x11, al
        mov     bl, [0x0FF9]
        out     0x10, al
        pop     ds
        popf
        mov     al, bl
        xor     ah, ah
        ret

        .global outb
outb:   mov     bx, sp
        mov     dx, [bx+2]
        mov     al, [bx+4]
        out     dx, al
        ret

        .global gfput
gfput:  push    bp
        mov     bp, sp
        push    si
        push    di
        push    es
        mov     es, [bp+4]
        mov     di, [bp+6]
        mov     si, [bp+8]
        mov     cx, [bp+10]
        rep     movsb
        pop     es
        pop     di
        pop     si
        pop     bp
        xor     ax, ax
        ret

        .global gfget
gfget:  push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        push    es
        mov     ax, ds                  # Ziel im eigenen Datensegment
        mov     es, ax
        mov     di, [bp+4]
        mov     si, [bp+8]
        mov     cx, [bp+10]
        mov     ds, [bp+6]
        rep     movsb
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     bp
        xor     ax, ax
        ret

        .global gfmov
gfmov:  push    bp
        mov     bp, sp
        push    si
        push    di
        push    ds
        push    es
        mov     di, [bp+6]
        mov     si, [bp+8]
        mov     cx, [bp+10]
        mov     ax, [bp+4]
        mov     ds, ax
        mov     es, ax
        rep     movsb
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     bp
        xor     ax, ax
        ret

# gdlst(liste): Eintraege n, Befehl, n-1 Parameter; n = 0 Ende
        .global gdlst
gdlst:  push    bp
        mov     bp, sp
        push    si
        mov     si, [bp+4]
gl1:    lodsb
        or      al, al
        jz      gl9
        mov     cl, al
        xor     ch, ch
gl2:    in      al, 0xA0                # FIFO voll?
        test    al, 2
        jnz     gl2
        lodsb
        out     0xA1, al                # Befehl
        dec     cx
        jz      gl1
gl3:    in      al, 0xA0
        test    al, 2
        jnz     gl3
        lodsb
        out     0xA0, al                # Parameter
        loop    gl3
        jmp     gl1
gl9:    pop     si
        pop     bp
        xor     ax, ax
        ret

        .global memcpy
        .global memmove
memmove:
memcpy: push    bp
        mov     bp, sp
        push    si
        push    di
        push    es
        mov     ax, ds
        mov     es, ax
        mov     di, [bp+4]
        mov     si, [bp+6]
        mov     cx, [bp+8]
        mov     ax, di
        cmp     di, si
        ja      mc2
        rep     movsb
        jmp     mc9
mc2:    add     di, cx                  # Ziel hinter der Quelle: von hinten
        add     si, cx
        dec     di
        dec     si
        std
        rep     movsb
        cld
mc9:    pop     es
        pop     di
        pop     si
        pop     bp
        ret

        .global memset
memset: push    bp
        mov     bp, sp
        push    di
        push    es
        mov     ax, ds
        mov     es, ax
        mov     di, [bp+4]
        mov     al, [bp+6]
        mov     cx, [bp+8]
        mov     dx, di
        rep     stosb
        mov     ax, dx
        pop     es
        pop     di
        pop     bp
        ret

        .global gcli
gcli:   pushf
        pop     ax
        cli
        ret

        .global gfl
gfl:    mov     bx, sp
        push    word ptr [bx+2]
        popf
        ret

        .global gpit
gpit:   mov     bx, sp
        mov     dx, [bx+2]
        pushf
        cli
        mov     al, 0x80                # Kanal 2 latchen
        or      dx, dx
        jnz     1f
        xor     al, al                  # Kanal 0
1:      out     0x83, al
        add     dx, 0x80
        in      al, dx
        mov     ah, al
        in      al, dx
        xchg    al, ah
        popf
        ret

        .global gpit2
gpit2:  pushf
        cli
        mov     al, 0xB4                # Kanal 2, beide Bytes, Modus 2
        out     0x83, al
        xor     al, al
        out     0x82, al
        out     0x82, al
        popf
        ret

        .global gcpu
gcpu:   push    bp
        mov     bp, sp
        mov     dx, sp
        .byte   0x6A, 0x00              # V20: PUSH 0, 8088: JPE $+2
        mov     cx, sp
        mov     sp, bp
        xor     bx, bx
        cmp     cx, dx
        je      1f
        or      bl, 1                   # SP gesunken: V20
1:      mov     ax, 0x0102
        .byte   0xD5, 0x10              # AAD 16: 8088 12h, V20 0Ch
        cmp     al, 0x0C
        jne     2f
        or      bl, 2
2:      mov     ax, bx
        pop     bp
        ret

        .global gfpu
gfpu:   push    bp
        mov     bp, sp
        push    si
        mov     si, [bp+4]
        mov     word ptr [si], 0x5A5A
        mov     word ptr [si+2], 0x5A5A
        .byte   0xDB, 0xE3              # FNINIT
        mov     cx, 50
1:      loop    1b
        .byte   0xDD, 0x3C              # FNSTSW [si]
        mov     cx, 50
2:      loop    2b
        .byte   0xD9, 0x7C, 0x02        # FNSTCW [si+2]
        mov     cx, 50
3:      loop    3b
        pop     si
        pop     bp
        ret

        .global gaam
gaam:   mov     bx, sp
        mov     cx, [bx+2]
1:      aam
        loop    1b
        ret

        .global gaam0
gaam0:  mov     bx, sp
        mov     cx, [bx+2]
1:      nop
        loop    1b
        ret

        .global grom
grom:   push    bp                      # grom(puffer, versatz, n)
        mov     bp, sp
        push    si
        push    di
        push    ds
        push    es
        mov     di, [bp+4]
        mov     si, [bp+6]
        mov     cx, [bp+8]
        mov     ax, ds
        mov     es, ax
        pushf
        cli
        xor     ax, ax
        mov     ds, ax
        out     0x11, al                # Firmware ueber 0-1FFFh
        cld
        rep     movsb
        out     0x10, al
        popf
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     bp
        ret

        .global gdsg
gdsg:   mov     ax, ds
        ret

# qshz(d, m, b, k, i0, n8, sh, letzt) -> 1, wenn die Daten leer sind.
# Bitzeile b (n8 Byte, letztes mit letzt maskiert) um sh Bit (1-7) nach
# links versetzt nach d (k Byte), ab Quellbyte i0 (0 oder -1); m bekommt
# dasselbe fuer eine Zeile aus Einsen (GRAF.C, qshift)
        .global qshz
qshz:   push    bp
        mov     bp, sp
        push    si
        push    di
        mov     di, [bp+4]
        mov     bx, [bp+6]
        mov     si, [bp+8]
        add     si, [bp+12]
        mov     ax, [bp+8]
        add     ax, [bp+14]
        dec     ax
        push    ax                      # [bp-6] letztes Quellbyte
        xor     ax, ax
        push    ax                      # [bp-8] ODER der Daten
        mov     cl, [bp+16]
        xor     dx, dx                  # DL, DH: Uebertrag Daten, Maske
1:      xor     ax, ax
        cmp     si, [bp+8]
        jb      3f
        cmp     si, [bp-6]
        ja      3f
        mov     al, [si]
        mov     ah, 0xFF
        jne     3f
        and     al, [bp+18]
        mov     ah, [bp+18]
3:      push    ax
        xor     ah, ah
        shl     ax, cl
        or      al, dl
        mov     dl, ah
        mov     [di], al
        or      [bp-8], al
        inc     di
        pop     ax
        mov     al, ah
        xor     ah, ah
        shl     ax, cl
        or      al, dh
        mov     dh, ah
        mov     [bx], al
        inc     bx
        inc     si
        dec     word ptr [bp+10]
        jnz     1b
        xor     ax, ax
        cmp     byte ptr [bp-8], 0
        jne     4f
        inc     ax
4:      lea     sp, [bp-4]
        pop     di
        pop     si
        pop     bp
        ret

# gblit(p): ganzes Bild direkt an den 7220 wie HOPPLERs BlitAsm (aus
# tools/inline/I86BLIT.ASM). p: Quelle Segment, Versatz, Zeilen,
# Zeilenabstand, Worte je Ebene, Maske fuer das letzte Wort, Wortadresse
# der ersten Zeile, Zahl der Eintraege, Tabelle (je 6 Worte, siehe dort).
        .global gblit
gblit:  push    bp
        mov     bp, sp
        push    si
        push    di
        push    es
        mov     si, [bp+4]
        mov     di, offset gbxv
        mov     ax, ds
        mov     es, ax
        mov     cx, 44
        cld
        rep     movsw
.macro par   # AL an A0h, vorher FIFO abwarten (AH frei)
1:    mov     ah, al
        in      al, 0xA0
        test    al, 2
        mov     al, ah
        jnz     1b
        out     0xA0, al
.endm
.macro pdl   # DL an A0h
1:    in      al, 0xA0
        test    al, 2
        jnz     1b
        mov     al, dl
        out     0xA0, al
.endm
.macro pdh   # DH an A0h
1:    in      al, 0xA0
        test    al, 2
        jnz     1b
        mov     al, dh
        out     0xA0, al
.endm
.macro bef   # AL an A1h
1:    mov     ah, al
        in      al, 0xA0
        test    al, 2
        mov     al, ah
        jnz     1b
        out     0xA1, al
.endm
gb_anfang: push    bp
        push    es
        mov     es, [gbxv+0]
        mov     si, [gbxv+2]
        mov     al, 0x4A   # MASK FFFF
        bef
        mov     al, 0xFF
        par
        par
        mov     al, 0x4C   # FIGS 02 00 00
        bef
        mov     al, 2
        par
        xor     al, al
        par
        par
        mov     ax, [gbxv+4]
        mov     [gbxv+90], ax
gb_zeile:  mov     bx, offset gbxv+16
        mov     ax, [gbxv+14]
        mov     [gbxv+88], ax
gb_tupel:  mov     al, 0x49   # CURS Wortadresse
        bef
        mov     dx, [gbxv+12]
        add     dx, [bx]
        mov     al, dl
        par
        mov     al, dh
        par
        mov     al, [bx+2]   # WDAT
        bef
        mov     cx, [gbxv+8]
        dec     cx   # letztes Wort extra (Maske hwXm)
        push    bx
        push    si
        cmp     word ptr [bx+10], 0
        jne     gb_null
        mov     di, [bx+4]   # Maske: 0 = die Zeile selbst
        mov     bp, [bx+6]   # X
        mov     bx, [bx+8]   # Versatz der Ebene
        or      di, di
        jnz     gb_nurp
        jcxz    gb_lz
gb_wortz:  mov     dx, es:[si+bx]   # (Ebene xor X) and Maske
        xor     dx, bp
        and     dx, es:[si]
        pdl
        pdh
        add     si, 2
        loop    gb_wortz
gb_lz:     mov     dx, es:[si+bx]
        xor     dx, bp
        and     dx, es:[si]
        and     dx, [gbxv+10]
        pdl
        pdh
        jmp     gb_fertig
gb_nurp:   jcxz    gb_lp
gb_wortp:  mov     dx, es:[si+bx]   # Maske FFh: nur die Ebene
        xor     dx, bp
        pdl
        pdh
        add     si, 2
        loop    gb_wortp
gb_lp:     mov     dx, es:[si+bx]
        xor     dx, bp
        and     dx, [gbxv+10]
        pdl
        pdh
        jmp     gb_fertig
gb_null:   inc     cx
        xor     dx, dx
gb_n1:     pdl
        pdh
        loop    gb_n1
gb_fertig: pop     si
        pop     bx
        add     bx, 12
        dec     word ptr [gbxv+88]
        jnz     gb_tupel
        add     si, [gbxv+6]
        add     word ptr [gbxv+12], 40
        dec     word ptr [gbxv+90]
        jnz     gb_zeile
        pop     es
        pop     bp
        pop     es
        pop     di
        pop     si
        pop     bp
        ret

        .data
        .global gpsp
gpsp:   .word   0

        .bss
gbxv:   .space  92
#ifndef TINY
gbase:  .space  256
#endif


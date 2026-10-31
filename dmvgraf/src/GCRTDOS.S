# GCRTDOS.S - Startstueck und Hilfen fuer C-Programme mit DMVGRAF unter
# MS-DOS (8088/V20), gcc-ia16, Speichermodell small (CS eigenes Segment,
# DS = SS = ES = Daten, 64 KB). Ohne fremde Bibliothek.
#
#   start                 Einsprung (CS:IP aus dem EXE-Kopf, SS:SP auf
#                         das Ende des Datensegments): DS/ES setzen, BSS
#                         loeschen, Kommandozeile aus dem PSP wie eine
#                         CP/M-Basisseite (Laenge 80h, Zeichen ab 81h) an
#                         gcmain (GARG68.C), Ende ueber INT 21h/4Ch
#   gdos(r)               INT 21h mit AX, BX, CX, DX aus/nach r[0..3];
#                         Ergebnis die Flaggen (Bit 0 CF, Bit 6 ZF)
#   inb(p), outb(p, w)    Anschluss lesen/schreiben
#   gfput(seg, off, q, n) n Byte von DS:q nach seg:off
#   gfget(z, seg, off, n) n Byte von seg:off nach DS:z
#   gfmov(seg, z, q, n)   innerhalb seg, z < q
#   gdlst(liste)          GDC-Liste wie BIOS 66 an den 7220 (A1h Befehl,
#                         A0h Parameter, vor jedem Byte warten, bis das
#                         FIFO nicht voll ist)
#   memcpy, memmove, memset
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

        .data
        .global gpsp
gpsp:   .word   0

        .bss
gbase:  .space  256

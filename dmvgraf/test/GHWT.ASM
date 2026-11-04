; GHWT.ASM - GHW.INC und GZEIT.INC aus MASM (COM), Gegenstueck zu
; KTEST.C: gh_schirm, gh_info mit Koprozessorprobe als Hexzeile (muss
; gleich sein wie bei KTEST), gh_zeile, mit K234 ein Lauf des 68008
; (gh_coblk, gh_vec, gh_corun: Summe 1 bis 100 = 5050), zum Schluss der
; Vergleich der 8088-Vektoren und des Warteplatzes.
;
; Bauen: jwasm -Zm -bin -Fo=GHWT.COM GHWT.ASM   (src im Suchpfad, -I)
; Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT

_TEXT   SEGMENT BYTE PUBLIC 'CODE'
        ASSUME  CS:_TEXT, DS:_TEXT, ES:_TEXT, SS:_TEXT
        ORG     100h
start:  jmp     haupt

        INCLUDE GZEIT.INC
        INCLUDE GHW.INC

t_tab   db      'tab:$'
t_68    db      '68008: $'
t_sum   db      ' Summe $'
t_gl    db      'Vektoren und Warteplatz gleich',13,10,'$'
t_ver   db      'Vektoren oder Warteplatz VERAENDERT',13,10,'$'
t_nl    db      13,10,'$'
m68     db      070h,000h,072h,064h,0D0h,041h,053h,041h,066h,0FAh
        db      041h,0FAh,0FEh,0F4h,030h,080h,011h,0C0h,0FFh,0DFh
        db      04Eh,072h,027h,000h
vor     db      55 dup (0)
nach    db      55 dup (0)

sichern PROC    NEAR                    ; DI: 0:0000 (40) und 0:F10E (15)
        push    ds
        push    es
        push    ds
        pop     es
        xor     ax,ax
        mov     ds,ax
        xor     si,si
        mov     cx,40
        rep     movsb
        mov     si,0F10Eh
        mov     cx,15
        rep     movsb
        pop     es
        pop     ds
        ret
sichern ENDP

puts    PROC    NEAR
        mov     ah,09h
        int     21h
        ret
puts    ENDP

hex4    PROC    NEAR                    ; AX
        push    cx
        mov     cx,4
hx1:    rol     ax,1
        rol     ax,1
        rol     ax,1
        rol     ax,1
        push    ax
        and     al,0Fh
        add     al,'0'
        cmp     al,'9'
        jbe     hx2
        add     al,7
hx2:    mov     dl,al
        mov     ah,02h
        int     21h
        pop     ax
        loop    hx1
        pop     cx
        ret
hex4    ENDP

dez     PROC    NEAR                    ; AX
        mov     bx,10
        xor     cx,cx
dz1:    xor     dx,dx
        div     bx
        push    dx
        inc     cx
        or      ax,ax
        jnz     dz1
dz2:    pop     dx
        add     dl,'0'
        mov     ah,02h
        int     21h
        loop    dz2
        ret
dez     ENDP

haupt:  cld
        mov     di,offset vor
        call    sichern
        call    gh_schirm
        mov     ax,GH_MITCOPRO
        call    gh_info
        mov     dx,offset t_tab
        call    puts
        xor     si,si
ht1:    mov     dl,' '
        mov     ah,02h
        int     21h
        mov     ax,gh_tab[si]
        call    hex4
        add     si,2
        cmp     si,GHI_UHR*2+2
        jb      ht1
        mov     dx,offset t_nl
        call    puts
        mov     ax,0100h                ; deutsch, ausgeben
        call    gh_zeile
        cmp     gh_tab[GHI_COPRO*2],2
        jne     ht5
        mov     bx,110h                 ; Block, Code nach P+100h
        call    gh_coblk
        jc      ht5
        mov     es,ax
        mov     di,100h
        mov     si,offset m68
        mov     cx,24
        rep     movsb
        mov     word ptr es:[0],0
        call    gh_vec
        mov     si,offset gh_stz
        mov     cx,8
        call    gh_corun
        push    ax
        mov     dx,offset t_68
        call    puts
        pop     ax
        call    dez
        mov     dx,offset t_sum
        call    puts
        mov     ax,es:[0]
        xchg    al,ah                   ; Big Endian
        call    dez
        mov     dx,offset t_nl
        call    puts
        push    ds
        pop     es
ht5:    mov     di,offset nach
        call    sichern
        mov     si,offset vor
        mov     di,offset nach
        mov     cx,55
        repe    cmpsb
        mov     dx,offset t_gl
        je      ht6
        mov     dx,offset t_ver
ht6:    call    puts
        mov     ax,4C00h
        int     21h

_TEXT   ENDS
        END     start

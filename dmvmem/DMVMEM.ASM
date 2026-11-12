;=====================================================================
; DMVMEM.COM  -  shows how the DMV's memory is used
;
;   DMVMEM        summary: DOS, command processor, resident programs
;                 (TSRs) with the interrupts they hook, free memory
;   DMVMEM /B     in addition every memory block of the DOS chain
;   DMVMEM ?      short help
;
; MS-DOS 3.30 has no MEM command (it came with DOS 4.0). DMVMEM walks
; the chain of memory control blocks (MCB) that DOS keeps in front of
; every allocated block and adds up the blocks by owner:
;
;  - "DOS" is everything below the first MCB (interrupt table, BIOS
;    and DOS data, IO.SYS, MSDOS.SYS, buffers, FILES, device drivers)
;    plus the blocks DOS itself owns (owner 0008).
;  - Every other owner is a program segment prefix (PSP). Its name is
;    taken from the end of its environment (DOS 3.0 and later put the
;    program's path there). Resident programs often give their
;    environment back; then DMVMEM looks for a name tag in the first
;    bytes of the program (e.g. "DMVKEY" right behind the jump at
;    100h). The command processor is the PSP that is its own parent.
;  - The interrupts whose vectors point into a program's block are
;    listed behind it - that is how a TSR is hooked into the system.
;  - DMVMEM's own blocks count as free: they are free again when it
;    ends. "largest block" is what the next program can get.
;  - Each block is counted with its 16-byte MCB, so the lines add up
;    exactly to the memory size.
;
; Run from GEM ("DOS-Kommandos eingeben") it shows GEM's own blocks as
; well (GEMVDI, GEM.EXE with AES, VDI driver and buffers).
;
; Plain MS-DOS calls (2.0 and later; program names need 3.0), no
; ports. Output goes to standard output and can be redirected.
;
; Build: BUILD.BAT (MASM 5.10) or tools/build.sh (JWasm under Linux).
;
; (C) 2026 rfka01, MIT License. Code: Claude (Anthropic).
;=====================================================================

	.model	tiny
	.8086

cr	equ	0Dh
lf	equ	0Ah
MAXOWN	equ	40		; owners kept in the table
NAMEW	equ	28		; width of the name column
MAXINT	equ	12		; interrupts listed per program

	.code
	org	100h
start:	jmp	main

;---------------------------------------------------------------------
; data
;---------------------------------------------------------------------
msg_kopf db	'DMVMEM 1.0 - Belegung des Arbeitsspeichers',cr,lf,cr,lf,'$'
msg_spal db	'Programm                         Byte   Hinweis',cr,lf
	db	'------------------------------------------------------------'
	db	'------------',cr,lf,'$'
msg_str	db	'------------------------------------------------------------'
	db	'------------',cr,lf,'$'
msg_dos	db	'DOS (Kern, Puffer, Treiber)$'
msg_com	db	'COMMAND.COM$'
msg_cmdh db	'Befehlsprozessor$'
msg_unb	db	'(Name unbekannt)$'
msg_ohne db	'(Daten ohne Programm)$'
msg_int	db	'INT$'
msg_frei db	'Frei$'
msg_gr	db	'groesster Block $'
msg_ges	db	'Gesamt$'
msg_ram	db	'Arbeitsspeicher$'
msg_kap	db	'Die Speicherkette von DOS ist beschaedigt (Block $'
msg_kap2 db	').',cr,lf,'$'
msg_voll db	'(weitere Programme nicht gezeigt)',cr,lf,'$'
msg_bk	db	cr,lf,'Bloecke:',cr,lf
	db	'MCB    Besitzer   Byte   Programm',cr,lf,'$'
msg_bfrei db	'frei$'
msg_bdos db	'DOS$'
msg_bumg db	'Umgebung von $'
msg_beig db	'DMVMEM selbst$'
msg_nl	db	cr,lf,'$'
msg_hlp	db	'DMVMEM zeigt, wie der Arbeitsspeicher belegt ist: DOS, der',cr,lf
	db	'Befehlsprozessor, residente Programme (mit den Interrupts, auf',cr,lf
	db	'die sie sich gesetzt haben) und der freie Speicher.',cr,lf,cr,lf
	db	'  DMVMEM      Uebersicht',cr,lf
	db	'  DMVMEM /B   dazu jeden Speicherblock einzeln',cr,lf,cr,lf
	db	'DMVMEM 1.0 fuer die NCR Decision Mate V.',cr,lf
	db	'https://github.com/rfka01/dmv_goodies',cr,lf,'$'

opt_b	db	0
mypsp	dw	0
mcb	dw	0
firstm	dw	0
nown	dw	0
voll	db	0
frei_lo	dw	0		; free + own, bytes (32 bit)
frei_hi	dw	0
lauf_lo	dw	0		; current run of free/own blocks
lauf_hi	dw	0
gr_lo	dw	0		; largest run
gr_hi	dw	0
ges_lo	dw	0
ges_hi	dw	0
nint	db	0
zahlbuf	db	16 dup (?)

; owner table: owner PSP, bytes (lo, hi), first block (MCB+1)
o_psp	dw	MAXOWN dup (0)
o_lo	dw	MAXOWN dup (0)
o_hi	dw	MAXOWN dup (0)

;---------------------------------------------------------------------
main:	mov	sp,offset stapel_ende
	mov	ah,4Ah			; keep only what we need
	mov	bx,(offset stapel_ende - start + 100h + 15) / 16
	push	cs
	pop	es
	int	21h
	mov	[mypsp],cs
	call	argumente
	mov	dx,offset msg_kopf
	call	text
	mov	ah,52h			; list of lists, first MCB at ES:BX-2
	int	21h
	mov	ax,es:[bx-2]
	mov	[firstm],ax
	; DOS gets everything below the first MCB
	mov	cx,16
	mul	cx
	mov	bx,8
	call	o_add
	call	kette			; walk the chain
	jc	ende_fehler
	call	uebersicht
	cmp	[opt_b],0
	je	ende
	call	bloecke
ende:	mov	ax,4C00h
	int	21h
ende_fehler:
	mov	ax,4C01h
	int	21h

;---------------------------------------------------------------------
; command line: /B, ?
;---------------------------------------------------------------------
argumente:
	mov	si,81h
arg1:	lodsb
	cmp	al,cr
	je	arg_raus
	cmp	al,'?'
	je	hilfe
	cmp	al,'/'
	jne	arg1
	lodsb
	and	al,0DFh
	cmp	al,'B'
	jne	hilfe
	mov	[opt_b],1
	jmp	arg1
arg_raus:
	ret
hilfe:	mov	dx,offset msg_hlp
	call	text
	mov	ax,4C00h
	int	21h

;---------------------------------------------------------------------
; walk the MCB chain, add every block (with its header) to its owner,
; collect free memory; CF on a broken chain
;---------------------------------------------------------------------
kette:	mov	ax,[firstm]
k_blk:	mov	[mcb],ax
	mov	es,ax
	mov	al,es:[0]
	cmp	al,'M'
	je	k_ok
	cmp	al,'Z'
	je	k_ok
	mov	dx,offset msg_kap
	call	text
	mov	ax,[mcb]
	call	hex4
	mov	dx,offset msg_kap2
	call	text
	stc
	ret
k_ok:	mov	ax,es:[3]		; bytes = (paragraphs + 1) * 16
	inc	ax
	mov	cx,16
	mul	cx			; DX:AX
	mov	bx,es:[1]
	or	bx,bx
	jz	k_frei
	cmp	bx,[mypsp]
	je	k_frei
	; occupied: end of a free run
	call	lauf_ende
	call	o_add
	jmp	short k_next
k_frei:	add	[frei_lo],ax
	adc	[frei_hi],dx
	add	[lauf_lo],ax
	adc	[lauf_hi],dx
k_next:	mov	es,[mcb]
	mov	ax,es:[3]
	cmp	byte ptr es:[0],'Z'
	je	k_ende
	add	ax,[mcb]
	inc	ax
	jmp	k_blk
k_ende:	add	ax,[mcb]		; memory size = end of the last block
	inc	ax
	mov	cx,16
	mul	cx
	mov	[ges_lo],ax
	mov	[ges_hi],dx
	call	lauf_ende
	clc
	ret

; a run of free blocks ends: keep the largest (usable = run - 16)
lauf_ende:
	push	ax
	push	dx
	mov	ax,[lauf_lo]
	mov	dx,[lauf_hi]
	mov	[lauf_lo],0
	mov	[lauf_hi],0
	sub	ax,16
	sbb	dx,0
	jc	le_raus
	cmp	dx,[gr_hi]
	jb	le_raus
	ja	le_neu
	cmp	ax,[gr_lo]
	jbe	le_raus
le_neu:	mov	[gr_lo],ax
	mov	[gr_hi],dx
le_raus:
	pop	dx
	pop	ax
	ret

; DX:AX bytes to owner BX
o_add:	push	si
	push	cx
	mov	cx,[nown]
	xor	si,si
oa_such:
	jcxz	oa_neu
	cmp	o_psp[si],bx
	je	oa_da
	add	si,2
	dec	cx
	jmp	oa_such
oa_neu:	cmp	[nown],MAXOWN
	jb	oa_platz
	mov	[voll],1
	jmp	short oa_raus
oa_platz:
	mov	o_psp[si],bx
	inc	[nown]
oa_da:	add	o_lo[si],ax
	adc	o_hi[si],dx
oa_raus:
	pop	cx
	pop	si
	ret

;---------------------------------------------------------------------
; summary
;---------------------------------------------------------------------
uebersicht:
	mov	dx,offset msg_spal
	call	text
	xor	si,si
u_zeile:
	cmp	si,[nown]
	jae	u_fuss
	push	si
	shl	si,1
	mov	bx,o_psp[si]
	mov	ax,o_lo[si]
	mov	dx,o_hi[si]
	call	zeile
	pop	si
	inc	si
	jmp	u_zeile
u_fuss:	cmp	[voll],0
	je	u_f1
	mov	dx,offset msg_voll
	call	text
u_f1:	mov	dx,offset msg_str
	call	text
	; free
	mov	dx,offset msg_frei
	mov	cx,4
	call	spalte1
	mov	ax,[frei_lo]
	mov	dx,[frei_hi]
	call	zahl
	call	leer3
	mov	dx,offset msg_gr
	call	text
	mov	ax,[gr_lo]
	mov	dx,[gr_hi]
	call	zahl_kurz
	call	nl
	; total
	mov	dx,offset msg_ges
	mov	cx,6
	call	spalte1
	mov	ax,[ges_lo]
	mov	dx,[ges_hi]
	call	zahl
	call	leer3
	mov	dx,offset msg_ram
	call	text
	call	nl
	ret

; one line for owner BX with DX:AX bytes
zeile:	push	ax
	push	dx
	cmp	bx,8
	jne	z_prog
	mov	dx,offset msg_dos
	mov	cx,27
	call	spalte1
	pop	dx
	pop	ax
	call	zahl
	jmp	nl
z_prog:	call	name_von		; prints the name, CX = length
	call	auffuellen
	pop	dx
	pop	ax
	call	zahl
	; remark: command processor, hooked interrupts
	mov	es,bx
	cmp	word ptr es:[0],20CDh
	jne	z_raus
	cmp	es:[16h],bx
	jne	z_ints
	call	leer3
	mov	dx,offset msg_cmdh
	call	text
	jmp	short z_raus
z_ints:	call	interrupts
z_raus:	jmp	nl

; name of the program with PSP BX; CX = characters written
name_von:
	push	bx
	push	si
	push	ds
	xor	cx,cx
	mov	es,bx
	cmp	word ptr es:[0],20CDh	; a real PSP?
	je	nv_psp
	mov	dx,offset msg_ohne
	call	text
	mov	cx,21
	jmp	nv_raus
nv_psp:	cmp	es:[16h],bx		; own parent: command processor
	jne	nv_umg
	mov	dx,offset msg_com
	call	text
	mov	cx,11
	jmp	nv_raus
nv_umg:	mov	ax,es:[2Ch]		; environment still ours?
	or	ax,ax
	jz	nv_kenn
	dec	ax
	mov	es,ax
	cmp	es:[1],bx
	jne	nv_kenn
	inc	ax
	mov	ds,ax			; DS = environment
	xor	si,si
nv_such:
	cmp	si,7F00h
	ja	nv_kenn0
	cmp	word ptr [si],0
	je	nv_end
	inc	si
	jmp	nv_such
nv_end:	cmp	word ptr [si+2],1	; count of strings after the env
	jne	nv_kenn0
	add	si,4
	mov	di,si			; file name = behind the last \ or :
nv_n1:	mov	al,[si]
	or	al,al
	jz	nv_n2
	inc	si
	cmp	al,'\'
	je	nv_n1b
	cmp	al,':'
	jne	nv_n1
nv_n1b:	mov	di,si
	jmp	nv_n1
nv_n2:	mov	si,di
nv_aus:	lodsb
	or	al,al
	jz	nv_raus
	cmp	cx,NAMEW-1
	jae	nv_raus
	call	zeichen
	inc	cx
	jmp	nv_aus
nv_kenn0:
	pop	ds
	push	ds
nv_kenn:
	; name tag in the first bytes of the program: 3-8 letters/digits
	; (upper case) followed by 0, blank or '$', starting within 100h-10Fh
	mov	ds,bx
	mov	si,100h
nk_start:
	cmp	si,110h
	jae	nk_keins
	mov	di,si
	xor	dx,dx
nk_lauf:
	mov	al,[di]
	cmp	al,'A'
	jb	nk_ziff
	cmp	al,'Z'
	jbe	nk_gut
	jmp	short nk_ende
nk_ziff:
	cmp	al,'0'
	jb	nk_ende
	cmp	al,'9'
	ja	nk_ende
	or	dx,dx			; not at the start
	jz	nk_ende
nk_gut:	inc	dx
	inc	di
	cmp	dx,9
	jb	nk_lauf
	jmp	short nk_weiter
nk_ende:
	cmp	dx,3
	jb	nk_weiter
	cmp	al,0
	je	nk_gef
	cmp	al,' '
	je	nk_gef
	cmp	al,'$'
	je	nk_gef
nk_weiter:
	inc	si
	jmp	nk_start
nk_gef:	mov	cx,dx
nk_aus:	lodsb
	call	zeichen
	dec	dx
	jnz	nk_aus
	jmp	short nv_raus
nk_keins:
	pop	ds
	push	ds
	mov	dx,offset msg_unb
	call	text
	mov	cx,16
nv_raus:
	pop	ds
	pop	si
	pop	bx
	ret

; interrupts whose vectors point into the block of PSP BX
interrupts:
	push	bx
	mov	ax,bx			; block: PSP .. PSP + size
	dec	ax
	mov	es,ax
	mov	cx,es:[3]		; paragraphs
	mov	[nint],0
	xor	si,si			; vector number * 4
	xor	ax,ax
	mov	es,ax
i_vek:	mov	ax,es:[si]		; offset
	mov	dx,es:[si+2]		; segment
	push	cx
	mov	cl,4
	shr	ax,cl
	pop	cx
	add	ax,dx			; paragraph of the target (approx.)
	jc	i_next
	cmp	ax,bx
	jb	i_next
	sub	ax,bx
	cmp	ax,cx
	jae	i_next
	cmp	[nint],MAXINT
	jae	i_next
	cmp	[nint],0
	jne	i_mehr
	call	leer3
	mov	dx,offset msg_int
	call	text
i_mehr:	inc	[nint]
	mov	dl,' '
	call	zeichen_dl
	mov	ax,si
	shr	ax,1
	shr	ax,1
	call	hex2
i_next:	add	si,4
	cmp	si,400h
	jb	i_vek
	pop	bx
	ret

;---------------------------------------------------------------------
; /B: every block
;---------------------------------------------------------------------
bloecke:
	mov	dx,offset msg_bk
	call	text
	mov	ax,[firstm]
b_blk:	mov	[mcb],ax
	mov	es,ax
	call	hex4			; MCB
	call	leer3
	mov	ax,es:[1]
	call	hex4			; owner
	call	leer3
	mov	ax,es:[3]
	inc	ax
	mov	cx,16
	mul	cx
	call	zahl_b
	call	leer3
	mov	es,[mcb]
	mov	bx,es:[1]
	or	bx,bx
	jnz	b_bel
	mov	dx,offset msg_bfrei
	call	text
	jmp	short b_nl
b_bel:	cmp	bx,8
	jne	b_eig
	mov	dx,offset msg_bdos
	call	text
	jmp	short b_nl
b_eig:	cmp	bx,[mypsp]
	jne	b_prog
	mov	dx,offset msg_beig
	call	text
	jmp	short b_nl
b_prog:	mov	ax,[mcb]		; environment of its owner?
	inc	ax
	cmp	ax,bx
	je	b_name
	mov	es,bx
	cmp	word ptr es:[0],20CDh
	jne	b_name
	cmp	es:[2Ch],ax
	jne	b_name
	mov	dx,offset msg_bumg
	call	text
b_name:	call	name_von
b_nl:	call	nl
	mov	es,[mcb]
	cmp	byte ptr es:[0],'Z'
	je	b_raus
	mov	ax,es:[3]
	add	ax,[mcb]
	inc	ax
	jmp	b_blk
b_raus:	ret

;---------------------------------------------------------------------
; output
;---------------------------------------------------------------------
text:	push	ax
	mov	ah,9
	int	21h
	pop	ax
	ret
zeichen:			; AL
	push	dx
	mov	dl,al
	call	zeichen_dl
	pop	dx
	ret
zeichen_dl:
	push	ax
	mov	ah,2
	int	21h
	pop	ax
	ret
nl:	mov	dx,offset msg_nl
	jmp	text
leer3:	push	dx
	mov	dl,' '
	call	zeichen_dl
	call	zeichen_dl
	call	zeichen_dl
	pop	dx
	ret
; text DX of length CX, then fill to the name column
spalte1:
	call	text
; blanks from column CX to NAMEW
auffuellen:
	push	dx
	mov	dl,' '
af1:	cmp	cx,NAMEW
	jae	af2
	call	zeichen_dl
	inc	cx
	jmp	af1
af2:	pop	dx
	ret

hex4:	push	ax
	mov	al,ah
	call	hex2
	pop	ax
hex2:	push	ax
	push	cx
	mov	cl,4
	shr	al,cl
	pop	cx
	call	hex1
	pop	ax
hex1:	push	ax
	push	dx
	and	al,0Fh
	add	al,'0'
	cmp	al,'9'
	jbe	h1
	add	al,7
h1:	mov	dl,al
	call	zeichen_dl
	pop	dx
	pop	ax
	ret

; DX:AX decimal with thousands dots: right aligned in 9 columns
; (zahl), 8 columns (zahl_b) or without padding (zahl_kurz)
zahl:	push	cx
	mov	cx,9
	jmp	short za
zahl_b:	push	cx
	mov	cx,8
	jmp	short za
zahl_kurz:
	push	cx
	xor	cx,cx
za:	push	bx
	push	si
	push	di
	mov	di,offset zahlbuf+15
	mov	byte ptr [di],'$'
	xor	si,si			; digits since the last dot
za_z:	cmp	si,3			; a dot before every fourth digit
	jne	za_d
	dec	di
	mov	byte ptr [di],'.'
	xor	si,si
za_d:	push	cx
	mov	bx,10
	push	ax
	mov	ax,dx
	xor	dx,dx
	div	bx
	mov	cx,ax			; high word / 10
	pop	ax
	div	bx			; (rest:low word) / 10
	add	dl,'0'
	dec	di
	mov	[di],dl
	mov	dx,cx
	pop	cx
	inc	si
	mov	bx,ax
	or	bx,dx
	jnz	za_z
	; padding
	mov	ax,offset zahlbuf+15
	sub	ax,di			; length
za_f:	cmp	ax,cx
	jae	za_aus
	dec	di
	mov	byte ptr [di],' '
	inc	ax
	jmp	za_f
za_aus:	mov	dx,di
	call	text
	pop	di
	pop	si
	pop	bx
	pop	cx
	ret

	even
stapel	dw	256 dup (?)
stapel_ende	label	word

	end	start

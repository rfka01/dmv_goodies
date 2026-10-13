	NAME	msxdmv
; File MSXDMV.ASM
	include mssdef.h
;
; Hardware layer for the NCR Decision Mate V (8088/V20 card K230/K231/K235)
; under NCR MS-DOS 2.11, the MS-DOS 3.30 OAK port and the DR-DOS 3.41 port.
;
; The IBM-PC version of MS-Kermit talks to the video and keyboard BIOS
; (Int 10h, Int 16h) and reads the BIOS data area at 40:00h. None of that
; exists on the DMV: under NCR's DOS 2.11 segment 40h even holds IO.SYS.
; This module stands in for those parts of the BIOS on private interrupt
; vectors, so that the screen and keyboard code of MS-Kermit can stay as
; it is:
;
;   Int DMVVID (0E5h)	video BIOS. The screen is a buffer in RAM (the
;			TopView "virtual screen" interface of MSYIBM), and
;			changes reach the real screen as ADM-3A/NCR control
;			sequences through Int 29h - the subset the consoles
;			of all three DOS versions understand:
;			  ESC = row col, ESC T (erase to end of line),
;			  1Ah (clear screen), ESC G 0/2/4 (normal, blink,
;			  inverse), ESC ) / ESC ( (half intensity on/off),
;			  LF on the bottom line to scroll.
;			A second buffer remembers what the DMV screen shows,
;			only differences are sent.
;   Int DMVKBD (0E6h)	keyboard BIOS. Reads the 8741 keyboard controller
;			at ports 40h/41h directly and turns its codes into
;			IBM-style scan codes; keys DOS has already taken
;			are fetched through Int 21h as a fallback.
;   Int DMVMSC (0E7h)	stands in for Int 15h: always "not supported".
;
; The serial ports (2651 on K211/K212, 2661 on K801) are driven in
; MSXIBM.ASM; this module finds them and supplies time, sound and the
; stand-in BIOS data area.
;
; Characters go to the console with bit 7 set: the character generator
; 76161 ignores bit 7, and NCR's DOS 2.11 then does not run them through
; its language table (CRTACTTBL), which on the German version shows '['
; as 'Ae'. Codes 80h-9Fh select the accented glyphs 00h-1Fh.
;
; NCR Decision Mate V port 2026. Kermit itself: Copyright (C) 1982, 1999,
; Trustees of Columbia University in the City of New York.

	public	dmvini, dmvfin, dmvtick, dmvcs, dmvbeep, dmvclr, dmvbda
	public	dmvsplit, dmvkbcty, dmvif, dmvifk, dmvifn, dmvemu
	extrn	fdmvpoll:far		; MSXDMVS.INC

KBDDAT	equ	40h		; 8741 keyboard controller, data
KBDSTA	equ	41h		; 8741 status (read) / command (write)
KBDOBF	equ	01h		;  status: byte for us
KBDIBF	equ	02h		;  status: 8741 has not taken our byte yet
KBDLANG	equ	80h		;  status: the byte is the country code

ROWS	equ	25		; DMV text screen
COLS	equ	80
SCRWDS	equ	ROWS*COLS	; words in a screen buffer
BLANK	equ	0720h		; space, normal attribute
DMVBDAP	equ	16		; paragraphs for the stand-in BIOS data area
SCRPARA	equ	(SCRWDS*2+15)/16 ; paragraphs per screen buffer

data	segment
	extrn	dosnum:word
dmvbda	dw	0		; segment of stand-in BIOS data area (40:xx)
vseg	dw	0		; Kermit's screen ("video memory")
dseg	dw	0		; what the DMV screen shows right now
vmode	db	7		; video mode we report (7 = monochrome)
dmvsplit db	0		; 1 = console scrolls only rows 0..23
dmvemu	db	0		; 1 = in the terminal emulator: only what
				;  differs is sent. 0 = command mode, DOS
				;  writes to the console too and the record
				;  of what it shows cannot be trusted
botrow	db	ROWS-1		; row on which a LF scrolls the console
curpos	dw	0		; cursor row:col as Kermit sees it
curtyp	dw	0B0Ch		; cursor type, for Int 10h AH=3
conpos	dw	0FFFFh		; where the console cursor is, FFFF unknown
dmvtcnt	db	1		; dmvtick: calls until DOS is asked again
	extrn	dmvpoll:byte		; MSXDMVS.INC: port open without IRQ
conatt	db	0FFh		; console attribute state, FF unknown
				;  bit 0 inverse, bit 1 blink, bit 2 half
dmvkbcty db	0		; keyboard country code (0 US, 3 German ...)
kbbuf	dw	16 dup (0)	; keys read, IBM form (scan * 256 + ascii)
kbflg	db	16 dup (0)	; their shift states (02h shift, 04h control)
kbhead	db	0		; next key to hand out
kbtail	db	0		; next free slot
lastflg	db	0		; shift state of the key last handed out
dospoll	db	0		; counts empty polls, DOS is asked every 16th
oldvid	dd	0		; previous contents of our three vectors
oldkbd	dd	0
oldmsc	dd	0
hooked	db	0
dmvif	db	4 dup (0)	; serial modules found: base port, 0 = end
dmvifk	db	4 dup (0)	;  and kind: 1 = 2651, 2 = 2661
dmvifn	db	0		; number found
				; work variables of the screen code
sc_n	db	0		; lines to scroll
sc_attr	db	0		; fill attribute
sc_top	db	0		; window
sc_left	db	0
sc_bot	db	0
sc_right db	0
sy_row	db	0		; syncrow: row, first and last column
sy_c0	db	0
sy_c1	db	0
sy_tail	db	0		; first column of the blank tail
				; order in which the IFSEL halves are searched
iforder	db	70h, 78h, 60h, 68h, 30h, 38h, 0B0h, 0B8h
	include	msxdmvt.inc
data	ends

code2	segment
	assume	cs:code2, ds:data, es:nothing

dmvnomem db	cr,lf,'?Kermit: zu wenig Speicher',cr,lf,'$'

; ---------------------------------------------------------------------------
; Start-up. Called once from MSSKER after memini, before anything touches
; the screen. Allocates the buffers, sets up the vectors and finds out
; what kind of console, keyboard and serial modules we have.
dmvini	proc	far
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	es
	mov	bx,DMVBDAP+2*SCRPARA	; BIOS data area and two screens
	mov	ah,alloc
	int	dos
	jnc	dmvini1
	push	ds
	mov	ax,cs
	mov	ds,ax
	mov	ah,prstr		; no memory: Kermit cannot run
	mov	dx,offset dmvnomem
	int	dos
	pop	ds
	mov	ax,4C01h
	int	dos
dmvini1:mov	dmvbda,ax
	mov	es,ax			; clear the stand-in BIOS data area
	xor	di,di
	mov	cx,DMVBDAP*8
	xor	ax,ax
	cld
	rep	stosw
	mov	al,vmode
	mov	byte ptr es:[49h],al	; video mode
	mov	word ptr es:[4Ah],COLS	; columns
	mov	byte ptr es:[84h],ROWS-1 ; rows - 1, as an EGA reports it
	mov	byte ptr es:[85h],16	; scan lines per character
	mov	ax,dmvbda
	add	ax,DMVBDAP
	mov	vseg,ax
	mov	es,ax			; Kermit's screen: blank
	xor	di,di
	mov	cx,SCRWDS
	mov	ax,BLANK
	rep	stosw
	mov	ax,vseg
	add	ax,SCRPARA
	mov	dseg,ax
					; what kind of console?
	mov	dmvsplit,0		; 25 rows scroll together (DOS 3.3,
	mov	botrow,ROWS-1		;  DR-DOS)
	cmp	byte ptr dosnum+1,2	; NCR MS-DOS 2.11?
	jne	dmvini2			; ne = no
	xor	ax,ax			; NCR's system definition area:
	mov	es,ax			;  CRT_ROWS at 0:041Dh. 24 means rows
	cmp	byte ptr es:[41Dh],24	;  0..23 scroll and row 24 is a
	jne	dmvini2			;  status line that stays put
	mov	dmvsplit,1
	mov	botrow,ROWS-2
dmvini2:call	kbinit			; keyboard country
	call	ifprobe			; serial modules
	call	hook			; our interrupt vectors
	call	dmvclr			; clear the screen, known state
	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
dmvini	endp

; Leaving Kermit: normal attributes, cursor to the bottom, vectors back.
dmvfin	proc	far
	push	ax
	push	bx
	push	dx
	cmp	hooked,0
	je	dmvfin1
	mov	al,0
	call	setatt
	mov	ax,(ROWS-1)*256		; row 24, column 0
	call	conmove
	call	unhook
dmvfin1:pop	dx
	pop	bx
	pop	ax
	ret
dmvfin	endp

hook	proc	near
	push	es
	push	ds
	mov	ax,3500h+DMVVID
	int	dos
	mov	word ptr oldvid,bx
	mov	word ptr oldvid+2,es
	mov	ax,3500h+DMVKBD
	int	dos
	mov	word ptr oldkbd,bx
	mov	word ptr oldkbd+2,es
	mov	ax,3500h+DMVMSC
	int	dos
	mov	word ptr oldmsc,bx
	mov	word ptr oldmsc+2,es
	mov	ax,cs
	mov	ds,ax
	mov	dx,offset dvid
	mov	ax,2500h+DMVVID
	int	dos
	mov	dx,offset dkbd
	mov	ax,2500h+DMVKBD
	int	dos
	mov	dx,offset dmsc
	mov	ax,2500h+DMVMSC
	int	dos
	pop	ds
	pop	es
	mov	hooked,1
	ret
hook	endp

unhook	proc	near
	push	ds
	lds	dx,oldvid
	mov	ax,2500h+DMVVID
	int	dos
	pop	ds
	push	ds
	lds	dx,oldkbd
	mov	ax,2500h+DMVKBD
	int	dos
	pop	ds
	push	ds
	lds	dx,oldmsc
	mov	ax,2500h+DMVMSC
	int	dos
	pop	ds
	mov	hooked,0
	ret
unhook	endp

; ---------------------------------------------------------------------------
; Int 15h stand-in: nothing is supported.
dmsc	proc	far
	mov	ah,86h
	stc
	retf	2			; return with the carry set
dmsc	endp

; ---------------------------------------------------------------------------
; Video BIOS. Only the functions MS-Kermit uses; everything else returns
; with the registers unchanged, which is what an IBM BIOS without the
; corresponding adapter does as well.
dvid	proc	far
	sti
	cld
	push	ds
	push	bp
	mov	bp,seg data
	mov	ds,bp
	cmp	ah,0FFh			; TopView: update the real screen
	jne	dvid1
	jmp	dv_sync
dvid1:	cmp	ah,0FEh			; TopView: where is the screen?
	jne	dvid2
	mov	es,vseg
	xor	di,di
	jmp	short dv_ret
dvid2:	cmp	ah,0Fh
	ja	dv_ret
	push	bx
	mov	bl,ah
	xor	bh,bh
	shl	bx,1
	mov	bp,cs:dvtab[bx]
	pop	bx
	jmp	bp
dv_ret:	pop	bp
	pop	ds
	iret

dvtab	dw	dv_mode, dv_ctype, dv_spos, dv_gpos, dv_ret, dv_ret
	dw	dv_scrup, dv_scrdn, dv_rdch, dv_wrca, dv_wrch, dv_ret
	dw	dv_ret, dv_ret, dv_tty, dv_gmode

dv_mode:push	ax			; set mode: clear the screen
	push	cx
	push	di
	push	es
	mov	es,vseg
	xor	di,di
	mov	cx,SCRWDS
	mov	ax,BLANK
	rep	stosw
	pop	es
	pop	di
	pop	cx
	pop	ax
	mov	curpos,0
	call	dmvclr
	jmp	dv_ret

dv_ctype:mov	curtyp,cx		; cursor type: remember only
	jmp	dv_ret

dv_spos:mov	curpos,dx		; set cursor position
	push	es
	mov	es,dmvbda
	mov	es:[50h],dx
	pop	es
	call	movecur
	jmp	dv_ret

dv_gpos:mov	dx,curpos		; get cursor position and type
	mov	cx,curtyp
	jmp	dv_ret

dv_gmode:mov	al,vmode		; get video mode
	mov	ah,COLS
	xor	bh,bh
	jmp	dv_ret

dv_rdch:push	si			; read char and attribute at cursor
	push	es
	mov	ax,curpos
	call	cellofs
	mov	si,ax
	mov	es,vseg
	mov	ax,es:[si]
	pop	es
	pop	si
	jmp	dv_ret

dv_wrca:push	ax			; write char AL, attribute BL, CX times
	push	cx
	mov	ah,bl
	call	wrcells
	pop	cx
	pop	ax
	jmp	dv_ret

dv_wrch:push	ax			; write char AL, CX times, keep attribute
	push	cx
	mov	ah,0FFh			; FF = keep
	call	wrcells
	pop	cx
	pop	ax
	jmp	dv_ret

dv_tty:	push	ax			; teletype output
	push	bx
	push	cx
	push	dx
	call	tty
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	jmp	dv_ret

dv_scrup:push	ax
	push	bx
	push	cx
	push	dx
	call	setwin
	jc	dv_scr9
	call	scrlup
dv_scr9:pop	dx
	pop	cx
	pop	bx
	pop	ax
	jmp	dv_ret

dv_scrdn:push	ax
	push	bx
	push	cx
	push	dx
	call	setwin
	jc	dv_scr9
	call	scrldn
	jmp	short dv_scr9

dv_sync:push	ax			; ES:DI start, CX words changed
	push	bx
	push	cx
	mov	ax,di
	shr	ax,1			; first cell
	mov	bx,ax
	add	bx,cx			; one past the last
	call	syncrange
	call	movecur
	pop	cx
	pop	bx
	pop	ax
	jmp	dv_ret
dvid	endp

; AX = row:col  ->  AX = byte offset of the cell
cellofs	proc	near
	push	cx
	push	dx
	mov	cl,ah
	xor	ah,ah
	push	ax
	mov	al,COLS
	mul	cl
	pop	cx
	add	ax,cx
	shl	ax,1
	pop	dx
	pop	cx
	ret
cellofs	endp

; Write char AL with attribute AH (FF = keep) CX times from the cursor on,
; without moving the cursor, then show the cells.
wrcells	proc	near
	jcxz	wrcel9
	push	bx
	push	si
	push	es
	push	ax
	mov	ax,curpos
	call	cellofs
	mov	si,ax
	pop	ax
	mov	es,vseg
	mov	bx,si
	shr	bx,1			; first cell
wrcel1:	cmp	si,SCRWDS*2
	jae	wrcel3
	cmp	ah,0FFh
	jne	wrcel2
	mov	es:[si],al
	jmp	short wrcel2a
wrcel2:	mov	es:[si],ax
wrcel2a:add	si,2
	loop	wrcel1
wrcel3:	mov	ax,bx
	mov	bx,si
	shr	bx,1			; one past the last
	call	syncrange
	call	movecur
	pop	es
	pop	si
	pop	bx
wrcel9:	ret
wrcells	endp

; Teletype output of AL (Int 10h AH=0Eh): BEL, BS, CR, LF, else write.
tty	proc	near
	mov	dx,curpos
	cmp	al,bell
	jne	tty1
	call	dmvbeep
	ret
tty1:	cmp	al,bs
	jne	tty2
	or	dl,dl
	jz	tty9
	dec	dl
	jmp	short tty9
tty2:	cmp	al,cr
	jne	tty3
	xor	dl,dl
	jmp	short tty9
tty3:	cmp	al,lf
	je	tty5
	mov	cx,1
	mov	ah,0FFh
	call	wrcells
	mov	dx,curpos
	inc	dl
	cmp	dl,COLS
	jb	tty9
	xor	dl,dl
tty5:	inc	dh
	cmp	dh,ROWS-1		; the BIOS scrolls the whole screen; we
	jb	tty9			;  keep the mode line out of it
	mov	dh,ROWS-2
	push	dx
	mov	ax,0601h
	mov	bh,07h
	xor	cx,cx
	mov	dx,(ROWS-2)*256+COLS-1
	call	setwin
	call	scrlup
	pop	dx
tty9:	mov	curpos,dx
	call	movecur
	ret
tty	endp

; ---------------------------------------------------------------------------
; Scrolling. Int 10h AH=6/7 parameters: AL lines (0 = clear), BH fill
; attribute, CH:CL top left, DH:DL bottom right. Kept in sc_xxx.
setwin	proc	near
	mov	sc_n,al
	mov	sc_attr,bh
	cmp	dh,ROWS-1
	jbe	setwin1
	mov	dh,ROWS-1
setwin1:cmp	dl,COLS-1
	jbe	setwin2
	mov	dl,COLS-1
setwin2:mov	sc_top,ch
	mov	sc_left,cl
	mov	sc_bot,dh
	mov	sc_right,dl
	cmp	ch,dh
	ja	setwin8
	cmp	cl,dl
	ja	setwin8
	mov	al,dh			; rows in the window
	sub	al,ch
	inc	al
	cmp	sc_n,0
	je	setwin3
	cmp	sc_n,al
	jb	setwin4
setwin3:mov	sc_n,al			; clear the whole window
setwin4:clc
	ret
setwin8:stc
	ret
setwin	endp

; Copy row AH of the window to row AL (Kermit's screen only).
rowcopy	proc	near
	push	cx
	push	si
	push	di
	push	ds
	push	es
	push	ax
	mov	ah,al
	mov	al,sc_left
	call	cellofs
	mov	di,ax
	pop	ax
	push	ax
	mov	al,sc_left
	call	cellofs
	mov	si,ax
	pop	ax
	mov	cl,sc_right
	sub	cl,sc_left
	inc	cl
	xor	ch,ch
	mov	es,vseg
	mov	ds,vseg
	rep	movsw
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	cx
	ret
rowcopy	endp

; Blank row AL of the window with the fill attribute (Kermit's screen).
rowfill	proc	near
	push	ax
	push	cx
	push	di
	push	es
	mov	ah,al
	mov	al,sc_left
	call	cellofs
	mov	di,ax
	mov	cl,sc_right
	sub	cl,sc_left
	inc	cl
	xor	ch,ch
	mov	al,' '
	mov	ah,sc_attr
	mov	es,vseg
	rep	stosw
	pop	es
	pop	di
	pop	cx
	pop	ax
	ret
rowfill	endp

; Scroll the window up by sc_n lines.
scrlup	proc	near
	push	ax
	push	bx
	push	dx
	mov	al,sc_top		; rows top .. bot-n get row + n
	mov	bl,sc_bot
	sub	bl,sc_n
scrlu1:	cmp	al,bl
	jg	scrlu2
	mov	ah,al
	add	ah,sc_n
	call	rowcopy
	inc	al
	jmp	short scrlu1
scrlu2:	cmp	al,sc_bot		; the last n rows: blank
	jg	scrlu3
	call	rowfill
	inc	al
	jmp	short scrlu2
scrlu3:	call	natscrl			; may scroll the console natively
	mov	dl,sc_top		;  and returns the last row to check
	call	syncrows
	call	movecur
	pop	dx
	pop	bx
	pop	ax
	ret
scrlup	endp

; Scroll the window down by sc_n lines. No native way on all consoles,
; the rows are redrawn.
scrldn	proc	near
	push	ax
	push	bx
	push	dx
	mov	al,sc_bot		; rows bot .. top+n get row - n
	mov	bl,sc_top
	add	bl,sc_n
scrld1:	cmp	al,bl
	jl	scrld2
	mov	ah,al
	sub	ah,sc_n
	call	rowcopy
	dec	al
	jmp	short scrld1
scrld2:	cmp	al,sc_top		; the first n rows: blank
	jl	scrld3
	call	rowfill
	dec	al
	jmp	short scrld2
scrld3:	call	natclr			; whole screen cleared?
	mov	dl,sc_top
	mov	dh,sc_bot
	call	syncrows
	call	movecur
	pop	dx
	pop	bx
	pop	ax
	ret
scrldn	endp

; The console scrolls when it gets a LF on its bottom line (botrow): rows
; 0..botrow move up one. Use that when a window starting at row 0 and
; spanning the full width scrolls up: the rows between the window and
; botrow that moved along are then redrawn by syncrows. Also handles a
; clear of the whole screen. Returns DH = last row to check.
natscrl	proc	near
	mov	dh,sc_bot
	call	natclr
	jnc	natsc0
	ret				; cleared, DH = sc_bot is fine
natsc0:	cmp	sc_left,0
	jne	natsc9
	cmp	sc_right,COLS-1
	jne	natsc9
	cmp	sc_top,0
	jne	natsc9
	mov	al,botrow		; rows dragged along: botrow - bot
	sub	al,sc_bot
	jb	natsc9			; window reaches below botrow
	cmp	al,3
	ja	natsc9			; too many to repair
	mov	al,sc_n
	cmp	al,8
	ja	natsc9			; many lines: just redraw
	mov	al,sc_n
	mov	ah,sc_bot
	inc	ah
	cmp	al,ah			; the whole window? natclr had it
	jae	natsc9
	push	cx
	mov	cl,sc_n
	xor	ch,ch
natsc1:	push	cx
	mov	al,0			; new line comes out blank-normal
	call	setatt
	mov	ah,botrow
	xor	al,al
	call	conmove
	mov	al,lf
	call	cputc
	call	dshift			; same to what we think it shows
	pop	cx
	loop	natsc1
	pop	cx
	mov	dh,botrow
	cmp	dh,sc_bot
	jae	natsc9
	mov	dh,sc_bot
natsc9:	ret
natscrl	endp

; If the window was cleared completely and covers the screen (full width,
; from row 0 down to at least row 23), clear the console with 1Ah and
; return carry set; the mode line is then redrawn by syncrows.
natclr	proc	near
	mov	al,sc_bot
	sub	al,sc_top
	inc	al
	cmp	sc_n,al
	jne	natcl9
	cmp	sc_top,0
	jne	natcl9
	cmp	sc_bot,ROWS-2
	jb	natcl9
	cmp	sc_left,0
	jne	natcl9
	cmp	sc_right,COLS-1
	jne	natcl9
	call	dmvclr
	mov	dh,ROWS-1		; check every row
	stc
	ret
natcl9:	clc
	ret
natclr	endp

; The console has scrolled rows 0..botrow up by one: do the same in the
; record of what it shows, the bottom row becomes blank.
dshift	proc	near
	push	ax
	push	cx
	push	si
	push	di
	push	ds
	push	es
	mov	cl,botrow
	xor	ch,ch
	mov	al,COLS
	mul	cl			; words to move
	mov	cx,ax
	mov	es,dseg
	mov	ds,dseg
	xor	di,di
	mov	si,COLS*2
	rep	movsw
	mov	cx,COLS			; DI is at the bottom row now
	mov	ax,BLANK
	rep	stosw
	pop	es
	pop	ds
	pop	di
	pop	si
	pop	cx
	pop	ax
	ret
dshift	endp

; Clear the console, attributes normal, and record that it is blank.
dmvclr	proc	far
	push	ax
	push	cx
	push	di
	push	es
	mov	al,0
	call	setatt
	mov	al,1Ah
	call	cputc
	mov	conpos,0FFFFh		; home on some consoles, unknown on others
	mov	es,dseg
	xor	di,di
	mov	cx,SCRWDS
	mov	ax,BLANK
	cld
	rep	stosw
	pop	es
	pop	di
	pop	cx
	pop	ax
	ret
dmvclr	endp

; ---------------------------------------------------------------------------
; Bring the console up to date with Kermit's screen.

; Cells AX up to (not including) BX.
syncrange proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	cmp	bx,SCRWDS
	jbe	syncra1
	mov	bx,SCRWDS
syncra1:cmp	ax,bx
	jae	syncra9
	mov	cl,COLS
	div	cl			; AL row, AH column of the first
	mov	sy_row,al
	mov	sy_c0,ah
	dec	bx			; last cell
	xchg	ax,bx
	div	cl			; AL row, AH column of the last
	mov	dl,al			; last row
	mov	dh,ah			; last column
syncra2:mov	al,sy_row
	mov	sy_c1,COLS-1
	cmp	al,dl
	jne	syncra3
	mov	sy_c1,dh		; last row: up to the last column
syncra3:call	syncrow
	cmp	sy_row,dl
	jae	syncra9
	inc	sy_row
	mov	sy_c0,0
	jmp	short syncra2
syncra9:pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
syncrange endp

; Rows DL to DH, completely.
syncrows proc	near
	push	ax
	cmp	dh,ROWS-1
	jbe	syncrw1
	mov	dh,ROWS-1
syncrw1:mov	sy_row,dl
syncrw2:mov	al,sy_row
	cmp	al,dh
	ja	syncrw9
	mov	sy_c0,0
	mov	sy_c1,COLS-1
	call	syncrow
	inc	sy_row
	jmp	short syncrw2
syncrw9:pop	ax
	ret
syncrows endp

; Row sy_row, columns sy_c0 to sy_c1.
syncrow	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	es
	cmp	dmvemu,0		; command mode: forget what we think
	jne	syncr01			;  the console shows in this range
	mov	ah,sy_row
	mov	al,sy_c0
	call	cellofs
	mov	di,ax
	mov	cl,sy_c1
	sub	cl,sy_c0
	inc	cl
	xor	ch,ch
	mov	es,dseg
	mov	ax,0FFFFh
	cld
	rep	stosw
syncr01:
	mov	ah,sy_row		; first differing column at or after c0
	mov	al,sy_c0
	call	cellofs
	mov	si,ax
	mov	cl,sy_c1
	sub	cl,sy_c0
	inc	cl
	xor	ch,ch
	mov	es,vseg
	push	ds
	mov	ds,dseg
	xchg	si,di			; ES:DI Kermit, DS:SI shown
	mov	si,di
	cld
	repe	cmpsw
	pop	ds
	je	syncro9			; all equal
	sub	di,2
	mov	ax,di			; offset of the first difference
	shr	ax,1
	mov	bl,COLS
	div	bl
	mov	sy_c0,ah
	mov	ah,sy_row		; last differing column at or before c1
	mov	al,sy_c1
	call	cellofs
	mov	di,ax
	mov	si,ax
	mov	cl,sy_c1
	sub	cl,sy_c0
	inc	cl
	xor	ch,ch
	push	ds
	mov	ds,dseg
	std
	repe	cmpsw
	cld
	pop	ds
	add	di,2
	mov	ax,di
	shr	ax,1
	div	bl
	mov	sy_c1,ah
	mov	sy_tail,COLS		; blank tail of Kermit's row, only if
	mov	ah,sy_row		;  the row is looked at up to its end
	mov	al,COLS-1
	call	cellofs
	mov	si,ax
syncro2:mov	ax,es:[si]
	cmp	al,' '
	je	syncro3
	or	al,al
	jne	syncro4
syncro3:call	attstate
	or	al,al
	jne	syncro4
	dec	sy_tail
	jz	syncro4
	sub	si,2
	jmp	short syncro2
syncro4:mov	al,sy_tail		; never erase left of c0: in command
	cmp	al,sy_c0		;  mode DOS may have written text there
	jae	syncro4a		;  that Kermit's buffer does not hold
	mov	al,sy_c0
	mov	sy_tail,al
syncro4a:cmp	al,sy_c1		; differences in the blank tail?
	ja	syncro6			; a = no, write c0..c1
	cmp	al,COLS-1		; worth an ESC T?
	jae	syncro6			; ae = only one cell
	cmp	al,sy_c0		; write c0 .. tail-1 first
	jbe	syncro5
	dec	al
	mov	sy_c1,al
	call	emit
syncro5:mov	al,sy_tail		; then erase the rest of the line
	mov	ah,sy_row
	call	conmove
	mov	al,0
	call	setatt
	mov	al,escape
	call	cputc
	mov	al,'T'
	call	cputc
	mov	ah,sy_row		; record: the tail as Kermit has it
	mov	al,sy_tail
	call	cellofs
	mov	si,ax
	mov	di,ax
	mov	cl,COLS
	sub	cl,sy_tail
	xor	ch,ch
	push	ds
	push	es
	mov	bx,dseg			; (read before DS changes)
	mov	ax,es
	mov	ds,ax			; DS:SI Kermit
	mov	es,bx			; ES:DI shown
	rep	movsw
	pop	es
	pop	ds
	jmp	short syncro9
syncro6:call	emit
syncro9:pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
syncrow	endp

; Write cells sy_c0 .. sy_c1 of row sy_row to the console. Runs of four
; or more cells that are already right are skipped. The very last cell of
; the screen (row 24, column 79) is never written: some consoles scroll
; when it is.
emit	proc	near
	push	ax
	push	bx
	push	cx
	push	si
	push	es
	mov	ah,sy_row
	mov	al,sy_c0
	call	cellofs
	mov	si,ax
	mov	es,vseg
	mov	bl,sy_c0		; BL = column
emit1:	cmp	bl,sy_c1
	ja	emit9
	mov	ax,es:[si]		; Kermit's cell
	push	ds
	mov	ds,dseg
	cmp	ax,ds:[si]
	pop	ds
	jne	emit3
	mov	cx,0			; count equal cells from here
	push	si
	push	bx
emit2:	cmp	bl,sy_c1
	ja	emit2a
	mov	ax,es:[si]
	push	ds
	mov	ds,dseg
	cmp	ax,ds:[si]
	pop	ds
	jne	emit2a
	inc	cx
	inc	bl
	add	si,2
	jmp	short emit2
emit2a:	cmp	cx,4
	jb	emit2b
	add	sp,4			; skip them
	jmp	short emit1
emit2b:	pop	bx			; few: write them, cheaper than ESC =
	pop	si
	mov	ax,es:[si]
emit3:	cmp	sy_row,ROWS-1
	jne	emit4
	cmp	bl,COLS-1
	je	emit5			; never the last cell
emit4:	push	ax
	mov	ah,sy_row
	mov	al,bl
	call	conmove
	pop	ax
	push	ax
	call	attstate
	call	setatt
	pop	ax
	push	ax
	push	bx
	mov	bx,offset dmvglyph
	xlat
	pop	bx
	call	cputc
	pop	ax
	inc	byte ptr conpos		; console cursor moved one right
	cmp	byte ptr conpos,COLS
	jb	emit5
	mov	conpos,0FFFFh		; end of line: depends on the console
emit5:	push	ds
	mov	ds,dseg
	mov	ds:[si],ax		; record it
	pop	ds
	add	si,2
	inc	bl
	jmp	emit1
emit9:	pop	es
	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
emit	endp

; IBM attribute AH -> console attribute state AL: bit 0 inverse (background
; brighter than foreground), bit 1 blink, bit 2 half intensity (the
; monochrome underline attribute 01h, which the DMV cannot show).
attstate proc	near
	push	bx
	xor	al,al
	mov	bl,ah
	and	bl,7			; foreground
	mov	bh,ah
	shr	bh,1
	shr	bh,1
	shr	bh,1
	shr	bh,1
	and	bh,7			; background
	cmp	bh,bl
	jbe	attst1
	or	al,1
attst1:	test	ah,80h
	jz	attst2
	or	al,2
attst2:	mov	bl,ah
	and	bl,77h
	cmp	bl,01h
	jne	attst3
	or	al,4
attst3:	pop	bx
	ret
attstate endp

; Console attribute state AL.
setatt	proc	near
	cmp	al,conatt
	je	setat9
	push	ax
	push	bx
	mov	bl,al
	mov	bh,conatt
	xor	bh,bl
	cmp	conatt,0FFh
	je	setat1
	test	bh,3			; inverse or blink changed?
	jz	setat3
setat1:	mov	al,escape
	call	cputc
	mov	al,'G'
	call	cputc
	mov	al,'0'
	call	cputc
	test	bl,1
	jz	setat2
	mov	al,escape
	call	cputc
	mov	al,'G'
	call	cputc
	mov	al,'4'
	call	cputc
setat2:	test	bl,2
	jz	setat3
	mov	al,escape
	call	cputc
	mov	al,'G'
	call	cputc
	mov	al,'2'
	call	cputc
setat3:	mov	al,escape		; half intensity, always stated again:
	call	cputc			;  some consoles reset it with ESC G 0
	mov	al,'('
	test	bl,4
	jz	setat4
	mov	al,')'
setat4:	call	cputc
	mov	conatt,bl
	pop	bx
	pop	ax
setat9:	ret
setatt	endp

; Put the console cursor on row:col AX.
conmove	proc	near
	cmp	dmvemu,0		; command mode: DOS moves the cursor
	je	conmov1			;  behind our back, always position
	cmp	ax,conpos
	je	conmov9
conmov1:
	mov	conpos,ax
	push	ax
	mov	al,escape
	call	cputc
	mov	al,'='
	call	cputc
	pop	ax
	push	ax
	mov	al,ah
	add	al,' '
	call	cputc
	pop	ax
	push	ax
	add	al,' '
	call	cputc
	pop	ax
conmov9:ret
conmove	endp

; Put the console cursor where Kermit wants it.
movecur	proc	near
	push	ax
	mov	ax,curpos
	cmp	ah,ROWS-1
	ja	movecu9
	cmp	al,COLS-1
	ja	movecu9
	call	conmove
movecu9:pop	ax
	ret
movecur	endp

; One byte to the console, through DOS's fast console output.
cputc	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	ds
	push	es
	call	fdmvpoll		; polled port: fetch what came
	int	29h
	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
cputc	endp

; ---------------------------------------------------------------------------
; Keyboard BIOS: AH = 0/10h read, 1/11h status, 2/12h shift state,
; 5 stuff a key. Return through RETF 2 so the zero flag reaches the caller.
dkbd	proc	far
	sti
	push	ds
	push	bx
	mov	bx,seg data
	mov	ds,bx
	cmp	ah,0
	je	dkbd_rd
	cmp	ah,10h
	je	dkbd_rd
	cmp	ah,1
	je	dkbd_st
	cmp	ah,11h
	je	dkbd_st
	cmp	ah,2
	je	dkbd_sh
	cmp	ah,12h
	je	dkbd_sh
	cmp	ah,5
	je	dkbd_stuff
	jmp	short dkbd_x

dkbd_rd:call	kbfill			; wait for a key
	call	kbpeek
	jz	dkbd_rd
	call	kbtake
	jmp	short dkbd_x

dkbd_st:call	kbfill			; is there a key?
	call	kbpeek			; ZF = 1 none, else AX = key
	pop	bx
	pop	ds
	retf	2

dkbd_sh:mov	al,lastflg		; shift state
	xor	ah,ah
	jmp	short dkbd_x

dkbd_stuff:mov	ax,cx			; put CX into the buffer
	xor	bl,bl
	call	kbput
	xor	al,al
dkbd_x:	pop	bx
	pop	ds
	iret
dkbd	endp

; Look at the next key: ZF = 1 none, else AX = key and lastflg its state.
kbpeek	proc	near
	push	bx
	mov	bl,kbhead
	cmp	bl,kbtail
	je	kbpeek9			; ZF = 1
	xor	bh,bh
	mov	al,kbflg[bx]
	mov	lastflg,al
	shl	bx,1
	mov	ax,kbbuf[bx]
	or	bx,1			; ZF = 0 (BX is never 0 after OR 1)
kbpeek9:pop	bx
	ret
kbpeek	endp

; Remove the next key (after kbpeek said there is one).
kbtake	proc	near
	inc	kbhead
	and	kbhead,0Fh
	ret
kbtake	endp

; Key AX with shift state BL into the buffer, unless it is full.
kbput	proc	near
	push	bx
	push	cx
	mov	cl,kbtail
	mov	ch,cl
	inc	ch
	and	ch,0Fh
	cmp	ch,kbhead
	je	kbput9			; full: drop it
	mov	bh,0
	xchg	bl,cl			; BX = slot, CL = shift state
	mov	kbflg[bx],cl
	shl	bx,1
	mov	kbbuf[bx],ax
	mov	kbtail,ch
kbput9:	pop	cx
	pop	bx
	ret
kbput	endp

; Collect what the 8741 has; if it has nothing and our buffer is empty,
; now and then ask DOS, which may have read a key before us.
kbfill	proc	near
	push	ax
	push	bx
	push	dx
kbfil1:	in	al,KBDSTA
	test	al,KBDOBF
	jz	kbfil5
	test	al,KBDLANG		; a late country code?
	jz	kbfil2
	in	al,KBDDAT
	and	al,7
	mov	dmvkbcty,al
	jmp	short kbfil1
kbfil2:	in	al,KBDDAT
	call	kbxlat			; AX = key, BL = shift state, ZF = none
	jz	kbfil1
	call	kbput
	jmp	short kbfil1
kbfil5:	mov	bl,kbhead
	cmp	bl,kbtail
	jne	kbfil9			; have keys
	inc	dospoll
	test	dospoll,0Fh
	jnz	kbfil9
	mov	ah,0Bh			; DOS: anything?
	int	dos
	or	al,al
	jz	kbfil9
	mov	ah,dconio		; DOS: read it, no echo, no wait
	mov	dl,0FFh
	int	dos
	jz	kbfil9
	call	kbxdos
	jz	kbfil9
	call	kbput
kbfil9:	pop	dx
	pop	bx
	pop	ax
	ret
kbfill	endp

; 8741 code AL -> AX = IBM key, BL = shift state; ZF = 1 if to be dropped.
kbxlat	proc	near
	push	si
	push	cx
	xor	ah,ah
	mov	si,ax
	mov	bl,dmvkeyf[si]
	shl	si,1
	mov	cx,dmvkeyw[si]		; the general table
	cmp	dmvkbcty,3		; German keyboard?
	jne	kbxla4
	push	ax
	mov	si,offset dmvkeyde	; its exceptions
	mov	ah,dmvkeyden
kbxla1:	or	ah,ah
	jz	kbxla3
	cmp	al,[si]
	je	kbxla2
	add	si,3
	dec	ah
	jmp	short kbxla1
kbxla2:	mov	cx,[si+1]
kbxla3:	pop	ax
kbxla4:	mov	ax,cx
	or	ax,ax
	pop	cx
	pop	si
	ret
kbxlat	endp

; Character AL that came from DOS -> AX = IBM key, BL = 0; ZF = 1 drop.
kbxdos	proc	near
	push	si
	push	cx
	xor	ah,ah
	mov	si,offset dmvdosk	; codes the DOS consoles already
	mov	cl,dmvdoskn		;  translated (umlauts, Ctrl-CLR)
kbxdo1:	or	cl,cl
	jz	kbxdo3
	cmp	al,[si]
	je	kbxdo2
	add	si,3
	dec	cl
	jmp	short kbxdo1
kbxdo2:	mov	ax,[si+1]
	xor	bl,bl
	or	ax,ax
	jmp	short kbxdo9
kbxdo3:	call	kbxlat			; the rest is what the 8741 sends
kbxdo9:	pop	cx
	pop	si
	ret
kbxdos	endp

; Ask the 8741 for the country code (command 01h). Keys that arrive
; meanwhile go into our buffer. Gives up after a while: an old
; controller may not answer.
kbinit	proc	near
	push	ax
	push	bx
	push	cx
	mov	cx,0
kbini1:	in	al,KBDSTA		; wait until it takes commands
	test	al,KBDIBF
	jz	kbini2
	loop	kbini1
	jmp	short kbini9
kbini2:	mov	al,1
	out	KBDSTA,al
	mov	cx,0
kbini3:	in	al,KBDSTA
	test	al,KBDOBF
	jz	kbini5
	test	al,KBDLANG
	jz	kbini4
	in	al,KBDDAT		; E8h + country code
	and	al,7
	mov	dmvkbcty,al
	jmp	short kbini9
kbini4:	in	al,KBDDAT		; a key
	call	kbxlat
	jz	kbini5
	call	kbput
kbini5:	loop	kbini3
kbini9:	pop	cx
	pop	bx
	pop	ax
	ret
kbinit	endp

; ---------------------------------------------------------------------------
; Bell: a short tone from the 8741 (command 06h, tone, length). The 8741
; does not read its input while a tone plays, but IBF is already clear
; then; so only the command is sent while IBF is clear, and tone and length
; once the 8741 has taken the command. Every wait has a limit.
dmvbeep	proc	far
	push	ax
	push	cx
	call	kbibf
	jc	dmvbee9
	mov	al,6
	out	KBDSTA,al
	call	kbibf
	jc	dmvbee9
	mov	al,37+20h		; tone 37: 880 Hz
	out	KBDDAT,al
	call	kbibf
	jc	dmvbee9
	mov	al,4+1Fh		; four units of 20.5 ms
	out	KBDDAT,al
dmvbee9:pop	cx
	pop	ax
	ret
dmvbeep	endp

kbibf	proc	near			; wait for IBF clear, carry on timeout
	mov	cx,4000
kbibf1:	in	al,KBDSTA
	test	al,KBDIBF
	jz	kbibf2
	loop	kbibf1
	stc
	ret
kbibf2:	clc
	ret
kbibf	endp

; ---------------------------------------------------------------------------
; Time. DX:AX = hundredths of a second since midnight, from DOS. That is
; the K803 clock or the system tick, whichever the DOS uses.
dmvcs	proc	far
	push	bx
	push	cx
	call	fdmvpoll		; polled port: fetch what came
	mov	ah,2Ch
	int	dos			; CH h, CL m, DH s, DL hundredths
	call	fdmvpoll		; polled port: look again after DOS
	push	dx
	mov	al,ch
	mov	bl,60
	mul	bl			; AX = hours * 60
	xor	ch,ch
	add	ax,cx			; minutes since midnight (< 1440)
	mov	bx,6000
	mul	bx			; DX:AX = hundredths for the minutes
	pop	cx
	push	ax
	push	dx
	mov	al,ch			; seconds
	mov	bl,100
	mul	bl
	xor	ch,ch
	add	ax,cx			; + hundredths
	pop	dx
	pop	bx
	add	ax,bx
	adc	dx,0
	pop	cx
	pop	bx
	ret
dmvcs	endp

; DX:AX = IBM BIOS clock ticks since midnight (18.2 per second), made
; from the hundredths: * 11 / 60 (0.7 % slow, good enough for timeouts).
; Also kept in the stand-in BIOS data area at 6Ch.
dmvtick	proc	far
	push	bx
	push	cx
	push	es
	cmp	dmvpoll,0		; polled port: the DOS clock call takes
	je	dmvtk1			;  longer than two characters at 9600
	dec	dmvtcnt			;  Baud, so ask DOS only every 8th time
	jz	dmvtk0			;  and hand out the last value between
	mov	es,dmvbda
	mov	ax,es:[6Ch]
	mov	dx,es:[6Eh]
	jmp	short dmvtk9
dmvtk0:	mov	dmvtcnt,8
dmvtk1:	call	dmvcs
	mov	cx,dx			; times 11
	mov	bx,ax
	shl	ax,1
	rcl	dx,1			; *2
	shl	ax,1
	rcl	dx,1			; *4
	shl	ax,1
	rcl	dx,1			; *8
	add	ax,bx
	adc	dx,cx			; *9
	add	ax,bx
	adc	dx,cx			; *10
	add	ax,bx
	adc	dx,cx			; *11
	mov	bx,60			; long division by 60
	mov	cx,ax
	mov	ax,dx
	xor	dx,dx
	div	bx			; high word
	push	ax
	mov	ax,cx
	div	bx			; low word, remainder in DX
	pop	dx
	mov	es,dmvbda
	mov	es:[6Ch],ax
	mov	es:[6Eh],dx
dmvtk9:	pop	es
	pop	cx
	pop	bx
	ret
dmvtick	endp

; ---------------------------------------------------------------------------
; Find the serial modules: 2651 (K211, K212, K213) or 2661 (K801), after
; the method of the DR-DOS ROS (ros_ifprobe) and the DOS 3.3 start-up
; screen. Only ports x0..x3 are read; on the 2651/2661 address bit 2 is
; the R/W line, a read of x4..x7 arrives as a write. The K211 family
; answers in both halves of its IFSEL group, so the B half counts only if
; the A half did not have a 2651. Half 1A is searched first: NCR's DOS
; has AUX and COM1 there.
ifprobe	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	mov	dmvifn,0
	xor	si,si
ifpr1:	cmp	si,8
	jae	ifpr9
	mov	al,iforder[si]
	inc	si
	xor	ah,ah
	mov	dx,ax
	call	ifone			; AL = kind, 0 = none
	or	al,al
	jz	ifpr1
	test	dl,08h			; a B half of IFSEL 0 or 1?
	jz	ifpr2
	cmp	dl,80h
	jae	ifpr2
	cmp	dl,60h
	jb	ifpr2
	push	dx			; mirror of the A half?
	and	dl,0F0h
	call	ifhave
	pop	dx
	jc	ifpr1			; c = yes, skip
ifpr2:	mov	bl,dmvifn
	cmp	bl,4
	jae	ifpr9
	xor	bh,bh
	mov	dmvif[bx],dl
	mov	dmvifk[bx],al
	inc	dmvifn
	jmp	short ifpr1
ifpr9:	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
ifprobe	endp

; Is base port DL already in the list? Carry set if so.
ifhave	proc	near
	push	bx
	xor	bx,bx
ifhav1:	cmp	bl,dmvifn
	jae	ifhav8
	cmp	dmvif[bx],dl
	je	ifhav9
	inc	bx
	jmp	short ifhav1
ifhav8:	clc
	pop	bx
	ret
ifhav9:	stc
	pop	bx
	ret
ifhave	endp

; Probe base port DX: AL = 1 for a 2651, 2 for a 2661, 0 for nothing.
ifone	proc	near
	push	bx
	push	cx
	push	dx
	mov	bx,dx
	mov	cx,4			; x0..x3 all FFh: nothing there
	mov	ah,0FFh
ifone1:	in	al,dx
	and	ah,al
	inc	dx
	loop	ifone1
	cmp	ah,0FFh
	je	ifone8
	mov	dx,bx
	add	dx,3			; command register: bit 7 is 0 on a
	in	al,dx			;  2651 in use; an 8255 gives FFh or
	test	al,80h			;  its control word
	jnz	ifone8
	add	dx,3			; x6: write the mode registers
	mov	al,4Eh			; MR1: async 16x, 8 bits, no parity, 1 stop
	out	dx,al
	mov	al,7Eh			; MR2: internal clocks, 9600 Bd, bit 6
	out	dx,al			;  kept by the 2661 only
	sub	dx,3
	in	al,dx			; reading x3 resets the MR pointer
	dec	dx			; x2: read them back
	in	al,dx
	mov	ah,al			; MR1
	in	al,dx			; MR2
	cmp	ah,4Eh
	jne	ifone8
	mov	cl,2
	cmp	al,7Eh
	je	ifone2
	mov	cl,1
	cmp	al,3Eh
	jne	ifone8
ifone2:	inc	dx			; leave it at 8N1, 9600 Bd, transmitter
	in	al,dx			;  and receiver off
	add	dx,3
	mov	al,4Eh
	out	dx,al
	mov	al,3Eh
	out	dx,al
	mov	al,cl
	jmp	short ifone9
ifone8:	xor	al,al
ifone9:	pop	dx
	pop	cx
	pop	bx
	ret
ifone	endp

code2	ends
	end

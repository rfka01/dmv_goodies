;=====================================================================
; MURPHY.COM  -  Murphy's Laws for the AUTOEXEC.BAT
;
; A new version of MURPHY 2.0 by Charles J. C. Lyall (June 1986).
; The laws are no longer compiled into the program; they are read from
; the text file MURPHY.LAW, so anyone can add to them with an editor.
; The command line is the same as in Lyall's program:
;
;   MURPHY              one law chosen at random
;   MURPHY n            law number n (counting from 0)
;   MURPHY n m          laws n to m
;   MURPHY ?            short help and the number of laws
;
;   /A   set the source line ("Murphy's Corollary:") off in inverse
;        video with ANSI codes: ESC [7m on, ESC [0m off. This is the
;        default. Inverse is the one attribute that looks the same on
;        all consoles: the DMV ones of NCR MS-DOS 2.11 and of the ports
;        of MS-DOS 3.30 and DR-DOS 3.41, and ANSI.SYS on IBM compatibles.
;        (Bold, ESC [1m, means "full intensity" on the DMV, which is
;        normal there; half intensity, ESC [8m, is "concealed" on a PC;
;        and ESC [0m leaves the colour unchanged under NCR 2.11.)
;   /T   plain text without any codes - for PCs without ANSI.SYS.
; The switches may stand anywhere on the command line. The source line
; is the first line of a law if it ends with ":". When the output is
; not the console (MURPHY >PRN, MURPHY >FILE) it is always plain text.
;
; The laws are printed with two empty lines before and one after.
;
; MURPHY.LAW
; ----------
; Plain text. The laws are separated by lines that contain only a "%"
; (the format of the Unix fortune files). A law may have any number of
; lines; a line should not be longer than 79 characters. CR/LF and LF
; line ends are both accepted, Ctrl-Z ends the file.
;
; Where the file is looked for:
;   1. in the directory of MURPHY.COM itself (MS-DOS 3.0 and later,
;      which pass the program's path in the environment),
;   2. in the current directory,
;   3. in every directory of the PATH.
;
; Random choice
; -------------
; The file is read once. Law k replaces the law kept so far with the
; probability 1/k ("reservoir sampling"), so each law has the same
; chance and the file need not be counted first - a second pass over a
; 40 KB file costs noticeable time on a floppy. The random generator is
; a 16-bit xorshift seeded from the DOS date and time.
;
; Plain MS-DOS calls only (2.0 and later), no ports: the program runs
; on the DMV and on any other MS-DOS machine. Output goes to handle 1,
; so it can be redirected.
;
; Exit code: 0 ok, 1 MURPHY.LAW not found, 2 bad arguments.
;
; Build: BUILD.BAT (MASM 5.10) or build.sh (JWasm under Linux).
;
; (C) 2026 rfka01, MIT License. Code: Claude (Anthropic).
;=====================================================================

	.model	tiny
	.8086

cr	equ	0Dh
lf	equ	0Ah
LINEMAX	equ	255		; longest line kept (longer ones are cut)
SELMAX	equ	4096		; longest law kept for the random choice
RDSIZE	equ	2048		; read buffer

M_RAND	equ	0		; modes
M_RANGE	equ	1
M_COUNT	equ	2

	.code
	org	100h
start:	jmp	main

;---------------------------------------------------------------------
; data
;---------------------------------------------------------------------
lawname	db	'MURPHY.LAW',0
LAWNLEN	equ	$-lawname

msg_nf	db	'MURPHY.LAW not found.',cr,lf,'$'
msg_rd	db	'Error reading MURPHY.LAW.',cr,lf,'$'
msg_emp	db	'MURPHY.LAW contains no laws.',cr,lf,'$'
msg_ord	db	'Law 1 < Law 2, please',cr,lf,'$'
msg_arg	db	'Arg1 = law number',cr,lf,'$'
msg_onl	db	'There are only $'
msg_onl2 db	' laws, numbered from 0.',cr,lf,'$'
msg_hlp	db	'MURPHY with no arguments will print one law selected at random.'
	db	cr,lf
	db	'MURPHY followed by one numeric argument will print the specified law.'
	db	cr,lf
	db	'MURPHY followed by two numeric arguments (the first argument less'
	db	cr,lf
	db	'than the second) will print the laws from first argument to last'
	db	cr,lf
	db	'argument. For example, MURPHY 0 8 will print the first nine laws.'
	db	cr,lf,cr,lf
	db	'The source of a law is shown in inverse video with ANSI codes (/A,'
	db	cr,lf
	db	'the default; on a PC this needs ANSI.SYS). /T prints plain text.'
	db	cr,lf,cr,lf
	db	'The laws are kept in the text file MURPHY.LAW, separated by lines'
	db	cr,lf
	db	'that contain only a "%". Add your own with any editor.'
	db	cr,lf,cr,lf
	db	'MURPHY 3.0 for the NCR Decision Mate V, after MURPHY 2.0 (1986)'
	db	cr,lf
	db	'by Charles J. C. Lyall. https://github.com/rfka01/dmv_goodies'
	db	cr,lf,cr,lf,'$'
msg_cnt	db	'There are currently $'
msg_cnt2 db	' laws in the system.',cr,lf,'$'
crlf	db	cr,lf
ans_on	db	1Bh,'[7m'		; /A: inverse on, normal
ans_off	db	1Bh,'[0m'

hl	db	1			; highlight: 1 /A (default), 0 /T
hlon	dw	0			; codes before and after the source
hlonl	dw	0
hloff	dw	0
hloffl	dw	0
nnum	db	0			; numbers on the command line
first	db	0			; 1 on the first line of a law

mode	db	M_RAND
lo	dw	0		; range of laws to print
hi	dw	0
handle	dw	0
seed	dw	0
count	dw	0		; laws seen so far
inlaw	db	0		; 1 while inside a law
sel	db	0		; 1 if the current law is printed / kept
printed	dw	0		; laws printed in range mode
sellen	dw	0		; bytes in selbuf
linelen	dw	0
rdpos	dw	0		; read buffer: next byte, bytes in it
rdcnt	dw	0
eof	db	0

;---------------------------------------------------------------------
; main
;---------------------------------------------------------------------
main:	cld
	call	args
	jnc	m_hl
	mov	al,2
	jmp	quit

m_hl:	cmp	hl,0			; highlight only on the console
	je	m_seed
	mov	ax,4400h		; IOCTL: device information
	mov	bx,1
	int	21h
	jc	m_hl0
	and	dl,82h			; device + console output
	cmp	dl,82h
	je	m_hl1
m_hl0:	mov	hl,0
	jmp	short m_seed
m_hl1:	mov	hlon,offset ans_on
	mov	hloff,offset ans_off
	mov	hlonl,4
	mov	hloffl,4

m_seed:	mov	ah,2Ah			; date: CX year, DH month, DL day
	int	21h
	mov	bx,cx
	xor	bx,dx
	mov	ah,2Ch			; time: CH h, CL min, DH s, DL 1/100
	int	21h
	xor	bx,cx
	mov	ax,dx
	mov	cl,5
	rol	ax,cl
	xor	bx,ax
	jnz	m_s1
	mov	bx,0ACE1h		; xorshift must not start at 0
m_s1:	mov	seed,bx
	mov	cx,8			; stir a little
m_s2:	call	rand
	loop	m_s2

	call	findlaw
	jnc	m_read
	mov	dx,offset msg_nf
	call	pstr
	mov	al,1
	jmp	quit

m_read:	call	scan			; read the file, print or keep
	pushf
	mov	bx,handle
	mov	ah,3Eh
	int	21h
	popf
	jnc	m_done
	mov	dx,offset msg_rd
	call	pstr
	mov	al,1
	jmp	quit

m_done:	cmp	mode,M_COUNT
	jne	m_d1
	mov	dx,offset msg_hlp
	call	pstr
	mov	dx,offset msg_cnt
	call	pstr
	mov	ax,count
	call	pnum
	mov	dx,offset msg_cnt2
	call	pstr
	jmp	short m_ok

m_d1:	cmp	count,0
	jne	m_d2
	mov	dx,offset msg_emp
	call	pstr
	jmp	short m_ok

m_d2:	cmp	mode,M_RAND
	jne	m_d3
	call	blank2			; two blank lines before
	mov	dx,offset selbuf	; the law kept by the random choice
	mov	cx,sellen
	call	wout
	call	blank1			; one blank line after
	jmp	short m_ok

m_d3:	cmp	printed,0		; range: was anything in it?
	je	m_d4
	call	blank1			; one blank line after the last law
	jmp	short m_ok
m_d4:
	mov	dx,offset msg_onl
	call	pstr
	mov	ax,count
	call	pnum
	mov	dx,offset msg_onl2
	call	pstr

m_ok:	mov	al,0
quit:	mov	ah,4Ch
	int	21h

;---------------------------------------------------------------------
; args - read the command line.  CF=1 on error (message printed)
;---------------------------------------------------------------------
args:	mov	si,81h
a_next:	call	skipsp
	cmp	al,cr
	je	a_end
	cmp	al,'?'
	je	a_q
	cmp	al,'/'
	je	a_sw
	call	getnum
	jc	a_bad
	cmp	nnum,0
	jne	a_n2
	mov	lo,ax
	mov	hi,ax
	inc	nnum
	jmp	a_next
a_n2:	cmp	nnum,1
	jne	a_bad			; at most two numbers
	mov	hi,ax
	inc	nnum
	jmp	a_next

a_q:	inc	si			; ?
	mov	mode,M_COUNT
	jmp	short a_sep
a_sw:	inc	si			; /x
	mov	al,[si]
	inc	si
	and	al,0DFh			; upper case ('?' becomes 1Fh)
	cmp	al,'T'
	je	a_t
	cmp	al,'A'
	je	a_a
	cmp	al,'?' and 0DFh
	jne	a_bad
	mov	mode,M_COUNT
	jmp	short a_sep
a_t:	mov	hl,0
	jmp	short a_sep
a_a:	mov	hl,1
a_sep:	mov	al,[si]			; a blank or the end must follow
	cmp	al,' '
	je	a_next
	cmp	al,9
	je	a_next
	cmp	al,cr
	je	a_end
	cmp	al,'/'
	je	a_next
	jmp	short a_bad

a_end:	cmp	mode,M_COUNT
	je	a_ok
	cmp	nnum,0
	je	a_ok			; no numbers: random
	mov	mode,M_RANGE
	mov	ax,lo
	cmp	ax,hi
	jbe	a_ok
	mov	dx,offset msg_ord
	call	pstr
	stc
	ret
a_ok:	clc
	ret
a_bad:	mov	dx,offset msg_arg
	call	pstr
	stc
	ret

; skipsp - skip blanks and tabs at SI; AL = next character (not taken)
skipsp:	mov	al,[si]
	cmp	al,' '
	je	sk1
	cmp	al,9
	jne	sk2
sk1:	inc	si
	jmp	skipsp
sk2:	ret

; getnum - decimal number at SI -> AX.  CF=1 if none or too large.
getnum:	xor	bx,bx
	xor	cx,cx			; CX = digits read
gn1:	mov	al,[si]
	sub	al,'0'
	cmp	al,9
	ja	gn2
	inc	si
	inc	cx
	mov	ah,0
	push	ax
	mov	ax,10
	mul	bx
	pop	bx
	jc	gn3
	add	bx,ax
	jc	gn3
	jmp	gn1
gn2:	mov	al,[si]			; must end at a blank or CR
	cmp	al,' '
	je	gn4
	cmp	al,9
	je	gn4
	cmp	al,'/'
	je	gn4
	cmp	al,cr
	jne	gn3
gn4:	jcxz	gn3
	mov	ax,bx
	clc
	ret
gn3:	stc
	ret

;---------------------------------------------------------------------
; findlaw - open MURPHY.LAW, handle -> [handle].  CF=1 if not found
;---------------------------------------------------------------------
findlaw:
	mov	ah,30h			; DOS 3+: directory of the program
	int	21h
	cmp	al,3
	jb	fl_cur
	mov	es,ds:[2Ch]		; environment
	xor	di,di
	mov	cx,8000h
	xor	al,al
fl_e1:	repne	scasb			; skip the strings up to "",0
	jcxz	fl_cur0
	cmp	byte ptr es:[di],0
	jne	fl_e1
	add	di,3			; 0, word count -> program path
	mov	si,di
	mov	di,offset pathbuf
	mov	bx,di			; BX = behind the last \ or :
fl_e2:	mov	al,es:[si]
	inc	si
	cmp	di,offset pathbuf+100
	jae	fl_cur0
	mov	[di],al
	inc	di
	cmp	al,'\'
	je	fl_e3
	cmp	al,':'
	jne	fl_e4
fl_e3:	mov	bx,di
fl_e4:	or	al,al
	jnz	fl_e2
	push	ds
	pop	es
	mov	di,bx
	call	tryname
	jnc	fl_ret
fl_cur0:
	push	ds
	pop	es
fl_cur:	mov	di,offset pathbuf	; current directory
	call	tryname
	jnc	fl_ret

	mov	es,ds:[2Ch]		; search the PATH
	xor	di,di
fl_p1:	cmp	byte ptr es:[di],0
	je	fl_none
	cmp	word ptr es:[di],'AP'	; "PATH="
	jne	fl_p2
	cmp	word ptr es:[di+2],'HT'
	jne	fl_p2
	cmp	byte ptr es:[di+4],'='
	jne	fl_p2
	lea	si,[di+5]
	jmp	short fl_p3
fl_p2:	xor	al,al			; next variable
	mov	cx,8000h
	repne	scasb
	jcxz	fl_none
	jmp	fl_p1

fl_p3:	mov	di,offset pathbuf	; one directory of the PATH
fl_p4:	mov	al,es:[si]
	or	al,al
	jz	fl_p5
	inc	si
	cmp	al,';'
	je	fl_p5
	cmp	di,offset pathbuf+100
	jae	fl_p4
	mov	[di],al
	inc	di
	jmp	fl_p4
fl_p5:	cmp	di,offset pathbuf
	je	fl_p7			; empty entry
	mov	al,[di-1]
	cmp	al,'\'
	je	fl_p6
	cmp	al,':'
	je	fl_p6
	mov	byte ptr [di],'\'
	inc	di
fl_p6:	push	es
	push	si
	push	ds
	pop	es
	call	tryname
	pop	si
	pop	es
	jnc	fl_ret
fl_p7:	cmp	byte ptr es:[si],0
	je	fl_none
	jmp	fl_p3

fl_none:
	push	ds
	pop	es
	stc
	ret
fl_ret:	push	ds
	pop	es
	clc
	ret

; tryname - append MURPHY.LAW at DI (in pathbuf) and open it
tryname:
	mov	si,offset lawname
	mov	cx,LAWNLEN
	rep	movsb
	mov	dx,offset pathbuf
	mov	ax,3D00h
	int	21h
	jc	tn1
	mov	handle,ax
tn1:	ret

;---------------------------------------------------------------------
; scan - read MURPHY.LAW line by line.  CF=1 on a read error
;---------------------------------------------------------------------
scan:
sc_ln:	mov	linelen,0		; collect one line
	mov	di,offset linebuf
sc_ch:	call	getc
	jc	sc_err
	jz	sc_eof
	cmp	al,lf
	je	sc_eol
	cmp	al,cr
	je	sc_ch
	cmp	al,1Ah
	je	sc_eof
	cmp	linelen,LINEMAX
	jae	sc_ch			; cut overlong lines
	stosb
	inc	linelen
	jmp	sc_ch

sc_eol:	call	doline
	jmp	sc_ln

sc_eof:	cmp	linelen,0		; last line without line end
	je	sc_e1
	call	doline
sc_e1:	clc
	ret
sc_err:	stc
	ret

; getc - next byte of the file in AL.  ZF=1 at end, CF=1 on error
getc:	mov	bx,rdpos
	cmp	bx,rdcnt
	jb	gc2
	cmp	eof,0
	jne	gc_end
	push	cx
	push	dx
	mov	bx,handle
	mov	cx,RDSIZE
	mov	dx,offset rdbuf
	mov	ah,3Fh
	int	21h
	pop	dx
	pop	cx
	jc	gc_er
	mov	rdcnt,ax
	mov	rdpos,0
	or	ax,ax
	jnz	gc1
	mov	eof,1
gc_end:	xor	al,al			; ZF=1, CF=0
	ret
gc1:	xor	bx,bx
gc2:	mov	al,rdbuf[bx]
	inc	bx
	mov	rdpos,bx
	or	bx,bx			; ZF=0, CF=0
	ret
gc_er:	ret

;---------------------------------------------------------------------
; doline - one line of the file (linebuf, linelen, without line end)
;---------------------------------------------------------------------
doline:	mov	cx,linelen		; separator? "%" + blanks only
	mov	si,offset linebuf
	jcxz	dl_txt
	cmp	byte ptr [si],'%'
	jne	dl_txt
dl_s1:	inc	si
	dec	cx
	jz	dl_sep
	cmp	byte ptr [si],' '
	je	dl_s1
	cmp	byte ptr [si],9
	je	dl_s1
	jmp	short dl_txt
dl_sep:	mov	inlaw,0
	ret

dl_txt:	cmp	inlaw,0			; first line of a new law?
	jne	dl_keep
	cmp	linelen,0		; blank lines before a law: skip
	je	dl_ret
	mov	inlaw,1
	mov	first,1
	mov	ax,count		; AX = number of this law
	inc	count
	cmp	mode,M_RAND
	je	dl_rnd
	cmp	mode,M_COUNT
	je	dl_no
	cmp	ax,lo			; range mode
	jb	dl_no
	cmp	ax,hi
	ja	dl_no
	cmp	printed,0		; blank line between laws,
	je	dl_r0			; two before the first
	call	blank1
	jmp	short dl_r1
dl_r0:	call	blank2
dl_r1:	inc	printed
	jmp	short dl_yes

dl_rnd:	call	rand			; keep it with probability 1/count
	xor	dx,dx
	div	count
	or	dx,dx
	jnz	dl_no
	mov	sellen,0
dl_yes:	mov	sel,1
	jmp	short dl_keep
dl_no:	mov	sel,0

dl_keep:
	mov	al,first		; AL = 1 on the first line of a law
	mov	first,0
	cmp	sel,0
	je	dl_ret
	mov	ah,0			; AH = 1: source line, set it off
	cmp	hl,0
	je	dl_k1
	or	al,al
	jz	dl_k1
	mov	bx,linelen		; ends with ":" (blanks ignored)?
dl_k0:	or	bx,bx
	jz	dl_k1
	dec	bx
	mov	cl,linebuf[bx]
	cmp	cl,' '
	je	dl_k0
	cmp	cl,':'
	jne	dl_k1
	mov	ah,1
dl_k1:	push	ax
	or	ah,ah
	jz	dl_k2
	mov	dx,hlon
	mov	cx,hlonl
	call	put
dl_k2:	mov	dx,offset linebuf
	mov	cx,linelen
	call	put
	pop	ax
	or	ah,ah
	jz	dl_k3
	mov	dx,hloff
	mov	cx,hloffl
	call	put
dl_k3:	mov	dx,offset crlf
	mov	cx,2
	jmp	put
dl_ret:	ret

; put - CX bytes at DX: print them (range mode) or keep them in selbuf
; (random mode; a law longer than SELMAX is cut)
put:	cmp	mode,M_RAND
	jne	wout
	mov	di,sellen
	mov	ax,di
	add	ax,cx
	cmp	ax,SELMAX
	ja	pu1
	add	di,offset selbuf
	mov	si,dx
	rep	movsb
	sub	di,offset selbuf
	mov	sellen,di
pu1:	ret

;---------------------------------------------------------------------
; helpers
;---------------------------------------------------------------------
; rand - 16-bit xorshift (7,9,8) -> AX
rand:	push	cx
	mov	ax,seed
	mov	cx,ax
	shl	cx,1
	shl	cx,1
	shl	cx,1
	shl	cx,1
	shl	cx,1
	shl	cx,1
	shl	cx,1
	xor	ax,cx			; x ^= x << 7
	mov	cx,ax
	mov	cl,ch
	mov	ch,0
	shr	cl,1			; x >> 9
	xor	ax,cx
	mov	cx,ax
	mov	ch,cl
	mov	cl,0			; x << 8
	xor	ax,cx
	mov	seed,ax
	pop	cx
	ret

; wout - write CX bytes at DX to standard output
wout:	jcxz	o1
	mov	bx,1
	mov	ah,40h
	int	21h
o1:	ret

; blank2 / blank1 - two or one empty lines
blank2:	call	blank1
blank1:	mov	dx,offset crlf
	mov	cx,2
	jmp	wout

; pstr - print a $-terminated string at DX
pstr:	mov	ah,9
	int	21h
	ret

; pnum - print AX in decimal
pnum:	mov	bx,10
	xor	cx,cx
pn1:	xor	dx,dx
	div	bx
	push	dx
	inc	cx
	or	ax,ax
	jnz	pn1
pn2:	pop	dx
	add	dl,'0'
	mov	ah,2
	int	21h
	loop	pn2
	ret

;---------------------------------------------------------------------
; buffers (uninitialised, at the end: not stored in the .COM file)
;---------------------------------------------------------------------
	.data?
	even
pathbuf	db	128 dup (?)
linebuf	db	LINEMAX+1 dup (?)
rdbuf	db	RDSIZE dup (?)
selbuf	db	SELMAX dup (?)

	end	start

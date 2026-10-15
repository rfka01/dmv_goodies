; kons.inc - Ausgabe auf die DOS-Konsole
;
; Im Befehlsbetrieb laeuft alles ueber DOS. Im Terminal geht jedes Byte
; ueber Int 29h (schnelle Konsolenausgabe), die alle drei DOS-Fassungen
; der DMV haben.

; $-Text DS:DX ausgeben
textd	proc	near
	push	ax
	mov	ah,9
	int	21h
	pop	ax
	ret
textd	endp

; 0-Text DS:SI ausgeben
text0	proc	near
	push	ax
	push	dx
	push	si
tx01:	lodsb
	or	al,al
	jz	tx09
	mov	dl,al
	mov	ah,2
	int	21h
	jmp	short tx01
tx09:	pop	si
	pop	dx
	pop	ax
	ret
text0	endp

; Zeichen AL ueber DOS
zeichen	proc	near
	push	ax
	push	dx
	mov	dl,al
	mov	ah,2
	int	21h
	pop	dx
	pop	ax
	ret
zeichen	endp

crlf	proc	near
	push	ax
	mov	al,cr
	call	zeichen
	mov	al,lf
	call	zeichen
	pop	ax
	ret
crlf	endp

; AX dezimal ausgeben
zahl	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	mov	bx,10
	xor	cx,cx
zl1:	xor	dx,dx
	div	bx
	push	dx
	inc	cx
	or	ax,ax
	jnz	zl1
zl2:	pop	ax
	add	al,'0'
	call	zeichen
	loop	zl2
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
zahl	endp

; DX:AX dezimal ausgeben (bis 655359999)
zahl32	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	mov	bx,10000
	div	bx			; AX = obere Stellen, DX = untere 4
	or	ax,ax
	jz	z32b
	call	zahl
	mov	ax,dx			; untere vier Stellen mit Nullen
	mov	cx,4
	mov	bx,10
z32a:	xor	dx,dx
	div	bx
	push	dx
	loop	z32a
	mov	cx,4
z32c:	pop	ax
	add	al,'0'
	call	zeichen
	loop	z32c
	jmp	short z32d
z32b:	mov	ax,dx
	call	zahl
z32d:	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
zahl32	endp

; AL hexadezimal (zwei Stellen)
hex2	proc	near
	push	ax
	push	cx
	mov	ah,al
	mov	cl,4
	shr	al,cl
	call	hex1
	mov	al,ah
	and	al,0Fh
	call	hex1
	pop	cx
	pop	ax
	ret
hex1:	add	al,'0'
	cmp	al,'9'
	jbe	hx1
	add	al,7
hx1:	jmp	zeichen
hex2	endp

; Byte AL roh an die Konsole (Int 29h)
kroh	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	es
	int	29h
	pop	es
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
kroh	endp

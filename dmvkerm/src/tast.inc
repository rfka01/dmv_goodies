; tast.inc - Tastatur der DMV, direkt am 8741 gelesen
;
; Im Terminal werden die Tasten selbst am 8741 (40h/41h) abgeholt, damit
; Cursor- und Funktionstasten ankommen. Tasten, die DOS schon gelesen und
; umgesetzt hat (sein Interrupt kann schneller sein), holen wir ueber
; Int 21h nach. Ergebnis: AH = 0 und AL das Zeichen, oder AH = 1 und AL
; ein Tastencode K_xxx (tabellen.inc).

KBDDAT	equ	40h
KBDSTA	equ	41h
KBDOBF	equ	01h		; Byte fuer uns da
KBDIBF	equ	02h		; 8741 hat unser Byte noch nicht
KBDLAND	equ	80h		; das Byte ist der Landescode

kbland	db	0		; Landescode des 8741, 3 = deutsch
kbkopf	db	0		; Ringpuffer der Tasten
kbende	db	0
kbpuf	dw	16 dup (0)
kbdos	db	0		; Zaehler fuer die DOS-Nachfrage

; Landescode erfragen (Befehl 01h). Ein altes 8741 antwortet vielleicht
; nicht: nach einer Weile aufgeben.
kbinit	proc	near
	push	ax
	push	cx
	xor	cx,cx
kbi1:	in	al,KBDSTA
	test	al,KBDIBF
	jz	kbi2
	loop	kbi1
	jmp	short kbi9
kbi2:	mov	al,1
	out	KBDSTA,al
	xor	cx,cx
kbi3:	in	al,KBDSTA
	test	al,KBDOBF
	jz	kbi5
	test	al,KBDLAND
	jz	kbi4
	in	al,KBDDAT
	and	al,7
	mov	kbland,al
	jmp	short kbi9
kbi4:	in	al,KBDDAT		; eine Taste dazwischen
	call	kbumsetz
	jz	kbi5
	call	kbrein
kbi5:	loop	kbi3
kbi9:	pop	cx
	pop	ax
	ret
kbinit	endp

; Taste holen ohne zu warten: ZF = 1 keine, sonst AX = Taste.
kbhol	proc	near
	call	kbfuell
	push	bx
	mov	bl,kbkopf
	cmp	bl,kbende
	je	kbhol9
	xor	bh,bh
	shl	bx,1
	mov	ax,kbpuf[bx]
	inc	kbkopf
	and	kbkopf,0Fh
	or	bl,1			; ZF = 0
kbhol9:	pop	bx
	ret
kbhol	endp

; Taste AX in den Puffer (voll: verwerfen)
kbrein	proc	near
	push	bx
	push	cx
	mov	bl,kbende
	mov	cl,bl
	inc	cl
	and	cl,0Fh
	cmp	cl,kbkopf
	je	kbrein9
	xor	bh,bh
	shl	bx,1
	mov	kbpuf[bx],ax
	mov	kbende,cl
kbrein9:pop	cx
	pop	bx
	ret
kbrein	endp

; Was der 8741 hat, in den Puffer; hat er nichts, ab und zu DOS fragen.
kbfuell	proc	near
	push	ax
	push	bx
	push	dx
kbf1:	in	al,KBDSTA
	test	al,KBDOBF
	jz	kbf5
	test	al,KBDLAND
	jz	kbf2
	in	al,KBDDAT
	and	al,7
	mov	kbland,al
	jmp	short kbf1
kbf2:	in	al,KBDDAT
	call	kbumsetz
	jz	kbf1
	call	kbrein
	jmp	short kbf1
kbf5:	mov	bl,kbkopf
	cmp	bl,kbende
	jne	kbf9
	inc	kbdos
	test	kbdos,0Fh
	jnz	kbf9
	mov	ah,0Bh			; DOS: liegt etwas an?
	int	21h
	or	al,al
	jz	kbf9
	mov	ah,6			; holen, ohne Echo und ohne Warten
	mov	dl,0FFh
	int	21h
	jz	kbf9
	call	kbdosum
	jz	kbf9
	call	kbrein
kbf9:	pop	dx
	pop	bx
	pop	ax
	ret
kbfuell	endp

; Rohcode AL -> AX Taste; ZF = 1 verwerfen
kbumsetz proc	near
	push	si
	push	cx
	xor	ah,ah
	mov	si,ax
	shl	si,1
	mov	cx,tastw[si]
	cmp	kbland,3		; deutsche Tastatur?
	jne	kbu4
	mov	si,offset tastde
	mov	ah,tastden
kbu1:	or	ah,ah
	jz	kbu4
	cmp	al,[si]
	je	kbu2
	add	si,3
	dec	ah
	jmp	short kbu1
kbu2:	mov	cx,[si+1]
kbu4:	mov	ax,cx
	or	ax,ax
	pop	cx
	pop	si
	ret
kbumsetz endp

; Zeichen AL von DOS -> AX Taste; ZF = 1 verwerfen
kbdosum	proc	near
	push	si
	push	cx
	xor	ah,ah
	mov	si,offset tastdos	; was die DOS-Konsolen schon umsetzten
	mov	cl,tastdosn		;  (Umlaute, Ctrl-CLR)
kbd1:	or	cl,cl
	jz	kbd3
	cmp	al,[si]
	je	kbd2
	add	si,3
	dec	cl
	jmp	short kbd1
kbd2:	mov	ax,[si+1]
	or	ax,ax
	jmp	short kbd9
kbd3:	call	kbumsetz		; sonst ist es, was der 8741 schickt
kbd9:	pop	cx
	pop	si
	ret
kbdosum	endp

; Auf eine Taste warten -> AX
kbwarte	proc	near
kbw1:	call	kbhol
	jz	kbw1
	ret
kbwarte	endp

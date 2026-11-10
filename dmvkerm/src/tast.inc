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
kbbrk	db	0		; DOS hat Ctrl-C gesehen (Int 23h)
kbwartl	db	0		; 1 = Antwort auf 01h (Landescode) steht aus

; Int 23h: Hat DOS selbst ein Ctrl-C gelesen (sein Tastaturinterrupt ist
; schneller als wir), wird es hier zur Taste 03h, statt DMVKERM zu beenden.
; DMVKERM haengt am seriellen Interrupt; ein Ende durch DOS liesse den
; Vektor ins Leere zeigen. DOS stellt Int 23h am Programmende selbst zurueck.
brkisr	proc	far
	mov	cs:kbbrk,1
	iret
brkisr	endp

; Landescode erfragen (Befehl 01h). Ein altes 8741 antwortet vielleicht
; nicht: nach einer Weile aufgeben; kommt die Antwort spaeter, faengt
; kbfuell sie ab, solange kbwartl steht.
; Geaenderte Firmware (rfka01/dmv_firmware) benutzt die Statusbits 7..4
; selbst: Keyclick Bit 6..4, der Synthesizer 7..4 dauernd ('C', 'E', 'D').
; Dort bedeutet Bit 7 nicht "Landescode", und 01h fragen wir nicht.
; Die Tastatur gilt dann als deutsch.
kbinit	proc	near
	push	ax
	push	cx
	in	al,KBDSTA
	test	al,0F0h
	jz	kbi0
	mov	kbland,3
	jmp	short kbi9
kbi0:	xor	cx,cx
kbi1:	in	al,KBDSTA
	test	al,KBDIBF
	jz	kbi2
	loop	kbi1
	jmp	short kbi9
kbi2:	mov	kbwartl,1
	mov	al,1
	out	KBDSTA,al
	xor	cx,cx
kbi3:	call	kbfuell			; Tasten dazwischen kommen in den Puffer
	cmp	kbwartl,0
	je	kbi9
	loop	kbi3
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
	cmp	kbwartl,0		; Bit 7 heisst nur dann Landescode, wenn
	je	kbf2			;  wir danach gefragt haben
	test	al,KBDLAND
	jz	kbf2
	in	al,KBDDAT
	and	al,7
	mov	kbland,al
	mov	kbwartl,0
	jmp	short kbf1
kbf2:	in	al,KBDDAT
	call	kbumsetz
	jz	kbf1
	call	kbrein
	jmp	short kbf1
kbf5:	mov	bl,kbkopf
	cmp	bl,kbende
	jne	kbf9
	cmp	kbbrk,0			; Ctrl-C hat DOS schon abgefangen
	jne	kbf7
	inc	kbdos
	test	kbdos,0Fh
	jnz	kbf9
	mov	ah,0Bh			; DOS: liegt etwas an?
	int	21h
	cmp	kbbrk,0
	je	kbf6
kbf7:	mov	kbbrk,0
	mov	ax,3
	call	kbrein
	jmp	short kbf9
kbf6:	or	al,al
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

; Tastenpuffer leeren: unser Ring, was der 8741 noch hat, der DOS-Puffer
kbleer	proc	near
	push	ax
	call	kbfuell
	mov	al,kbende
	mov	kbkopf,al
	mov	ax,0C00h
	int	21h
	mov	kbbrk,0
	pop	ax
	ret
kbleer	endp

; Auf eine Taste warten -> AX
kbwarte	proc	near
kbw1:	call	kbhol
	jz	kbw1
	ret
kbwarte	endp

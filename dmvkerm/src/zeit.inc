; zeit.inc - Wartezeiten und Uhr
;
; Die DMV hat keinen PC-kompatiblen Zeitgeber. Gewartet wird mit einer
; Schleife, die beim Start gegen die DOS-Uhr geeicht wird; Zeitgrenzen der
; Protokolle laufen ueber die DOS-Uhr (Hundertstel).

schl_ms	dw	200			; Schleifendurchlaeufe je Millisekunde

; Hundertstel seit Mitternacht -> DX:AX
uhr	proc	near
	push	bx
	push	cx
	mov	ah,2Ch
	int	21h			; CH h, CL min, DH s, DL 1/100
	push	dx
	mov	al,ch
	mov	bl,60
	mul	bl
	xor	ch,ch
	add	ax,cx			; Minuten seit Mitternacht
	mov	bx,6000
	mul	bx			; DX:AX Hundertstel der Minuten
	pop	cx			; CH s, CL 1/100
	push	dx
	push	ax
	mov	al,ch
	mov	bl,100
	mul	bl
	xor	ch,ch
	add	ax,cx			; Hundertstel der Sekunden
	pop	cx
	pop	dx
	add	ax,cx
	adc	dx,0
	pop	cx
	pop	bx
	ret
uhr	endp

; AX Millisekunden warten
wartems	proc	near
	push	ax
	push	cx
	push	dx
	mov	dx,ax
	or	dx,dx
	jz	wms9
wms1:	mov	cx,schl_ms
wms2:	in	al,41h			; Bus anfassen (Tastaturstatus)
	loop	wms2
	dec	dx
	jnz	wms1
wms9:	pop	dx
	pop	cx
	pop	ax
	ret
wartems	endp

; Eichen: Schleifen zu je 1000 Durchlaeufen zaehlen, bis die Uhr
; mindestens 20 Hundertstel weiter ist (sie springt je nach DOS in 1, 5,5
; oder 10 Hundertstel). Steht die Uhr, bleibt die Vorgabe.
zeitini	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	call	uhr
	mov	bx,ax
	xor	cx,cx
zti1:	call	uhr			; auf einen Sprung warten
	cmp	ax,bx
	jne	zti1a
	loop	zti1			; (CX zaehlt nur gegen Haengen)
	jmp	short zti8
zti1a:	mov	bx,ax			; Anfang
	xor	si,si			; Zaehler
zti2:	mov	cx,1000
zti3:	in	al,41h
	loop	zti3
	inc	si
	jz	zti8
	call	uhr
	sub	ax,bx
	cmp	ax,20
	jb	zti2
	cmp	ax,500			; Mitternacht oder Unsinn
	ja	zti8
	mov	bx,ax			; je ms = Zaehler*1000 / (Hundertstel*10)
	mov	ax,si
	mov	dx,100
	mul	dx			; DX:AX = Zaehler*100
	div	bx
	or	ax,ax
	jz	zti8
	mov	schl_ms,ax
zti8:	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
zeitini	endp

; kbef.inc - Befehle fuer die Kermit-Uebertragung

; Rest der Zeile (Originalschreibung) als Muster; Carry, wenn leer
restmuster proc	near
	call	naechstes
	jc	rm9
	mov	si,wanf			; gleiche Stelle in zorig
	sub	si,offset zpuf+2
	add	si,offset zorig
	mov	cx,wlen
	mov	di,offset roh
	cmp	cx,63
	jbe	rm1
	mov	cx,63
rm1:	rep	movsb
	mov	byte ptr [di],0
	clc
rm9:	ret
restmuster endp

; Gemeinsamer Anfang: Port da? Carry wenn nicht
kbereit	proc	near
	cmp	poffen,0
	jne	kb1
	mov	dx,offset m_keinp
	call	textd
	stc
	ret
kb1:	call	kstart
	clc
	ret
kbereit	endp

b_send	proc	near
	call	restmuster
	jnc	bse1
	mov	dx,offset m_fehlt
	jmp	textd
bse1:	call	kbereit
	jc	bse9
	mov	si,offset b_tkerm
	call	bildan
	mov	si,offset roh
	call	setmuster
	call	ksend
	call	bildaus
bse9:	ret
b_send	endp

b_receive proc	near
	call	kbereit
	jc	bre9
	mov	si,offset b_tkerm
	call	bildan
	mov	dx,offset a_warte
	call	hinweis
	mov	habS,0
	call	kempf
	call	bildaus
bre9:	ret
b_receive endp

b_get	proc	near
	call	restmuster
	jnc	bge1
	mov	dx,offset m_fehlt
	jmp	textd
bge1:	call	kbereit
	jc	bge9
	mov	si,offset roh		; Name unveraendert an den Server
	mov	di,offset fmuster
bge2:	lodsb
	stosb
	or	al,al
	jnz	bge2
	mov	si,offset b_tkerm
	call	bildan
	call	kget
	call	bildaus
bge9:	ret
b_get	endp

b_server proc	near
	call	kbereit
	jc	bsv9
	mov	si,offset b_tkerm
	call	bildan
	call	kserver
	call	bildaus
bsv9:	ret
b_server endp

b_finish proc	near
	call	kbereit
	jc	bfi9
	mov	al,'F'
	call	kgeneric
bfi9:	ret
b_finish endp

b_bye	proc	near
	call	kbereit
	jc	bby9
	mov	al,'L'
	call	kgeneric
bby9:	ret
b_bye	endp

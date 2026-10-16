; xmodem.inc - XMODEM: XSEND und XRECEIVE
;
; Pruefsumme oder CRC (die Empfangsseite waehlt mit NAK bzw. 'C'),
; Bloecke zu 128 oder 1024 Byte (STX). Gesendet werden 1-KB-Bloecke nur
; im CRC-Betrieb und nur, wenn SET XMODEM 1K gilt (Vorgabe); der Rest am
; Dateiende geht in 128-Byte-Bloecken. Der letzte Block wird mit 1Ah
; aufgefuellt, so will es XMODEM.

X_SOH	equ	01h
X_STX	equ	02h
X_EOT	equ	04h
X_ACK	equ	06h
X_NAK	equ	15h
X_CAN	equ	18h

x1k	db	1		; SET XMODEM 1K / 128
xcrc	db	0		; 1 = CRC-Betrieb
xblk	db	0		; Blocknummer
xlen	dw	0		; Blockgroesse
xrest	dw	0, 0		; noch zu sendende Bytes
	.data?
xpuf	db	1024 dup (?)
	.code

x_par	db	'?XMODEM braucht SET PARITY NONE.',cr,lf,'$'
x_warte	db	'Warte auf die Gegenseite (Abbruch mit ESC) ...',cr,lf,'$'
x_abbr	db	cr,lf,'?Abbruch durch die Gegenseite.',cr,lf,'$'
x_folge	db	cr,lf,'?Blocknummer passt nicht, Abbruch.',cr,lf,'$'

; Frist AL Sekunden fuer rzeich setzen
frist	proc	near
	push	ax
	push	bx
	push	dx
	mov	bl,100
	mul	bl
	mov	bx,ax
	call	uhr
	mov	rstart,ax
	mov	rstart+2,dx
	add	ax,bx
	adc	dx,0
	mov	rzeit,ax
	mov	rzeit+2,dx
	pop	dx
	pop	bx
	pop	ax
	ret
frist	endp

; Zeichen mit Frist AL Sekunden -> AL; Carry bei Ablauf/Abbruch
xzeich	proc	near
	call	frist
	jmp	rzeich
xzeich	endp

; Gegenseite abwinken: zweimal CAN
xcan	proc	near
	push	ax
	mov	al,X_CAN
	call	sputroh
	call	sputroh
	call	sputroh
	pop	ax
	ret
xcan	endp

; CRC-16 XMODEM (Polynom 1021h, Startwert 0) ueber CX Byte ab SI -> DX
crcx	proc	near
	push	ax
	push	bx
	push	cx
	push	si
	xor	dx,dx
crx1:	lodsb
	xor	dh,al
	mov	bx,8
crx2:	shl	dx,1
	jnc	crx3
	xor	dx,1021h
crx3:	dec	bx
	jnz	crx2
	loop	crx1
	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
crcx	endp

; Summe der CX Byte ab SI -> AL
summx	proc	near
	push	cx
	push	si
	xor	ah,ah
smx1:	lodsb
	add	ah,al
	loop	smx1
	mov	al,ah
	pop	si
	pop	cx
	ret
summx	endp

; Gemeinsamer Anfang: Port und Paritaet pruefen; Carry, wenn es nicht geht
xbereit	proc	near
	call	kbereit
	jc	xb9
	cmp	pparit,0
	je	xb1
	mov	dx,offset x_par
	call	textd
	stc
	ret
xb1:	mov	al,pfluss		; XON/XOFF stoert binaere Bloecke
	mov	xfluss,al
	mov	pfluss,0
	mov	kbytes,0
	mov	kbytes+2,0
	mov	fname,0
	clc
xb9:	ret
xbereit	endp
xfluss	db	0
xende	proc	near
	mov	al,xfluss
	mov	pfluss,al
	ret
xende	endp

; --- XSEND datei ------------------------------------------------------------
b_xsend	proc	near
	call	restmuster
	jnc	bxs1
	mov	dx,offset m_fehlt
	jmp	textd
bxs1:	call	xbereit
	jc	bxs9
	mov	si,offset roh		; Name fuer die Anzeige
	mov	di,offset fname
	mov	cx,13
bxs1a:	lodsb
	stosb
	or	al,al
	loopnz	bxs1a
	mov	byte ptr [di],0
	mov	ax,3D00h
	mov	dx,offset roh
	int	21h
	jnc	bxs2
	call	xende
	mov	dx,offset a_nicht
	jmp	meldung
bxs2:	mov	fhand,ax
	mov	fpos,0
	mov	fend,0
	mov	bx,ax			; Groesse: ans Ende und zurueck
	mov	ax,4202h
	xor	cx,cx
	xor	dx,dx
	int	21h
	mov	xrest,ax
	mov	xrest+2,dx
	mov	ax,4200h
	xor	cx,cx
	xor	dx,dx
	int	21h
	mov	a_rich,offset a_sende
	mov	si,offset b_txmod
	call	bildan
	mov	ax,xrest
	mov	kgross,ax
	mov	ax,xrest+2
	mov	kgross+2,ax
	mov	si,offset b_laeuft
	call	zustand
	mov	dx,offset x_warte
	call	hinweis
	call	xsenden
	call	fzu
	call	bildaus
	call	xende
bxs9:	ret
b_xsend	endp

xsenden	proc	near
	mov	cx,6			; auf C oder NAK warten (bis 60 s)
xs1:	mov	al,10
	call	xzeich
	jc	xs1t
	cmp	al,'C'
	je	xs1c
	cmp	al,X_NAK
	je	xs1n
	cmp	al,X_CAN
	je	xs_can
	jmp	short xs1
xs1t:	cmp	kabbr,0
	jne	xs_abbr
	loop	xs1
	mov	dx,offset a_zeit
	jmp	meldung
xs1c:	mov	xcrc,1
	jmp	short xs2
xs1n:	mov	xcrc,0
xs2:	mov	xblk,1
xs3:	mov	ax,xrest		; noch etwas da?
	or	ax,xrest+2
	jnz	xs3a
	jmp	xs_eot
xs3a:	mov	xlen,128		; Blockgroesse
	cmp	xcrc,0
	je	xs4
	cmp	x1k,0
	je	xs4
	cmp	xrest+2,0
	jne	xs3b
	cmp	xrest,1024
	jb	xs4
xs3b:	mov	xlen,1024
xs4:	mov	di,offset xpuf		; Block fuellen
	mov	cx,xlen
xs5:	call	fget
	jnc	xs5a
	mov	al,1Ah
	jmp	short xs5b
xs5a:	sub	xrest,1
	sbb	xrest+2,0
	add	kbytes,1
	adc	kbytes+2,0
xs5b:	stosb
	loop	xs5
	mov	versuch,0
xs6:	call	sget			; Veraltetes verwerfen
	jnz	xs6
	mov	al,X_SOH		; Block senden
	cmp	xlen,128
	je	xs6a
	mov	al,X_STX
xs6a:	call	sputroh
	mov	al,xblk
	call	sputroh
	not	al
	call	sputroh
	mov	si,offset xpuf
	mov	cx,xlen
xs7:	lodsb
	call	sputroh
	loop	xs7
	mov	si,offset xpuf
	mov	cx,xlen
	cmp	xcrc,0
	je	xs7s
	call	crcx
	mov	al,dh
	call	sputroh
	mov	al,dl
	call	sputroh
	jmp	short xs8
xs7s:	call	summx
	call	sputroh
xs8:	inc	kpakete
	mov	al,10			; Antwort
	call	xzeich
	jc	xs8t
	cmp	al,X_ACK
	je	xs9
	cmp	al,X_CAN
	je	xs_can
	cmp	al,X_NAK
	je	xs8w
	cmp	al,'C'			; 'C' nach dem ersten Block: wie NAK
	je	xs8w
	jmp	short xs8		; Muell: weiter warten
xs8t:	cmp	kabbr,0
	jne	xs_abbr
xs8w:	inc	kwdh
	call	zeige
	inc	versuch
	cmp	versuch,VERSUCHE
	jb	xs6
	call	xcan
	mov	dx,offset a_zeit
	jmp	meldung
xs9:	cmp	kfx,0			; ^X/^Z: abbrechen
	je	xs9a
	jmp	xs_abbr
xs9a:	inc	xblk
	call	zeige
	jmp	xs3
xs_eot:	mov	versuch,0
xs10:	call	sget
	jnz	xs10
	mov	al,X_EOT
	call	sputroh
	mov	al,10
	call	xzeich
	jc	xs10a
	cmp	al,X_ACK
	je	xs_ok
xs10a:	cmp	kabbr,0
	jne	xs_abbr
	inc	versuch
	cmp	versuch,VERSUCHE
	jb	xs10
xs_ok:	mov	dx,offset a_ok
	jmp	meldung
xs_can:	mov	dx,offset x_abbr
	jmp	meldung
xs_abbr: call	xcan
	mov	dx,offset a_abbr
	jmp	meldung
xsenden	endp

; --- XRECEIVE datei ---------------------------------------------------------
b_xreceive proc	near
	call	restmuster
	jnc	bxr1
	mov	dx,offset m_fehlt
	jmp	textd
bxr1:	call	xbereit
	jc	bxr9
	mov	si,offset roh
	mov	di,offset fpfad
	mov	cx,64
bxr2:	lodsb
	stosb
	or	al,al
	loopnz	bxr2
	mov	si,offset roh		; Anzeigename
	mov	di,offset fname
	mov	cx,13
bxr3:	lodsb
	stosb
	or	al,al
	loopnz	bxr3
	mov	byte ptr [di],0
	mov	ah,3Ch
	xor	cx,cx
	mov	dx,offset fpfad
	int	21h
	jnc	bxr4
	call	xende
	mov	dx,offset a_dfehl
	jmp	meldung
bxr4:	mov	fhand,ax
	mov	fpos,0
	mov	a_rich,offset a_empf
	mov	si,offset b_txmod
	call	bildan
	mov	si,offset b_laeuft
	call	zustand
	mov	dx,offset x_warte
	call	hinweis
	call	xempf
	jnc	bxr5
	call	ekaputt			; halbe Datei weg
	call	bildaus
	jmp	xende
bxr5:	call	fflush
	call	fzu
	call	bildaus
	jmp	xende
bxr9:	ret
b_xreceive endp

; Carry bei Fehler/Abbruch
xempf	proc	near
	mov	xcrc,1			; erst CRC anbieten ('C'), dann NAK
	mov	xblk,1
	mov	versuch,0
xr1:	mov	al,'C'
	cmp	versuch,4
	jb	xr1a
	mov	xcrc,0
	mov	al,X_NAK
xr1a:	call	sputroh
	mov	al,5			; erstes Zeichen (5 s)
	call	xzeich
	jnc	xr3
	cmp	kabbr,0
	jne	xr_abbr
	inc	versuch
	cmp	versuch,12		; gut eine Minute
	jb	xr1
	jmp	xr_zeit
xr2:	mov	al,10			; Kopf des naechsten Blocks
	call	xzeich
	jnc	xr3
	cmp	kabbr,0
	jne	xr_abbr
	jmp	xr_nak
xr3:	cmp	al,X_SOH
	je	xr3s
	cmp	al,X_STX
	je	xr3k
	cmp	al,X_EOT
	jne	xr3a
	jmp	xr_eot
xr3a:	cmp	al,X_CAN
	jne	xr2			; Muell ueberlesen
	jmp	xr_can
xr3s:	mov	xlen,128
	jmp	short xr4
xr3k:	mov	xlen,1024
xr4:	mov	al,1			; Blocknummer und Komplement
	call	xzeich
	jc	xr_nak
	mov	bl,al
	mov	al,1
	call	xzeich
	jc	xr_nak
	not	al
	cmp	al,bl
	jne	xr_nak0
	mov	di,offset xpuf		; Daten
	mov	cx,xlen
xr5:	mov	al,1
	call	xzeich
	jc	xr_nak
	stosb
	loop	xr5
	mov	si,offset xpuf		; Pruefung
	mov	cx,xlen
	cmp	xcrc,0
	je	xr6s
	call	crcx
	mov	al,1
	call	xzeich
	jc	xr_nak
	cmp	al,dh
	jne	xr_nak0
	mov	al,1
	call	xzeich
	jc	xr_nak
	cmp	al,dl
	jne	xr_nak0
	jmp	short xr7
xr6s:	call	summx
	mov	ah,al
	mov	al,1
	call	xzeich
	jc	xr_nak
	cmp	al,ah
	jne	xr_nak0
xr7:	inc	kpakete
	cmp	bl,xblk			; der erwartete Block
	je	xr8
	mov	al,xblk			; der vorige noch einmal: nur bestaetigen
	dec	al
	cmp	bl,al
	je	xr_ack
	call	xcan
	mov	dx,offset x_folge
	call	meldung
	stc
	ret
xr8:	mov	si,offset xpuf		; schreiben
	mov	cx,xlen
xr8a:	lodsb
	call	fputz
	jc	xr_dfe
	loop	xr8a
	inc	xblk
	call	zeige
xr_ack:	cmp	kfx,0			; ^X/^Z: abbrechen
	je	xr_ack1
	jmp	xr_abbr
xr_ack1: mov	versuch,0
	mov	al,X_ACK
	call	sputroh
	jmp	xr2
xr_nak0: mov	al,1			; Rest des kaputten Blocks abwarten
	call	xzeich
	jnc	xr_nak0
	cmp	kabbr,0
	jne	xr_abbr
xr_nak:	inc	kwdh
	inc	versuch
	cmp	versuch,VERSUCHE
	jae	xr_zeit
	call	zeige
	mov	al,X_NAK
	call	sputroh
	jmp	xr2
xr_eot:	mov	al,X_ACK
	call	sputroh
	mov	dx,offset a_ok
	call	meldung
	clc
	ret
xr_dfe:	call	xcan
	mov	dx,offset a_dfehl
	call	meldung
	stc
	ret
xr_can:	mov	dx,offset x_abbr
	call	meldung
	stc
	ret
xr_zeit: call	xcan
	mov	dx,offset a_zeit
	call	meldung
	stc
	ret
xr_abbr: call	xcan
	mov	dx,offset a_abbr
	call	meldung
	stc
	ret
xempf	endp

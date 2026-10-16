; kermit.inc - Kermit-Protokoll
;
; Pakete: SOH LEN SEQ TYPE [LENX1 LENX2 HCHECK] DATA CHECK CR
; Prueffolgen: Art 1 (6 Bit) und Art 3 (CRC-16 wie Kermit). Lange Pakete
; bis MEINLX Byte, Wiederholungskompression (~), Praefix fuer das 8. Bit
; (&) bei Paritaet, Steuerzeichenpraefix (#). Attributpaket mit der
; Dateigroesse. Keine Schiebefenster.
;
; Dateien werden so gespeichert, wie sie ankommen: DOS und Kermit fassen
; Textzeilen gleich ab (CR LF), eine Umsetzung ist nicht noetig.

SOH	equ	01h
MEINLX	equ	1000		; laengstes Paket, das wir annehmen
PKTGR	equ	MEINLX+20
VERSUCHE equ	10

; Unsere Wuensche
m_time	db	10		; Sekunden, die die Gegenseite auf uns wartet
m_chkt	db	'3'

; Was ausgehandelt ist
s_maxl	dw	80		; laengstes Paket, das wir senden duerfen
s_time	db	10		; Zeitgrenze fuer unseren Empfang
s_eol	db	cr
s_qctl	db	'#'		; Praefix, das die Gegenseite benutzt
s_npad	db	0
s_padc	db	0
chkt	db	1		; aktuelle Pruefart 1 oder 3
chkn	db	'1'		; ausgehandelte Pruefart (fuer nach S/I)
qbin	db	0		; 0 oder Praefix fuer das 8. Bit
rpref	db	0		; 0 oder Wiederholungspraefix
lang	db	0		; lange Pakete erlaubt
attr	db	0		; Attributpakete erlaubt

sseq	db	0		; Folgenummer
versuch	db	0

; Empfangenes Paket
rtyp	db	0
rseq	db	0
rlen	dw	0		; Laenge der (kodierten) Daten
rdat	dw	0		; Anfang der Daten in rpuf
	.data?
rpuf	db	PKTGR dup (?)
	.code
	.data?
spuf	db	PKTGR dup (?)
	.code
	.data?
sdat	db	PKTGR dup (?)	; kodierte Daten fuer das naechste Paket
	.code
sdlen	dw	0

; Zaehler fuer die Anzeige
kbytes	dw	0, 0		; Bytes der Datei (32 Bit)
kpakete	dw	0
kwdh	dw	0
kabbr	db	0		; 1 = Benutzer hat abgebrochen

; --- Pruefsummen ------------------------------------------------------------
; CRC-16 (Kermit: CCITT, umgekehrt, Startwert 0) ueber CX Byte ab SI -> DX
crc16	proc	near
	push	ax
	push	bx
	push	cx
	push	si
	xor	dx,dx
	jcxz	crc9
crc1:	lodsb
	xor	dl,al			; ein Byte, bitweise
	mov	bx,8
crc2:	shr	dx,1
	jnc	crc3
	xor	dx,8408h
crc3:	dec	bx
	jnz	crc2
	loop	crc1
crc9:	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
crc16	endp

; Summe ueber CX Byte ab SI -> AX
summe	proc	near
	push	cx
	push	si
	xor	ax,ax
	jcxz	sum9
	push	dx
	xor	dh,dh
sum1:	mov	dl,[si]
	add	ax,dx
	inc	si
	loop	sum1
	pop	dx
sum9:	pop	si
	pop	cx
	ret
summe	endp

; 6-Bit-Pruefzeichen aus Summe AX -> AL (schon tochar)
chk1	proc	near
	push	bx
	mov	bx,ax
	and	bl,0C0h
	shr	bl,1
	shr	bl,1
	shr	bl,1
	shr	bl,1
	shr	bl,1
	shr	bl,1
	add	al,bl
	and	al,3Fh
	add	al,' '
	pop	bx
	ret
chk1	endp

; Prueffolge der aktuellen Art ueber CX Byte ab SI an DI anhaengen
pruefen	proc	near
	push	ax
	push	dx
	cmp	chkt,3
	je	prf3
	call	summe
	call	chk1
	stosb
	jmp	short prf9
prf3:	call	crc16
	mov	al,dh
	shr	al,1
	shr	al,1
	shr	al,1
	shr	al,1
	add	al,' '
	stosb
	mov	ax,dx
	shr	ax,1
	shr	ax,1
	shr	ax,1
	shr	ax,1
	shr	ax,1
	shr	ax,1
	and	al,3Fh
	add	al,' '
	stosb
	mov	al,dl
	and	al,3Fh
	add	al,' '
	stosb
prf9:	pop	dx
	pop	ax
	ret
pruefen	endp

; --- Paket senden -----------------------------------------------------------
; AL = Typ, Daten in sdat (sdlen), Folgenummer sseq
spack	proc	near
	push	ax
	push	bx
	push	cx
	push	si
	push	di
	mov	bl,al			; Typ
spk_v:	call	sget			; Veraltetes (doppelte Pakete) verwerfen
	jnz	spk_v
	mov	di,offset spuf
	mov	al,SOH
	stosb
	mov	cx,sdlen
	mov	ax,cx
	add	ax,2			; SEQ TYPE
	add	al,chkt			; + Pruefzeichen (1 oder 3)
	adc	ah,0
	cmp	ax,94
	ja	spk_l
	add	al,' '			; kurz: LEN
	stosb
	mov	al,sseq
	add	al,' '
	stosb
	mov	al,bl
	stosb
	jmp	short spk_d
spk_l:	mov	al,' '			; lang: LEN = 0
	stosb
	mov	al,sseq
	add	al,' '
	stosb
	mov	al,bl
	stosb
	mov	ax,cx			; LENX = Daten + Pruefzeichen
	add	al,chkt
	adc	ah,0
	push	cx
	mov	cl,95
	div	cl			; AL = LENX1, AH = LENX2
	pop	cx
	add	al,' '
	stosb
	mov	al,ah
	add	al,' '
	stosb
	push	cx			; HCHECK ueber LEN..LENX2 (Art 1)
	mov	si,offset spuf+1
	mov	cx,5
	call	summe
	call	chk1
	stosb
	pop	cx
spk_d:	mov	si,offset sdat		; Daten
	rep	movsb
	mov	si,offset spuf+1	; Prueffolge ueber alles ab LEN
	mov	cx,di
	sub	cx,si
	call	pruefen
	mov	al,s_eol
	stosb
	mov	cl,s_npad		; Fuellzeichen vorweg
	xor	ch,ch
	jcxz	spk1
spk0:	mov	al,s_padc
	call	sput
	loop	spk0
spk1:	mov	cx,di
	mov	si,offset spuf
	sub	cx,si
	mov	spuflen,cx
	cmp	bl,'D'			; Laenge fuer die Anzeige
	jne	spk1a
	push	ax
	mov	ax,sdlen
	mov	kplen,ax
	pop	ax
spk1a:
spk2:	lodsb
	call	sput
	loop	spk2
	inc	kpakete
	pop	di
	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
spack	endp

; Paket mit Zeichenfolge DS:SI (0-Ende) als Daten senden, Typ AL
spackt	proc	near
	push	ax
	push	cx
	push	si
	push	di
	mov	di,offset sdat
	xor	cx,cx
spt1:	mov	ah,[si]
	or	ah,ah
	jz	spt2
	mov	[di],ah
	inc	si
	inc	di
	inc	cx
	jmp	short spt1
spt2:	mov	sdlen,cx
	pop	di
	pop	si
	pop	cx
	pop	ax
	jmp	spack
spackt	endp

; Leeres Paket Typ AL
spackl	proc	near
	mov	sdlen,0
	jmp	spack
spackl	endp

; --- Paket empfangen --------------------------------------------------------
; Wartet hoechstens s_time Sekunden. rtyp = Typ, 'T' bei Zeitablauf, 'Q'
; bei kaputtem Paket, 'a' bei Abbruch durch den Benutzer.
rzeit	dw	0, 0
rstart	dw	0, 0
rchkt	db	0		; Pruefart vor einem S/I
rpack	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	mov	al,s_time		; Frist: jetzt + s_time Sekunden
	or	al,al
	jnz	rpk0
	mov	al,10
rpk0:	mov	bl,100
	mul	bl
	mov	bx,ax
	call	uhr
	mov	rstart,ax
	mov	rstart+2,dx
	add	ax,bx
	adc	dx,0
	mov	rzeit,ax
	mov	rzeit+2,dx
rpk1:	call	rzeich			; auf SOH warten
	jc	rpk_t
	cmp	al,SOH
	jne	rpk1
rpk2:	mov	di,offset rpuf		; LEN
	call	rzeich
	jc	rpk_t
	cmp	al,SOH
	je	rpk2
	stosb
	sub	al,' '
	mov	bl,al			; BL = LEN
	call	rzeich			; SEQ
	jc	rpk_t
	cmp	al,SOH
	je	rpk2
	stosb
	sub	al,' '
	mov	rseq,al
	call	rzeich			; TYPE
	jc	rpk_t
	cmp	al,SOH
	je	rpk2
	stosb
	mov	rtyp,al
	cmp	rchkt,0			; S und I tragen immer Pruefart 1,
	jne	rpk2r			;  auch wenn sie wiederholt kommen
	mov	ah,chkt
	mov	rchkt,ah
rpk2r:
	cmp	al,'S'
	je	rpk2s
	cmp	al,'I'
	jne	rpk2t
rpk2s:	mov	chkt,1
rpk2t:	or	bl,bl
	jz	rpk_l
	cmp	bl,3			; kurz: Daten = LEN - 2 - Pruefzeichen
	jb	rpk_q
	xor	bh,bh
	sub	bx,2
	jmp	short rpk_d
rpk_l:	call	rzeich			; lang: LENX1 LENX2 HCHECK
	jc	rpk_t
	stosb
	sub	al,' '
	mov	cl,95
	mul	cl
	mov	bx,ax
	call	rzeich
	jc	rpk_t
	stosb
	sub	al,' '
	xor	ah,ah
	add	bx,ax
	call	rzeich
	jc	rpk_t
	push	ax
	push	bx
	mov	si,offset rpuf
	mov	cx,5
	call	summe
	call	chk1
	pop	bx
	pop	cx
	cmp	al,cl
	jne	rpk_q
	mov	al,cl
	stosb
	cmp	bx,MEINLX+5
	ja	rpk_q
rpk_d:	mov	rdat,di			; BX = Daten + Pruefzeichen
	mov	cx,bx
	jcxz	rpk_q
rpk3:	call	rzeich
	jc	rpk_t
	cmp	al,SOH			; neues Paket mitten drin
	je	rpk2
	stosb
	loop	rpk3
	; Pruefart: die laut Paketlaenge (I/S und ihre Antworten: Art 1)
	mov	cl,chkt
	xor	ch,ch
	sub	bx,cx
	jc	rpk_q
	mov	rlen,bx
	cmp	rtyp,'D'		; Laenge fuer die Anzeige
	jne	rpk3a
	mov	kplen,bx
rpk3a:	mov	si,offset rpuf
	mov	cx,di
	sub	cx,si
	sub	cl,chkt
	sbb	ch,0
	push	di
	mov	di,offset sdat+PKTGR-8	; Prueffolge rechnen (Ablage hinten)
	push	di
	call	pruefen
	pop	si
	pop	di
	mov	cl,chkt
	xor	ch,ch
	sub	di,cx			; empfangene Prueffolge
rpk4:	mov	al,[si]
	cmp	al,[di]
	jne	rpk_q
	inc	si
	inc	di
	loop	rpk4
	jmp	short rpk9
rpk_q:	mov	rtyp,'Q'
	jmp	short rpk9
rpk_t:	cmp	kabbr,0
	je	rpk_t1
	mov	rtyp,'a'
	jmp	short rpk9
rpk_t1:	mov	rtyp,'T'
rpk9:	mov	al,rchkt
	or	al,al
	jz	rpk9a
	mov	chkt,al
	mov	rchkt,0
rpk9a:	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
rpack	endp

; Ein Zeichen von der Leitung, bis zur Frist rzeit. Carry bei Ablauf oder
; Abbruch (ESC, Ctrl-C, Ctrl-CLR).
h_x	db	'Datei wird abgebrochen ...$'
h_z	db	'Alle Dateien werden abgebrochen ...$'
rzeich	proc	near
	push	bx
	push	dx
rz1:	call	sget
	jnz	rz8
	call	kbhol
	jz	rz2
	or	ah,ah
	jnz	rz2
	cmp	al,esc_
	je	rz_a
	cmp	al,3
	je	rz_a
	cmp	al,5			; ^E
	je	rz_a
	cmp	al,FLUCHT
	je	rz_a
	cmp	al,18h			; ^X: diese Datei abbrechen
	jne	rz1x
	cmp	fhand,0			; noch keine Datei offen: ganz abbrechen
	je	rz_a
	mov	kfx,1
	mov	dx,offset h_x
	call	hinweis
	jmp	short rz2
rz1x:	cmp	al,1Ah			; ^Z: alle abbrechen
	jne	rz1y
	cmp	fhand,0
	je	rz_a
	mov	kfx,2
	mov	dx,offset h_z
	call	hinweis
	jmp	short rz2
rz1y:	cmp	al,cr			; Return: sofort wiederholen
	je	rz_t
rz2:	call	uhr
	cmp	dx,rstart+2		; Mitternacht: als Ablauf werten
	jb	rz_t
	ja	rz2a
	cmp	ax,rstart
	jb	rz_t
rz2a:	cmp	dx,rzeit+2
	jb	rz1
	ja	rz_t
	cmp	ax,rzeit
	jb	rz1
rz_t:	stc
	jmp	short rz9
rz_a:	mov	kabbr,1
	stc
	jmp	short rz9
rz8:	cmp	pparit,0		; bei Paritaet nur 7 Bit
	je	rz8a
	and	al,7Fh
rz8a:	clc
rz9:	pop	dx
	pop	bx
	ret
rzeich	endp

; --- Parameter (S, I und ihre Bestaetigung) ---------------------------------
; Unsere Parameter in sdat
mparam	proc	near
	push	ax
	push	di
	mov	di,offset sdat
	mov	al,94+' '		; MAXL
	stosb
	mov	al,m_time
	add	al,' '
	stosb
	mov	al,' '			; NPAD 0
	stosb
	mov	al,'@'			; PADC (ctl 0)
	stosb
	mov	al,cr+' '		; EOL
	stosb
	mov	al,'#'			; QCTL
	stosb
	mov	al,'Y'			; QBIN: ja, wenn ihr wollt
	cmp	pparit,0
	je	mpa1
	mov	al,'&'			; Paritaet: wir brauchen es
mpa1:	stosb
	mov	al,m_chkt		; CHKT
	stosb
	mov	al,'~'			; REPT
	stosb
	mov	al,2+8+' '		; CAPAS: lange Pakete, Attribute
	stosb
	mov	al,' '			; WINDO
	stosb
	mov	al,MEINLX/95+' '	; MAXLX1, MAXLX2
	stosb
	mov	al,MEINLX mod 95+' '
	stosb
	mov	sdlen,13
	pop	di
	pop	ax
	ret
mparam	endp

; Parameter der Gegenseite aus dem Paket (rdat, rlen) uebernehmen
gparam	proc	near
	push	ax
	push	bx
	push	cx
	push	si
	mov	si,rdat
	mov	cx,rlen
	mov	s_maxl,80		; Vorgaben
	mov	s_time,10
	mov	s_npad,0
	mov	s_eol,cr
	mov	s_qctl,'#'
	mov	qbin,0
	mov	chkn,'1'
	mov	rpref,0
	mov	lang,0
	mov	attr,0
	or	cx,cx
	jnz	gpa0
	jmp	gpa_q
gpa0:
	lodsb				; MAXL
	sub	al,' '
	xor	ah,ah
	cmp	al,10
	jb	gpa1
	mov	s_maxl,ax
gpa1:	dec	cx
	jz	gpa_q
	lodsb				; TIME
	sub	al,' '
	jz	gpa2
	mov	s_time,al
gpa2:	dec	cx
	jz	gpa_q
	lodsb				; NPAD
	sub	al,' '
	mov	s_npad,al
	dec	cx
	jz	gpa_q
	lodsb				; PADC
	xor	al,40h
	mov	s_padc,al
	dec	cx
	jz	gpa_q
	lodsb				; EOL
	sub	al,' '
	jz	gpa3
	mov	s_eol,al
gpa3:	dec	cx
	jz	gpa_q
	lodsb				; QCTL
	mov	s_qctl,al
	dec	cx
	jz	gpa_q
	lodsb				; QBIN
	call	qbinwahl
	dec	cx
	jz	gpa_q
	lodsb				; CHKT
	cmp	al,'3'			; wir koennen 1 und 3
	jne	gpa4
	cmp	m_chkt,'3'
	jne	gpa4
	mov	chkn,'3'
gpa4:	dec	cx
	jz	gpa_q
	lodsb				; REPT
	cmp	al,'~'
	jne	gpa5
	mov	rpref,'~'
gpa5:	dec	cx
	jz	gpa_q
	lodsb				; CAPAS (erstes Byte)
	sub	al,' '
	test	al,2
	jz	gpa6
	mov	lang,1
gpa6:	test	al,8
	jz	gpa7
	mov	attr,1
gpa7:	mov	ah,al			; weitere CAPAS-Bytes ueberspringen
gpa7a:	test	ah,1
	jz	gpa8
	dec	cx
	jz	gpa_q
	lodsb
	sub	al,' '
	mov	ah,al
	jmp	short gpa7a
gpa8:	dec	cx			; WINDO
	jz	gpa_q
	lodsb
	dec	cx
	jz	gpa_q
	cmp	lang,0
	je	gpa_q
	lodsb				; MAXLX1 MAXLX2
	sub	al,' '
	mov	bl,95
	mul	bl
	mov	bx,ax
	dec	cx
	jz	gpa_q
	lodsb
	sub	al,' '
	xor	ah,ah
	add	ax,bx
	jnz	gpa9
	mov	ax,500			; 0 heisst 500
gpa9:	cmp	ax,MEINLX
	jbe	gpa10
	mov	ax,MEINLX
gpa10:	mov	s_maxl,ax
gpa_q:	cmp	lang,0			; ohne lange Pakete hoechstens 94
	jne	gpa_r
	cmp	s_maxl,94
	jbe	gpa_r
	mov	s_maxl,94
gpa_r:	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
gparam	endp

; QBIN der Gegenseite AL bewerten
qbinwahl proc	near
	mov	qbin,0
	cmp	al,'Y'
	jne	qbw1
	cmp	pparit,0		; sie wuerde, wir wollen nur bei Paritaet
	je	qbw9
	mov	qbin,'&'
	ret
qbw1:	cmp	al,'!'			; ein gueltiges Praefix verlangt
	jb	qbw9
	cmp	al,'?'
	jbe	qbw2
	cmp	al,'`'
	jb	qbw9
	cmp	al,'~'
	ja	qbw9
qbw2:	mov	qbin,al
qbw9:	ret
qbinwahl endp

; --- Kodieren und Dekodieren ------------------------------------------------
; Byte AL (Wiederholung CL, 1..94) kodiert an DI anhaengen
kodier	proc	near
	push	ax
	push	bx
	cmp	cl,1
	jbe	kod1
	cmp	rpref,0
	je	kod1
	cmp	cl,3			; Wiederholung erst ab 3 lohnend
	jb	kod0
	mov	ah,rpref
	mov	[di],ah
	inc	di
	mov	ah,cl
	add	ah,' '
	mov	[di],ah
	inc	di
	jmp	short kod1
kod0:	push	cx			; 2 mal: einfach zweimal
	mov	cl,1
	call	kodier
	pop	cx
kod1:	cmp	qbin,0
	je	kod2
	test	al,80h
	jz	kod2
	mov	ah,qbin
	mov	[di],ah
	inc	di
	and	al,7Fh
kod2:	mov	bl,al
	and	bl,7Fh
	cmp	bl,' '			; Steuerzeichen
	jb	kod3
	cmp	bl,7Fh
	je	kod3
	cmp	bl,'#'			; eigene Praefixe
	je	kod4
	cmp	qbin,0
	je	kod2a
	cmp	bl,qbin
	je	kod4
kod2a:	cmp	rpref,0
	je	kod5
	cmp	bl,rpref
	je	kod4
	jmp	short kod5
kod3:	xor	al,40h
kod4:	mov	byte ptr [di],'#'
	inc	di
kod5:	mov	[di],al
	inc	di
	pop	bx
	pop	ax
	ret
kodier	endp

; Daten rdat/rlen dekodieren; jedes Byte geht an die Routine in BX
; (bekommt AL). Bei Fehler im Schreiben Carry.
dekod	proc	near
	push	ax
	push	cx
	push	dx
	push	si
	mov	si,rdat
	mov	cx,rlen
dek1:	jcxz	dek9
	mov	dl,1			; Wiederholung
	lodsb
	dec	cx
	cmp	rpref,0
	je	dek2
	cmp	al,rpref
	jne	dek2
	jcxz	dek9
	lodsb
	dec	cx
	sub	al,' '
	mov	dl,al
	jcxz	dek9
	lodsb
	dec	cx
dek2:	xor	dh,dh			; 8. Bit
	cmp	qbin,0
	je	dek3
	cmp	al,qbin
	jne	dek3
	mov	dh,80h
	jcxz	dek9
	lodsb
	dec	cx
dek3:	cmp	al,s_qctl
	jne	dek5
	jcxz	dek9
	lodsb
	dec	cx
	mov	ah,al
	and	ah,7Fh
	cmp	ah,'?'			; '?' -> DEL
	je	dek4
	cmp	ah,40h
	jb	dek5
	cmp	ah,5Fh
	ja	dek5
dek4:	xor	al,40h
dek5:	or	al,dh
	or	dl,dl
	jz	dek1
dek6:	call	bx
	jc	dek_f
	dec	dl
	jnz	dek6
	jmp	short dek1
dek9:	clc
dek_f:	pop	si
	pop	dx
	pop	cx
	pop	ax
	ret
dekod	endp

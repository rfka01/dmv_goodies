; extras.inc - LOG SESSION, CLOSE, TRANSMIT, DIR, CD, DEL, TAKE

; --- Mitschnitt -------------------------------------------------------------
lhand	dw	0		; Handle der Mitschnittdatei, 0 = aus
lpos	dw	0
	.data?
lpuf	db	512 dup (?)
	.code
lstd	db	'SESSION.LOG',0
l_an	db	'Mitschnitt in $'
l_aus	db	'Mitschnitt geschlossen.',cr,lf,'$'
l_kein	db	'?Kein Mitschnitt offen.',cr,lf,'$'
l_fehl	db	'?Datei l',k_ae,'sst sich nicht anlegen.',cr,lf,'$'

; LOG SESSION [datei] - haengt an eine vorhandene Datei an
lg_tab	db	7,'SESSION'
	dw	0
	db	0
b_log	proc	near
	mov	si,offset lg_tab
	call	wortaus
	jc	blg9
	call	lzu			; alter Mitschnitt zu
	call	restmuster
	jnc	blg1
	mov	si,offset lstd		; Vorgabe SESSION.LOG
	mov	di,offset roh
	mov	cx,12
	rep	movsb
blg1:	mov	ax,3D01h		; vorhanden: anhaengen
	mov	dx,offset roh
	int	21h
	jc	blg2
	mov	bx,ax
	mov	ax,4202h
	xor	cx,cx
	xor	dx,dx
	int	21h
	mov	ax,bx
	jmp	short blg3
blg2:	mov	ah,3Ch
	xor	cx,cx
	mov	dx,offset roh
	int	21h
	jnc	blg3
	mov	dx,offset l_fehl
	jmp	textd
blg3:	mov	lhand,ax
	mov	lpos,0
	mov	dx,offset l_an
	call	textd
	mov	si,offset roh
	call	text0
	call	crlf
blg9:	ret
b_log	endp

; CLOSE [SESSION]
b_close	proc	near
	cmp	lhand,0
	jne	bcl1
	mov	dx,offset l_kein
	jmp	textd
bcl1:	call	lzu
	mov	dx,offset l_aus
	jmp	textd
b_close	endp

; Zeichen AL mitschreiben (wenn ein Mitschnitt offen ist)
lzeich	proc	near
	cmp	lhand,0
	je	lz9
	push	bx
	mov	bx,lpos
	mov	lpuf[bx],al
	inc	bx
	mov	lpos,bx
	cmp	bx,512
	pop	bx
	jb	lz9
	call	lleer
lz9:	ret
lzeich	endp

lleer	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	mov	cx,lpos
	jcxz	lle9
	mov	ah,40h
	mov	bx,lhand
	mov	dx,offset lpuf
	int	21h
	mov	lpos,0
lle9:	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
lleer	endp

lzu	proc	near
	cmp	lhand,0
	je	lzu9
	call	lleer
	push	ax
	push	bx
	mov	ah,3Eh
	mov	bx,lhand
	int	21h
	mov	lhand,0
	pop	bx
	pop	ax
lzu9:	ret
lzu	endp

; --- TRANSMIT ---------------------------------------------------------------
; Textdatei so senden, als wuerde sie getippt: CR am Zeilenende (das LF
; danach entfaellt), nach jeder Zeile auf das Echo warten (hoechstens 1 s).
; Empfangenes erscheint auf dem Bildschirm. ESC bricht ab.
tr_text	db	'Sende die Datei als Text, ESC bricht ab.',cr,lf,'$'
tr_ok	db	cr,lf,'[TRANSMIT fertig]',cr,lf,'$'

b_transmit proc	near
	mov	trbin,0
	call	restmuster
	jc	btr0
	cmp	roh,'/'			; Schalter?
	jne	btr1
	mov	si,offset trsw_tab
	call	suche			; wanf/wlen zeigen noch auf das Wort
	jnc	btr0a
	mov	dx,offset tr_schal
	jmp	textd
btr0a:	mov	trbin,al
	call	restmuster
	jnc	btr1
btr0:	mov	dx,offset m_fehlt
	jmp	textd
btr1:	cmp	poffen,0
	jne	btr2
	mov	dx,offset m_keinp
	jmp	textd
btr2:	mov	ax,3D00h
	mov	dx,offset roh
	int	21h
	jnc	btr3
	mov	dx,offset a_nicht
	jmp	textd
btr3:	mov	fhand,ax
	mov	fpos,0
	mov	fend,0
	mov	kabbr,0
	cmp	trbin,0
	je	btr3a
	jmp	trbinaer
btr3a:	mov	dx,offset tr_text
	call	textd
btr4:	call	tzeigen			; Empfangenes zeigen, Abbruch?
	jc	btr8
	call	fget
	jc	btr8
	cmp	al,lf			; LF nach CR weglassen
	je	btr4
	cmp	al,1Ah			; Ctrl-Z: Dateiende
	je	btr8
	call	sput
	call	trwarte
	cmp	al,cr
	jne	btr4
	mov	al,1			; auf das Echo der Zeile warten
	call	frist
btr5:	call	sget
	jz	btr6
	call	tzeichen
	cmp	al,lf
	je	btr4
	jmp	short btr5
btr6:	call	kbhol
	jz	btr7
	cmp	al,esc_
	je	btr8
btr7:	call	uhr
	cmp	dx,rzeit+2
	jb	btr5
	ja	btr4
	cmp	ax,rzeit
	jb	btr5
	jmp	short btr4
btr8:	call	fzu
	mov	ax,300			; Nachzuegler zeigen
	call	wartems
	call	tzeigen
	mov	dx,offset tr_ok
	jmp	textd
b_transmit endp

trsw_tab db	7,'/BINARY'
	dw	1
	db	0
trbin	db	0
tr_schal db	'?Unbekannter Schalter, m',k_oe,'glich ist /BINARY',cr,lf,'$'
trpause	dw	0		; SET TRANSMIT PAUSE: ms nach jedem Zeichen
tr_bin	db	'Sende die Datei Byte f',k_ue,'r Byte ohne Protokoll, ESC bricht ab.',cr,lf,'$'
tr_7bit	db	'Achtung: 7 Datenbits, das oberste Bit wird nicht ',k_ue,'bertragen',cr,lf
	db	'(f',k_ue,'r RIM/BIN: SET SERIAL 8N1).',cr,lf,'$'
tr_byt	db	cr,'Gesendet: $'
tr_byte	db	' Bytes$'
tr_abbr	db	cr,lf,'[TRANSMIT abgebrochen]',cr,lf,'$'
tr_lese	db	cr,lf,'?Lesefehler, TRANSMIT abgebrochen',cr,lf,'$'
trzahl	dw	0,0

; Pause nach einem Zeichen (SET TRANSMIT PAUSE). Der Baustein nimmt das
; naechste Zeichen schon an, waehrend das vorige noch hinausgeht; damit
; auf der Leitung wirklich eine Luecke entsteht, kommt die Dauer eines
; Zeichens (Start, Daten, Pruefbit, Stopp: pzbits) dazu.
trwarte	proc	near
	cmp	trpause,0
	je	trw9
	push	ax
	push	bx
	push	dx
	mov	bl,pbaud
	xor	bh,bh
	shl	bx,1
	mov	bx,baudtab[bx]
	mov	al,pzbits		; Bits je Zeichen
	xor	ah,ah
	mov	dx,1000
	mul	dx			; DX:AX = Bits * 1000
	div	bx			; ms, aufgerundet
	or	dx,dx
	jz	trw1
	inc	ax
trw1:	add	ax,trpause
	call	wartems
	pop	dx
	pop	bx
	pop	ax
trw9:	ret
trwarte	endp

; TRANSMIT /BINARY: jedes Byte der Datei unveraendert senden (auch 1Ah,
; 00h, 80h), ohne Zeilenumbau und ohne auf ein Echo zu warten. Was
; ankommt, wird verworfen (etwa ein Echo der Gegenseite). Fuer
; Bandabbilder (RIM/BIN) von Rechnern wie der PDP-8.
trbinaer proc	near
	mov	dx,offset tr_bin
	call	textd
	cmp	pdaten,8
	je	trb0
	mov	dx,offset tr_7bit
	call	textd
trb0:
	mov	flesef,0
	mov	trzahl,0
	mov	trzahl+2,0
	call	trzeig
trb1:	call	sget			; Empfangenes verwerfen
	jnz	trb1
	call	kbhol
	jz	trb2
	or	ah,ah
	jnz	trb2
	cmp	al,esc_
	je	trb7
	cmp	al,3			; Ctrl-C
	je	trb7
trb2:	call	fget
	jc	trb6
	call	sput
	call	trwarte
	add	trzahl,1
	adc	trzahl+2,0
	test	trzahl,0FFh		; alle 256 Bytes anzeigen
	jnz	trb1
	call	trzeig
	jmp	short trb1
trb6:	call	trzeig
	call	fzu
	mov	dx,offset tr_ok
	cmp	flesef,0
	je	trb8
	mov	dx,offset tr_lese
	jmp	short trb8
trb7:	call	trzeig
	call	fzu
	mov	dx,offset tr_abbr
trb8:	call	textd
	call	kbleer
	ret
trbinaer endp

trzeig	proc	near
	push	ax
	push	dx
	mov	dx,offset tr_byt
	call	textd
	mov	ax,trzahl
	mov	dx,trzahl+2
	call	zahl32
	mov	dx,offset tr_byte
	call	textd
	pop	dx
	pop	ax
	ret
trzeig	endp

; Alles Empfangene zeigen; Carry bei ESC
tzeigen	proc	near
tzg1:	call	sget
	jz	tzg2
	call	tzeichen
	jmp	short tzg1
tzg2:	call	kbhol
	jz	tzg8
	or	ah,ah
	jnz	tzg8
	cmp	al,esc_
	jne	tzg8
	stc
	ret
tzg8:	clc
	ret
tzeigen	endp

; Empfangenes Zeichen zeigen (und mitschreiben)
tzeichen proc	near
	push	ax
	call	lzeich
	and	al,7Fh
	cmp	al,' '
	jae	tzc1
	cmp	al,cr
	je	tzc1
	cmp	al,lf
	je	tzc1
	cmp	al,bs
	jne	tzc9
tzc1:	call	kroh
tzc9:	pop	ax
	ret
tzeichen endp

; --- DIR, CD, DEL -----------------------------------------------------------
d_alle	db	'*.*',0
d_frei	db	' Byte frei',cr,lf,'$'
d_dir	db	'  <DIR>$'
c_fehl	db	'?Verzeichnis nicht gefunden.',cr,lf,'$'
e_ok	db	' gel',k_oe,'scht.',cr,lf,'$'
e_fehl	db	'?Nicht gel',k_oe,'scht (fehlt oder schreibgesch',k_ue,'tzt).',cr,lf,'$'

b_dir	proc	near
	call	restmuster
	jnc	bdi1
	mov	si,offset d_alle
	mov	di,offset roh
	mov	cx,4
	rep	movsb
bdi1:	mov	dx,offset dta
	mov	ah,1Ah
	int	21h
	mov	dx,offset roh
	mov	cx,10h			; auch Verzeichnisse
	mov	ah,4Eh
	int	21h
	jnc	bdi2
	mov	dx,offset a_nicht
	jmp	textd
bdi2:	mov	si,offset dta+1Eh	; Name auf 13 Stellen
	call	text0
	xor	cx,cx			; Laenge des Namens
bdi2a:	lodsb
	or	al,al
	jz	bdi3
	inc	cx
	jmp	short bdi2a
bdi3:	cmp	cx,14
	jae	bdi4
	mov	al,' '
	call	zeichen
	inc	cx
	jmp	short bdi3
bdi4:	test	byte ptr dta+15h,10h
	jz	bdi5
	mov	dx,offset d_dir
	call	textd
	jmp	short bdi6
bdi5:	mov	ax,word ptr dta+1Ah
	mov	dx,word ptr dta+1Ch
	call	zahl32
bdi6:	call	crlf
	mov	ah,4Fh
	int	21h
	jnc	bdi2
	mov	ah,36h			; freier Platz
	mov	dl,0
	int	21h
	cmp	ax,0FFFFh
	je	bdi9
	mul	cx			; Sektoren je Cluster * Bytes
	mul	bx			;  * freie Cluster -> DX:AX
	call	zahl32
	mov	dx,offset d_frei
	call	textd
bdi9:	ret
b_dir	endp

b_cd	proc	near
	call	restmuster
	jc	bcd5
	mov	dx,offset roh
	mov	ah,3Bh
	int	21h
	jnc	bcd5
	mov	dx,offset c_fehl
	jmp	textd
bcd5:	mov	al,'\'			; aktuelles Verzeichnis zeigen
	call	zeichen
	mov	si,offset roh
	mov	ah,47h
	mov	dl,0
	int	21h
	call	text0
	jmp	crlf
b_cd	endp

b_del	proc	near
	call	restmuster
	jnc	bde1
	mov	dx,offset m_fehlt
	jmp	textd
bde1:	mov	dx,offset roh
	mov	ah,41h
	int	21h
	jnc	bde2
	mov	dx,offset e_fehl
	jmp	textd
bde2:	mov	si,offset roh
	call	text0
	mov	dx,offset e_ok
	jmp	textd
b_del	endp

; --- TAKE -------------------------------------------------------------------
; Befehle aus einer Datei. Die Befehlsschleife holt ihre Zeilen von hier,
; solange thand offen ist; TAKE in TAKE ersetzt die laufende Datei.
thand	dw	0
tpos	dw	0
tend	dw	0
	.data?
tpuf	db	256 dup (?)
	.code
ini	db	'DMVKERM.INI',0
t_fehl	db	'?Befehlsdatei nicht gefunden.',cr,lf,'$'

b_take	proc	near
	call	restmuster
	jnc	bta1
	mov	dx,offset m_fehlt
	jmp	textd
bta1:	mov	dx,offset roh
	call	topen
	jnc	bta9
	mov	dx,offset t_fehl
	call	textd
bta9:	ret
b_take	endp

; Befehlsdatei DX oeffnen; Carry, wenn es sie nicht gibt
topen	proc	near
	call	tzu
	mov	ax,3D00h
	int	21h
	jc	top9
	mov	thand,ax
	mov	tpos,0
	mov	tend,0
top9:	ret
topen	endp

tzu	proc	near
	push	ax
	push	bx
	mov	bx,thand
	or	bx,bx
	jz	tzu9
	mov	ah,3Eh
	int	21h
	mov	thand,0
tzu9:	pop	bx
	pop	ax
	ret
tzu	endp

; Naechste Zeile aus der Befehlsdatei nach zpuf (wie DOS 0Ah, Laenge in
; zpuf+1); Carry am Dateiende (Datei ist dann zu)
tzeile	proc	near
	push	bx
	push	cx
	push	dx
	push	di
	mov	di,offset zpuf+2
	xor	cx,cx
tzl1:	mov	bx,tpos			; ein Byte holen
	cmp	bx,tend
	jb	tzl2
	push	cx
	mov	ah,3Fh
	mov	bx,thand
	mov	cx,256
	mov	dx,offset tpuf
	int	21h
	pop	cx
	jc	tzl_e
	or	ax,ax
	jz	tzl_e
	mov	tend,ax
	mov	tpos,0
	xor	bx,bx
tzl2:	mov	al,tpuf[bx]
	inc	tpos
	cmp	al,lf
	je	tzl1
	cmp	al,1Ah
	je	tzl_e
	cmp	al,cr
	je	tzl8
	cmp	cx,zeilmax
	jae	tzl1
	stosb
	inc	cx
	jmp	short tzl1
tzl_e:	call	tzu
	jcxz	tzl_f			; letzte Zeile ohne CR gilt auch
tzl8:	mov	zpuf+1,cl
	clc
	jmp	short tzl9
tzl_f:	stc
tzl9:	pop	di
	pop	dx
	pop	cx
	pop	bx
	ret
tzeile	endp

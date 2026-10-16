; transfer.inc - Kermit-Ablaeufe: SEND, RECEIVE, GET, FINISH, BYE, SERVER
;
; Anzeige im Befehlsbetrieb ueber DOS: eine Zeile je Datei, die mit CR
; immer wieder ueberschrieben wird.

; --- Dateien ----------------------------------------------------------------
fhand	dw	0		; DOS-Handle, 0 = keine Datei offen
fquelle	db	0		; 1 = fget liest aus mtext (Server-Antworten)
	.data?
fpuf	db	1024 dup (?)
	.code
fpos	dw	0
fend	dw	0
dta	db	43 dup (0)	; Suchergebnis (Name ab +1Eh, Groesse +1Ah)
fmuster	db	80 dup (0)	; SEND-Muster mit Pfad
fpfad	db	80 dup (0)	; voller Name der aktuellen Datei
fname	db	16 dup (0)	; Name ohne Pfad (gesendet/empfangen)
fpfadl	dw	0		; Laenge des Pfadteils im Muster

; Byte aus der Datei -> AL, Carry am Ende
fget	proc	near
	call	fpeek
	jc	fg9
	inc	fpos
fg9:	ret
fget	endp

; Naechstes Byte ansehen -> AL, Carry am Ende
fpeek	proc	near
	push	bx
	mov	bx,fpos
	cmp	bx,fend
	jb	fpk5
	cmp	fquelle,0		; Text im Speicher: zu Ende
	jne	fpk8
	push	cx
	push	dx
	mov	ah,3Fh			; nachlesen
	mov	bx,fhand
	mov	cx,1024
	mov	dx,offset fpuf
	int	21h
	pop	dx
	pop	cx
	jc	fpk8
	or	ax,ax
	jz	fpk8
	mov	fend,ax
	mov	fpos,0
	xor	bx,bx
fpk5:	cmp	fquelle,0
	jne	fpk6
	mov	al,fpuf[bx]
	clc
	pop	bx
	ret
fpk6:	mov	al,mtext[bx]
	clc
	pop	bx
	ret
fpk8:	stc
	pop	bx
	ret
fpeek	endp

; Byte AL in die Datei (gepuffert); Carry bei Fehler
fput	proc	near
	push	bx
	mov	bx,fpos
	mov	fpuf[bx],al
	inc	bx
	mov	fpos,bx
	cmp	bx,1024
	jb	fpu9
	call	fflush
	jmp	short fpu_r
fpu9:	clc
fpu_r:	pop	bx
	ret
fput	endp

; dasselbe mit Zaehlen fuer die Anzeige
fputz	proc	near
	add	kbytes,1
	adc	kbytes+2,0
	jmp	fput
fputz	endp

fflush	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	mov	cx,fpos
	jcxz	ffl8
	mov	ah,40h
	mov	bx,fhand
	mov	dx,offset fpuf
	int	21h
	jc	ffl9
	cmp	ax,cx			; Diskette voll
	jb	ffl_f
ffl8:	mov	fpos,0
	clc
	jmp	short ffl9
ffl_f:	stc
ffl9:	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
fflush	endp

fzu	proc	near
	push	ax
	push	bx
	mov	bx,fhand
	or	bx,bx
	jz	fzu9
	mov	ah,3Eh
	int	21h
	mov	fhand,0
fzu9:	pop	bx
	pop	ax
	ret
fzu	endp

; Byte zaehlen (kbytes += CX)
zaehl	proc	near
	add	kbytes,cx
	adc	kbytes+2,0
	ret
zaehl	endp

; --- Anzeige ----------------------------------------------------------------
a_sende	db	'Sende    $'
a_empf	db	'Empfange $'
a_bytes	db	'  Bytes $'
a_pak	db	'  Pakete $'
a_wdh	db	'  Wdh. $'
a_ok	db	'  fertig',cr,lf,'$'
a_abbr	db	cr,lf,'Abgebrochen.',cr,lf,'$'
a_gegen	db	cr,lf,'Gegenseite meldet: $'
a_zeit	db	cr,lf,'Keine Antwort von der Gegenseite.',cr,lf,'$'
a_nicht	db	'?Datei nicht gefunden.',cr,lf,'$'
a_dfehl	db	cr,lf,'?Dateifehler (Diskette voll oder schreibgeschuetzt).',cr,lf,'$'
a_warte	db	'Warte auf die Gegenseite (Abbruch mit ESC) ...',cr,lf,'$'
a_rich	dw	0		; a_sende oder a_empf

zeigez	proc	near
	push	ax
	push	dx
	mov	al,cr
	call	zeichen
	mov	dx,a_rich
	call	textd
	mov	si,offset fname
	call	text0
	mov	dx,offset a_bytes
	call	textd
	mov	ax,kbytes
	mov	dx,kbytes+2
	call	zahl32
	mov	dx,offset a_pak
	call	textd
	mov	ax,kpakete
	call	zahl
	mov	dx,offset a_wdh
	call	textd
	mov	ax,kwdh
	call	zahl
	pop	dx
	pop	ax
	ret
zeigez	endp

; Fehlerpaket der Gegenseite zeigen
zeigefehler proc	near
	mov	erfolg,0
	push	bx
	cmp	vollbild,0
	jne	zf1
	mov	dx,offset a_gegen
	call	textd
	mov	bx,offset zeig_z
	call	dekod
	call	crlf
	pop	bx
	ret
zf1:	push	ax			; mit Maske: ins Fehlerfeld
	push	si
	mov	ah,Z_FEHL
	call	feld
	cmp	rlen,56			; nicht ueber den Rand
	jbe	zf2
	mov	rlen,56
zf2:	mov	bx,offset zeig_z
	call	dekod
	mov	si,offset b_fehl
	call	zustand
	pop	si
	pop	ax
	pop	bx
	ret
zeigefehler endp
zeig_z	proc	near
	call	zeichen
	clc
	ret
zeig_z	endp

; Fehlerpaket senden und Abbruch melden
f_abbr	db	'Abbruch durch den Benutzer',0
abbrechen proc	near
	push	si
	mov	si,offset f_abbr
	mov	al,'E'
	call	spackt
	mov	dx,offset a_abbr
	call	meldung
	pop	si
	ret
abbrechen endp

; --- Gemeinsames ------------------------------------------------------------
; Vor jedem Vorgang: Folgenummer 0, Pruefart 1, nichts ausgehandelt
kstart	proc	near
	mov	kfx,0
	mov	kstop,0
	mov	sseq,0
	mov	chkt,1
	mov	qbin,0
	mov	rpref,0
	mov	kabbr,0
	mov	kbytes,0
	mov	kbytes+2,0
	mov	kpakete,0
	mov	kwdh,0
	mov	s_time,10
	mov	s_eol,cr
	mov	s_npad,0
	mov	s_qctl,'#'
	ret
kstart	endp

; Letztes Paket noch einmal senden
nochmal	proc	near
	push	ax
	push	cx
	push	si
	inc	kwdh
	mov	si,offset spuf
	mov	cx,spuflen
nm1:	lodsb
	call	sput
	loop	nm1
	pop	si
	pop	cx
	pop	ax
	ret
nochmal	endp
spuflen	dw	0

; Paket Typ AL senden und auf die Bestaetigung warten. Carry, wenn es
; nicht klappt (Meldung ist gegeben). Bestaetigungsdaten in rdat/rlen.
sendewarte proc	near
	call	spack
	mov	versuch,0
swa1:	call	rpack
	mov	al,rtyp
	cmp	al,'Y'
	jne	swa2
	mov	al,rseq
	cmp	al,sseq
	je	swa_ok
	jmp	short swa5
swa2:	cmp	al,'N'			; NAK fuer das naechste = ACK
	jne	swa3
	mov	al,sseq
	inc	al
	and	al,63
	cmp	al,rseq
	jne	swa5
	mov	rlen,0
	jmp	short swa_ok
swa3:	cmp	al,'E'
	jne	swa4
	call	zeigefehler
	stc
	ret
swa4:	cmp	al,'a'
	jne	swa5
	call	abbrechen
	stc
	ret
swa5:	inc	versuch
	cmp	versuch,VERSUCHE
	jae	swa_f
	call	nochmal
	jmp	short swa1
swa_f:	mov	dx,offset a_zeit
	call	meldung
	stc
	ret
swa_ok:	inc	sseq
	and	sseq,63
	clc
	ret
sendewarte endp

; --- SEND -------------------------------------------------------------------
; Muster in fmuster. Carry bei Fehler.
ksend	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	mov	a_rich,offset a_sende
	mov	dx,offset dta		; Suchbereich
	mov	ah,1Ah
	int	21h
	mov	dx,offset fmuster
	mov	cx,0			; normale Dateien
	mov	ah,4Eh
	int	21h
	jnc	ks1
	mov	dx,offset a_nicht
	call	meldung
	mov	al,'E'			; Gegenseite (Server) informieren
	mov	si,offset f_nicht
	call	spackt
	stc
	jmp	ks9
ks1:	call	mparam			; S-Paket, Pruefart 1
	mov	al,'S'
	call	sendewarte
	jc	ks9
	call	gparam
	mov	al,chkn
	sub	al,'0'
	mov	chkt,al
ks2:	call	ksdatei			; eine Datei
	jc	ks8
	mov	dx,offset dta		; die naechste
	mov	ah,1Ah
	int	21h
	mov	ah,4Fh
	int	21h
	jnc	ks2
	mov	al,'B'			; Ende der Sendung
	mov	sdlen,0
	call	sendewarte
	jmp	short ks9
ks8:	stc
ks9:	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
ksend	endp
f_nicht	db	'Datei nicht gefunden',0

; Die Datei aus dta senden. Carry: Sendung abbrechen.
ksdatei	proc	near
	mov	si,offset dta+1Eh	; Name ohne Pfad
	mov	di,offset fname
ksd0:	lodsb
	stosb
	or	al,al
	jnz	ksd0
	mov	si,offset fmuster	; voller Name = Pfadteil + Name
	mov	di,offset fpfad
	mov	cx,fpfadl
	rep	movsb
	mov	si,offset fname
ksd1:	lodsb
	stosb
	or	al,al
	jnz	ksd1
	mov	kbytes,0
	mov	kbytes+2,0
	mov	ax,3D00h		; lesen
	mov	dx,offset fpfad
	int	21h
	jnc	ksd2
	mov	dx,offset a_nicht
	call	meldung
	clc				; diese ueberspringen
	ret
ksd2:	mov	fhand,ax
	mov	fpos,0
	mov	fend,0
	mov	kstop,0
	mov	ax,word ptr dta+1Ah	; Groesse fuer die Prozente
	mov	kgross,ax
	mov	ax,word ptr dta+1Ch
	mov	kgross+2,ax
	mov	si,offset b_laeuft
	call	zustand
	call	zeige
	mov	di,offset sdat		; F-Paket mit dem Namen
	mov	si,offset fname
ksd3:	lodsb
	or	al,al
	jz	ksd4
	mov	cl,1
	call	kodier
	jmp	short ksd3
ksd4:	sub	di,offset sdat
	mov	sdlen,di
	mov	al,'F'
	call	sendewarte
	jc	ksd_f
	cmp	attr,0			; A-Paket mit der Groesse
	je	ksd6
	mov	di,offset sdat
	mov	byte ptr [di],'1'
	add	di,2
	mov	ax,word ptr dta+1Ah
	mov	dx,word ptr dta+1Ch
	call	zahlbuf			; Ziffern an DI
	mov	ax,di
	sub	ax,offset sdat+2
	add	al,' '
	mov	sdat+1,al
	sub	di,offset sdat
	mov	sdlen,di
	mov	al,'A'
	call	sendewarte
	jc	ksd_f
	cmp	rlen,0			; Gegenseite will die Datei nicht
	je	ksd6
	mov	si,rdat
	cmp	byte ptr [si],'N'
	jne	ksd6
	call	fzu
	clc
	ret
ksd6:	call	ksdaten			; D-Pakete
	jc	ksd_f
	mov	sdlen,0			; Z-Paket
	cmp	kstop,0
	je	ksd7
	mov	sdat,'D'		; verworfen
	mov	sdlen,1
ksd7:	mov	al,'Z'
	call	sendewarte
	jc	ksd_f
	call	fzu
	mov	dx,offset a_ok
	call	meldung
	cmp	kstop,2			; keine weiteren Dateien
	jne	ksd8
	mov	sdlen,0
	mov	al,'B'
	call	sendewarte
	stc
	ret
ksd8:	clc
	ret
ksd_f:	call	fzu
	stc
	ret
ksdatei	endp

; D-Pakete bis zum Dateiende. Carry bei Fehler.
ksdaten	proc	near
	mov	bx,s_maxl		; Nutzlast: s_maxl - Kopf - Pruefung
	sub	bx,2
	mov	al,chkt
	xor	ah,ah
	sub	bx,ax
	sub	bx,5			; Platz fuer die laengste Kodierung
ksl1:	mov	di,offset sdat
ksl2:	mov	ax,di
	sub	ax,offset sdat
	cmp	ax,bx
	jae	ksl4
	call	fget
	jc	ksl3
	mov	cl,1			; gleiche Bytes zaehlen
	cmp	rpref,0
	je	ksl2b
ksl2a:	cmp	cl,94
	jae	ksl2b
	mov	dl,al
	call	fpeek
	xchg	al,dl
	jc	ksl2b
	cmp	al,dl
	jne	ksl2b
	call	fget
	inc	cl
	jmp	short ksl2a
ksl2b:	call	kodier
	push	cx
	xor	ch,ch
	call	zaehl
	pop	cx
	jmp	short ksl2
ksl3:	cmp	di,offset sdat		; Ende: Rest senden
	je	ksl9
ksl4:	sub	di,offset sdat
	mov	sdlen,di
	mov	al,'D'
	call	sendewarte
	jc	ksl_f
	call	zeige
	cmp	kfx,0			; Taste ^X oder ^Z
	je	ksl4a
	mov	al,kfx
	mov	kfx,0
	cmp	al,1
	je	ksl5
	mov	kstop,2
	clc
	ret
ksl4a:	cmp	rlen,0			; X: Datei abbrechen, Z: alles
	je	ksl1
	mov	si,rdat
	mov	al,[si]
	cmp	al,'X'
	je	ksl5
	cmp	al,'Z'
	jne	ksl1
	mov	kstop,2
	clc
	ret
ksl5:	mov	kstop,1
ksl9:	clc
	ret
ksl_f:	stc
	ret
ksdaten	endp

; DX:AX dezimal an DI schreiben
zahlbuf	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	xor	cx,cx
zb1:	push	ax			; DX:AX / 10
	mov	ax,dx
	xor	dx,dx
	mov	bx,10
	div	bx
	mov	bx,ax			; hoher Teil
	pop	ax
	push	bx
	mov	bx,10
	div	bx			; DX = Rest
	pop	bx
	push	dx
	inc	cx
	mov	dx,bx
	or	bx,ax
	jnz	zb1
zb2:	pop	ax
	add	al,'0'
	stosb
	loop	zb2
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
zahlbuf	endp

; --- RECEIVE ----------------------------------------------------------------
; habS = 1, wenn das S-Paket schon in rpuf liegt (GET, SERVER)
habS	db	0
kfx	db	0		; Taste: 1 = ^X (Datei), 2 = ^Z (alle)
kstop	db	0		; Senden: 1 = diese Datei, 2 = alle abbrechen
kbild	db	0		; 1 = Text der Gegenseite (X) wird gezeigt
rerw	db	0		; erwartete Folgenummer

kempf	proc	near
	mov	a_rich,offset a_empf
	mov	fname,0
	mov	kbild,0
	cmp	habS,0
	jne	ke2
	mov	versuch,0
ke1:	call	rpack			; auf S warten
	cmp	rtyp,'S'
	je	ke2
	cmp	rtyp,'a'
	jne	ke1a
	call	abbrechen
	stc
	ret
ke1a:	cmp	rtyp,'E'
	jne	ke1b
	call	zeigefehler
	stc
	ret
ke1b:	inc	versuch
	cmp	versuch,VERSUCHE*3	; geduldig: der Benutzer tippt noch
	jae	ke_z
	mov	sseq,0
	mov	al,'N'
	call	spackl
	jmp	short ke1
ke2:	mov	habS,0
	call	gparam			; Parameter tauschen
	call	mparam
	mov	al,rseq
	mov	sseq,al
	mov	chkt,1
	mov	al,'Y'
	call	spack
	mov	al,chkn
	sub	al,'0'
	mov	chkt,al
	mov	al,rseq
	inc	al
	and	al,63
	mov	rerw,al
	mov	versuch,0
ke3:	call	rpack
	mov	al,rtyp
	cmp	al,'T'
	je	ke_n
	cmp	al,'Q'
	je	ke_n
	cmp	al,'a'
	jne	ke3a
	call	ekaputt
	call	abbrechen
	stc
	ret
ke3a:	cmp	al,'E'
	jne	ke3b
	call	ekaputt
	call	zeigefehler
	stc
	ret
ke3b:	mov	ah,rseq			; Wiederholung des vorigen: noch einmal
	cmp	ah,rerw			;  bestaetigen
	je	ke4
	mov	ah,rerw
	dec	ah
	and	ah,63
	cmp	ah,rseq
	jne	ke_n
	call	nochmal
	jmp	short ke3
ke4:	mov	versuch,0
	cmp	al,'F'
	je	ke_f
	cmp	al,'X'			; Text fuer den Bildschirm (REMOTE)
	je	ke_x
	cmp	al,'A'
	je	ke_a
	cmp	al,'D'
	je	ke_d
	cmp	al,'Z'
	je	ke_zz
	cmp	al,'B'
	je	ke_b
	jmp	ke_ack			; Unbekanntes einfach bestaetigen
ke_n:	inc	versuch
	cmp	versuch,VERSUCHE
	jae	ke_z
	mov	al,rerw			; NAK fuer das erwartete
	mov	sseq,al
	mov	al,'N'
	call	spackl
	inc	kwdh
	jmp	ke3
ke_z:	call	ekaputt
	mov	dx,offset a_zeit
	call	meldung
	stc
	ret
ke_a:	call	aparse			; Attribute: Groesse merken
	jmp	ke_ack
ke_f:	mov	kgross,0
	mov	kgross+2,0
	call	eoeffne			; Datei anlegen
	jc	ke_ff
	push	si
	mov	si,offset b_laeuft
	call	zustand
	pop	si
	mov	si,offset fname		; Bestaetigung mit dem Namen
	mov	al,rseq
	mov	sseq,al
	mov	al,'Y'
	call	spackt
	call	zeige
	jmp	ke_weiter
ke_ff:	mov	al,rseq
	mov	sseq,al
	mov	si,offset a_dfe
	mov	al,'E'
	call	spackt
	mov	dx,offset a_dfehl
	call	meldung
	stc
	ret
ke_d:	cmp	kbild,0
	je	ke_d1
	mov	bx,offset zeig_z	; Text: auf den Bildschirm
	call	dekod
	jmp	ke_ack
ke_d1:	mov	bx,offset fputz
	call	dekod
	jc	ke_ff
	call	zeige
	jmp	ke_ack
ke_x:	mov	kbild,1
	call	crlf
	jmp	ke_ack
ke_zz:	cmp	kbild,0
	jne	ke_ack
	call	fflush
	call	fzu
	cmp	rlen,0			; D: verwerfen
	je	ke_zz1
	mov	si,rdat
	cmp	byte ptr [si],'D'
	jne	ke_zz1
	mov	dx,offset fpfad
	mov	ah,41h
	int	21h
	mov	si,offset b_abbr
	call	zustand
	jmp	short ke_zz2
ke_zz1:	mov	dx,offset a_ok
	call	meldung
ke_zz2:	cmp	kfx,1			; ^X gilt nur fuer diese Datei
	jne	ke_ack
	mov	kfx,0
	jmp	ke_ack
ke_b:	mov	al,rseq
	mov	sseq,al
	mov	al,'Y'
	call	spackl
	clc
	ret
ke_ack:	mov	al,rseq
	mov	sseq,al
	cmp	rtyp,'D'		; ^X/^Z: in der Bestaetigung melden
	jne	ke_ack1
	cmp	kfx,0
	je	ke_ack1
	mov	al,'X'
	cmp	kfx,1
	je	ke_ack0
	mov	al,'Z'
ke_ack0: mov	sdat,al
	mov	sdlen,1
	mov	al,'Y'
	call	spack
	jmp	ke_weiter
ke_ack1: mov	al,'Y'
	call	spackl
ke_weiter:
	mov	al,rerw
	inc	al
	and	al,63
	mov	rerw,al
	jmp	ke3
kempf	endp
a_dfe	db	'Dateifehler',0

; Halbe Datei bei Abbruch loeschen
ekaputt	proc	near
	cmp	fhand,0
	je	ekp9
	call	fzu
	mov	dx,offset fpfad
	mov	ah,41h
	int	21h
ekp9:	ret
ekaputt	endp

; Namen aus dem F-Paket nach DOS-Art (8.3) bilden und Datei anlegen
roh	db	64 dup (0)
rohl	dw	0
eoeffne	proc	near
	mov	rohl,0
	mov	bx,offset roh_z
	call	dekod
	mov	bx,rohl
	mov	roh[bx],0
	mov	si,offset roh		; letzter Pfadteil
	mov	di,si
eo1:	lodsb
	or	al,al
	jz	eo2
	cmp	al,'/'
	je	eo1a
	cmp	al,'\'
	je	eo1a
	cmp	al,':'
	jne	eo1
eo1a:	mov	di,si
	jmp	short eo1
eo2:	mov	si,di			; letzter Punkt
	xor	bx,bx
eo3:	lodsb
	or	al,al
	jz	eo4
	cmp	al,'.'
	jne	eo3
	mov	bx,si
	jmp	short eo3
eo4:	mov	si,di			; Grundname bis 8 gueltige Zeichen
	mov	di,offset fname
	mov	cx,8
eo5:	lodsb
	or	al,al
	jz	eo6
	cmp	si,bx			; der letzte Punkt
	je	eo6
	call	gueltig
	jc	eo5
	jcxz	eo5
	stosb
	dec	cx
	jmp	short eo5
eo6:	cmp	di,offset fname		; leer: DATEI
	jne	eo6a
	mov	si,offset s_datei
	mov	cx,5
	rep	movsb
eo6a:	or	bx,bx			; Erweiterung bis 3
	jz	eo8
	mov	al,'.'
	stosb
	mov	si,bx
	mov	cx,3
eo7:	lodsb
	or	al,al
	jz	eo8
	call	gueltig
	jc	eo7
	stosb
	loop	eo7
eo8:	mov	byte ptr [di],0
	mov	si,offset fname		; Pfad: aktuelles Verzeichnis
	mov	di,offset fpfad
eo9:	lodsb
	stosb
	or	al,al
	jnz	eo9
	mov	cl,'1'			; schon da? dann umbenennen
eo10:	mov	ax,3D00h
	mov	dx,offset fpfad
	int	21h
	jc	eo12
	mov	bx,ax
	mov	ah,3Eh
	int	21h
	call	umbenenn		; letzte Stelle des Grundnamens = CL
	inc	cl
	cmp	cl,'9'
	jbe	eo10
eo12:	mov	ah,3Ch			; anlegen
	xor	cx,cx
	mov	dx,offset fpfad
	int	21h
	jc	eo_f
	mov	fhand,ax
	mov	fpos,0
	mov	kbytes,0
	mov	kbytes+2,0
	clc
	ret
eo_f:	stc
	ret
eoeffne	endp
s_datei	db	'DATEI'

roh_z	proc	near
	push	bx
	mov	bx,rohl
	cmp	bx,63
	jae	rz_9
	mov	roh[bx],al
	inc	rohl
rz_9:	pop	bx
	clc
	ret
roh_z	endp

; AL gross machen; Carry, wenn es in DOS-Namen nicht taugt
gueltig	proc	near
	cmp	al,'a'
	jb	gu1
	cmp	al,'z'
	ja	gu1
	sub	al,20h
gu1:	cmp	al,'A'
	jb	gu2
	cmp	al,'Z'
	jbe	gu8
gu2:	cmp	al,'0'
	jb	gu3
	cmp	al,'9'
	jbe	gu8
gu3:	cmp	al,'_'
	je	gu8
	cmp	al,'-'
	je	gu8
	cmp	al,'$'
	je	gu8
	stc
	ret
gu8:	clc
	ret
gueltig	endp

; Letzte Stelle des Grundnamens in fname und fpfad durch CL ersetzen
umbenenn proc	near
	push	si
	mov	si,offset fname
umb1:	mov	al,[si]
	or	al,al
	jz	umb2
	cmp	al,'.'
	je	umb2
	inc	si
	jmp	short umb1
umb2:	cmp	si,offset fname
	je	umb9
	dec	si
	mov	[si],cl
	push	si
	sub	si,offset fname
	mov	byte ptr fpfad[si],cl
	pop	si
umb9:	pop	si
	ret
umbenenn endp

; --- GET, FINISH, BYE -------------------------------------------------------
; Paket Typ AL mit Text fmuster an einen Server, dann (GET) empfangen
kserver_paket proc	near
	push	ax
	mov	di,offset sdat
	mov	si,offset fmuster
ksp1:	lodsb
	or	al,al
	jz	ksp2
	mov	cl,1
	call	kodier
	jmp	short ksp1
ksp2:	sub	di,offset sdat
	mov	sdlen,di
	pop	ax
	ret
kserver_paket endp

kget	proc	near
	call	kstart
	call	kserver_paket
	mov	versuch,0
kg1:	mov	al,'R'
	call	spack
kg2:	call	rpack
	cmp	rtyp,'S'
	je	kg5
	cmp	rtyp,'E'
	jne	kg3
	call	zeigefehler
	stc
	ret
kg3:	cmp	rtyp,'a'
	jne	kg4
	call	abbrechen
	stc
	ret
kg4:	inc	versuch
	cmp	versuch,VERSUCHE
	jb	kg1
	mov	dx,offset a_zeit
	call	meldung
	stc
	ret
kg5:	mov	habS,1
	jmp	kempf
kget	endp

; Allgemeiner Serverbefehl (G) mit einem Buchstaben AL
kgeneric proc	near
	call	kstart
	mov	sdat,al
	mov	sdlen,1
	mov	al,'G'
	call	sendewarte
	ret
kgeneric endp

; --- SERVER -----------------------------------------------------------------
sv_text	db	'Server: warte auf die Gegenseite, ESC beendet$'
sv_nein	db	'Nicht unterstuetzt',0

kserver	proc	near
	mov	dx,offset sv_text
	call	hinweis
ksv1:	call	kstart
	mov	s_time,30		; im Leerlauf alle 30 s ein NAK
ksv2:	call	rpack
	mov	al,rtyp
	cmp	al,'a'
	je	ksv9
	cmp	al,'T'
	jne	ksv3
	mov	al,'N'
	call	spackl
	jmp	short ksv2
ksv3:	cmp	al,'I'			; Parameter
	jne	ksv4
	call	gparam
	call	mparam
	mov	al,rseq
	mov	sseq,al
	mov	al,'Y'
	call	spack
	jmp	short ksv2
ksv4:	cmp	al,'S'			; Gegenseite sendet
	jne	ksv5
	mov	habS,1
	call	kempf
	jmp	short ksv1
ksv5:	cmp	al,'R'			; Gegenseite will eine Datei
	jne	ksv6
	mov	rohl,0
	mov	bx,offset roh_z
	call	dekod
	mov	bx,rohl
	mov	roh[bx],0
	mov	si,offset roh
	call	setmuster
	mov	sseq,0
	call	ksend
	jmp	short ksv1
ksv6:	cmp	al,'G'			; F und L beenden den Server
	jne	ksv8
	mov	si,rdat
	mov	al,[si]
	and	al,0DFh
	cmp	al,'F'
	je	ksv7
	cmp	al,'L'
	je	ksv7
	call	sgeneric		; REMOTE-Befehle (remote.inc)
	jnc	ksv1a
	jmp	ksv_e
ksv1a:	jmp	ksv1
ksv7:	mov	al,rseq
	mov	sseq,al
	mov	al,'Y'
	call	spackl
	ret
ksv8:	cmp	al,'Q'
	je	ksv2
	cmp	al,'E'
	je	ksv1
	cmp	al,'Y'
	je	ksv2
	cmp	al,'N'
	je	ksv2
ksv_e:	mov	al,rseq
	mov	sseq,al
	mov	si,offset sv_nein
	mov	al,'E'
	call	spackt
	jmp	ksv1
ksv9:	ret
kserver	endp

; Muster DS:SI (0-Ende) nach fmuster, Pfadlaenge bestimmen
setmuster proc	near
	push	ax
	push	di
	mov	di,offset fmuster
	mov	fpfadl,0
	mov	cx,79
sm1:	lodsb
	cmp	al,'a'			; gross
	jb	sm0
	cmp	al,'z'
	ja	sm0
	sub	al,20h
sm0:	cmp	al,'/'			; Unix-Pfade erlauben
	jne	sm1a
	mov	al,'\'
sm1a:	stosb
	or	al,al
	jz	sm3
	cmp	al,'\'
	je	sm2
	cmp	al,':'
	jne	sm2a
sm2:	mov	ax,di
	sub	ax,offset fmuster
	mov	fpfadl,ax
sm2a:	loop	sm1
	mov	byte ptr [di],0
sm3:	pop	di
	pop	ax
	ret
setmuster endp

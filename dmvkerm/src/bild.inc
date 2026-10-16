; bild.inc - Bildschirm waehrend einer Uebertragung (nach MS-Kermit 2.32)
;
; bildan zeichnet die Maske, zeige traegt die Werte ein, meldung schreibt
; Fehler und Hinweise in ihre Felder, bildaus raeumt die Fusszeile weg und
; setzt den Cursor unter die Maske, damit DOS dort weiterschreibt.
; Ohne Maske (vollbild = 0) bleibt es bei einer Zeile (zeigez).

SP_W	equ	22		; Spalte der Werte
Z_TITEL	equ	1
Z_DATEI	equ	3
Z_BYTES	equ	4
Z_PROZ	equ	5
Z_STAT	equ	6
Z_PAK	equ	8
Z_PLEN	equ	9
Z_WDH	equ	10
Z_FEHL	equ	11
Z_MELD	equ	12
Z_ENDE	equ	14

vollbild db	0
kgross	dw	0, 0		; Dateigroesse fuer die Prozente (0 = unbekannt)
kplen	dw	0		; Laenge des letzten Datenpakets

b_titel	dw	0
b_tkerm	db	'DMVKERM 0.1  -  Dateiuebertragung mit Kermit',0
b_txmod	db	'DMVKERM 0.1  -  Dateiuebertragung mit XMODEM',0
b_masks	db	Z_DATEI,'               Datei:',0
	db	Z_BYTES,'   Bytes uebertragen:',0
	db	Z_PROZ,' Prozent uebertragen:',0
	db	Z_PAK,'       Anzahl Pakete:',0
	db	Z_PLEN,'         Paketlaenge:',0
	db	Z_WDH,'      Wiederholungen:',0
	db	Z_FEHL,'      Letzter Fehler:',0
	db	Z_MELD,'      Letzte Meldung:',0
	db	0
b_fuss	db	' ^X Datei abbrechen  ^Z alle abbrechen  ^E/^C/ESC Ende  Return wiederholen',0
bm_send	db	'              Senden:',0
bm_empf	db	'           Empfangen:',0
b_laeuft db	'laeuft',0
b_fertig db	'fertig',0
b_abbr	db	'abgebrochen',0
b_fehl	db	'fehlgeschlagen',0
b_als	db	'  als  ',0

; Maske zeichnen; SI = Titel (0-Ende)
bildan	proc	near
	push	ax
	push	si
	mov	b_titel,si
	call	zeitan
	mov	vollbild,1
	mov	kgross,0
	mov	kgross+2,0
	mov	kplen,0
	mov	katt,0FFh
	mov	al,0
	call	kattr
	mov	al,1Ah			; Bildschirm loeschen
	call	kroh
	mov	kpos,0FFFFh
	mov	ax,Z_TITEL*256+16
	call	kgehe
	mov	si,b_titel
	call	k0
	mov	si,offset b_masks
bda1:	lodsb
	or	al,al
	jz	bda2
	mov	ah,al
	xor	al,al
	call	kgehe
	call	k0
	jmp	short bda1
bda2:	mov	ax,VZ*256		; Fusszeile invers
	call	kgehe
	mov	al,1
	call	kattr
	mov	si,offset b_fuss
	call	k0
	mov	al,0
	call	kattr
	pop	si
	pop	ax
	ret
bildan	endp

; Maske verlassen: Fusszeile weg, Cursor unter die Maske
bildaus	proc	near
	call	zeitaus
	cmp	vollbild,0
	je	bdx9
	push	ax
	mov	ax,VZ*256
	call	kgehe
	mov	al,esc_
	call	kroh
	mov	al,'T'
	call	kroh
	mov	kpos,0FFFFh
	mov	ax,Z_ENDE*256
	call	kgehe
	mov	kpos,0FFFFh
	mov	vollbild,0
	pop	ax
bdx9:	ret
bildaus	endp

; Zeichenfolge DS:SI (0-Ende) ueber kroh; SI dahinter
k0	proc	near
k0a:	lodsb
	or	al,al
	jz	k0b
	call	kroh
	jmp	short k0a
k0b:	ret
k0	endp

; Feld in Zeile AH ansteuern und leeren
feld	proc	near
	push	ax
	mov	kpos,0FFFFh
	mov	al,SP_W
	call	kgehe
	mov	al,esc_			; Rest der Zeile loeschen
	call	kroh
	mov	al,'T'
	call	kroh
	mov	kpos,0FFFFh		; DOS schreibt gleich weiter
	pop	ax
	ret
feld	endp

; Text DS:SI (0-Ende) in das Feld der Zeile AH
feldtext proc	near
	push	si
	call	feld
	call	text0
	pop	si
	ret
feldtext endp

; Zustand der Datei: SI = b_laeuft, b_fertig, ...
zustand	proc	near
	cmp	vollbild,0
	jne	zst0
	ret
zst0:	push	ax
	push	si
	mov	ah,Z_STAT		; Beschriftung nach Richtung
	xor	al,al
	mov	kpos,0FFFFh
	call	kgehe
	mov	si,offset bm_send
	cmp	a_rich,offset a_sende
	je	zst1
	mov	si,offset bm_empf
zst1:	call	k0
	pop	si
	mov	ah,Z_STAT
	call	feldtext
	pop	ax
	ret
zustand	endp

; Stand eintragen
zeige	proc	near
	cmp	vollbild,0
	jne	zg0
	jmp	zeigez
zg0:	push	ax
	push	dx
	push	si
	mov	ah,Z_DATEI		; Name (beim Empfang: Original als DOS-Name)
	call	feld
	cmp	a_rich,offset a_sende
	je	zg1
	cmp	rohl,0
	je	zg1
	mov	si,offset roh
	call	text0
	mov	si,offset b_als
	call	text0
zg1:	mov	si,offset fname
	call	text0
	mov	ah,Z_BYTES
	call	feld
	mov	ax,kbytes
	mov	dx,kbytes+2
	call	zahl32
	mov	ah,Z_PROZ		; Prozent, wenn die Groesse bekannt ist
	call	feld
	call	prozent
	jc	zg2
	call	zahl
	mov	al,'%'
	call	zeichen
zg2:	mov	ah,Z_PAK
	call	feld
	mov	ax,kpakete
	call	zahl
	mov	ah,Z_PLEN
	call	feld
	mov	ax,kplen
	call	zahl
	mov	ah,Z_WDH
	call	feld
	mov	ax,kwdh
	call	zahl
	pop	si
	pop	dx
	pop	ax
	ret
zeige	endp

; kbytes * 100 / kgross -> AX (hoechstens 100); Carry, wenn unbekannt
prozent	proc	near
	push	bx
	push	cx
	push	dx
	mov	ax,kgross		; Teiler = kgross / 100
	mov	dx,kgross+2
	mov	cx,ax
	or	cx,dx
	jz	pz_u
	cmp	dx,100			; zu gross fuer die Teilung
	jae	pz_u
	mov	bx,100
	div	bx
	or	ax,ax
	jnz	pz1
	inc	ax
pz1:	mov	bx,ax
	mov	ax,kbytes
	mov	dx,kbytes+2
	cmp	dx,bx			; Ergebnis passt nicht in 16 Bit
	jae	pz100
	div	bx
	cmp	ax,100
	jbe	pz9
pz100:	mov	ax,100
pz9:	clc
	jmp	short pz_r
pz_u:	stc
pz_r:	pop	dx
	pop	cx
	pop	bx
	ret
prozent	endp

; Meldung DX ($-Text): ohne Maske ausgeben, mit Maske in ihr Feld
meldung	proc	near
	cmp	dx,offset a_ok		; jeder Fehler gilt fuer IF FAILURE
	je	mld0
	mov	erfolg,0
mld0:	cmp	vollbild,0
	jne	mld1
	jmp	textd
mld1:	push	ax
	push	si
	cmp	dx,offset a_ok
	jne	mld2
	mov	si,offset b_fertig
	call	zustand
	jmp	short mld9
mld2:	mov	ah,Z_FEHL		; Text ohne CR/LF ins Fehlerfeld
	call	feld
	mov	si,dx
mld3:	lodsb
	cmp	al,'$'
	je	mld4
	cmp	al,' '
	jb	mld3
	call	zeichen
	jmp	short mld3
mld4:	mov	si,offset b_fehl
	cmp	dx,offset a_abbr
	jne	mld5
	mov	si,offset b_abbr
mld5:	call	zustand
mld9:	pop	si
	pop	ax
	ret
meldung	endp

; Hinweis DX ($-Text) ins Feld "Letzte Meldung" (ohne Maske: ausgeben)
hinweis	proc	near
	cmp	vollbild,0
	jne	hnw1
	jmp	textd
hnw1:	push	ax
	push	si
	mov	ah,Z_MELD
	call	feld
	mov	si,dx
hnw2:	lodsb
	cmp	al,'$'
	je	hnw9
	cmp	al,' '
	jb	hnw2
	call	zeichen
	jmp	short hnw2
hnw9:	pop	si
	pop	ax
	ret
hinweis	endp

; Attributpaket (rdat/rlen, schon dekodiert gleich kodiert: nur Ziffern)
; auswerten: Tag '1' = Groesse in Byte -> kgross
aparse	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	mov	si,rdat
	mov	cx,rlen
ap1:	cmp	cx,2
	jb	ap9
	lodsb				; Tag
	mov	bl,al
	lodsb				; Laenge
	sub	al,' '
	xor	ah,ah
	sub	cx,2
	cmp	ax,cx
	jbe	ap2
	mov	ax,cx
ap2:	sub	cx,ax
	cmp	bl,'1'
	je	ap3
	add	si,ax			; anderes Attribut ueberspringen
	jmp	short ap1
ap3:	mov	bx,ax			; Ziffern -> kgross (32 Bit)
	mov	kgross,0
	mov	kgross+2,0
ap4:	or	bx,bx
	jz	ap1
	lodsb
	dec	bx
	sub	al,'0'
	cmp	al,9
	ja	ap4
	push	ax			; kgross = kgross * 10 + Ziffer
	mov	ax,kgross+2
	mov	dx,10
	mul	dx
	mov	kgross+2,ax
	mov	ax,kgross
	mov	dx,10
	mul	dx
	add	kgross+2,dx
	mov	kgross,ax
	pop	ax
	xor	ah,ah
	add	kgross,ax
	adc	kgross+2,0
	jmp	short ap4
ap9:	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
aparse	endp

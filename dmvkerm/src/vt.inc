; vt.inc - VT102-Teilmenge auf der DOS-Konsole der DMV
;
; Die Konsolen der drei DOS-Fassungen verstehen gemeinsam nur eine kleine
; ADM-3A/NCR-Teilmenge: ESC = Zeile Spalte (je +20h), ESC T (Rest der
; Zeile loeschen), 1Ah (Bildschirm loeschen), ESC G 0/2/4 (normal,
; blinkend, invers), ESC ) / ESC ( (halbhell an/aus) und LF in der
; untersten Rollzeile (rollt). Alles andere wird hier nachgebildet: ein
; Puffer mit 24 Zeilen haelt, was die Gegenstelle zeigen will; Rollen in
; Teilbereichen, Einfuegen und Loeschen von Zeilen zeichnet die
; betroffenen Zeilen aus dem Puffer neu. Zeile 25 ist die Statuszeile.
;
; Unter NCR DOS 2.11 (CRT_ROWS = 24 in 0:041Dh) rollen nur die Zeilen
; 0..23, Zeile 24 bleibt stehen. Unter DOS 3.3 und DR-DOS rollen alle 25;
; dann wird nach dem Rollen die Statuszeile neu geschrieben.
;
; Zeichen ab 80h werden als CP437 gezeigt (Mailboxen); Rahmenzeichen
; erscheinen als + - |. Zeichen werden mit gesetztem Bit 7 ausgegeben
; (siehe glyph in tabellen.inc).

VZ	equ	24		; Zeilen der Emulation
VS	equ	80
VGR	equ	VZ*VS

	.data?
vchr	db	VGR dup (?)
	.code
	.data?
vatt	db	VGR dup (?)	; Bit 0 invers, 1 blinkend, 2 halbhell
	.code
vzl	db	0		; Cursor
vsp	db	0
vwrap	db	0		; Spalte 79 beschrieben, naechstes Zeichen bricht um
voben	db	0		; Rollbereich
vunten	db	VZ-1
vattr	db	0		; aktuelle Attribute
vorig	db	0		; DECOM
vauto	db	1		; DECAWM
vgraf	db	0		; G0 = DEC-Grafik
vg1graf	db	0		; G1 = DEC-Grafik
vshift	db	0		; SO aktiv (G1)
vsz	db	0		; gesicherter Cursor
vss	db	0
vsa	db	0
vstat	dw	vt_norm		; Zustand des Automaten
vpar	dw	8 dup (0)
vnpar	db	0
vpriv	db	0		; '?' nach CSI
vinit	db	0		; Puffer schon einmal angezeigt
rollz	db	VZ		; Zeile, in der LF rollt (24, unter 2.11: 23)
kpos	dw	0FFFFh		; Konsolencursor Zeile:Spalte, FFFF unbekannt
katt	db	0FFh		; Konsolenattribut, FF unbekannt

; --- Konsole ----------------------------------------------------------------
; Konsolencursor auf AH:AL
kgehe	proc	near
	cmp	ax,kpos
	je	kg9
	mov	kpos,ax
	push	ax
	mov	al,esc_
	call	kroh
	mov	al,'='
	call	kroh
	pop	ax
	push	ax
	mov	al,ah
	add	al,' '
	call	kroh
	pop	ax
	push	ax
	add	al,' '
	call	kroh
	pop	ax
kg9:	ret
kgehe	endp

; Konsolenattribut AL (Bits wie vatt)
kattr	proc	near
	cmp	al,katt
	je	ka9
	push	ax
	push	bx
	mov	bl,al
	mov	bh,katt
	mov	katt,al
	mov	al,esc_
	call	kroh
	mov	al,'G'
	call	kroh
	mov	al,'0'
	test	bl,1
	jz	ka1
	mov	al,'4'
	jmp	short ka2
ka1:	test	bl,2
	jz	ka2
	mov	al,'2'
ka2:	call	kroh
	mov	al,bl			; halbhell nur bei Aenderung
	xor	al,bh
	cmp	bh,0FFh
	jne	ka3
	mov	al,4
ka3:	test	al,4
	jz	ka8
	mov	al,esc_
	call	kroh
	mov	al,'('			; ESC ( aus, ESC ) an
	test	bl,4
	jz	ka4
	mov	al,')'
ka4:	call	kroh
ka8:	pop	bx
	pop	ax
ka9:	ret
kattr	endp

; Zelle BX (Index in vchr) an der Konsole zeigen, Konsolencursor steht dort
kzelle	proc	near
	push	ax
	push	bx
	mov	al,vatt[bx]
	call	kattr
	mov	al,vchr[bx]
	cmp	al,' '
	je	kz1
	push	bx
	mov	bx,offset glyph
	xlat
	pop	bx
kz1:	call	kroh
	inc	byte ptr kpos		; Spalte weiter
	cmp	byte ptr kpos,VS
	jb	kz9
	mov	kpos,0FFFFh		; nach Spalte 79: haengt von der Konsole ab
kz9:	pop	bx
	pop	ax
	ret
kzelle	endp

; Zeile AL ab Spalte AH neu zeichnen
kzeile	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	mov	dl,al			; DL Zeile, DH erste Spalte
	mov	dh,ah
	mov	al,VS			; BX = Index der Zeile
	mul	dl
	mov	bx,ax
	mov	cl,VS			; CL = Spalte hinter dem letzten Nicht-
kzl1:	cmp	cl,dh			;  leerzeichen (normal)
	jbe	kzl2
	push	bx
	mov	al,cl
	dec	al
	xor	ah,ah
	add	bx,ax
	cmp	vchr[bx],' '
	jne	kzl1a
	cmp	vatt[bx],0
	jne	kzl1a
	pop	bx
	dec	cl
	jmp	short kzl1
kzl1a:	pop	bx
kzl2:	mov	al,dh			; ab DH zeichnen bis CL
	mov	ah,dl
	call	kgehe
	mov	al,dh
kzl3:	cmp	al,cl
	jae	kzl4
	push	bx
	xor	ah,ah
	add	bx,ax
	call	kzelle
	pop	bx
	inc	al
	jmp	short kzl3
kzl4:	cmp	cl,VS			; Rest der Zeile loeschen
	jae	kzl9
	mov	al,cl
	mov	ah,dl
	call	kgehe
	mov	al,0
	call	kattr
	mov	al,esc_
	call	kroh
	mov	al,'T'
	call	kroh
kzl9:	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
kzeile	endp

; Zeilen AL bis AH neu zeichnen
kzeilen	proc	near
	push	ax
	push	cx
	mov	cl,al
	mov	ch,ah
kzs1:	cmp	cl,ch
	ja	kzs9
	mov	al,cl
	xor	ah,ah
	call	kzeile
	inc	cl
	jmp	short kzs1
kzs9:	pop	cx
	pop	ax
	ret
kzeilen	endp

; Ganzen Bildschirm neu: loeschen, alle Zeilen, Statuszeile
kalles	proc	near
	push	ax
	mov	al,0
	call	kattr
	mov	al,1Ah
	call	kroh
	mov	kpos,0FFFFh
	mov	ax,(VZ-1)*256		; Zeilen 0..23
	call	kzeilen
	call	kstatus
	pop	ax
	ret
kalles	endp

; Statuszeile (Zeile 24, invers; Spalte 79 bleibt frei)
st_text	db	' DMVKERM   Ctrl-CLR: Befehl (C = zurueck)     Port '
st_port	db	'1  '
st_baud	db	' 9600 '
st_par	db	'8N1  VT102 '
st_len	equ	$-st_text

kstatus	proc	near
	push	ax
	push	bx
	push	cx
	push	si
	call	stfuell
	mov	ax,VZ*256		; Zeile 24, Spalte 0
	call	kgehe
	mov	al,1
	call	kattr
	mov	si,offset st_text
	mov	cx,st_len
kst1:	lodsb
	call	kroh
	loop	kst1
	mov	cx,VS-1-st_len		; mit Leerzeichen bis Spalte 78
kst2:	mov	al,' '
	call	kroh
	loop	kst2
	mov	kpos,(VZ*256)+VS-1
	mov	al,0
	call	kattr
	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
kstatus	endp

; Werte in die Statuszeile eintragen
stfuell	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	di
	mov	al,portnr
	add	al,'0'
	mov	st_port,al
	mov	di,offset st_baud	; Baudrate rechtsbuendig in 5 Stellen
	mov	cx,5
stf1:	mov	byte ptr [di],' '
	inc	di
	loop	stf1
	mov	bl,pbaud
	xor	bh,bh
	shl	bx,1
	mov	ax,baudtab[bx]
	mov	bx,10
stf2:	dec	di
	xor	dx,dx
	div	bx
	add	dl,'0'
	mov	[di],dl
	or	ax,ax
	jnz	stf2
	mov	al,'8'
	mov	ah,'N'
	cmp	pparit,0
	je	stf3
	mov	al,'7'
	mov	ah,'E'
	cmp	pparit,1
	je	stf3
	mov	ah,'O'
	cmp	pparit,2
	je	stf3
	mov	al,'8'
	mov	ah,'M'
	cmp	pparit,3
	je	stf3
	mov	ah,'S'
stf3:	mov	st_par,al
	mov	st_par+1,ah
	pop	di
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
stfuell	endp


; --- Puffer -----------------------------------------------------------------
; Index der Zelle Zeile AL, Spalte AH -> BX
vidx	proc	near
	push	ax
	push	dx
	mov	dl,ah
	mov	ah,VS
	mul	ah
	xor	dh,dh
	add	ax,dx
	mov	bx,ax
	pop	dx
	pop	ax
	ret
vidx	endp

; CX Zellen ab Index BX loeschen (Leerzeichen, normal)
vleer	proc	near
	push	ax
	push	cx
	push	di
	jcxz	vle9
	push	cx
	mov	di,offset vchr
	add	di,bx
	mov	al,' '
	rep	stosb
	pop	cx
	mov	di,offset vatt
	add	di,bx
	xor	al,al
	rep	stosb
vle9:	pop	di
	pop	cx
	pop	ax
	ret
vleer	endp

; Zeilen AL..AH: eine nach oben schieben, unterste leer
vhoch	proc	near
	push	ax
	push	bx
	push	cx
	push	si
	push	di
	mov	cl,ah
	sub	cl,al			; Anzahl zu schiebender Zeilen
	push	ax
	mov	ah,0
	call	vidx			; BX = Anfang Zeile AL
	pop	ax
	push	ax
	mov	al,VS
	mul	cl
	mov	cx,ax			; Zellen
	pop	ax
	jcxz	vho2
	push	cx
	mov	di,offset vchr
	add	di,bx
	mov	si,di
	add	si,VS
	rep	movsb
	pop	cx
	mov	di,offset vatt
	add	di,bx
	mov	si,di
	add	si,VS
	rep	movsb
vho2:	mov	al,ah			; unterste Zeile leer
	mov	ah,0
	call	vidx
	mov	cx,VS
	call	vleer
	pop	di
	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
vhoch	endp

; Zeilen AL..AH: eine nach unten schieben, oberste leer
vrunter	proc	near
	push	ax
	push	bx
	push	cx
	push	si
	push	di
	mov	cl,ah
	sub	cl,al
	push	ax
	mov	al,VS
	mul	cl
	mov	cx,ax
	pop	ax
	push	ax
	mov	al,ah			; BX = letzte Zelle der Zeile AH
	mov	ah,VS-1
	call	vidx
	pop	ax
	jcxz	vru2
	std
	push	cx
	mov	di,offset vchr
	add	di,bx
	mov	si,di
	sub	si,VS
	rep	movsb
	pop	cx
	mov	di,offset vatt
	add	di,bx
	mov	si,di
	sub	si,VS
	rep	movsb
	cld
vru2:	mov	ah,0			; oberste Zeile leer
	call	vidx
	mov	cx,VS
	call	vleer
	pop	di
	pop	si
	pop	cx
	pop	bx
	pop	ax
	ret
vrunter	endp

; Rollbereich eine Zeile hoch (Puffer und Konsole)
vrollh	proc	near
	push	ax
	mov	al,voben
	mov	ah,vunten
	call	vhoch
	cmp	al,0			; ganzer Bildschirm: die Konsole rollt
	jne	vrh5
	cmp	ah,VZ-1
	jne	vrh5
	mov	al,0
	call	kattr
	mov	ah,rollz		; LF in der Rollzeile
	mov	al,0
	call	kgehe
	mov	al,lf
	call	kroh
	cmp	rollz,VZ-1		; DOS 2.11: Zeile 24 blieb stehen
	je	vrh9
	mov	kpos,0FFFFh		; sonst stand die Statuszeile jetzt in
	mov	ax,VZ-1			;  Zeile 23: leeren, Statuszeile neu
	call	kzeile
	call	kstatus
	jmp	short vrh9
vrh5:	call	kzeilen			; Teilbereich: neu zeichnen
vrh9:	pop	ax
	ret
vrollh	endp

; Rollbereich eine Zeile runter
vrollr	proc	near
	push	ax
	mov	al,voben
	mov	ah,vunten
	call	vrunter
	call	kzeilen
	pop	ax
	ret
vrollr	endp

; --- Automat ----------------------------------------------------------------
; Empfangenes Zeichen AL verarbeiten (alle Register bleiben)
vtzeich	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	call	word ptr vstat
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
vtzeich	endp

; Konsolencursor an den VT-Cursor
vcursor	proc	near
	push	ax
	mov	ah,vzl
	mov	al,vsp
	call	kgehe
	pop	ax
	ret
vcursor	endp

vt_norm	proc	near
	cmp	al,' '
	jb	vtsteuer
	cmp	al,7Fh
	je	vtn9
	jmp	vtdruck
vtn9:	ret
vt_norm	endp

; Steuerzeichen AL
vtsteuer proc	near
	cmp	al,esc_
	jne	vst1
	mov	vstat,offset vt_esc
	ret
vst1:	cmp	al,cr
	jne	vst2
	mov	vsp,0
	mov	vwrap,0
	ret
vst2:	cmp	al,lf
	je	vst3
	cmp	al,0Bh
	je	vst3
	cmp	al,0Ch
	jne	vst4
vst3:	jmp	vtindex
vst4:	cmp	al,bs
	jne	vst5
	mov	vwrap,0
	cmp	vsp,0
	je	vst9
	dec	vsp
	ret
vst5:	cmp	al,tab
	jne	vst6
	mov	al,vsp
	or	al,7
	inc	al
	cmp	al,VS-1
	jbe	vst5a
	mov	al,VS-1
vst5a:	mov	vsp,al
	ret
vst6:	cmp	al,bel
	jne	vst7
	call	kroh
	ret
vst7:	cmp	al,0Eh			; SO: G1
	jne	vst8
	mov	vshift,1
	ret
vst8:	cmp	al,0Fh			; SI: G0
	jne	vst8a
	mov	vshift,0
	ret
vst8a:	cmp	al,18h			; CAN, SUB: Folge abbrechen
	je	vst8b
	cmp	al,1Ah
	jne	vst9
vst8b:	mov	vstat,offset vt_norm
vst9:	ret
vtsteuer endp

; DEC-Grafik 5Fh..7Eh als einfache Zeichen
dgraf	db	' *#    o+  ++++++----_++++|<>n=$.'

; druckbares Zeichen AL
vtdruck	proc	near
	mov	ah,vgraf
	cmp	vshift,0
	je	vdr0
	mov	ah,vg1graf
vdr0:	or	ah,ah
	jz	vdr1
	cmp	al,5Fh
	jb	vdr1
	cmp	al,7Eh
	ja	vdr1
	sub	al,5Fh
	mov	bx,offset dgraf
	xlat
vdr1:	cmp	vwrap,0			; aufgeschobener Umbruch
	je	vdr2
	mov	vwrap,0
	cmp	vauto,0
	je	vdr2
	mov	vsp,0
	push	ax
	call	vtindex
	pop	ax
vdr2:	push	ax
	mov	al,vzl
	mov	ah,vsp
	call	vidx
	pop	ax
	mov	vchr[bx],al
	mov	al,vattr
	mov	vatt[bx],al
	mov	ah,vzl
	mov	al,vsp
	call	kgehe
	call	kzelle
	cmp	vsp,VS-1
	jae	vdr3
	inc	vsp
	ret
vdr3:	mov	vwrap,1
	ret
vtdruck	endp

; LF: im Rollbereich nach unten, unten rollen
vtindex	proc	near
	mov	al,vzl
	cmp	al,vunten
	jne	vti1
	call	vrollh
	ret
vti1:	cmp	al,VZ-1
	jae	vti9
	inc	vzl
vti9:	ret
vtindex	endp

vtrindex proc	near
	mov	al,vzl
	cmp	al,voben
	jne	vtr1
	call	vrollr
	ret
vtr1:	cmp	al,0
	je	vtr9
	dec	vzl
vtr9:	ret
vtrindex endp

; Zeichen nach ESC
vt_esc	proc	near
	mov	vstat,offset vt_norm
	cmp	al,' '			; Steuerzeichen mitten in der Folge
	jae	ves0
	jmp	vtsteuer
ves0:	cmp	al,'['
	jne	ves1
	mov	vstat,offset vt_csi
	xor	ax,ax
	mov	vnpar,al
	mov	vpriv,al
	mov	di,offset vpar
	mov	cx,8
	rep	stosw
	ret
ves1:	cmp	al,'('
	jne	ves2
	mov	vstat,offset vt_g0
	ret
ves2:	cmp	al,')'
	jne	ves3
	mov	vstat,offset vt_g1
	ret
ves3:	cmp	al,'#'
	jne	ves4
	mov	vstat,offset vt_eins
	ret
ves4:	cmp	al,'7'
	jne	ves5
	jmp	vtsichern
ves5:	cmp	al,'8'
	jne	ves6
	jmp	vtholen
ves6:	cmp	al,'D'
	jne	ves7
	jmp	vtindex
ves7:	cmp	al,'M'
	jne	ves8
	jmp	vtrindex
ves8:	cmp	al,'E'
	jne	ves9
	mov	vsp,0
	jmp	vtindex
ves9:	cmp	al,'c'
	jne	vesx
	jmp	vtreset
vesx:	ret				; ESC = ESC > ESC H usw.: ohne Wirkung
vt_esc	endp

vt_g0	proc	near
	mov	vstat,offset vt_norm
	mov	vgraf,0
	cmp	al,'0'
	jne	vg09
	mov	vgraf,1
vg09:	ret
vt_g0	endp

vt_g1	proc	near
	mov	vstat,offset vt_norm
	mov	vg1graf,0
	cmp	al,'0'
	jne	vg19
	mov	vg1graf,1
vg19:	ret
vt_g1	endp

vt_eins	proc	near			; ein Zeichen ueberlesen (ESC # n)
	mov	vstat,offset vt_norm
	ret
vt_eins	endp

vtsichern proc	near
	mov	al,vzl
	mov	vsz,al
	mov	al,vsp
	mov	vss,al
	mov	al,vattr
	mov	vsa,al
	ret
vtsichern endp

vtholen	proc	near
	mov	al,vsz
	mov	vzl,al
	mov	al,vss
	mov	vsp,al
	mov	al,vsa
	mov	vattr,al
	mov	vwrap,0
	ret
vtholen	endp

vtreset	proc	near
	mov	vattr,0
	mov	voben,0
	mov	vunten,VZ-1
	mov	vorig,0
	mov	vauto,1
	mov	vgraf,0
	mov	vg1graf,0
	mov	vshift,0
	mov	vzl,0
	mov	vsp,0
	mov	vwrap,0
	mov	vappcur,0
	xor	bx,bx
	mov	cx,VGR
	call	vleer
	call	kalles
	ret
vtreset	endp
vappcur	db	0		; Cursortasten im Anwendungsmodus (ESC O x)

; Zeichen innerhalb CSI
vt_csi	proc	near
	cmp	al,' '
	jae	vcs0
	jmp	vtsteuer		; Steuerzeichen wirken mitten in der Folge
vcs0:	cmp	al,'?'
	jne	vcs1
	mov	vpriv,1
	ret
vcs1:	cmp	al,'0'
	jb	vcs3
	cmp	al,'9'
	ja	vcs2
	sub	al,'0'			; Ziffer
	xor	ah,ah
	mov	cx,ax
	mov	bl,vnpar
	xor	bh,bh
	shl	bx,1
	mov	ax,vpar[bx]
	mov	dx,10
	mul	dx
	add	ax,cx
	mov	vpar[bx],ax
	ret
vcs2:	cmp	al,';'
	jne	vcs3a
	cmp	vnpar,7
	jae	vcs2a
	inc	vnpar
vcs2a:	ret
vcs3:	cmp	al,'0'			; 20h..2Fh: Zwischenzeichen, uebergehen
	jb	vcs2a
vcs3a:	cmp	al,'@'			; ':' '<' '=' '>': uebergehen
	jb	vcs2a
	mov	vstat,offset vt_norm	; Endzeichen
	mov	si,offset csitab
vcs4:	mov	ah,[si]
	or	ah,ah
	jz	vcs9
	cmp	ah,al
	je	vcs5
	add	si,3
	jmp	short vcs4
vcs5:	jmp	word ptr [si+1]
vcs9:	ret
vt_csi	endp

csitab	db	'A'
	dw	c_cuu
	db	'B'
	dw	c_cud
	db	'C'
	dw	c_cuf
	db	'D'
	dw	c_cub
	db	'H'
	dw	c_cup
	db	'f'
	dw	c_cup
	db	'G'
	dw	c_cha
	db	'd'
	dw	c_vpa
	db	'J'
	dw	c_ed
	db	'K'
	dw	c_el
	db	'L'
	dw	c_il
	db	'M'
	dw	c_dl
	db	'@'
	dw	c_ich
	db	'P'
	dw	c_dch
	db	'X'
	dw	c_ech
	db	'm'
	dw	c_sgr
	db	'r'
	dw	c_stbm
	db	'h'
	dw	c_sm
	db	'l'
	dw	c_rm
	db	'n'
	dw	c_dsr
	db	'c'
	dw	c_da
	db	's'
	dw	vtsichern
	db	'u'
	dw	vtholen
	db	0

; erster Parameter, mindestens 1 -> CX
par1	proc	near
	mov	cx,vpar
	or	cx,cx
	jnz	p19
	inc	cx
p19:	ret
par1	endp

c_cuu	proc	near
	call	par1
	mov	vwrap,0
	mov	al,0			; Grenze: Rollbereich, wenn darin
	mov	ah,vzl
	cmp	ah,voben
	jb	cuu1
	mov	al,voben
cuu1:	mov	ah,vzl
	cmp	cx,0FFh
	jbe	cuu2
	mov	cx,0FFh
cuu2:	sub	ah,cl
	jc	cuu3
	cmp	ah,al
	jae	cuu4
cuu3:	mov	ah,al
cuu4:	mov	vzl,ah
	ret
c_cuu	endp

c_cud	proc	near
	call	par1
	mov	vwrap,0
	mov	al,VZ-1
	mov	ah,vzl
	cmp	ah,vunten
	ja	cud1
	mov	al,vunten
cud1:	cmp	cx,VZ
	jbe	cud2
	mov	cx,VZ
cud2:	add	ah,cl
	cmp	ah,al
	jbe	cud3
	mov	ah,al
cud3:	mov	vzl,ah
	ret
c_cud	endp

c_cuf	proc	near
	call	par1
	mov	vwrap,0
	cmp	cx,VS
	jbe	cuf1
	mov	cx,VS
cuf1:	mov	al,vsp
	add	al,cl
	cmp	al,VS-1
	jbe	cuf2
	mov	al,VS-1
cuf2:	mov	vsp,al
	ret
c_cuf	endp

c_cub	proc	near
	call	par1
	mov	vwrap,0
	cmp	cx,VS
	jbe	cub1
	mov	cx,VS
cub1:	mov	al,vsp
	sub	al,cl
	jnc	cub2
	xor	al,al
cub2:	mov	vsp,al
	ret
c_cub	endp

; Zeile (1-basiert in AX) mit Ursprungsmodus begrenzt setzen
setzl	proc	near
	or	ax,ax
	jz	szl1
	dec	ax
szl1:	cmp	vorig,0
	je	szl2
	add	al,voben
	cmp	al,vunten
	jbe	szl3
	mov	al,vunten
	jmp	short szl3
szl2:	cmp	ax,VZ-1
	jbe	szl3
	mov	al,VZ-1
szl3:	mov	vzl,al
	ret
setzl	endp

setsp	proc	near
	or	ax,ax
	jz	ssp_1
	dec	ax
ssp_1:	cmp	ax,VS-1
	jbe	ssp_2
	mov	ax,VS-1
ssp_2:	mov	vsp,al
	ret
setsp	endp

c_cup	proc	near
	mov	vwrap,0
	mov	ax,vpar
	call	setzl
	mov	ax,vpar+2
	call	setsp
	ret
c_cup	endp

c_cha	proc	near
	mov	vwrap,0
	mov	ax,vpar
	call	setsp
	ret
c_cha	endp

c_vpa	proc	near
	mov	vwrap,0
	mov	ax,vpar
	call	setzl
	ret
c_vpa	endp

; ED: 0 ab Cursor, 1 bis Cursor, 2 alles
c_ed	proc	near
	mov	ax,vpar
	cmp	ax,2
	jne	ced1
	xor	bx,bx
	mov	cx,VGR
	call	vleer
	call	kalles
	ret
ced1:	cmp	ax,1
	je	ced2
	mov	al,vzl			; 0: Cursor bis Ende
	mov	ah,vsp
	call	vidx
	mov	cx,VGR
	sub	cx,bx
	call	vleer
	mov	al,vzl
	mov	ah,vsp
	call	kzeile
	mov	al,vzl
	cmp	al,VZ-1
	jae	ced9
	inc	al
	mov	ah,VZ-1
	call	kzeilen
ced9:	ret
ced2:	mov	al,vzl			; 1: Anfang bis Cursor
	mov	ah,vsp
	call	vidx
	mov	cx,bx
	inc	cx
	xor	bx,bx
	call	vleer
	mov	ah,vzl
	mov	al,0
	call	kzeilen
	ret
c_ed	endp

; EL: 0 ab Cursor, 1 bis Cursor, 2 ganze Zeile
c_el	proc	near
	mov	ax,vpar
	cmp	ax,1
	je	cel1
	cmp	ax,2
	je	cel2
	mov	al,vzl
	mov	ah,vsp
	call	vidx
	mov	cx,VS
	sub	cl,vsp
	call	vleer
	mov	al,vzl
	mov	ah,vsp
	call	kzeile
	ret
cel1:	mov	al,vzl
	mov	ah,0
	call	vidx
	mov	cl,vsp
	xor	ch,ch
	inc	cx
	call	vleer
	mov	al,vzl
	mov	ah,0
	call	kzeile
	ret
cel2:	mov	al,vzl
	mov	ah,0
	call	vidx
	mov	cx,VS
	call	vleer
	mov	al,vzl
	mov	ah,0
	call	kzeile
	ret
c_el	endp

; IL/DL: n Zeilen ab Cursor im Rollbereich einfuegen/loeschen
c_il	proc	near
	mov	al,vzl
	cmp	al,voben
	jb	cil9
	cmp	al,vunten
	ja	cil9
	call	par1
	cmp	cx,VZ
	jbe	cil1
	mov	cx,VZ
cil1:	mov	al,vzl
	mov	ah,vunten
	call	vrunter
	loop	cil1
	mov	al,vzl
	mov	ah,vunten
	call	kzeilen
	mov	vsp,0
	mov	vwrap,0
cil9:	ret
c_il	endp

c_dl	proc	near
	mov	al,vzl
	cmp	al,voben
	jb	cdl9
	cmp	al,vunten
	ja	cdl9
	call	par1
	cmp	cx,VZ
	jbe	cdl1
	mov	cx,VZ
cdl1:	mov	al,vzl
	mov	ah,vunten
	call	vhoch
	loop	cdl1
	mov	al,vzl
	mov	ah,vunten
	call	kzeilen
	mov	vsp,0
	mov	vwrap,0
cdl9:	ret
c_dl	endp

; ICH: n Leerzeichen am Cursor einfuegen, Rest nach rechts
c_ich	proc	near
	call	par1
	mov	al,VS
	sub	al,vsp			; Platz bis Zeilenende
	xor	ah,ah
	cmp	cx,ax
	jbe	cic1
	mov	cx,ax
cic1:	push	cx
	mov	al,vzl			; von rechts her schieben
	mov	ah,VS-1
	call	vidx			; BX = letzte Zelle
	mov	dx,ax			; (AX unveraendert von vidx)
	mov	al,VS
	sub	al,vsp
	xor	ah,ah
	sub	ax,cx			; zu schiebende Zellen
	mov	cx,ax
	pop	dx			; DX = n
	jcxz	cic2
	std
	push	cx
	mov	di,offset vchr
	add	di,bx
	mov	si,di
	sub	si,dx
	rep	movsb
	pop	cx
	mov	di,offset vatt
	add	di,bx
	mov	si,di
	sub	si,dx
	rep	movsb
	cld
cic2:	mov	al,vzl
	mov	ah,vsp
	call	vidx
	mov	cx,dx
	call	vleer
	mov	al,vzl
	mov	ah,vsp
	call	kzeile
	ret
c_ich	endp

; DCH: n Zeichen am Cursor loeschen, Rest nach links
c_dch	proc	near
	call	par1
	mov	al,VS
	sub	al,vsp
	xor	ah,ah
	cmp	cx,ax
	jbe	cdc1
	mov	cx,ax
cdc1:	mov	dx,cx			; DX = n
	mov	al,vzl
	mov	ah,vsp
	call	vidx
	mov	al,VS
	sub	al,vsp
	xor	ah,ah
	sub	ax,dx
	mov	cx,ax
	jcxz	cdc2
	push	cx
	mov	di,offset vchr
	add	di,bx
	mov	si,di
	add	si,dx
	rep	movsb
	pop	cx
	mov	di,offset vatt
	add	di,bx
	mov	si,di
	add	si,dx
	rep	movsb
cdc2:	mov	al,vzl			; hinten n Leerzeichen
	mov	ah,VS
	sub	ah,dl
	call	vidx
	mov	cx,dx
	call	vleer
	mov	al,vzl
	mov	ah,vsp
	call	kzeile
	ret
c_dch	endp

c_ech	proc	near
	call	par1
	mov	al,VS
	sub	al,vsp
	xor	ah,ah
	cmp	cx,ax
	jbe	cec1
	mov	cx,ax
cec1:	mov	al,vzl
	mov	ah,vsp
	call	vidx
	call	vleer
	mov	al,vzl
	mov	ah,vsp
	call	kzeile
	ret
c_ech	endp

; SGR: 0 normal, 1 2 4 halbhell, 5 blinkend, 7 invers, 2x aus; Farben
; werden uebergangen (die DMV zeigt hier einfarbig)
c_sgr	proc	near
	mov	cl,vnpar
	xor	ch,ch
	inc	cx
	mov	si,offset vpar
csg1:	lodsw
	cmp	ax,0
	jne	csg2
	mov	vattr,0
	jmp	short csg8
csg2:	cmp	ax,1
	je	csg2a
	cmp	ax,2
	je	csg2a
	cmp	ax,4
	jne	csg3
csg2a:	or	vattr,4
	jmp	short csg8
csg3:	cmp	ax,5
	jne	csg4
	or	vattr,2
	jmp	short csg8
csg4:	cmp	ax,7
	jne	csg5
	or	vattr,1
	jmp	short csg8
csg5:	cmp	ax,22
	je	csg5a
	cmp	ax,24
	jne	csg6
csg5a:	and	vattr,not 4
	jmp	short csg8
csg6:	cmp	ax,25
	jne	csg7
	and	vattr,not 2
	jmp	short csg8
csg7:	cmp	ax,27
	jne	csg8
	and	vattr,not 1
csg8:	loop	csg1
	ret
c_sgr	endp

; DECSTBM: Rollbereich, Cursor nach oben links
c_stbm	proc	near
	mov	ax,vpar
	or	ax,ax
	jz	cst1
	dec	ax
cst1:	mov	dx,vpar+2
	or	dx,dx
	jnz	cst2
	mov	dx,VZ
cst2:	dec	dx
	cmp	dx,VZ-1
	jbe	cst3
	mov	dx,VZ-1
cst3:	cmp	ax,dx
	jae	cst9
	mov	voben,al
	mov	vunten,dl
	mov	vsp,0
	mov	vwrap,0
	mov	al,0
	cmp	vorig,0
	je	cst4
	mov	al,voben
cst4:	mov	vzl,al
cst9:	ret
c_stbm	endp

; SM/RM: nur private Modi ?1 (Cursortasten), ?6 (Ursprung), ?7 (Umbruch)
c_sm	proc	near
	mov	dl,1
	jmp	short csmrm
c_sm	endp
c_rm	proc	near
	mov	dl,0
c_rm	endp
csmrm	proc	near
	cmp	vpriv,0
	je	csr9
	mov	ax,vpar
	cmp	ax,1
	jne	csr1
	mov	vappcur,dl
	ret
csr1:	cmp	ax,6
	jne	csr2
	mov	vorig,dl
	mov	vsp,0
	mov	al,0
	or	dl,dl
	jz	csr1a
	mov	al,voben
csr1a:	mov	vzl,al
	ret
csr2:	cmp	ax,7
	jne	csr9
	mov	vauto,dl
csr9:	ret
csmrm	endp

; DSR: 5 Zustand, 6 Cursorposition
c_dsr	proc	near
	mov	ax,vpar
	cmp	ax,5
	jne	cds1
	mov	si,offset dsr_ok
	jmp	vsende
cds1:	cmp	ax,6
	jne	cds9
	mov	al,esc_
	call	sput
	mov	al,'['
	call	sput
	mov	al,vzl
	cmp	vorig,0
	je	cds2
	sub	al,voben
cds2:	inc	al
	call	vsendz
	mov	al,';'
	call	sput
	mov	al,vsp
	inc	al
	call	vsendz
	mov	al,'R'
	call	sput
cds9:	ret
c_dsr	endp
dsr_ok	db	4,1Bh,'[0n'
da_vt	db	5,1Bh,'[?6c'

c_da	proc	near
	cmp	vpar,0
	jne	cda9
	mov	si,offset da_vt
	jmp	vsende
cda9:	ret
c_da	endp

; Zaehllaenge-Folge SI senden
vsende	proc	near
	push	ax
	push	cx
	lodsb
	mov	cl,al
	xor	ch,ch
vse1:	lodsb
	call	sput
	loop	vse1
	pop	cx
	pop	ax
	ret
vsende	endp

; Zahl AL (0..255) dezimal senden
vsendz	proc	near
	push	ax
	push	bx
	push	cx
	xor	ah,ah
	mov	bl,10
	xor	cx,cx
vsz1:	div	bl
	push	ax
	inc	cx
	xor	ah,ah
	or	al,al
	jnz	vsz1
vsz2:	pop	ax
	mov	al,ah
	add	al,'0'
	call	sput
	loop	vsz2
	pop	cx
	pop	bx
	pop	ax
	ret
vsendz	endp

; Konsolenart bestimmen: unter NCR DOS 2.11 mit CRT_ROWS = 24 rollt die
; Konsole nur Zeilen 0..23.
vtini	proc	near
	push	ax
	push	es
	mov	rollz,VZ
	cmp	dosver,2
	jne	vtin9
	xor	ax,ax
	mov	es,ax
	cmp	byte ptr es:[41Dh],24
	jne	vtin9
	mov	rollz,VZ-1
vtin9:	pop	es
	pop	ax
	ret
vtini	endp

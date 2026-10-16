; skript.inc - Skriptbefehle, PUSH/RUN, TYPE, STATISTICS
;
; OUTPUT text          Text senden (\13 = CR, \nnn dezimal, \\ = \)
; INPUT n text         bis n Sekunden auf den Text warten (gross/klein egal)
; PAUSE [n]            n Sekunden warten (Vorgabe 1), Empfangenes zeigen
; CLEAR                Empfangspuffer leeren
; ECHO text            Text auf den Bildschirm
; IF SUCCESS|FAILURE befehl
; GOTO marke           in einer TAKE-Datei zur Zeile ":marke"
; STOP                 TAKE-Datei beenden
; PUSH, RUN befehl     DOS-Befehlsprozessor bzw. ein Programm aufrufen
; TYPE datei           Datei auf dem Bildschirm zeigen
; STATISTICS           Werte der letzten Uebertragung

erfolg	db	1		; Ergebnis des letzten Befehls
erfalt	db	1		; ... des vorletzten (fuer IF)
spuf2	db	80 dup (0)	; Text nach Ersatz der \-Folgen
spl	dw	0

s_ok	db	'[INPUT: gefunden]',cr,lf,'$'
s_nein	db	cr,lf,'[INPUT: Zeit abgelaufen]',cr,lf,'$'
s_keint	db	'?GOTO geht nur in einer TAKE-Datei.',cr,lf,'$'
s_marke	db	'?Marke nicht gefunden.',cr,lf,'$'
s_exec	db	'?Programm laesst sich nicht starten.',cr,lf,'$'
s_zur	db	cr,lf,'[Zurueck bei DMVKERM.]',cr,lf,'$'
if_tab	db	7,'SUCCESS'
	dw	1
	db	7,'FAILURE'
	dw	0
	db	0

; Rest der Zeile nach spuf2, \-Folgen ersetzen; CX = Laenge
sesc	proc	near
	call	restzeile		; -> roh
	mov	si,offset roh
	mov	di,offset spuf2
	xor	cx,cx
se1:	lodsb
	or	al,al
	jz	se9
	cmp	al,'\'
	jne	se5
	mov	al,[si]
	cmp	al,'\'
	jne	se2
	inc	si
	jmp	short se5
se2:	sub	al,'0'			; \nnn dezimal
	cmp	al,9
	ja	se4
	xor	ah,ah
	mov	bx,ax
	inc	si
se3:	mov	al,[si]
	sub	al,'0'
	cmp	al,9
	ja	se3a
	xor	ah,ah
	push	ax
	mov	ax,bx
	mov	dx,10
	mul	dx
	pop	bx
	add	bx,ax
	inc	si
	jmp	short se3
se3a:	mov	al,bl
	jmp	short se5
se4:	mov	al,'\'			; kein Code: \ bleibt
se5:	cmp	cx,79
	jae	se1
	stosb
	inc	cx
	jmp	short se1
se9:	mov	spl,cx
	ret
sesc	endp

b_output proc	near
	cmp	poffen,0
	jne	bou1
	mov	dx,offset m_keinp
	jmp	textd
bou1:	call	sesc
	jcxz	bou9
	mov	si,offset spuf2
bou2:	lodsb
	call	sput
	loop	bou2
bou9:	ret
b_output endp

b_echo	proc	near
	call	sesc
	mov	si,offset spuf2
	jcxz	bec9
bec1:	lodsb
	call	zeichen
	loop	bec1
bec9:	jmp	crlf
b_echo	endp

b_clear	proc	near
bcr1:	call	sget
	jnz	bcr1
	ret
b_clear	endp

; Empfangenes zeigen, bis zur Frist rzeit; ESC beendet vorzeitig (Carry)
; Bei INPUT wird nach spuf2/spl gesucht: gefunden -> ZF ... siehe b_input
b_pause	proc	near
	call	naechstes
	mov	ax,1
	jc	bpa1
	mov	si,wanf			; Wort zurueck, Zahl lesen
	mov	zpos,si
	call	wortzahl
	jc	bpa9
bpa1:	call	sekfrist
bpa2:	call	sget
	jz	bpa3
	call	tzeichen
	jmp	short bpa2
bpa3:	call	kbhol
	jz	bpa4
	cmp	al,esc_
	je	bpa8
bpa4:	call	abgel
	jnc	bpa2
	ret
bpa8:	mov	erfolg,0
bpa9:	ret
b_pause	endp

; Frist in AX Sekunden ab jetzt (rstart/rzeit)
sekfrist proc	near
	push	ax
	push	bx
	push	dx
	mov	bx,100
	mul	bx			; DX:AX Hundertstel
	push	dx
	push	ax
	call	uhr
	mov	rstart,ax
	mov	rstart+2,dx
	pop	bx
	add	ax,bx
	pop	bx
	adc	dx,bx
	mov	rzeit,ax
	mov	rzeit+2,dx
	pop	dx
	pop	bx
	pop	ax
	ret
sekfrist endp

; Frist abgelaufen? Carry ja (auch ueber Mitternacht)
abgel	proc	near
	push	ax
	push	dx
	call	uhr
	cmp	dx,rstart+2
	jb	abg_j
	ja	abg1
	cmp	ax,rstart
	jb	abg_j
abg1:	cmp	dx,rzeit+2
	jb	abg_n
	ja	abg_j
	cmp	ax,rzeit
	jb	abg_n
abg_j:	stc
	jmp	short abg9
abg_n:	clc
abg9:	pop	dx
	pop	ax
	ret
abgel	endp

; INPUT n text
b_input	proc	near
	cmp	poffen,0
	jne	bin0
	mov	dx,offset m_keinp
	jmp	textd
bin0:	call	wortzahl
	jc	bin9
	call	sekfrist
	call	sesc
	mov	erfolg,0
	xor	bx,bx			; so viel passt schon
	jcxz	bin_ok
bin1:	call	sget
	jz	bin4
	call	tzeichen
	call	gross
	mov	ah,al
	mov	al,spuf2[bx]
	call	gross
	cmp	al,ah
	je	bin2
	xor	bx,bx			; neu anfangen, vielleicht mit diesem
	mov	al,spuf2
	call	gross
	cmp	al,ah
	jne	bin1
bin2:	inc	bx
	cmp	bx,spl
	jb	bin1
bin_ok:	mov	erfolg,1
	call	crlf
	mov	dx,offset s_ok
	jmp	textd
bin4:	call	kbhol
	jz	bin5
	cmp	al,esc_
	je	bin8
bin5:	call	abgel
	jnc	bin1
bin8:	mov	dx,offset s_nein
	call	textd
bin9:	ret
b_input	endp

gross	proc	near
	cmp	al,'a'
	jb	gr9
	cmp	al,'z'
	ja	gr9
	sub	al,20h
gr9:	ret
gross	endp

; IF SUCCESS|FAILURE befehl
b_if	proc	near
	mov	si,offset if_tab
	call	wortaus
	jc	bif9
	cmp	al,erfalt
	jne	bif8
	mov	al,erfalt		; der Befehl sieht das alte Ergebnis
	mov	erfolg,al
	jmp	ausfuehren
bif8:	mov	al,erfalt		; nicht ausgefuehrt: Ergebnis bleibt
	mov	erfolg,al
bif9:	ret
b_if	endp

; GOTO marke
b_goto	proc	near
	cmp	thand,0
	jne	bgo1
	mov	dx,offset s_keint
	jmp	textd
bgo1:	call	naechstes
	jnc	bgo2
	mov	dx,offset m_fehlt
	jmp	textd
bgo2:	mov	si,wanf			; Marke nach spuf2 (gross)
	mov	di,offset spuf2
	mov	cx,wlen
	cmp	cx,40
	jbe	bgo3
	mov	cx,40
bgo3:	mov	spl,cx
	rep	movsb
	mov	ax,4200h		; Datei von vorn lesen
	mov	bx,thand
	xor	cx,cx
	xor	dx,dx
	int	21h
	mov	tpos,0
	mov	tend,0
bgo4:	call	tzeile			; Zeile ":marke" suchen
	jc	bgo8
	mov	bl,zpuf+1
	xor	bh,bh
	mov	byte ptr zpuf+2[bx],0
	cmp	zpuf+2,':'
	jne	bgo4
	mov	si,offset zpuf+3
	mov	di,offset spuf2
	mov	cx,spl
bgo5:	lodsb
	call	gross
	cmp	al,[di]
	jne	bgo4
	inc	di
	loop	bgo5
	mov	al,[si]			; danach muss das Wort zu Ende sein
	cmp	al,' '
	jbe	bgo9
	jmp	short bgo4
bgo8:	mov	erfolg,0
	mov	dx,offset s_marke
	jmp	textd
bgo9:	ret
b_goto	endp

b_stop	proc	near
	jmp	tzu
b_stop	endp

; --- PUSH, RUN --------------------------------------------------------------
comspec	db	'COMSPEC='
cmdname	db	64 dup (0)
cmdstd	db	'\COMMAND.COM',0
ctail	db	0, 80 dup (0)	; Laenge, Text, CR
epb	dw	0		; Umgebung des Vaters
	dw	offset ctail
eseg1	dw	0
	dw	5Ch
eseg2	dw	0
	dw	6Ch
eseg3	dw	0
altsp	dw	0
altss	dw	0

; Namen des Befehlsprozessors aus der Umgebung holen
holcomspec proc	near
	push	ds
	push	es
	mov	si,offset cmdstd	; Vorgabe
	mov	di,offset cmdname
	mov	cx,13
	rep	movsb
	mov	es,ds:[2Ch]		; Umgebung
	xor	di,di
hc1:	cmp	byte ptr es:[di],0	; Ende der Umgebung
	je	hc9
	mov	si,offset comspec
	mov	cx,8
	push	di
	repe	cmpsb
	jne	hc5
	pop	ax			; gefunden: Wert kopieren
	push	es
	push	ds
	pop	es
	pop	ds			; DS = Umgebung, ES = wir
	mov	si,di
	mov	di,offset cmdname
	mov	cx,63
hc2:	lodsb
	stosb
	or	al,al
	loopnz	hc2
	mov	byte ptr es:[di],0
	jmp	short hc9
hc5:	pop	di			; naechster Eintrag
	xor	al,al
	mov	cx,0FFFFh
	repne	scasb
	jmp	short hc1
hc9:	pop	es
	pop	ds
	ret
holcomspec endp

b_push	proc	near
	mov	ctail,0
	mov	ctail+1,cr
	jmp	short starte
b_push	endp

b_run	proc	near
	call	restzeile		; " /C befehl"
	mov	si,offset ctail+1
	mov	byte ptr [si],' '
	mov	byte ptr [si+1],'/'
	mov	byte ptr [si+2],'C'
	mov	byte ptr [si+3],' '
	lea	di,[si+4]
	mov	si,offset roh
	mov	bx,4
brn1:	lodsb
	or	al,al
	jz	brn2
	cmp	bx,76
	jae	brn2
	stosb
	inc	bx
	jmp	short brn1
brn2:	mov	byte ptr [di],cr
	mov	ctail,bl
	jmp	short starte
b_run	endp

; Befehlsprozessor mit ctail starten (Speicher ist beim Start verkleinert)
starte	proc	near
	call	holcomspec
	mov	eseg1,cs
	mov	eseg2,cs
	mov	eseg3,cs
	mov	altsp,sp
	mov	altss,ss
	mov	dx,offset cmdname
	mov	bx,offset epb
	mov	ax,4B00h
	int	21h
	cli				; DOS 2 laesst SS:SP nicht stehen
	mov	ss,cs:altss
	mov	sp,cs:altsp
	sti
	push	cs
	pop	ds
	push	cs
	pop	es
	cld
	jnc	sta1
	mov	erfolg,0
	mov	dx,offset s_exec
	jmp	textd
sta1:	mov	dx,offset s_zur
	jmp	textd
starte	endp

; --- TYPE -------------------------------------------------------------------
b_type	proc	near
	call	restmuster
	jnc	bty1
	mov	dx,offset m_fehlt
	jmp	textd
bty1:	mov	ax,3D00h
	mov	dx,offset roh
	int	21h
	jnc	bty2
	mov	erfolg,0
	mov	dx,offset a_nicht
	jmp	textd
bty2:	mov	bx,ax
bty3:	mov	ah,3Fh			; Stueckweise auf die Standardausgabe
	mov	cx,512
	mov	dx,offset fpuf
	int	21h
	jc	bty8
	or	ax,ax
	jz	bty8
	mov	cx,ax
	push	bx
	mov	ah,40h
	mov	bx,1
	int	21h
	pop	bx
	call	kbhol			; ESC bricht ab
	jz	bty3
	cmp	al,esc_
	jne	bty3
bty8:	mov	ah,3Eh
	int	21h
	jmp	crlf
b_type	endp

; --- STATISTICS -------------------------------------------------------------
tstart	dw	0, 0
tdauer	dw	0, 0		; Hundertstel
st_kopf	db	'Letzte Uebertragung:',cr,lf,'$'
st_dat	db	'  Datei:           $'
st_by	db	cr,lf,'  Bytes:           $'
st_pk	db	cr,lf,'  Pakete gesendet: $'
st_wd	db	cr,lf,'  Wiederholungen:  $'
st_dau	db	cr,lf,'  Dauer:           $'
st_sek	db	' s$'
st_rate	db	cr,lf,'  Bytes/s:         $'

; Zeit beim Beginn merken / Dauer bestimmen (aus bildan/bildaus)
zeitan	proc	near
	push	ax
	push	dx
	call	uhr
	mov	tstart,ax
	mov	tstart+2,dx
	pop	dx
	pop	ax
	ret
zeitan	endp

zeitaus	proc	near
	push	ax
	push	dx
	call	uhr
	sub	ax,tstart
	sbb	dx,tstart+2
	jnc	zta1
	add	ax,6000h		; ueber Mitternacht: + 8640000
	adc	dx,83h
zta1:	mov	tdauer,ax
	mov	tdauer+2,dx
	pop	dx
	pop	ax
	ret
zeitaus	endp

b_stat	proc	near
	mov	dx,offset st_kopf
	call	textd
	mov	dx,offset st_dat
	call	textd
	mov	si,offset fname
	call	text0
	mov	dx,offset st_by
	call	textd
	mov	ax,kbytes
	mov	dx,kbytes+2
	call	zahl32
	mov	dx,offset st_pk
	call	textd
	mov	ax,kpakete
	call	zahl
	mov	dx,offset st_wd
	call	textd
	mov	ax,kwdh
	call	zahl
	mov	dx,offset st_dau	; Sekunden = Hundertstel / 100
	call	textd
	mov	ax,tdauer
	mov	dx,tdauer+2
	mov	bx,100
	cmp	dx,bx
	jae	bst9
	div	bx
	mov	cx,ax			; CX = Sekunden
	xor	dx,dx
	call	zahl32
	mov	dx,offset st_sek
	call	textd
	jcxz	bst9
	mov	dx,offset st_rate
	call	textd
	mov	ax,kbytes
	mov	dx,kbytes+2
	cmp	dx,cx
	jae	bst9
	div	cx
	xor	dx,dx
	call	zahl32
bst9:	jmp	crlf
b_stat	endp

; verbind.inc - CONNECT (Terminalbetrieb)
;
; Empfangene Zeichen gehen an die Konsole, Tasten an die Leitung.
; Ctrl-CLR leitet einen Befehl ein (die Auswahl steht dann in der
; Statuszeile): C zurueck, B Break, H auflegen, Ctrl-CLR sendet Ctrl-].
; Stufe 1: Steuerzeichen CR LF BS BEL TAB werden durchgereicht, alle
; anderen Zeichen ueber die Glyphentabelle. Die VT102-Auswertung folgt.

v_zur	db	'[Zurueck bei DMVKERM.]$'
v_hilfe	db	' Befehl?  C zurueck  B Break  H auflegen  Ctrl-CLR nochmal: sendet Ctrl-]      '
v_hlen	equ	$-v_hilfe

b_connect proc	near
	cmp	poffen,0
	jne	bco1
	mov	dx,offset m_keinp
	call	textd
	ret
bco1:	mov	kpos,0FFFFh
	mov	katt,0FFh
	cmp	vinit,0			; erstes Mal: VT zuruecksetzen,
	jne	bco1a			;  sonst den Puffer wieder zeigen
	mov	vinit,1
	call	vtreset
	jmp	short bco1b
bco1a:	call	kalles
bco1b:	call	vcursor
bco2:	call	sget			; erst alles Empfangene zeigen
	jz	bco3
	call	lzeich
	call	vtzeich
	jmp	short bco2
bco3:	call	vcursor
	call	kbhol
	jz	bco2
	or	ah,ah
	jnz	bco6			; Sondertaste
	cmp	al,FLUCHT
	je	bco5
	call	sput
	jmp	short bco2
bco5:	call	vhilfe			; Befehl nach Ctrl-CLR: Auswahl in der
	or	ah,ah			;  Statuszeile zeigen, Taste holen
	jnz	bco2
	cmp	al,FLUCHT
	jne	bco5a
	call	sput
	jmp	short bco2
bco5a:	and	al,0DFh			; gross
	cmp	al,'C'
	je	bco9
	cmp	al,'B'
	jne	bco5b
	call	sbreak
	jmp	short bco2
bco5b:	cmp	al,'H'
	jne	bco5c
	call	shangup
	jmp	short bco2
bco5c:	jmp	short bco2		; andere Taste: nichts tun
bco6:	call	vtaste			; Cursor- und Funktionstasten
	jmp	short bco2
bco9:	mov	al,0			; Statuszeile weg, DOS schreibt unten
	call	kattr
	mov	ax,VZ*256
	call	kgehe
	mov	al,esc_
	call	kroh
	mov	al,'T'
	call	kroh
	mov	kpos,0FFFFh
	mov	dx,offset v_zur
	call	textd
	ret
b_connect endp

; Auswahl in der Statuszeile zeigen, auf eine Taste warten (-> AX), dabei
; weiter empfangen; danach wieder die normale Statuszeile
vhilfe	proc	near
	push	cx
	push	si
	mov	ax,VZ*256
	call	kgehe
	mov	al,1
	call	kattr
	mov	si,offset v_hilfe
	mov	cx,v_hlen
vhi1:	lodsb
	call	kroh
	loop	vhi1
	mov	kpos,0FFFFh
	mov	al,0
	call	kattr
vhi2:	call	sget			; derweil weiter empfangen
	jz	vhi3
	call	vtzeich
	jmp	short vhi2
vhi3:	call	kbhol
	jz	vhi2
	push	ax
	call	kstatus
	call	vcursor
	pop	ax
	pop	si
	pop	cx
	ret
vhilfe	endp

; Sondertaste AL (Tastencode) als VT-Folge senden
vt_hoch	db	3,1Bh,'[A'
vt_runt	db	3,1Bh,'[B'
vt_rech	db	3,1Bh,'[C'
vt_link	db	3,1Bh,'[D'
vt_pos1	db	3,1Bh,'[H'
vt_ahoch db	3,1Bh,'OA',3,1Bh,'OB',3,1Bh,'OC',3,1Bh,'OD'
vt_pf	db	3,1Bh,'OP',3,1Bh,'OQ',3,1Bh,'OR',3,1Bh,'OS'

vtaste	proc	near
	push	ax
	push	cx
	push	si
	mov	si,offset vt_hoch
	cmp	vappcur,0		; Anwendungsmodus: ESC O x
	je	vta0
	mov	si,offset vt_ahoch
vta0:	cmp	al,K_HOCH
	je	vta5
	add	si,4
	cmp	al,K_RUNTER
	je	vta5
	add	si,4
	cmp	al,K_RECHTS
	je	vta5
	add	si,4
	cmp	al,K_LINKS
	je	vta5
	mov	si,offset vt_pos1
	cmp	al,K_POS1
	je	vta5
	cmp	al,K_F1			; F1..F4 = PF1..PF4
	jb	vta9
	cmp	al,K_F4
	ja	vta9
	sub	al,K_F1
	mov	ah,4
	mul	ah
	mov	si,offset vt_pf
	add	si,ax
vta5:	lodsb
	mov	cl,al
	xor	ch,ch
vta6:	lodsb
	call	sput
	loop	vta6
vta9:	pop	si
	pop	cx
	pop	ax
	ret
vtaste	endp

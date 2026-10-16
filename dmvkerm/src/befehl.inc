; befehl.inc - Befehlszeile
;
; Die Zeile wird mit DOS (Int 21h, 0Ah) gelesen und in Woerter zerlegt.
; Befehle duerfen abgekuerzt werden; es gilt der erste Eintrag der Tabelle,
; zu dem die Abkuerzung passt (haeufige Befehle stehen vorn).

zeilmax	equ	120
zpuf	db	zeilmax, 0, zeilmax+2 dup (0)
zpos	dw	0		; Lesestelle in zpuf
wanf	dw	0		; aktuelles Wort: Anfang
wlen	dw	0		;  und Laenge
fertig	db	0		; 1 = EXIT
zorig	db	zeilmax+2 dup (0)	; Zeile, wie getippt

prompt	db	cr,lf,'DMVKERM>$'
m_unbek	db	'?Unbekannter Befehl, HELP zeigt die Befehle.',cr,lf,'$'
m_fehlt	db	'?Es fehlt etwas, HELP zeigt die Befehle.',cr,lf,'$'
m_zahl	db	'?Zahl erwartet.',cr,lf,'$'
m_keinp	db	'?Kein serielles Modul gewaehlt.',cr,lf,'$'

; Tabelle: Laenge, Name (gross), Adresse. Ende mit Laenge 0.
bef_tab	db	7,'CONNECT'
	dw	b_connect
	db	3,'SET'
	dw	b_set
	db	4,'SHOW'
	dw	b_show
	db	4,'HELP'
	dw	b_help
	db	6,'HANGUP'
	dw	b_hangup
	db	4,'SEND'
	dw	b_send
	db	7,'RECEIVE'
	dw	b_receive
	db	6,'REMOTE'
	dw	b_remote
	db	3,'GET'
	dw	b_get
	db	5,'XSEND'
	dw	b_xsend
	db	8,'XRECEIVE'
	dw	b_xreceive
	db	6,'SERVER'
	dw	b_server
	db	6,'FINISH'
	dw	b_finish
	db	3,'BYE'
	dw	b_bye
	db	3,'DIR'
	dw	b_dir
	db	2,'CD'
	dw	b_cd
	db	3,'DEL'
	dw	b_del
	db	4,'TAKE'
	dw	b_take
	db	8,'TRANSMIT'
	dw	b_transmit
	db	3,'LOG'
	dw	b_log
	db	5,'CLOSE'
	dw	b_close
	db	6,'OUTPUT'
	dw	b_output
	db	5,'INPUT'
	dw	b_input
	db	2,'IF'
	dw	b_if
	db	5,'PAUSE'
	dw	b_pause
	db	4,'GOTO'
	dw	b_goto
	db	4,'ECHO'
	dw	b_echo
	db	5,'CLEAR'
	dw	b_clear
	db	10,'STATISTICS'
	dw	b_stat
	db	4,'STOP'
	dw	b_stop
	db	4,'PUSH'
	dw	b_push
	db	3,'RUN'
	dw	b_run
	db	4,'TYPE'
	dw	b_type
	db	4,'EXIT'
	dw	b_exit
	db	4,'QUIT'
	dw	b_exit
	db	1,'?'
	dw	b_help
	db	0

set_tab	db	4,'PORT'
	dw	s_port
	db	5,'SPEED'
	dw	s_speed
	db	4,'BAUD'		; wie bei MS-Kermit: dasselbe wie SPEED
	dw	s_speed
	db	6,'PARITY'
	dw	s_parity
	db	4,'FLOW'
	dw	s_flow
	db	6,'XMODEM'
	dw	s_xmodem
	db	0

xm_tab	db	2,'1K'
	dw	1
	db	3,'128'
	dw	0
	db	0

par_tab	db	4,'NONE'
	dw	0
	db	4,'EVEN'
	dw	1
	db	3,'ODD'
	dw	2
	db	4,'MARK'
	dw	3
	db	5,'SPACE'
	dw	4
	db	0

flow_tab db	4,'NONE'
	dw	0
	db	8,'XON/XOFF'
	dw	1
	db	0

befehlsschleife proc	near
bs1:	mov	dx,offset prompt
	call	textd
	cmp	thand,0			; aus einer Befehlsdatei?
	je	bs1k
	call	tzeile
	jc	bs1k
	mov	bl,zpuf+1		; Zeile zeigen
	xor	bh,bh
	mov	byte ptr zpuf+2[bx],0
	mov	si,offset zpuf+2
	call	text0
	call	crlf
	cmp	zpuf+2,';'		; Kommentar
	je	bs1
	cmp	zpuf+2,':'		; Marke fuer GOTO
	je	bs1
	jmp	short bs1z
bs1k:	mov	dx,offset zpuf
	mov	ah,0Ah
	int	21h
	call	crlf
bs1z:
	mov	bl,zpuf+1		; Zeile mit 0 abschliessen, gross machen
	xor	bh,bh
	mov	byte ptr zpuf+2[bx],0
	mov	si,offset zpuf+2	; Abschrift in Originalschreibung
	mov	di,offset zorig
	mov	cx,bx
	inc	cx
	rep	movsb
	mov	si,offset zpuf+2
bs2:	mov	al,[si]
	or	al,al
	jz	bs3
	cmp	al,'a'
	jb	bs2a
	cmp	al,'z'
	ja	bs2a
	sub	al,20h
	mov	[si],al
bs2a:	inc	si
	jmp	short bs2
bs3:	mov	zpos,offset zpuf+2
	mov	al,erfolg		; Ergebnis des vorigen Befehls fuer IF
	mov	erfalt,al
	mov	erfolg,1
	call	ausfuehren
	cmp	fertig,0
	jne	bs9
	jmp	bs1
bs9:	ret
befehlsschleife endp

; Naechstes Wort als Befehl ausfuehren (auch fuer IF)
ausfuehren proc	near
	call	naechstes
	jc	af9			; leere Zeile
	mov	si,offset bef_tab
	call	suche
	jc	af8
	jmp	ax
af8:	mov	erfolg,0
	mov	dx,offset m_unbek
	call	textd
af9:	ret
ausfuehren endp

; Naechstes Wort: wanf/wlen. Carry, wenn keins mehr da ist.
naechstes proc	near
	push	ax
	push	si
	mov	si,zpos
nx1:	mov	al,[si]
	cmp	al,' '
	je	nx2
	cmp	al,tab
	jne	nx3
nx2:	inc	si
	jmp	short nx1
nx3:	or	al,al
	jz	nx8
	mov	wanf,si
nx4:	mov	al,[si]
	or	al,al
	jz	nx5
	cmp	al,' '
	je	nx5
	cmp	al,tab
	je	nx5
	inc	si
	jmp	short nx4
nx5:	mov	ax,si
	sub	ax,wanf
	mov	wlen,ax
	mov	zpos,si
	pop	si
	pop	ax
	clc
	ret
nx8:	mov	zpos,si
	pop	si
	pop	ax
	stc
	ret
naechstes endp

; Wort wanf/wlen in Tabelle SI suchen -> AX Wert. Carry: nicht gefunden.
suche	proc	near
	push	bx
	push	cx
	push	si
	push	di
su1:	mov	cl,[si]
	xor	ch,ch
	jcxz	su8
	cmp	cx,wlen			; Abkuerzung darf nicht laenger sein
	jb	su5
	push	si
	inc	si
	mov	di,wanf
	mov	cx,wlen
su2:	mov	al,[si]
	cmp	al,[di]
	jne	su4
	inc	si
	inc	di
	loop	su2
	pop	si
	mov	bl,[si]			; passt
	xor	bh,bh
	mov	ax,[si+bx+1]
	jmp	short su9
su4:	pop	si
su5:	mov	bl,[si]
	xor	bh,bh
	add	si,bx
	add	si,3
	jmp	short su1
su8:	stc
	jmp	short su9a
su9:	clc
su9a:	pop	di
	pop	si
	pop	cx
	pop	bx
	ret
suche	endp

; Naechstes Wort als Zahl -> AX. Carry bei Fehler (Meldung ist gegeben).
wortzahl proc	near
	push	bx
	push	cx
	push	dx
	push	si
	call	naechstes
	jc	wz8
	mov	si,wanf
	mov	cx,wlen
	xor	ax,ax
wz1:	mov	bl,[si]
	sub	bl,'0'
	cmp	bl,9
	ja	wz7
	mov	dx,10
	mul	dx
	jc	wz7
	xor	bh,bh
	add	ax,bx
	jc	wz7
	inc	si
	loop	wz1
	clc
	jmp	short wz9
wz7:	mov	dx,offset m_zahl
	call	textd
	stc
	jmp	short wz9
wz8:	mov	dx,offset m_fehlt
	call	textd
	stc
wz9:	pop	si
	pop	dx
	pop	cx
	pop	bx
	ret
wortzahl endp

; Naechstes Wort in Tabelle SI -> AX. Carry bei Fehler (Meldung gegeben).
wortaus	proc	near
	call	naechstes
	jc	wa8
	call	suche
	jnc	wa9
	mov	dx,offset m_unbek
	call	textd
	stc
	ret
wa8:	mov	dx,offset m_fehlt
	call	textd
	stc
wa9:	ret
wortaus	endp

; --- Befehle ----------------------------------------------------------------
b_exit	proc	near
	mov	fertig,1
	ret
b_exit	endp

b_set	proc	near
	mov	si,offset set_tab
	call	wortaus
	jc	bset9
	jmp	ax
bset9:	ret
b_set	endp

s_port	proc	near
	call	wortzahl
	jc	spo9
	call	portwahl
	jnc	spo9
	mov	dx,offset m_keinmod
	call	textd
spo9:	ret
s_port	endp

s_speed	proc	near
	call	wortzahl
	jc	ssp9
	xor	bx,bx
ssp1:	cmp	bx,baudn
	jae	ssp8
	push	bx
	shl	bx,1
	cmp	ax,baudtab[bx]
	pop	bx
	je	ssp2
	inc	bx
	jmp	short ssp1
ssp2:	mov	pbaud,bl
	cmp	poffen,0
	je	ssp9
	call	setmode
	ret
ssp8:	mov	dx,offset m_baud
	call	textd
ssp9:	ret
s_speed	endp
m_baud	db	'?Moeglich: 50 75 110 134 150 300 600 1200 1800 2000 2400',cr,lf
	db	'  3600 4800 7200 9600 19200',cr,lf,'$'

s_parity proc	near
	mov	si,offset par_tab
	call	wortaus
	jc	spa_9
	mov	pparit,al
	call	setparit
spa_9:	ret
s_parity endp

s_flow	proc	near
	mov	si,offset flow_tab
	call	wortaus
	jc	sfl9
	mov	pfluss,al
sfl9:	ret
s_flow	endp

s_xmodem proc	near
	mov	si,offset xm_tab
	call	wortaus
	jc	sxm9
	mov	x1k,al
sxm9:	ret
s_xmodem endp

b_hangup proc	near
	cmp	poffen,0
	je	bhu8
	call	shangup
	ret
bhu8:	mov	dx,offset m_keinp
	call	textd
	ret
b_hangup endp

; SHOW: Module, Port, Einstellungen
sh_mod	db	'Serielle Module: $'
sh_keine db	'keine$'
sh_2651	db	' (2651)$'
sh_2661	db	' (2661)$'
sh_k211	db	'K211$'
sh_k212	db	'K212$'
sh_k801	db	'K801$'
sh_port	db	cr,lf,'Port:            $'
sh_irq	db	', IRQ $'
sh_poll	db	', abgefragt (kein IRQ)$'
sh_ein	db	cr,lf,'Einstellungen:   $'
sh_baud	db	' Baud, Paritaet $'
sh_fl	db	cr,lf,'Flusssteuerung:  $'
sh_xon	db	'XON/XOFF$'
sh_kein	db	'keine$'
sh_lin	db	cr,lf,'Leitungen:       DSR $'
sh_dcd	db	', DCD $'
sh_an	db	'an$'
sh_aus	db	'aus$'
sh_verl	db	cr,lf,'Verlorene Zeichen: $'
partxt	db	'keine   $gerade  $ungerade$mark    $space   $'

b_show	proc	near
	mov	dx,offset sh_mod
	call	textd
	cmp	ifn,0
	jne	bsh1
	mov	dx,offset sh_keine
	call	textd
	jmp	bsh9
bsh1:	xor	bx,bx
bsh2:	cmp	bl,ifn
	jae	bsh3
	or	bx,bx
	jz	bsh2a
	mov	al,','
	call	zeichen
	mov	al,' '
	call	zeichen
bsh2a:	call	modname
	inc	bx
	jmp	short bsh2
bsh3:	cmp	poffen,0
	je	bsh9
	mov	dx,offset sh_port
	call	textd
	mov	al,portnr
	xor	ah,ah
	call	zahl
	mov	al,' '
	call	zeichen
	mov	al,'='
	call	zeichen
	mov	al,' '
	call	zeichen
	mov	bl,portnr
	dec	bl
	xor	bh,bh
	call	modname
	mov	dx,offset sh_poll
	cmp	pirq,0FFh
	je	bsh4
	mov	dx,offset sh_irq
	call	textd
	mov	al,pirq
	xor	ah,ah
	call	zahl
	jmp	short bsh5
bsh4:	call	textd
bsh5:	mov	dx,offset sh_ein
	call	textd
	mov	bl,pbaud
	xor	bh,bh
	shl	bx,1
	mov	ax,baudtab[bx]
	call	zahl
	mov	dx,offset sh_baud
	call	textd
	mov	al,pparit
	mov	ah,9
	mul	ah
	add	ax,offset partxt
	mov	dx,ax
	call	textd
	mov	dx,offset sh_fl
	call	textd
	mov	dx,offset sh_xon
	cmp	pfluss,0
	jne	bsh6
	mov	dx,offset sh_kein
bsh6:	call	textd
	mov	dx,offset sh_lin
	call	textd
	call	sstatus
	mov	ah,al
	mov	dx,offset sh_an
	test	ah,80h
	jnz	bsh7
	mov	dx,offset sh_aus
bsh7:	call	textd
	mov	dx,offset sh_dcd
	call	textd
	mov	dx,offset sh_an
	test	ah,40h
	jnz	bsh8
	mov	dx,offset sh_aus
bsh8:	call	textd
	mov	dx,offset sh_verl
	call	textd
	mov	ax,rxverl
	call	zahl
bsh9:	mov	dx,offset sh_xm		; XMODEM und Mitschnitt
	call	textd
	mov	dx,offset sh_1k
	cmp	x1k,0
	jne	bsh10
	mov	dx,offset sh_128
bsh10:	call	textd
	mov	dx,offset sh_log
	call	textd
	mov	dx,offset sh_aus
	cmp	lhand,0
	je	bsh11
	mov	dx,offset sh_an
bsh11:	call	textd
	call	crlf
	ret
b_show	endp
sh_xm	db	cr,lf,'XMODEM-Bloecke:  $'
sh_1k	db	'1K (mit CRC)$'
sh_128	db	'128$'
sh_log	db	cr,lf,'Mitschnitt:      $'

; Name des Moduls BX (Index) ausgeben: K211/K212 nach der Haelfte,
; K801 fuer einen 2661.
modname	proc	near
	push	ax
	push	dx
	mov	dx,offset sh_k801
	cmp	ifart[bx],2
	je	mn1
	mov	dx,offset sh_k211
	mov	al,ifbase[bx]
	and	al,0F0h
	cmp	al,70h
	je	mn1
	mov	dx,offset sh_k212
mn1:	call	textd
	mov	al,' '
	call	zeichen
	mov	al,ifbase[bx]
	call	hex2
	mov	al,'h'
	call	zeichen
	pop	dx
	pop	ax
	ret
modname	endp

hilfe	db	'Befehle (abkuerzbar):',cr,lf
	db	'  CONNECT            Terminal (Ctrl-CLR: Befehl, C zurueck)',cr,lf
	db	'  SET PORT n         serielles Modul n (1, 2, ...)',cr,lf
	db	'  SET SPEED n        Baudrate (300 ... 19200), auch SET BAUD n',cr,lf
	db	'  SET PARITY p       NONE, EVEN, ODD, MARK, SPACE',cr,lf
	db	'  SET FLOW f         NONE, XON/XOFF',cr,lf
	db	'  SET XMODEM 1K|128  Blockgroesse beim Senden mit XMODEM',cr,lf
	db	'  SHOW, HANGUP       Einstellungen zeigen, auflegen',cr,lf
	db	'  SEND datei         Datei(en) senden, * und ? erlaubt',cr,lf
	db	'  RECEIVE            Dateien empfangen',cr,lf
	db	'  GET datei          Datei(en) von einem Kermit-Server holen',cr,lf
	db	'  SERVER             als Kermit-Server bedienen',cr,lf
	db	'  FINISH, BYE        Server beenden / beenden und abmelden',cr,lf
	db	'  REMOTE DIR, CD, DELETE, TYPE, SPACE, HELP, HOST  an den Server',cr,lf
	db	'  XSEND, XRECEIVE datei  mit XMODEM senden, empfangen',cr,lf
	db	'  STATISTICS         Werte der letzten Uebertragung',cr,lf
	db	'  EXIT               beenden',cr,lf
	db	'-- weiter mit einer Taste --$'
hilfe2	db	30 dup (' '),cr
	db	'  TRANSMIT datei     Textdatei wie getippt senden',cr,lf
	db	'  LOG SESSION [datei] Terminal mitschreiben, CLOSE beendet',cr,lf
	db	'  DIR [muster], CD [pfad], DEL datei, TYPE datei',cr,lf
	db	'  PUSH, RUN befehl   DOS aufrufen (EXIT kehrt zurueck)',cr,lf
	db	'  TAKE datei         Befehle aus einer Datei (DMVKERM.INI beim Start)',cr,lf
	db	'Skripte (vor allem in TAKE-Dateien):',cr,lf
	db	'  OUTPUT text        senden (\13 = CR, \nnn dezimal)',cr,lf
	db	'  INPUT n text       bis n Sekunden auf text warten',cr,lf
	db	'  PAUSE [n], CLEAR, ECHO text, STOP',cr,lf
	db	'  IF SUCCESS|FAILURE befehl, GOTO marke (Zeile :marke)',cr,lf,'$'

b_help	proc	near
	mov	dx,offset hilfe
	call	textd
	call	kbwarte
	mov	al,cr			; Hinweis ueberschreiben
	call	zeichen
	mov	dx,offset hilfe2
	jmp	textd
b_help	endp

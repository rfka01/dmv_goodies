; seri.inc - serielle Module der DMV: 2651 (K211/K212/K213), 2661 (K801),
;            K215 (2651 hinter einem 8741 mit 2 KB Empfangspuffer)
;
; Register relativ zur Basis der IFSEL-Haelfte (A2 ist die R/W-Leitung):
;	lesen:     +0 Daten  +1 Status  +2 Mode (MR1/MR2)  +3 Befehl
;	schreiben: +4 Daten  +5 SYN/DLE +6 Mode (MR1/MR2)  +7 Befehl
; Status:  01 TxRDY  02 RxRDY  04 TxEMT  08 PE  10 Ueberlauf  20 FE
;	   40 DCD  80 DSR
; Befehl:  01 TxEN  02 DTR  04 RxEN  08 Break  10 Fehler loeschen  20 RTS
;
; Interrupt. Ab Werk legen K211/K212 (Schaltbild 017-0031865) und K801
; RxRDY oder TxRDY auf die IRQ-Leitung des Steckplatzes (P1-a2); die K235
; macht daraus IRQ3/4 (Steckplatz 3/4) oder IRQ5/7 (2/2a, 5/6, je nach
; DIP-Schalter). Nach Abschnitt 3.2 des NCR-RS-232-Handbuchs umgebaut
; (TxRDY aufgetrennt, INT/ auf c31) meldet die Karte nur RxRDY, auch auf
; IRQ2. Der 8259 der DMV ist FLANKENgesteuert: solange TxRDY die Leitung
; haelt, erzeugt ein ankommendes Zeichen keine Flanke. Darum ist der
; Sender nur eingeschaltet, solange ein Zeichen uebergeben wird, und jeder
; Empfangsweg sieht vor dem Aufhoeren noch einmal nach.
; Ohne Interruptcontroller (K230/K231) wird abgefragt; der 2651 haelt nur
; zwei Zeichen, Empfang dann sicher bis 2400 Baud.

SR	equ	1
MRR	equ	2
CRR	equ	3
THR	equ	4
MRW	equ	6
CRW	equ	7
PIC0	equ	90h
PIC1	equ	91h
RXGR	equ	8192		; Empfangspuffer (Zweierpotenz)
RXHOCH	equ	RXGR/2		; ab hier XOFF senden (viel Luft fuer Nachzuegler)
RXTIEF	equ	RXGR/8		; ab hier wieder XON

iforder	db	70h, 78h, 60h, 68h, 30h, 38h, 0B0h, 0B8h
ifbase	db	4 dup (0)	; gefundene Module: Basis
ifart	db	4 dup (0)	;  1 = 2651, 2 = 2661, 3 = K215
ifirq	db	4 dup (0FEh)	;  IRQ, FE = noch nicht gesucht, FF = keiner
ifn	db	0

portnr	db	0		; 1.. aktuelles Modul, 0 = keins
pbase	dw	0
pirq	db	0FFh
partk	db	0		; 1 = aktuelles Modul ist eine K215
pbefehl	db	26h		; Befehlsregister ohne TxEN: RTS, RxEN, DTR
palt	db	0		; Befehlsregister vor uns (DOS-AUX)
pmr1	db	4Eh		; MR1: asynchron 16x, 8 Bit, keine Paritaet, 1 Stopp
pbaud	db	0Eh		; MR2 unten: Baudcode, 0Eh = 9600
pparit	db	0		; 0 keine, 1 gerade, 2 ungerade, 3 mark, 4 space
pfluss	db	1		; 1 = XON/XOFF
paltvek	dd	0
paltmask db	0
poffen	db	0

	.data?
rxpuf	db	RXGR dup (?)
	.code
rxrein	dw	0
rxraus	dw	0
rxanz	dw	0
rxverl	dw	0		; verlorene Zeichen (Ueberlauf)
xoffges	db	0		; wir haben XOFF gesendet
xoffemp	db	0		; Gegenseite hat XOFF gesendet
inleer	db	0		; Sperre gegen Wiedereintritt

; --- Module suchen ----------------------------------------------------------
; Wie DR-DOS-ROS und der Startbildschirm des DOS-3.3-Ports: nur x0..x3
; lesen (x4..x7 waeren Schreibzugriffe). Die K211-Familie antwortet in
; beiden Haelften ihrer IFSEL-Gruppe; die B-Haelfte zaehlt darum nur, wenn
; die A-Haelfte keinen 2651 hatte. 1A zuerst: NCRs DOS hat AUX dort.
ifprobe	proc	near
	push	ax
	push	bx
	push	dx
	push	si
	mov	ifn,0
	xor	si,si
ifp1:	cmp	si,8
	jae	ifp9
	mov	al,iforder[si]
	inc	si
	xor	ah,ah
	mov	dx,ax
	call	ifeins
	or	al,al
	jz	ifp1
	test	dl,08h
	jz	ifp2
	cmp	dl,80h
	jae	ifp2
	cmp	dl,60h
	jb	ifp2
	push	dx
	and	dl,0F0h
	call	ifhat
	pop	dx
	jc	ifp1
ifp2:	mov	bl,ifn
	cmp	bl,4
	jae	ifp9
	xor	bh,bh
	mov	ifbase[bx],dl
	mov	ifart[bx],al
	inc	ifn
	jmp	short ifp1
ifp9:	pop	si
	pop	dx
	pop	bx
	pop	ax
	ret
ifprobe	endp

ifhat	proc	near			; DL schon gefunden? Carry = ja
	push	bx
	xor	bx,bx
ifh1:	cmp	bl,ifn
	jae	ifh8
	cmp	ifbase[bx],dl
	je	ifh9
	inc	bx
	jmp	short ifh1
ifh8:	clc
	pop	bx
	ret
ifh9:	stc
	pop	bx
	ret
ifhat	endp

; Basis DX pruefen: AL = 1 fuer 2651, 2 fuer 2661, 3 fuer K215, 0 fuer nichts
ifeins	proc	near
	push	bx
	push	cx
	push	dx
	call	ifk215			; zuerst die K215 (schreibt nur, wenn
	jnc	ife0			;  der 8741 wie einer aussieht)
	mov	al,3
	jmp	ife9
ife0:	mov	bx,dx
	mov	cx,4
	mov	ah,0FFh
ife1:	in	al,dx
	and	ah,al
	inc	dx
	loop	ife1
	cmp	ah,0FFh
	je	ife8
	mov	dx,bx
	add	dx,3			; Befehlsregister: Bit 7 ist 0
	in	al,dx
	test	al,80h
	jnz	ife8
	add	dx,3			; x6: Moderegister schreiben
	mov	al,4Eh
	out	dx,al
	mov	al,7Eh			; Bit 6 haelt nur der 2661
	out	dx,al
	sub	dx,3
	in	al,dx			; x3 lesen setzt den MR-Zeiger zurueck
	dec	dx
	in	al,dx
	mov	ah,al
	in	al,dx
	cmp	ah,4Eh
	jne	ife8
	mov	cl,2
	cmp	al,7Eh
	je	ife2
	mov	cl,1
	cmp	al,3Eh
	jne	ife8
ife2:	inc	dx			; 8N1 9600, Sender und Empfaenger aus
	in	al,dx
	add	dx,3
	mov	al,4Eh
	out	dx,al
	mov	al,3Eh
	out	dx,al
	mov	al,cl
	jmp	short ife9
ife8:	xor	al,al
ife9:	pop	dx
	pop	cx
	pop	bx
	ret
ifeins	endp

; K215 an Basis DX? Carry = ja. Vorgehen wie NCRs Diagnose V1K215:
; Status des 8741 (x1, gespiegelt auf x3) hat die oberen vier Bits 0, und
; der Echo-Befehl 0Bh gibt 55h und AAh zurueck. Auf einer K211 ist ein
; OUT auf x0/x1 nur ein Lesezyklus des 2651. Nicht auf 30h (Maus K806,
; auch ein 8741).
ifk215	proc	near
	push	ax
	push	cx
	push	dx
	cmp	dl,30h
	je	ifk_n
	mov	pbase,dx
	inc	dx			; x1 und x3: oberes Halbbyte 0 (F0/F1
	in	al,dx			;  darf der 8741 dabei aendern)
	test	al,0F0h
	jnz	ifk_n
	add	dx,2
	in	al,dx
	test	al,0F0h
	jnz	ifk_n
	test	al,01h			; liegt noch ein Byte bereit: abholen
	jz	ifk1
	sub	dx,3
	in	al,dx
ifk1:	mov	al,55h
	call	kecho
	jc	ifk_n
	mov	al,0AAh
	call	kecho
	jc	ifk_n
	pop	dx
	pop	cx
	pop	ax
	stc
	ret
ifk_n:	pop	dx
	pop	cx
	pop	ax
	clc
	ret
ifk215	endp

; Echo AL ueber den 8741 an pbase; Carry, wenn es nicht zurueckkommt
kecho	proc	near
	push	bx
	mov	bl,al
	mov	al,0Bh
	call	kcmd
	jc	kec9
	mov	al,bl
	call	kdat
	jc	kec9
	call	kles
	jc	kec9
	cmp	al,bl
	je	kec9			; gleich: Carry ist 0
	stc
kec9:	pop	bx
	ret
kecho	endp

; --- K215: Zugriff auf den 8741 ---------------------------------------------
; Status an x1: Bit 0 OBF (Byte zum Abholen), Bit 1 IBF (8741 hat das
; letzte Byte noch nicht genommen). Alle mit Zeitgrenze, Carry bei Ablauf.
kwarte	proc	near			; bis IBF = 0
	push	cx
	push	dx
	mov	dx,pbase
	inc	dx
	xor	cx,cx
kw1:	in	al,dx
	test	al,02h
	jz	kw9
	loop	kw1
	stc
kw9:	pop	dx
	pop	cx
	ret
kwarte	endp

; Ein Befehl wird auch geschrieben, wenn IBF nicht frei wird: steckt ein
; einzelnes Datenbyte ohne Befehl im 8741 (F1 = 0), liest die Firmware es
; nie ab (Hauptschleife 03Dh), und nur ein Befehl loest die Sperre.
kcmd	proc	near			; Befehl AL
	push	dx
	push	ax
	call	kwarte
	pop	ax
	mov	dx,pbase
	inc	dx
	out	dx,al
	clc
	pop	dx
	ret
kcmd	endp

kdat	proc	near			; Datenbyte AL
	push	dx
	push	ax
	call	kwarte
	pop	ax
	jc	kda9
	mov	dx,pbase
	out	dx,al
	clc
kda9:	pop	dx
	ret
kdat	endp

kles	proc	near			; Ergebnis -> AL
	push	cx
	push	dx
	mov	dx,pbase
	inc	dx
	xor	cx,cx
kl1:	in	al,dx
	test	al,01h
	jnz	kl2
	loop	kl1
	stc
	jmp	short kl9
kl2:	dec	dx
	in	al,dx
	clc
kl9:	pop	dx
	pop	cx
	ret
kles	endp

kfrag	proc	near			; Befehl AL, Ergebnis -> AL
	call	kcmd
	jc	kfr9
	call	kles
kfr9:	ret
kfrag	endp

kbefr	proc	near			; Befehlsregister des 2651 := AL
	push	ax
	mov	al,00h
	call	kcmd
	pop	ax
	jc	kbr9
	call	kdat
kbr9:	ret
kbefr	endp

kstat	proc	near			; Status -> AL (Bit 1: Puffer nicht leer)
	mov	al,03h
	call	kfrag
	jnc	kst9
	xor	al,al			; keine Antwort: nichts da, nicht bereit
kst9:	ret
kstat	endp

; --- Modul waehlen ----------------------------------------------------------
; AL = 1..ifn. Schliesst das alte, sucht beim ersten Mal den IRQ, oeffnet.
; Carry, wenn es das Modul nicht gibt.
portwahl proc	near
	push	ax
	push	bx
	or	al,al
	jz	pw8
	cmp	al,ifn
	ja	pw8
	push	ax
	call	serrst
	pop	ax
	mov	portnr,al
	mov	bl,al
	dec	bl
	xor	bh,bh
	mov	al,ifbase[bx]
	xor	ah,ah
	mov	pbase,ax
	mov	partk,0
	cmp	ifart[bx],3		; K215: gepuffert, wird abgefragt
	jne	pw1
	mov	partk,1
	mov	ifirq[bx],0FFh
pw1:	mov	al,ifirq[bx]
	cmp	al,0FEh
	jne	pw2
	call	irqsuche
	mov	ifirq[bx],al
pw2:	mov	pirq,al
	call	serini
	pop	bx
	pop	ax
	clc
	ret
pw8:	pop	bx
	pop	ax
	stc
	ret
portwahl endp

; --- IRQ suchen -------------------------------------------------------------
; Erst ueber TxRDY (Sender ein, Halteregister leer: TxRDY steigt), dann,
; falls nichts kam, ueber RxRDY im lokalen Loopback (ein Zeichen an sich
; selbst; der 2651 haelt dabei TxD auf Mark, die Leitung bleibt ruhig).
; Je zweimal, beide Male muss dasselbe herauskommen. AL = IRQ oder FFh.
tvek	dd	5 dup (0)
tirq	db	2, 3, 4, 5, 7
treffer	db	0
treff2	db	0
tloop	db	0

irqsuche proc	near
	push	bx
	push	cx
	push	dx
	push	si
	push	es
	in	al,PIC1			; gibt es einen 8259? (K230/K231: nein)
	mov	ah,al
	mov	al,0AAh
	out	PIC1,al
	in	al,PIC1
	mov	bl,al
	mov	al,55h
	out	PIC1,al
	in	al,PIC1
	mov	bh,al
	mov	al,ah
	out	PIC1,al
	cmp	bx,55AAh
	je	irs1
	mov	al,0FFh
	jmp	irs9
irs1:	mov	dx,pbase
	add	dx,CRW
	xor	al,al
	out	dx,al
	cli
	xor	si,si			; Testvektoren fuer IRQ 2,3,4,5,7
irs2:	mov	bl,tirq[si]
	xor	bh,bh
	add	bl,8
	shl	bx,1
	shl	bx,1
	push	si
	shl	si,1
	shl	si,1
	xor	ax,ax
	mov	es,ax
	mov	ax,es:[bx]
	mov	word ptr tvek[si],ax
	mov	ax,es:[bx+2]
	mov	word ptr tvek[si+2],ax
	pop	si
	mov	ax,si
	mov	cl,4			; Testroutinen liegen 16 Byte auseinander
	shl	ax,cl
	add	ax,offset tisr
	mov	es:[bx],ax
	mov	es:[bx+2],cs
	inc	si
	cmp	si,5
	jb	irs2
	in	al,PIC1			; IRQ 2,3,4,5,7 freigeben
	push	ax
	and	al,not 10111100b
	out	PIC1,al
	sti
	mov	tloop,0
	call	irs2mal
	cmp	al,0FFh
	jne	irs3
	mov	tloop,1
	call	irs2mal
irs3:	mov	cl,al
	cli
	pop	ax
	out	PIC1,al			; Maske wie vorher
	xor	si,si			; Vektoren zurueck
irs4:	mov	bl,tirq[si]
	xor	bh,bh
	add	bl,8
	shl	bx,1
	shl	bx,1
	push	si
	shl	si,1
	shl	si,1
	xor	ax,ax
	mov	es,ax
	mov	ax,word ptr tvek[si]
	mov	es:[bx],ax
	mov	ax,word ptr tvek[si+2]
	mov	es:[bx+2],ax
	pop	si
	inc	si
	cmp	si,5
	jb	irs4
	sti
	mov	al,cl
irs9:	pop	es
	pop	si
	pop	dx
	pop	cx
	pop	bx
	ret
irqsuche endp

irs2mal	proc	near
	push	cx
	call	irseins
	mov	cl,al
	call	irseins
	cmp	al,cl
	je	irs2m9
	mov	al,0FFh
irs2m9:	pop	cx
	ret
irs2mal	endp

irseins	proc	near			; ein Versuch, bis 50 ms
	push	cx
	push	dx
	mov	treffer,0FFh
	mov	treff2,0FFh
	mov	dx,pbase
	cmp	tloop,0
	je	ire0
	add	dx,CRR
	in	al,dx			; MR-Zeiger auf MR1
	add	dx,MRW-CRR
	mov	al,4Eh
	out	dx,al
	mov	al,3Eh			; 9600 Baud
	out	dx,al
	mov	dx,pbase
	in	al,dx			; Empfaenger leeren
	in	al,dx
	add	dx,CRW
	mov	al,87h			; Loopback, RxEN, DTR, TxEN
	out	dx,al
	sub	dx,CRW-THR
	mov	al,55h
	out	dx,al
	sub	dx,THR
	add	dx,CRW
	jmp	short ire1
ire0:	add	dx,CRW
	mov	al,01h			; nur TxEN: TxRDY steigt
	out	dx,al
ire1:	mov	cx,50
ire2:	cmp	treffer,0FFh
	jne	ire3
	mov	ax,1
	call	wartems
	loop	ire2
ire3:	mov	ax,2			; ein spaetes IRQ2 auch noch
	call	wartems
	xor	al,al
	out	dx,al			; Sender aus
	sub	dx,CRW			; Empfaenger leer
	in	al,dx
	in	al,dx
	mov	al,treffer
	cmp	al,0FFh
	jne	ire9
	mov	al,treff2
ire9:	pop	dx
	pop	cx
	ret
irseins	endp

; Testroutinen, je 16 Byte: AL = IRQ, dann der gemeinsame Teil
	even
tisr	label	near
	push	ax
	mov	al,2
	jmp	near ptr tisr9
	db	10 dup (90h)
	push	ax
	mov	al,3
	jmp	near ptr tisr9
	db	10 dup (90h)
	push	ax
	mov	al,4
	jmp	near ptr tisr9
	db	10 dup (90h)
	push	ax
	mov	al,5
	jmp	near ptr tisr9
	db	10 dup (90h)
	push	ax
	mov	al,7
	jmp	near ptr tisr9
	db	10 dup (90h)
tisr9:	push	dx
	mov	ah,al
	mov	dx,cs:pbase		; unser Modul: TxRDY oder RxRDY?
	inc	dx
	in	al,dx
	test	al,03h
	jz	tisr8
	cmp	ah,2
	jne	tisr7
	cmp	cs:treff2,0FFh
	jne	tisr8
	mov	cs:treff2,ah
	jmp	short tisr8
tisr7:	cmp	cs:treffer,0FFh
	jne	tisr8
	mov	cs:treffer,ah
	dec	dx			; Leitung beruhigen: Empfaenger leer,
	in	al,dx			;  Sender aus
	add	dx,CRW
	xor	al,al
	out	dx,al
tisr8:	mov	al,20h
	out	PIC0,al
	pop	dx
	pop	ax
	iret

; --- Oeffnen und schliessen -------------------------------------------------
serini	proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	cmp	partk,0
	je	sin0
	mov	al,04h			; K215: Befehlsregister merken (setzt
	call	kfrag			;  auch den MR-Zeiger zurueck)
	mov	palt,al
	call	setmode
	mov	rxrein,0
	mov	rxraus,0
	mov	rxanz,0
	mov	xoffges,0
	mov	xoffemp,0
	mov	al,pbefehl		; Sender bleibt an: es gibt keinen IRQ
	or	al,01h
	call	kbefr
	mov	cx,2100			; alten Inhalt des Kartenpuffers weg
sin0k:	call	kstat
	test	al,02h
	jz	sin0e
	mov	al,08h
	call	kfrag
	loop	sin0k
sin0e:	mov	poffen,1
	jmp	sin9
sin0:	mov	dx,pbase
	add	dx,CRR
	in	al,dx			; setzt auch den MR-Zeiger zurueck
	mov	palt,al
	call	setmode
	mov	dx,pbase		; Empfaenger leeren
	in	al,dx
	in	al,dx
	mov	rxrein,0
	mov	rxraus,0
	mov	rxanz,0
	mov	xoffges,0
	mov	xoffemp,0
	cmp	pirq,0FFh
	je	sin4
	mov	al,pirq
	add	al,8
	mov	ah,35h
	int	21h
	mov	word ptr paltvek,bx
	mov	word ptr paltvek+2,es
	mov	al,pirq
	add	al,8
	mov	ah,25h
	mov	dx,offset serisr
	int	21h
	cli
	mov	cl,pirq			; Maske lesen, aendern, schreiben
	mov	ah,1
	shl	ah,cl
	in	al,PIC1
	mov	paltmask,al
	and	paltmask,ah
	not	ah
	and	al,ah
	out	PIC1,al
	sti
sin4:	mov	al,pbefehl
	mov	dx,pbase
	add	dx,CRW
	out	dx,al
	mov	poffen,1
sin9:	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
serini	endp

; MR1 und MR2 schreiben (pmr1, pbaud)
setmode	proc	near
	push	ax
	push	dx
	cmp	partk,0
	je	smo0
	mov	al,04h			; K215: MR-Zeiger zuruecksetzen
	call	kfrag
	mov	al,01h			; MR1
	call	kcmd
	mov	al,pmr1
	call	kdat
	mov	al,01h			; MR2
	call	kcmd
	mov	al,pbaud
	or	al,30h
	call	kdat
	jmp	short smo9
smo0:	mov	dx,pbase
	add	dx,CRR
	in	al,dx
	add	dx,MRW-CRR
	mov	al,pmr1
	out	dx,al
	mov	al,pbaud
	or	al,30h			; interne Takte
	out	dx,al
smo9:	pop	dx
	pop	ax
	ret
setmode	endp

; Paritaet pparit -> pmr1
setparit proc	near
	push	ax
	mov	al,4Eh			; 8 Bit ohne Paritaet (auch mark/space)
	cmp	pparit,1
	jne	spa1
	mov	al,3Ah			; 7 Bit, Paritaet gerade
spa1:	cmp	pparit,2
	jne	spa2
	mov	al,1Ah			; 7 Bit, Paritaet ungerade
spa2:	mov	pmr1,al
	cmp	poffen,0
	je	spa9
	call	setmode
spa9:	pop	ax
	ret
setparit endp

serrst	proc	near
	cmp	poffen,0
	jne	srs0
	ret
srs0:	push	ax
	push	cx
	push	dx
	cmp	partk,0
	je	srs0a
	mov	cx,2000			; K215: Sender leer, dann altes
srsk1:	call	kstat			;  Befehlsregister zurueck
	test	al,04h
	jnz	srsk2
	mov	ax,1
	call	wartems
	loop	srsk1
srsk2:	mov	al,palt
	call	kbefr
	mov	poffen,0
	jmp	short srs9
srs0a:	mov	dx,pbase
	inc	dx
	mov	cx,2000
srs1:	in	al,dx			; Sender leer (TxEMT), hoechstens ~2 s
	test	al,04h
	jnz	srs2
	mov	ax,1
	call	wartems
	loop	srs1
srs2:	cmp	pirq,0FFh
	je	srs3
	cli
	in	al,PIC1			; unser Maskenbit wie vorher
	mov	cl,pirq
	mov	ah,1
	shl	ah,cl
	not	ah
	and	al,ah
	or	al,paltmask
	out	PIC1,al
	push	ds
	mov	al,pirq
	add	al,8
	mov	ah,25h
	lds	dx,paltvek
	int	21h
	pop	ds
	sti
srs3:	mov	dx,pbase
	add	dx,CRW
	mov	al,palt
	out	dx,al
	mov	poffen,0
srs9:	pop	dx
	pop	cx
	pop	ax
	ret
serrst	endp

; --- Empfang ----------------------------------------------------------------
serisr	proc	far
	push	ax
	push	bx
	push	dx
	push	ds
	mov	ax,cs
	mov	ds,ax
	call	rxleer
	mov	al,20h
	out	PIC0,al
	pop	ds
	pop	dx
	pop	bx
	pop	ax
	iret
serisr	endp

; Alles, was der Empfaenger hat, in den Puffer. Aus dem Interrupt und beim
; Abfragen gerufen. Verlassen erst, wenn RxRDY wirklich unten ist.
rxleer	proc	near
	cmp	inleer,0
	je	rxl0
	ret
rxl0:	mov	inleer,1
	pushf
	push	ax
	push	bx
	push	dx
rxl1:	cmp	partk,0
	je	rxl1a
	call	kstat			; K215: Puffer der Karte leeren
	test	al,02h
	jz	rxl8
	mov	ah,al
	mov	al,08h
	call	kfrag
	test	ah,10h			; Ueberlauf im 2651
	jz	rxl2
	push	ax
	mov	al,pbefehl
	or	al,11h			; Fehler loeschen, Sender bleibt an
	call	kbefr
	inc	rxverl
	pop	ax
	jmp	short rxl2
rxl1a:	cli
	mov	dx,pbase
	inc	dx
	in	al,dx
	test	al,02h
	jz	rxl8
	mov	ah,al
	dec	dx
	in	al,dx
	test	ah,10h			; Ueberlauf: Fehler loeschen, zaehlen
	jz	rxl2
	push	ax
	add	dx,CRW
	mov	al,pbefehl
	or	al,10h
	out	dx,al
	inc	rxverl
	pop	ax
rxl2:	cmp	pfluss,0
	je	rxl4
	mov	ah,al
	and	ah,7Fh
	cmp	ah,xoff
	jne	rxl3
	mov	xoffemp,1
	jmp	short rxl1
rxl3:	cmp	ah,xon
	jne	rxl4
	mov	xoffemp,0
	jmp	short rxl1
rxl4:	cmp	rxanz,RXGR
	jb	rxl4a
	inc	rxverl			; voll: verwerfen und zaehlen
	jmp	rxl1
rxl4a:
	mov	bx,rxrein
	mov	rxpuf[bx],al
	inc	bx
	and	bx,RXGR-1
	mov	rxrein,bx
	inc	rxanz
	jmp	rxl1
rxl8:	sti
	cmp	pfluss,0		; Puffer fast voll: XOFF
	je	rxl9
	cmp	xoffges,0
	jne	rxl9
	cmp	rxanz,RXHOCH
	jb	rxl9
	mov	xoffges,1
	mov	al,xoff
	call	sputroh
rxl9:	cli
	cmp	partk,0			; K215: oben schon bis leer gelesen
	jne	rxl9a
	mov	dx,pbase
	inc	dx
	in	al,dx
	test	al,02h
	jz	rxl9a
	jmp	rxl1
rxl9a:	mov	inleer,0
	pop	dx
	pop	bx
	pop	ax
	popf
	ret
rxleer	endp

; Zeichen holen: ZF = 1 keins, sonst AL
sget	proc	near
	push	bx
	call	rxleer			; auch bei IRQ: verpasste Flanke
	cli
	cmp	rxanz,0
	je	sget8
	mov	bx,rxraus
	mov	al,rxpuf[bx]
	inc	bx
	and	bx,RXGR-1
	mov	rxraus,bx
	dec	rxanz
	sti
	cmp	xoffges,0		; wieder Platz: XON
	je	sget7
	cmp	rxanz,RXTIEF
	ja	sget7
	push	ax
	mov	xoffges,0
	mov	al,xon
	call	sputroh
	pop	ax
sget7:	or	bl,1			; ZF = 0 (BL ist nie 0 nach OR 1)
	pop	bx
	ret
sget8:	sti
	xor	bl,bl			; ZF = 1
	pop	bx
	ret
sget	endp

; --- Senden -----------------------------------------------------------------
; Zeichen AL senden, mit Paritaet mark/space in Software und Warten auf
; XON. Carry bei Zeitueberschreitung (kein CTS, Modul weg).
sput	proc	near
	push	ax
	push	cx
	cmp	pparit,3
	jne	sp1
	or	al,80h
sp1:	cmp	pparit,4
	jne	sp2
	and	al,7Fh
sp2:	mov	cx,3000			; auf XON hoechstens ~3 s warten
sp3:	cmp	xoffemp,0
	je	sp4
	call	rxleer
	push	ax
	mov	ax,1
	call	wartems
	pop	ax
	loop	sp3
	mov	xoffemp,0		; dann eben weiter
sp4:	call	sputroh
	pop	cx
	pop	ax
	ret
sput	endp

; AL senden: Sender ein, auf TxRDY warten, schreiben, warten bis der
; Baustein es uebernommen hat, Sender aus.
sputroh	proc	near
	push	ax
	push	cx
	push	dx
	cmp	partk,0
	je	spr0
	mov	ah,al			; K215: auf TxRDY warten, dann Befehl 09h
	mov	cx,20000
sprk1:	call	kstat
	test	al,01h
	jnz	sprk2
	loop	sprk1
	jmp	short sprk8
sprk2:	mov	al,09h
	call	kcmd
	jc	sprk8
	mov	al,ah
	call	kdat
	jc	sprk8
	pop	dx
	pop	cx
	pop	ax
	clc
	ret
sprk8:	pop	dx
	pop	cx
	pop	ax
	stc
	ret
spr0:	mov	ah,al
	mov	dx,pbase
	add	dx,CRW
	mov	al,pbefehl
	or	al,21h			; TxEN (und RTS)
	out	dx,al
	sub	dx,CRW-SR
	xor	cx,cx
spr1:	in	al,dx
	test	al,01h
	jnz	spr2
	loop	spr1
	jmp	short spr8
spr2:	add	dx,THR-SR
	mov	al,ah
	out	dx,al
	sub	dx,THR-SR
	xor	cx,cx
spr3:	in	al,dx
	test	al,01h
	jnz	spr4
	loop	spr3
	jmp	short spr8
spr4:	add	dx,CRW-SR
	mov	al,pbefehl
	out	dx,al
	cmp	inleer,0		; kam derweil ein Zeichen, gleich holen
	jne	spr5
	sub	dx,CRW-SR
	in	al,dx
	test	al,02h
	jz	spr5
	call	rxleer
spr5:	pop	dx
	pop	cx
	pop	ax
	clc
	ret
spr8:	add	dx,CRW-SR
	mov	al,pbefehl
	out	dx,al
	pop	dx
	pop	cx
	pop	ax
	stc
	ret
sputroh	endp

; Break (Leitung 250 ms auf Space)
sbreak	proc	near
	push	ax
	push	dx
	cmp	partk,0
	je	sbr0
	mov	al,pbefehl		; K215
	or	al,09h
	call	kbefr
	mov	ax,250
	call	wartems
	mov	al,pbefehl
	or	al,01h
	call	kbefr
	jmp	short sbr9
sbr0:	mov	dx,pbase
	add	dx,CRW
	mov	al,pbefehl
	or	al,09h
	out	dx,al
	mov	ax,250
	call	wartems
	mov	al,pbefehl
	out	dx,al
sbr9:	pop	dx
	pop	ax
	ret
sbreak	endp

; Auflegen: DTR und RTS fuer eine Sekunde weg
shangup	proc	near
	push	ax
	push	dx
	cmp	partk,0
	je	sha0
	mov	al,05h			; K215: nur RxEN und TxEN
	call	kbefr
	mov	ax,1000
	call	wartems
	mov	al,pbefehl
	or	al,01h
	call	kbefr
	jmp	short sha9
sha0:	mov	dx,pbase
	add	dx,CRW
	mov	al,04h			; nur RxEN
	out	dx,al
	mov	ax,1000
	call	wartems
	mov	al,pbefehl
	out	dx,al
sha9:	pop	dx
	pop	ax
	ret
shangup	endp

; Status: AL = Statusregister
sstatus	proc	near
	cmp	partk,0
	je	sst0
	jmp	kstat
sst0:	push	dx
	mov	dx,pbase
	inc	dx
	in	al,dx
	pop	dx
	ret
sstatus	endp

; Baudraten: Wert (geteilt durch 10 bei 50/75/110/134) und 2651-Code
baudtab	dw	50, 75, 110, 134, 150, 300, 600, 1200, 1800, 2000
	dw	2400, 3600, 4800, 7200, 9600, 19200
baudn	equ	16

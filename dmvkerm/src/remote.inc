; remote.inc - REMOTE-Befehle (Client) und ihre Bedienung im Server
;
; Client: REMOTE DIR/CD/DELETE/TYPE/SPACE/HELP/HOST schickt ein G-Paket
; (HOST ein C-Paket). Kurze Antworten kommen als Daten der Bestaetigung,
; lange als Textuebertragung (S, X, D..., Z, B), die kempf auf den
; Bildschirm statt in eine Datei leitet.
; Server: dieselben Befehle (ohne HOST) fuer einen Client auf der Gegenseite.

MTGR	equ	4096
	.data?
mtext	db	MTGR dup (?)	; Antworttext des Servers
	.code
mlen	dw	0
rembef	db	0
garg	db	64 dup (0)	; Argument eines G-Befehls (Server)

rem_tab	db	9,'DIRECTORY'
	dw	'D'
	db	2,'CD'
	dw	'C'
	db	3,'CWD'
	dw	'C'
	db	6,'DELETE'
	dw	'E'
	db	4,'TYPE'
	dw	'T'
	db	5,'SPACE'
	dw	'U'
	db	4,'HELP'
	dw	'H'
	db	4,'HOST'
	dw	'K'
	db	0

; --- Client -----------------------------------------------------------------
b_remote proc	near
	mov	si,offset rem_tab
	call	wortaus
	jc	brm9
	mov	rembef,al
	call	kbereit
	jc	brm9
	call	restzeile		; Argument -> roh, CX Laenge
	mov	di,offset sdat
	mov	bl,'G'
	cmp	rembef,'K'		; HOST: C-Paket mit dem Befehl
	jne	brm1
	jcxz	brm_f
	mov	bl,'C'
	jmp	short brm3
brm1:	mov	al,rembef		; G: Buchstabe [Laenge Argument]
	push	cx
	mov	cl,1
	call	kodier
	pop	cx
	jcxz	brm4
	mov	al,cl
	add	al,' '
	push	cx
	mov	cl,1
	call	kodier
	pop	cx
brm3:	mov	si,offset roh		; Argument
brm3a:	lodsb
	or	al,al
	jz	brm4
	push	cx
	mov	cl,1
	call	kodier
	pop	cx
	jmp	short brm3a
brm4:	sub	di,offset sdat
	mov	sdlen,di
	mov	al,bl
	jmp	kfrage
brm_f:	mov	dx,offset m_fehlt
	call	textd
brm9:	ret
b_remote endp

; Paket Typ AL (Daten in sdat) an den Server, Antwort zeigen
kfrage	proc	near
	mov	bl,al
	mov	versuch,0
kfr1:	mov	al,bl
	call	spack
kfr2:	call	rpack
	mov	al,rtyp
	cmp	al,'Y'
	je	kfr_y
	cmp	al,'S'
	je	kfr_s
	cmp	al,'E'
	jne	kfr3
	jmp	zeigefehler
kfr3:	cmp	al,'a'
	jne	kfr4
	jmp	abbrechen
kfr4:	inc	versuch
	cmp	versuch,VERSUCHE
	jb	kfr1
	mov	dx,offset a_zeit
	jmp	meldung
kfr_y:	cmp	rlen,0			; kurze Antwort in der Bestaetigung
	je	kfr_y9
	mov	bx,offset zeig_z
	call	dekod
	call	crlf
kfr_y9:	ret
kfr_s:	mov	habS,1			; lange Antwort als Text
	call	kempf
	ret
kfrage	endp

; Rest der Zeile (Originalschreibung, mit Leerzeichen) -> roh; CX Laenge
restzeile proc	near
	mov	si,zpos
rz0:	mov	al,[si]
	cmp	al,' '
	je	rz0a
	cmp	al,tab
	jne	rz0b
rz0a:	inc	si
	jmp	short rz0
rz0b:	sub	si,offset zpuf+2
	add	si,offset zorig
	mov	di,offset roh
	xor	cx,cx
rz1:	lodsb
	or	al,al
	jz	rz2
	cmp	cx,63
	jae	rz2
	stosb
	inc	cx
	jmp	short rz1
rz2:	mov	byte ptr [di],0
	ret
restzeile endp

; --- Server -----------------------------------------------------------------
sv_hilfe db	'DMVKERM-Server. Moeglich sind:',cr,lf
	db	'  SEND, GET, FINISH, BYE',cr,lf
	db	'  REMOTE DIR [muster], REMOTE CD [pfad], REMOTE DELETE datei,',cr,lf
	db	'  REMOTE TYPE datei, REMOTE SPACE, REMOTE HELP',cr,lf,0
sv_weg	db	' geloescht',0
sv_frei	db	' Byte frei',0
sv_kdir	db	'Verzeichnis nicht gefunden',0
sv_kdel	db	'Nicht geloescht',0
sv_kdat	db	'Datei nicht gefunden',0
sv_dir	db	'DIR',0
sv_hlp	db	'HELP',0

; G-Paket in rdat bedienen (Buchstabe in AL, gross). Carry: unbekannt.
sgeneric proc	near
	push	ax
	mov	rohl,0			; Daten dekodieren
	mov	bx,offset roh_z
	call	dekod
	mov	garg,0			; erstes Feld = Argument
	cmp	rohl,2
	jb	sgn1
	mov	cl,roh+1
	sub	cl,' '
	xor	ch,ch
	mov	ax,rohl
	sub	ax,2
	cmp	cx,ax
	jbe	sgn0
	mov	cx,ax
sgn0:	mov	si,offset roh+2
	mov	di,offset garg
	rep	movsb
	mov	byte ptr [di],0
sgn1:	pop	ax
	mov	ah,rseq			; Antwort mit dieser Folgenummer
	mov	sseq,ah
	cmp	al,'D'
	jne	sgn2
	jmp	sg_dir
sgn2:	cmp	al,'C'
	jne	sgn3
	jmp	sg_cd
sgn3:	cmp	al,'E'
	jne	sgn4
	jmp	sg_del
sgn4:	cmp	al,'T'
	jne	sgn5
	jmp	sg_type
sgn5:	cmp	al,'U'
	jne	sgn6
	jmp	sg_space
sgn6:	cmp	al,'H'
	jne	sgn7
	jmp	sg_help
sgn7:	stc
	ret
sgeneric endp

; Kurze Antwort: Text DS:SI (0-Ende) als Daten einer Bestaetigung
sy_text	proc	near
	mov	di,offset sdat
sy1:	lodsb
	or	al,al
	jz	sy2
	mov	cl,1
	call	kodier
	jmp	short sy1
sy2:	sub	di,offset sdat
	mov	sdlen,di
	mov	al,'Y'
	call	spack
	clc
	ret
sy_text	endp

; Fehler DS:SI an den Client
sy_fehl	proc	near
	mov	al,'E'
	call	spackt
	clc
	ret
sy_fehl	endp

; Text aus mtext (mlen) als Textuebertragung senden; Kopf DS:SI
sy_mtext proc	near
	mov	fquelle,1
	mov	fpos,0
	mov	ax,mlen
	mov	fend,ax
	call	sy_lang
	mov	fquelle,0
	clc
	ret
sy_mtext endp

; Textuebertragung S, X (Kopf DS:SI), D..., Z, B aus fget
sy_lang	proc	near
	push	si
	mov	a_rich,offset a_sende
	mov	di,offset fname		; Kopf auch fuer die Anzeige
	mov	cx,12
syl0:	lodsb
	stosb
	or	al,al
	loopnz	syl0
	mov	byte ptr [di],0
	mov	sseq,0
	call	mparam
	mov	al,'S'
	call	sendewarte
	pop	si
	jc	syl9
	call	gparam
	mov	al,chkn
	sub	al,'0'
	mov	chkt,al
	mov	di,offset sdat		; X mit dem Kopf
syl1:	lodsb
	or	al,al
	jz	syl2
	mov	cl,1
	call	kodier
	jmp	short syl1
syl2:	sub	di,offset sdat
	mov	sdlen,di
	mov	al,'X'
	call	sendewarte
	jc	syl9
	mov	kbytes,0
	mov	kbytes+2,0
	mov	kgross,0
	mov	kgross+2,0
	push	si
	mov	si,offset b_laeuft
	call	zustand
	pop	si
	call	ksdaten
	jc	syl9
	mov	sdlen,0
	mov	al,'Z'
	call	sendewarte
	jc	syl9
	mov	al,'B'
	call	sendewarte
	jc	syl9
	mov	dx,offset a_ok
	call	meldung
syl9:	ret
sy_lang	endp

; Zeichen AL an mtext anhaengen (voll: verwerfen)
mput	proc	near
	push	bx
	mov	bx,mlen
	cmp	bx,MTGR
	jae	mpu9
	mov	mtext[bx],al
	inc	mlen
mpu9:	pop	bx
	ret
mput	endp

; Text DS:SI (0-Ende) an mtext
mtext0	proc	near
	push	si
mt1:	lodsb
	or	al,al
	jz	mt9
	call	mput
	jmp	short mt1
mt9:	pop	si
	ret
mtext0	endp

; DX:AX dezimal an mtext
mzahl	proc	near
	push	di
	mov	di,offset roh+40	; erst in einen Zwischenpuffer
	push	di
	call	zahlbuf
	mov	byte ptr [di],0
	pop	si
	call	mtext0
	pop	di
	ret
mzahl	endp

mcrlf	proc	near
	mov	al,cr
	call	mput
	mov	al,lf
	jmp	mput
mcrlf	endp

sg_dir:	mov	mlen,0
	cmp	garg,0
	jne	sgd0
	mov	si,offset d_alle
	mov	di,offset garg
	mov	cx,4
	rep	movsb
sgd0:	mov	dx,offset dta
	mov	ah,1Ah
	int	21h
	mov	dx,offset garg
	mov	cx,10h
	mov	ah,4Eh
	int	21h
	jc	sgd8
sgd1:	mov	si,offset dta+1Eh	; Name auf 14 Stellen
	call	mtext0
	xor	cx,cx
sgd2:	lodsb
	or	al,al
	jz	sgd3
	inc	cx
	jmp	short sgd2
sgd3:	cmp	cx,14
	jae	sgd4
	mov	al,' '
	call	mput
	inc	cx
	jmp	short sgd3
sgd4:	test	byte ptr dta+15h,10h
	jz	sgd5
	mov	si,offset d_dir+2	; '<DIR>'
	call	mtext0d
	jmp	short sgd6
sgd5:	mov	ax,word ptr dta+1Ah
	mov	dx,word ptr dta+1Ch
	call	mzahl
sgd6:	call	mcrlf
	mov	ah,4Fh
	int	21h
	jnc	sgd1
sgd8:	mov	ah,36h			; freier Platz
	mov	dl,0
	int	21h
	cmp	ax,0FFFFh
	je	sgd9
	mul	cx
	mul	bx
	call	mzahl
	mov	si,offset sv_frei
	call	mtext0
	call	mcrlf
sgd9:	mov	si,offset sv_dir
	jmp	sy_mtext

; $-Text DS:SI an mtext (fuer d_dir)
mtext0d	proc	near
mtd1:	lodsb
	cmp	al,'$'
	je	mtd9
	call	mput
	jmp	short mtd1
mtd9:	ret
mtext0d	endp

sg_cd:	cmp	garg,0
	je	sgc1
	mov	dx,offset garg
	mov	ah,3Bh
	int	21h
	jnc	sgc1
	mov	si,offset sv_kdir
	jmp	sy_fehl
sgc1:	mov	ah,19h			; Laufwerk:\Pfad zurueck
	int	21h
	add	al,'A'
	mov	roh,al
	mov	roh+1,':'
	mov	roh+2,'\'
	mov	si,offset roh+3
	mov	ah,47h
	mov	dl,0
	int	21h
	mov	si,offset roh
	jmp	sy_text

sg_del:	mov	dx,offset garg
	mov	ah,41h
	int	21h
	jnc	sge1
	mov	si,offset sv_kdel
	jmp	sy_fehl
sge1:	mov	si,offset garg		; "NAME geloescht"
	mov	di,offset roh
sge2:	lodsb
	or	al,al
	jz	sge3
	stosb
	jmp	short sge2
sge3:	mov	si,offset sv_weg
sge4:	lodsb
	stosb
	or	al,al
	jnz	sge4
	mov	si,offset roh
	jmp	sy_text

sg_type: mov	ax,3D00h
	mov	dx,offset garg
	int	21h
	jnc	sgt1
	mov	si,offset sv_kdat
	jmp	sy_fehl
sgt1:	mov	fhand,ax
	mov	fpos,0
	mov	fend,0
	mov	fquelle,0
	mov	si,offset garg
	call	sy_lang
	call	fzu
	clc
	ret

sg_space: mov	ah,36h
	mov	dl,0
	int	21h
	mul	cx
	mul	bx
	mov	di,offset roh
	call	zahlbuf
	mov	si,offset sv_frei
sgs1:	lodsb
	stosb
	or	al,al
	jnz	sgs1
	mov	si,offset roh
	jmp	sy_text

sg_help: mov	mlen,0
	mov	si,offset sv_hilfe
	call	mtext0
	mov	si,offset sv_hlp
	jmp	sy_mtext

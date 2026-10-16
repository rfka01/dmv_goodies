; DMVKERM - Kermit und Terminal fuer die NCR Decision Mate V
;
; Ein kleines Kermit fuer die seriellen Module K211/K212/K213 (2651) und
; K801 (2661) unter NCR MS-DOS 2.11, dem MS-DOS-3.30-Port und dem
; DR-DOS-3.41-Port. Terminal mit VT102-Teilmenge, Dateiuebertragung mit
; Kermit und XMODEM. Eine .COM-Datei, laeuft ab 64 KB.
;
; Aufbau (alles in einem Segment, CS = DS = ES = SS):
;   tabellen.inc  Zeichen- und Tastaturtabellen (tools/mktab.py)
;   zeit.inc      Wartezeiten und Uhr
;   tast.inc      8741-Tastatur direkt gelesen
;   seri.inc      Modulsuche, IRQ-Suche, 2651/2661-Treiber
;   kons.inc      Ausgabe auf die DOS-Konsole
;   befehl.inc    Befehlszeile
;   verbind.inc   CONNECT
;   kermit.inc    Kermit-Pakete
;   bild.inc      Bildschirm waehrend einer Uebertragung
;   transfer.inc  SEND, RECEIVE, GET, SERVER
;   remote.inc    REMOTE-Befehle (Client und Server)
;   kbef.inc      die Befehle dazu
;   xmodem.inc    XSEND, XRECEIVE
;   extras.inc    LOG, TRANSMIT, DIR, CD, DEL, TAKE
;   skript.inc    OUTPUT, INPUT, IF, GOTO ..., PUSH, RUN, TYPE, STATISTICS
;
; Bau: tools/build.sh (JWasm). Befehle englisch, Meldungen deutsch.
;
; (C) 2026 rfka01, MIT-Lizenz.

	.model	tiny
	.8086

cr	equ	0Dh
lf	equ	0Ah
bs	equ	08h
tab	equ	09h
bel	equ	07h
esc_	equ	1Bh
xon	equ	11h
xoff	equ	13h
FLUCHT	equ	1Dh		; Ctrl-CLR liefert Ctrl-] wie bei MS-Kermit

	.code
	org	100h
start:	jmp	anfang

	include	tabellen.inc
	include	zeit.inc
	include	tast.inc
	include	seri.inc
	include	kons.inc
	include	befehl.inc
	include	vt.inc
	include	verbind.inc
	include	kermit.inc
	include	bild.inc
	include	transfer.inc
	include	remote.inc
	include	kbef.inc
	include	xmodem.inc
	include	extras.inc
	include	skript.inc

; ---------------------------------------------------------------------------
titel	db	'DMVKERM 0.1 - Kermit fuer die NCR Decision Mate V',cr,lf
	db	'HELP zeigt die Befehle, EXIT beendet.',cr,lf,'$'

anfang:	cld
	mov	sp,offset stapel_ende
	mov	bx,offset stapel_ende	; Speicher auf das Noetige verkleinern
	add	bx,15			;  (fuer PUSH und RUN)
	mov	cl,4
	shr	bx,cl
	mov	ah,4Ah
	int	21h
	mov	ah,30h			; DOS-Version
	int	21h
	mov	dosver,al
	mov	dx,offset titel
	call	textd
	call	zeitini			; Warteschleife eichen
	call	kbinit			; Tastaturland
	call	vtini			; Konsolenart
	call	ifprobe			; serielle Module
	cmp	ifn,0
	jne	anf1
	mov	dx,offset m_keinmod
	call	textd
	jmp	anf2
anf1:	mov	al,1			; erstes Modul
	call	portwahl
anf2:	mov	dx,offset ini		; DMVKERM.INI, wenn vorhanden
	call	topen
	call	befehlsschleife		; kehrt bei EXIT zurueck
	call	lzu
	call	serrst
	mov	ax,4C00h
	int	21h

m_keinmod db	'Kein serielles Modul (K211, K212, K801) gefunden.',cr,lf,'$'
dosver	db	0

; Stapel und Puffer am Ende
	.data?
	even
		db	512 dup (?)
stapel_ende	label	word
	.code

	end	start

; File MSXDMVS.INC - serial ports of the NCR Decision Mate V for MSXIBM.ASM
;
; Included at the end of MSXIBM.ASM when assembling with -Ddmv. Drives the
; Signetics 2651 (K211 modem, K212 printer, K213 plotter: one board, three
; cables) and the 2661 (K801, switchable). MSXDMV.ASM finds the modules at
; start-up (dmvif, dmvifk, dmvifn); COM1 and COM2 are the first two found,
; half 1A first.
;
; Registers relative to the base of the IFSEL half (A2 is the R/W line):
;	read:  +0 data   +1 status   +2 mode (MR1/MR2)   +3 command
;	write: +4 data   +5 SYN/DLE  +6 mode (MR1/MR2)   +7 command
; Status:  01 TxRDY  02 RxRDY  04 TxEMT  08 PE  10 overrun  20 FE
;	   40 DCD  80 DSR
; Command: 01 TxEN  02 DTR  04 RxEN  08 force break  10 reset errors
;	   20 RTS
;
; Interrupts. RxRDY and TxRDY of the chip drive one line together; the
; slot routes it to an IRQ of the K235's 8259 (slot 3: IRQ3, slot 4: IRQ4,
; slot 2/2a: IRQ5, slot 5/6: IRQ7, after the K235 DIP switches). The DMV's
; 8259 is EDGE triggered (NCR's ICW1 17h). While the transmitter is enabled
; and its holding register empty, TxRDY holds the line active, and a
; character arriving then makes no edge: its interrupt is lost. Hence:
;   - the transmitter is enabled only to send a character and switched
;     off again as soon as the chip has taken it (it finishes the
;     character by itself);
;   - the IRQ is found out by enabling the transmitter once and seeing
;     which vector is hit;
;   - whenever Kermit finds its buffer empty, the status is polled as well
;     (dmvrcv), so a missed edge cannot block reception; without an IRQ
;     (K230/K231 have no interrupt controller) that polling is all there is.
; The 8259 runs with automatic EOI under NCR's set-up; a non-specific EOI
; is sent anyway, it does no harm.
;
; NCR Decision Mate V port 2026.

dmvSR	equ	1		; status register
dmvMR	equ	2		; mode registers, read
dmvCR	equ	3		; command register, read
dmvTHR	equ	4		; transmit holding register
dmvMRW	equ	6		; mode registers, write
dmvCRW	equ	7		; command register, write
PIC0	equ	90h		; 8259 of the K235
PIC1	equ	91h

data	segment
dmvcmd	db	26h		; command register without TxEN: RTS, RxEN, DTR
dmvsavcmd db	0		; command register before Kermit took the port
dmvmr1	db	4Eh		; MR1: async 16x, 8 bits, no parity, 1 stop bit
dmvirq	db	0FFh		; IRQ of the current port, FF = poll
dmvirqt	db	4 dup (0FEh)	; IRQ per module, FE = not looked for yet
dmvhit	db	0		; IRQ test: first IRQ seen
dmvhit2	db	0		;  IRQ2 seen
dmvsavvec dd	0		; vector before we took it
dmvoldmask db	0		; 8259 mask bit of our IRQ before (1 = masked)
dmvindrain db	0		; nonzero while draining the receiver
dmvpoll	db	0		; 1 = port open without IRQ: poll from MSXDMV too
dmvcal	db	0		; pcwait calibrated
dmvtvec	dd	5 dup (0)	; IRQ test: saved vectors 0Ah, 0Bh, 0Ch, 0Dh, 0Fh
dmvtirq	db	2, 3, 4, 5, 7	; IRQs tried
				; Kermit's speed index -> 2651 baud code
dmvbdtab db	0FFh, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0Ah, 0Ch, 0Eh, 0FFh
	db	0Fh, 0FFh, 0FFh, 0FFh, 0FFh, 0FFh
dmvnoif	db	cr,lf,'?No serial module (K211, K212, K801) for this port$'
dmvifmsg db	cr,lf,'  Module:             $'
dmvk211	db	'K211 (2651)$'
dmvk212	db	'K212 (2651)$'
dmvk801	db	'K801 (2661)$'
dmvpmsg	db	', port \x$'
dmvirqm	db	', IRQ $'
dmvpolm	db	', polled (no IRQ)$'
data	ends

code	segment
	assume	cs:code, ds:data, es:nothing

; SET PORT COM1/COM2 on the DMV: BL = 1 or 2. Sets the modem structure,
; finds the IRQ and reads the speed. Carry set if there is no such module.
dmvcomsel proc	near
	push	ax
	push	bx
	push	dx
	xor	bh,bh
	dec	bl			; module index 0..
	cmp	bl,dmvifn
	jb	dmvcse1
	mov	ah,prstr
	mov	dx,offset dmvnoif
	int	dos
	pop	dx
	pop	bx
	pop	ax
	stc
	ret
dmvcse1:mov	al,dmvif[bx]
	xor	ah,ah
	mov	modem.mddat,ax		; base: data register
	inc	ax
	mov	modem.mdstat,ax		; status
	add	ax,2
	mov	modem.mdcom,ax		; command register (read)
	mov	al,dmvirqt[bx]		; IRQ known already?
	cmp	al,0FEh
	jne	dmvcse2
	call	dmvfindirq		; AL = IRQ or FF
	mov	dmvirqt[bx],al
dmvcse2:mov	dmvirq,al
	cmp	al,0FFh
	je	dmvcse3
	add	al,8			; IRQ 0 is Int 8 on the DMV too
	xor	ah,ah
	mov	modem.mdintv,ax
dmvcse3:pop	dx
	pop	bx
	pop	ax
	call	getbaud
	clc
	ret
dmvcomsel endp

; Which IRQ does the module at modem.mddat raise? Returns AL = IRQ or
; FFh (no 8259 - K230/K231 - or no answer). Enables the transmitter with
; the holding register empty: TxRDY rises, and one of the candidate
; vectors is hit. Done twice; both must agree. An IRQ2 hit counts only if
; no slot IRQ answered (the K801 drives both lines, the K806 mouse is on
; IRQ2 as well).
dmvfindirq proc	near
	push	bx
	push	cx
	push	dx
	push	si
	push	es
	in	al,PIC1			; is there an 8259? (mask register is
	mov	ah,al			;  read/write; without a K235 the bus
	mov	al,0AAh			;  reads FFh)
	out	PIC1,al
	in	al,PIC1
	mov	bl,al
	mov	al,55h
	out	PIC1,al
	in	al,PIC1
	mov	bh,al
	mov	al,ah
	out	PIC1,al			; mask back
	cmp	bx,55AAh
	je	dmvfi1
	mov	al,0FFh			; no 8259: poll
	jmp	dmvfi9
dmvfi1:	mov	dx,modem.mddat		; transmitter and receiver off
	add	dx,dmvCRW
	xor	al,al
	out	dx,al
	cli
	xor	si,si			; our test vectors for IRQ 2,3,4,5,7
dmvfi2:	mov	bl,dmvtirq[si]
	xor	bh,bh
	add	bl,8
	shl	bx,1
	shl	bx,1			; vector address
	push	si
	shl	si,1
	shl	si,1
	xor	ax,ax
	mov	es,ax
	mov	ax,es:[bx]
	mov	word ptr dmvtvec[si],ax
	mov	ax,es:[bx+2]
	mov	word ptr dmvtvec[si+2],ax
	pop	si
	mov	ax,si
	mov	cl,4			; dmvtisr entries are 16 bytes apart
	shl	ax,cl
	add	ax,offset dmvtisr
	mov	es:[bx],ax
	mov	es:[bx+2],cs
	inc	si
	cmp	si,5
	jb	dmvfi2
	in	al,PIC1			; unmask IRQ 2,3,4,5,7
	push	ax
	and	al,not 10111100b
	out	PIC1,al
	sti
	call	dmvfione		; AL = IRQ or FF
	mov	cl,al
	call	dmvfione
	cmp	al,cl
	je	dmvfi3
	mov	al,0FFh			; disagree: poll
dmvfi3:	mov	cl,al
	cli
	pop	ax
	out	PIC1,al			; mask as it was
	xor	si,si			; vectors back
dmvfi4:	mov	bl,dmvtirq[si]
	xor	bh,bh
	add	bl,8
	shl	bx,1
	shl	bx,1
	push	si
	shl	si,1
	shl	si,1
	xor	ax,ax
	mov	es,ax
	mov	ax,word ptr dmvtvec[si]
	mov	es:[bx],ax
	mov	ax,word ptr dmvtvec[si+2]
	mov	es:[bx+2],ax
	pop	si
	inc	si
	cmp	si,5
	jb	dmvfi4
	sti
	mov	al,cl
dmvfi9:	pop	es
	pop	si
	pop	dx
	pop	cx
	pop	bx
	ret
dmvfindirq endp

; One try: TxEN on, wait up to 50 ms for a hit. AL = IRQ or FF.
dmvfione proc	near
	push	cx
	push	dx
	mov	dmvhit,0FFh
	mov	dmvhit2,0FFh
	mov	dx,modem.mddat
	add	dx,dmvCRW
	mov	al,01h			; TxEN only: TxRDY rises
	out	dx,al
	mov	cx,50
dmvfo1:	cmp	dmvhit,0FFh
	jne	dmvfo2
	mov	ax,1
	call	pcwait
	loop	dmvfo1
dmvfo2:	mov	ax,2			; let a late IRQ2 arrive too
	call	pcwait
	xor	al,al			; transmitter off again
	out	dx,al
	mov	al,dmvhit
	cmp	al,0FFh
	jne	dmvfo9
	mov	al,dmvhit2
dmvfo9:	pop	dx
	pop	cx
	ret
dmvfione endp

; Test interrupt handlers, 16 bytes each: AL = IRQ, then the common part.
	even
dmvtisr	label	near
	push	ax			; IRQ 2 (1 byte)
	mov	al,2			; (2 bytes)
	jmp	near ptr dmvtis9	; (3 bytes)
	db	10 dup (90h)
	push	ax			; IRQ 3
	mov	al,3
	jmp	near ptr dmvtis9
	db	10 dup (90h)
	push	ax			; IRQ 4
	mov	al,4
	jmp	near ptr dmvtis9
	db	10 dup (90h)
	push	ax			; IRQ 5
	mov	al,5
	jmp	near ptr dmvtis9
	db	10 dup (90h)
	push	ax			; IRQ 7
	mov	al,7
	jmp	near ptr dmvtis9
	db	10 dup (90h)
dmvtis9:push	dx
	push	ds
	mov	dx,seg data
	mov	ds,dx
	mov	ah,al
	mov	dx,modem.mddat		; our module: TxRDY up?
	inc	dx
	in	al,dx
	test	al,01h
	jz	dmvtis8			; z = not ours
	cmp	ah,2
	jne	dmvtis7
	cmp	dmvhit2,0FFh
	jne	dmvtis8
	mov	dmvhit2,ah
	jmp	short dmvtis8
dmvtis7:cmp	dmvhit,0FFh
	jne	dmvtis8
	mov	dmvhit,ah		; first slot IRQ seen
	add	dx,dmvCRW-dmvSR		; quiet the line: transmitter off
	xor	al,al
	out	dx,al
dmvtis8:mov	al,20h			; non-specific EOI
	out	PIC0,al
	pop	ds
	pop	dx
	pop	ax
	iret

; SERINI for a DMV module (modem.mddat set by dmvcomsel).
dmvserini proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	push	es
	mov	dx,modem.mddat
	add	dx,dmvCR
	in	al,dx			; also resets the MR pointer
	mov	dmvsavcmd,al		; as it was (DOS's AUX may want TxEN)
	mov	bx,portval		; MR1 from parity and stop bits
	mov	al,4Eh			; async 16x, 8 bits, 1 stop bit
	test	[bx].parflg,PARHARDWARE
	jz	dmvsi1
	cmp	[bx].parflg,PAREVNH
	jne	dmvsi0
	or	al,30h			; even
	jmp	short dmvsi1
dmvsi0:	cmp	[bx].parflg,PARODDH
	jne	dmvsi1			; mark/space: 2651 cannot
	or	al,10h			; odd
dmvsi1:	cmp	[bx].stopbits,1
	je	dmvsi2
	or	al,0C0h			; two stop bits
dmvsi2:	mov	dmvmr1,al
	call	dmvsetmode		; MR1 and MR2 (speed)
	mov	dx,modem.mddat		; flush the receiver
	in	al,dx
	in	al,dx
	cmp	dmvirq,0FFh		; interrupts?
	je	dmvsi4			; e = poll
	mov	al,byte ptr modem.mdintv
	mov	ah,getintv
	int	dos
	mov	word ptr dmvsavvec,bx
	mov	word ptr dmvsavvec+2,es
	call	dmvsetvec		; vector -> dmvisr
	cli
	mov	cl,dmvirq		; unmask our IRQ (read-modify-write,
	mov	ah,1			;  never write the whole mask)
	shl	ah,cl
	in	al,PIC1
	mov	dmvoldmask,al
	and	dmvoldmask,ah		; remember our bit
	not	ah
	and	al,ah
	out	PIC1,al
	sti
dmvsi4:	mov	al,26h			; RTS, RxEN, DTR; transmitter off
	cmp	dupflg,0		; full duplex?
	je	dmvsi5
	mov	al,06h			; half duplex: RTS only when sending
dmvsi5:	mov	dmvcmd,al
	mov	dx,modem.mddat
	add	dx,dmvCRW
	out	dx,al
	mov	portin,1
	mov	dmvpoll,0
	cmp	dmvirq,0FFh		; polled port: let the console and
	jne	dmvsi6			;  clock code look at it as well
	mov	dmvpoll,1
dmvsi6:
	pop	es
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
dmvserini endp

; Point the vector of our IRQ at dmvisr.
dmvsetvec proc	near
	push	ax
	push	dx
	push	ds
	mov	al,byte ptr modem.mdintv
	mov	ah,setintv
	mov	dx,seg dmvisr
	mov	ds,dx
	mov	dx,offset dmvisr
	int	dos
	pop	ds
	pop	dx
	pop	ax
	ret
dmvsetvec endp

; Write MR1 (dmvmr1) and MR2 (internal clocks, speed of the port).
dmvsetmode proc	near
	push	ax
	push	bx
	push	dx
	mov	bx,portval
	mov	bx,[bx].baud
	cmp	bx,baudlen
	jae	dmvsm1
	mov	ah,dmvbdtab[bx]
	cmp	ah,0FFh
	jne	dmvsm2
dmvsm1:	mov	ah,0Eh			; unknown: 9600
dmvsm2:	or	ah,30h			; internal transmit and receive clock
	mov	dx,modem.mddat
	add	dx,dmvCR
	in	al,dx			; MR pointer to MR1
	add	dx,dmvMRW-dmvCR
	mov	al,dmvmr1
	out	dx,al
	mov	al,ah
	out	dx,al
	pop	dx
	pop	bx
	pop	ax
	ret
dmvsetmode endp

; SERRST for a DMV module: wait for the transmitter, give back IRQ, mask
; bit and vector, and the command register as it was.
dmvserrst proc	near
	push	ax
	push	cx
	push	dx
	mov	dmvpoll,0
	mov	dx,modem.mddat
	inc	dx			; status
	mov	cx,2000
dmvsr1:	in	al,dx			; TxEMT, at most about 2 s
	test	al,04h
	jnz	dmvsr2
	mov	ax,1
	call	pcwait
	loop	dmvsr1
dmvsr2:	cmp	dmvirq,0FFh
	je	dmvsr3
	cli
	in	al,PIC1			; our mask bit as it was
	mov	cl,dmvirq
	mov	ah,1
	shl	ah,cl
	not	ah
	and	al,ah
	or	al,dmvoldmask
	out	PIC1,al
	push	ds
	mov	al,byte ptr modem.mdintv
	mov	ah,setintv
	lds	dx,dmvsavvec
	int	dos
	pop	ds
	sti
dmvsr3:	mov	dx,modem.mddat
	add	dx,dmvCRW
	mov	al,dmvsavcmd
	out	dx,al
	pop	dx
	pop	cx
	pop	ax
	ret
dmvserrst endp

; Called from MSXDMV.ASM before slow work (console output, the DOS clock):
; on a polled port the 2651 holds only two characters, at 9600 Baud
; that is 2 ms.
fdmvpoll proc	far
	push	ds
	push	ax
	mov	ax,seg data
	mov	ds,ax
	cmp	dmvpoll,0
	je	fdmvpo1
	call	dmvrxdrain
fdmvpo1:pop	ax
	pop	ds
	ret
fdmvpoll endp

; Interrupt handler.
dmvisr	proc	far
	push	ax
	push	bx
	push	cx
	push	dx
	push	ds
	mov	ax,seg data
	mov	ds,ax
	call	dmvrxdrain
	mov	al,20h			; non-specific EOI
	out	PIC0,al
	pop	ds
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	iret
dmvisr	endp

; Take every character the receiver has into Kermit's buffer, with the
; flow control of SERINT. Called from the interrupt handler and when
; Kermit polls. Interrupts are off while the buffer is changed.
dmvrxdrain proc	near
	cmp	dmvindrain,0
	je	dmvrd0
	ret
dmvrd0:	mov	dmvindrain,1
	pushf
	push	ax
	push	bx
	push	dx
dmvrd1:	cli
	mov	dx,modem.mddat
	inc	dx			; status
	in	al,dx
	test	al,02h			; RxRDY?
	jz	dmvrd8
	mov	ah,al
	and	ah,10h			; overrun?
	dec	dx
	in	al,dx			; the character
	test	ah,10h
	jz	dmvrd2
	push	ax			; reset the error flags
	add	dx,dmvCRW
	mov	al,dmvcmd
	or	al,10h
	out	dx,al
	pop	ax
	mov	ah,1			; mark: a character was lost
dmvrd2:	test	flowcnt,2		; incoming XON/XOFF?
	jz	dmvrd4
	mov	dh,al
	and	dh,parmsk
	cmp	dh,flowoff
	jne	dmvrd3
	cmp	xofsnt,0		; sent an XOFF ourselves? (echo)
	jne	dmvrd1
	mov	xofrcv,bufon
	jmp	short dmvrd1
dmvrd3:	cmp	dh,flowon
	jne	dmvrd4
	mov	xofrcv,off
	jmp	short dmvrd1
dmvrd4:	or	ah,ah			; lost one before this: BEL in its place
	jz	dmvrd5
	push	ax
	mov	al,bell
	call	dmvstore
	pop	ax
dmvrd5:	call	dmvstore
	jmp	short dmvrd1
dmvrd8:	sti
	cmp	count,mntrgh		; past the high mark?
	jbe	dmvrd9
	test	xofsnt,bufon		; already said stop?
	jnz	dmvrd9
	test	flowcnt,4		; RTS/CTS?
	jz	dmvrd8a
	and	dmvcmd,not 20h		; drop RTS
	mov	dx,modem.mddat
	add	dx,dmvCRW
	mov	al,dmvcmd
	out	dx,al
	mov	xofsnt,bufon
	jmp	short dmvrd9
dmvrd8a:test	flowcnt,1		; send XOFF?
	jz	dmvrd9
	mov	al,flowoff
	or	al,al
	jz	dmvrd9
	call	fdopar
	call	dmvputc
	mov	xofsnt,bufon
dmvrd9:	cli				; leave with interrupts off and the
	mov	dx,modem.mddat		;  receiver empty: an interrupt that
	inc	dx			;  came while we were busy found
	in	al,dx			;  dmvindrain set and did nothing,
	test	al,02h			;  and the 8259 (edge triggered)
	jz	dmvrd9a			;  sees no new edge while RxRDY
	jmp	dmvrd1			;  stays up
dmvrd9a:mov	dmvindrain,0
	pop	dx
	pop	bx
	pop	ax
	popf
	ret
dmvrxdrain endp

; Character AL into the circular buffer "source" (as SERINT does).
dmvstore proc	near
	push	bx
	mov	bx,srcpnt
	mov	byte ptr [bx],al
	inc	bx
	cmp	bx,offset source + bufsiz
	jb	dmvst1
	mov	bx,offset source
dmvst1:	mov	srcpnt,bx
	cmp	count,bufsiz
	jae	dmvst2
	inc	count
dmvst2:	pop	bx
	ret
dmvstore endp

; Send AL: transmitter on, wait for TxRDY, write, wait until the chip has
; taken the character, transmitter off. Carry set on timeout (no CTS, or
; the module gone).
dmvputc	proc	near
	push	ax
	push	cx
	push	dx
	mov	ah,al
	mov	dx,modem.mddat
	add	dx,dmvCRW
	mov	al,dmvcmd
	or	al,21h			; TxEN, and RTS (half duplex)
	out	dx,al
	sub	dx,dmvCRW-dmvSR
	xor	cx,cx
dmvpc1:	in	al,dx			; holding register free?
	test	al,01h
	jnz	dmvpc2
	loop	dmvpc1
	jmp	short dmvpc8
dmvpc2:	add	dx,dmvTHR-dmvSR
	mov	al,ah
	out	dx,al
	sub	dx,dmvTHR-dmvSR
	xor	cx,cx
dmvpc3:	in	al,dx			; taken into the shift register?
	test	al,01h
	jnz	dmvpc4
	loop	dmvpc3
	jmp	short dmvpc8
dmvpc4:	add	dx,dmvCRW-dmvSR		; transmitter off (it finishes the
	mov	al,dmvcmd		;  character on its own)
	out	dx,al
	cmp	dmvindrain,0		; a character came while TxRDY held
	jne	dmvpc5			;  the line? fetch it now
	sub	dx,dmvCRW-dmvSR
	in	al,dx
	test	al,02h
	jz	dmvpc5
	call	dmvrxdrain
dmvpc5:	pop	dx
	pop	cx
	pop	ax
	clc
	ret
dmvpc8:	add	dx,dmvCRW-dmvSR		; timeout: transmitter off
	mov	al,dmvcmd
	out	dx,al
	pop	dx
	pop	cx
	pop	ax
	stc
	ret
dmvputc	endp

; sndproc of the DMV ports: send AH.
dmvsnd	proc	near
	push	bx
	mov	bx,portval
	cmp	[bx].portrdy,0
	pop	bx
	je	dmvsnd8
	call	dmvcarrier		; carrier lost?
	jc	dmvsnd8
	mov	al,ah
	call	dmvputc
	jc	dmvsnd8
	clc
	ret
dmvsnd8:call	beep
	mov	kbdflg,'C'		; leave Connect mode
	stc
	ret
dmvsnd	endp

; rcvproc of the DMV ports: collect what the receiver has. Carry clear if
; the buffer has characters now.
dmvrcv	proc	near
	call	dmvcarrier
	call	dmvrxdrain
	cmp	count,0
	je	dmvrcv1
	clc
	ret
dmvrcv1:stc
	ret
dmvrcv	endp

; SET CARRIER: update cardet from DCD. Carry set if the carrier was on
; and is gone now.
dmvcarrier proc	near
	cmp	flags.carrier,0
	je	dmvca8
	cmp	dupflg,0
	jne	dmvca8
	push	ax
	push	dx
	call	dmvmdmstat		; AL in 8250 form, 80h = DCD
	pop	dx
	and	al,80h
	jnz	dmvca2
	test	cardet,80h
	jz	dmvca2
	mov	al,01h			; was on, is off
	mov	flags.cxzflg,'C'
dmvca2:	mov	cardet,al
	cmp	al,1
	pop	ax
	jne	dmvca8
	push	bx
	mov	bx,portval
	mov	[bx].portrdy,0
	pop	bx
	mov	kbdflg,'C'
	stc
	ret
dmvca8:	clc
	ret
dmvcarrier endp

; Modem lines in the form of the 8250's modem status register: 80h DCD,
; 20h DSR, 10h CTS. The 2651 does not report CTS (it gates the
; transmitter with it); it is shown as on.
dmvmdmstat proc	near
	push	dx
	mov	dx,modem.mddat
	inc	dx
	in	al,dx
	mov	ah,10h
	test	al,40h
	jz	dmvms1
	or	ah,80h
dmvms1:	test	al,80h
	jz	dmvms2
	or	ah,20h
dmvms2:	mov	al,ah
	xor	ah,ah
	pop	dx
	ret
dmvmdmstat endp

fdmvmdm	proc	far			; the same for code1
	call	dmvmdmstat
	ret
fdmvmdm	endp

; Speed of the module -> index into bdtab, in the port structure.
dmvgetbaud proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	mov	dx,modem.mddat
	add	dx,dmvCR
	in	al,dx			; MR pointer to MR1
	sub	dx,dmvCR-dmvMR
	in	al,dx			; MR1
	in	al,dx			; MR2
	and	al,0Fh
	xor	bx,bx
dmvgb1:	cmp	bx,baudlen
	jae	dmvgb9
	cmp	dmvbdtab[bx],al
	je	dmvgb2
	inc	bx
	jmp	short dmvgb1
dmvgb2:	mov	cx,bx
	mov	bx,portval
	mov	[bx].baud,cx
dmvgb9:	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
dmvgetbaud endp

; Break for AX milliseconds.
dmvbreak proc	near
	push	ax
	push	dx
	mov	dx,modem.mddat
	add	dx,dmvCRW
	push	ax
	mov	al,dmvcmd
	or	al,09h			; TxEN and force break
	out	dx,al
	pop	ax
	call	pcwait
	mov	al,dmvcmd
	out	dx,al
	pop	dx
	pop	ax
	ret
dmvbreak endp

; RTS for hardware flow control: AL = 0 off, else on.
dmvrts	proc	near
	push	dx
	or	al,al
	jz	dmvrts1
	or	dmvcmd,20h
	jmp	short dmvrts2
dmvrts1:and	dmvcmd,not 20h
dmvrts2:mov	dx,modem.mddat
	add	dx,dmvCRW
	mov	al,dmvcmd
	out	dx,al
	pop	dx
	ret
dmvrts	endp

; Hang up: DTR and RTS off.
dmvhangup proc	near
	push	ax
	push	dx
	mov	dx,modem.mddat
	add	dx,dmvCRW
	xor	al,al
	out	dx,al
	pop	dx
	pop	ax
	ret
dmvhangup endp

; Calibrate pcwait against the DOS clock (hundredths of a second), once.
; The DMV has no IBM timer channel 2 to measure with. 1 s is measured:
; DOS time moves in 55 ms steps under DOS 3.3 with the K235's tick, so a
; shorter window would be too coarse.
dmvpcwtst proc	near
	cmp	dmvcal,0
	je	dmvpw0
	ret
dmvpw0:	mov	dmvcal,1
	push	ax
	push	bx
	push	cx
	push	dx
	mov	cx,2			; first 200 ms roughly, then 1000 ms
	mov	bx,200
dmvpw1:	push	cx
	push	bx
	call	dmvcs			; wait for the clock to move
	mov	cx,ax
dmvpw2:	call	dmvcs
	cmp	ax,cx
	je	dmvpw2
	push	ax			; start
	mov	ax,bx
	call	pcwait
	call	dmvcs
	pop	cx
	sub	ax,cx			; hundredths elapsed
	pop	bx
	jbe	dmvpw4			; midnight or nothing: leave it
	mov	cx,ax
	mov	ax,bx			; wanted hundredths = ms / 10
	xor	dx,dx
	push	cx
	mov	cx,10
	div	cx
	pop	cx
	mul	pcwcnt			; pcwcnt * wanted / elapsed
	div	cx
	or	ax,ax
	jz	dmvpw4
	mov	pcwcnt,ax
dmvpw4:	pop	cx
	mov	bx,1000
	loop	dmvpw1
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
dmvpcwtst endp

; SHOW MODEM, DMV part: module, port, IRQ.
dmvshomod proc	near
	push	ax
	push	bx
	push	cx
	push	dx
	mov	ah,prstr
	mov	dx,offset dmvifmsg
	int	dos
	mov	bl,flags.comflg
	dec	bl
	xor	bh,bh
	mov	dx,offset dmvk801
	cmp	dmvifk[bx],2
	je	dmvsh1
	mov	dx,offset dmvk211
	mov	al,byte ptr modem.mddat
	and	al,0F0h
	cmp	al,70h
	je	dmvsh1
	mov	dx,offset dmvk212
dmvsh1:	int	dos
	mov	dx,offset dmvpmsg
	int	dos
	mov	ax,modem.mddat
	mov	cx,16
	call	valout
	mov	ah,prstr
	cmp	dmvirq,0FFh
	jne	dmvsh2
	mov	dx,offset dmvpolm
	int	dos
	jmp	short dmvsh3
dmvsh2:	mov	dx,offset dmvirqm
	int	dos
	mov	al,dmvirq
	xor	ah,ah
	call	decout
dmvsh3:	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret
dmvshomod endp

code	ends

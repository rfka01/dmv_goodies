; GDCTAB.INC - Tabellen fuer die GDC-Routinen (Ebenen, Kreise, PRAM, Muster, Zeichensatz)
; gemeinsam fuer DMVDEMO und DMVRACE (per INCLUDE an derselben Stelle)

PLTAB   dw      0000h
        db      GRUEN
        dw      4000h
        db      ROT
        dw      8000h
        db      BLAU

ONTAB   db      23h,21h,22h,23h         ; Ebene an:  malen=SET, XOR=COMP, loeschen=RESET, odern=SET
OFFTAB  db      22h,0,0,0               ; Ebene aus: malen=RESET, sonst nichts

; Kreisoktanten: Richtung, Startpunkt x/y in Radien
; (4. Byte: DM = 1 laesst den gemeinsamen Startpunkt beim zweiten Oktanten aus)
CIRTAB  db      0,-1,0,0
        db      3,-1,0,1
        db      1,0,-1,0
        db      6,0,-1,1
        db      2,0,1,0
        db      5,0,1,1
        db      4,1,0,0
        db      7,1,0,1
        db      0FFh

PRGRAF  db      00h,00h,00h,59h,00h,00h,00h,59h
        db      0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh
PRTEXT  db      00h,00h,00h,19h,00h,00h,00h,19h
        db      0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh

RAINB   db      ROT,GELB,GRUEN,CYAN,BLAU,MAGENTA
EXTCOL  db      0,BLAU,MAGENTA,ROT,GELB,GRUEN,CYAN,BLAU

; Mischmuster (Bit gesetzt = Farbe A), Zeile 0 oben, Bit 7 links
PSOLID  db      0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh,0FFh
P75     db      0FFh,055h,0FFh,0AAh,0FFh,055h,0FFh,0AAh
P50     db      0AAh,055h,0AAh,055h,0AAh,055h,0AAh,055h
P25     db      000h,0AAh,000h,055h,000h,0AAh,000h,055h
P12     db      000h,022h,000h,000h,000h,088h,000h,000h

MONOPAT LABEL   BYTE
        INCLUDE MONOPAT.INC
FONT    LABEL   BYTE
        INCLUDE FONT.INC
SINTAB  LABEL   WORD
        INCLUDE SINTAB.INC

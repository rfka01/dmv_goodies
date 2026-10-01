; M68.INC - erzeugt aus MANDEL68.S (m68k-linux-gnu-as -m68000)
; Briefkasten-Vorlage (32 Bytes, 68000-Format) und 68008-Maschinencode
; gleicher Ausschnitt wie die 8088-Rechnung: x0 = -6446/8192, y0 = 1048/8192,
; Schritt 1/8192, 160 x 100 Zellen, Tiefe 100 (Werte in 4.28)
M68BOX  LABEL   BYTE
        db      000h,000h,000h,000h,0F3h,069h,000h,000h
        db      002h,00Ch,000h,000h,000h,000h,080h,000h
        db      000h,000h,080h,000h,000h,0A0h,000h,064h
        db      000h,064h,000h,000h,000h,000h,000h,000h
M68CODE LABEL   BYTE
        db      04Bh,0FAh,0FEh,0FEh,049h,0EDh,010h,000h,026h,07Ch,020h,000h
        db      000h,000h,02Ch,07Ch,040h,000h,000h,000h,01Ah,0BCh,000h,001h
        db      042h,02Dh,000h,001h,024h,06Dh,000h,008h,03Bh,06Dh,000h,016h
        db      000h,01Ch,022h,06Dh,000h,004h,03Bh,06Dh,000h,014h,000h,01Ah
        db      070h,000h,072h,000h,07Ah,000h,03Ch,02Dh,000h,018h,024h,000h
        db      06Ah,002h,044h,082h,0B4h,08Bh,064h,07Ch,02Eh,001h,06Ah,002h
        db      044h,087h,0BEh,08Bh,064h,072h,028h,002h,048h,044h,03Ch,004h
        db      0C8h,0C7h,026h,007h,048h,043h,0C6h,0C2h,0D6h,084h,078h,00Ch
        db      0E8h,0ABh,028h,007h,048h,044h,0C8h,0C6h,0E9h,08Ch,0D8h,083h
        db      02Fh,004h,026h,002h,048h,043h,038h,003h,0C8h,0C4h,0E9h,08Ch
        db      0C4h,0C3h,076h,00Bh,0E6h,0AAh,0D4h,084h,026h,007h,048h,043h
        db      038h,003h,0C8h,0C4h,0E9h,08Ch,0CEh,0C3h,076h,00Bh,0E6h,0AFh
        db      0DEh,084h,026h,002h,0D6h,087h,0B6h,08Eh,062h,024h,094h,087h
        db      028h,01Fh,0D8h,084h,026h,000h,0B3h,083h,06Ah,002h,044h,084h
        db      0D8h,08Ah,022h,004h,0D4h,089h,020h,002h,03Ch,02Dh,000h,018h
        db      052h,045h,0BAh,046h,065h,000h,0FFh,080h,060h,002h,058h,08Fh
        db      018h,0C5h,0D3h,0EDh,000h,00Ch,053h,06Dh,000h,01Ah,066h,000h
        db      0FFh,064h,0D5h,0EDh,000h,010h,052h,02Dh,000h,001h,04Ah,02Dh
        db      000h,002h,066h,012h,053h,06Dh,000h,01Ch,066h,000h,0FFh,044h
        db      01Ah,0BCh,000h,002h,04Eh,072h,027h,000h,060h,0FAh,01Ah,0BCh
        db      000h,003h,060h,0F4h
M68LEN  EQU     244

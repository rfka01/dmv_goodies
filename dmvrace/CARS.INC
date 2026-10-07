; erzeugt von tools/mkcars.py - Wagen 8x8 in 8 Richtungen (0 rechts, 1 rechts unten,
; 2 runter, ... 7 rechts oben); je Richtung 8 Bytes Karosserie, 8 Bytes Reifen
; (Zeile 0 oben, Bit 7 links)
CARSPR  LABEL   BYTE
        db      000h,000h,0FFh,0FBh,0FBh,0FBh,0FFh,000h   ; 0 B
        db      000h,066h,000h,000h,000h,000h,000h,066h   ; 0 T
        db      000h,070h,078h,07Ch,03Ah,016h,00Fh,002h   ; 1 B
        db      010h,008h,000h,082h,041h,000h,010h,008h   ; 1 T
        db      07Ch,07Ch,07Ch,07Ch,07Ch,044h,07Ch,07Ch   ; 2 B
        db      000h,082h,082h,000h,000h,082h,082h,000h   ; 2 T
        db      000h,00Eh,01Eh,03Eh,05Ch,068h,0F0h,040h   ; 3 B
        db      008h,010h,000h,041h,082h,000h,008h,010h   ; 3 T
        db      000h,0FFh,0DFh,0DFh,0DFh,0FFh,000h,000h   ; 4 B
        db      066h,000h,000h,000h,000h,000h,066h,000h   ; 4 T
        db      040h,0F0h,068h,05Ch,03Eh,01Eh,00Eh,000h   ; 5 B
        db      010h,008h,000h,082h,041h,000h,010h,008h   ; 5 T
        db      03Eh,03Eh,022h,03Eh,03Eh,03Eh,03Eh,03Eh   ; 6 B
        db      000h,041h,041h,000h,000h,041h,041h,000h   ; 6 T
        db      002h,00Fh,016h,03Ah,07Ch,078h,070h,000h   ; 7 B
        db      008h,010h,000h,041h,082h,000h,008h,010h   ; 7 T
SNAIL   LABEL   BYTE
        db      000h,00Fh,030h,04Eh,051h,04Dh,031h,0FFh   ; SNAIL links B
        db      000h,000h,000h,000h,000h,000h,000h,000h   ; SNAIL links T
        db      000h,005h,0C5h,027h,02Fh,02Eh,07Ch,0F8h   ; SNAIL rechts B
        db      00Ah,000h,000h,000h,000h,000h,000h,000h   ; SNAIL rechts T
HUND    LABEL   BYTE
        db      000h,000h,000h,0BFh,07Fh,028h,045h,082h   ; HUND links B
        db      000h,000h,000h,000h,000h,000h,000h,000h   ; HUND links T
        db      000h,018h,03Bh,0F0h,0E0h,0A0h,010h,008h   ; HUND rechts B
        db      000h,000h,004h,000h,000h,000h,000h,000h   ; HUND rechts T

; Z80.INC - erzeugt von tools/mkz80inc.py aus RACEZ80.ASM (z80asm)
; Z80-Kern fuer ORG 0000h; Z80REL = Bytes, zu denen die Seite (Basis/256) addiert wird
Z80LEN  EQU     1019               ; Code und Parameter
Z80SIZE EQU     12338               ; mit Arbeitsspeicher (Basis + Z80SIZE)
Z80NREL EQU     164
ZSTARTO EQU     0
ZTESTO  EQU     54
ZSTATO  EQU     55
ZRESULTO EQU     56
ZX0O    EQU     60
ZRESO   EQU     1147
ZSIERO  EQU     2147
ZDIVO   EQU     3447
ZCIRCO  EQU     3847
ZFLAGSO EQU     4147
Z80CODE LABEL   BYTE
        db      0F3h,031h,07Bh,004h,03Eh,001h,032h,037h,000h,021h,000h,000h
        db      022h,038h,000h,022h,03Ah,000h,03Ah,036h,000h,0FEh,001h,0CAh
        db      06Dh,000h,0FEh,002h,0CAh,0A7h,001h,0FEh,003h,0CAh,034h,002h
        db      0FEh,004h,0CAh,097h,002h,0FEh,005h,0CAh,02Ah,003h,03Eh,002h
        db      032h,037h,000h,0C3h,00Eh,0F1h,000h,000h,000h,000h,000h,000h
        db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
        db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
        db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
        db      000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h,000h
        db      000h,0DDh,021h,07Bh,004h,02Ah,03Eh,000h,022h,04Ah,000h,03Ah
        db      045h,000h,032h,055h,000h,02Ah,03Ch,000h,022h,048h,000h,03Ah
        db      044h,000h,032h,054h,000h,0CDh,0B8h,000h,0DDh,077h,000h,0DDh
        db      023h,02Ah,048h,000h,0EDh,05Bh,040h,000h,019h,022h,048h,000h
        db      021h,054h,000h,035h,020h,0E7h,02Ah,04Ah,000h,0EDh,05Bh,042h
        db      000h,0B7h,0EDh,052h,022h,04Ah,000h,021h,055h,000h,035h,020h
        db      0C8h,0C3h,02Eh,000h,021h,000h,000h,022h,04Ch,000h,022h,04Eh
        db      000h,0AFh,032h,056h,000h,02Ah,04Ch,000h,0CDh,064h,001h,07Ch
        db      0FEh,040h,0D2h,060h,001h,022h,057h,000h,02Ah,04Eh,000h,0CDh
        db      064h,001h,07Ch,0FEh,040h,0D2h,060h,001h,022h,059h,000h,0EDh
        db      04Bh,057h,000h,050h,059h,0CDh,06Fh,001h,0CDh,08Bh,001h,022h
        db      050h,000h,0EDh,04Bh,059h,000h,050h,059h,0CDh,06Fh,001h,0CDh
        db      08Bh,001h,022h,052h,000h,0EDh,05Bh,050h,000h,019h,011h,001h
        db      080h,0B7h,0EDh,052h,0D2h,060h,001h,0EDh,04Bh,057h,000h,0EDh
        db      05Bh,059h,000h,0CDh,06Fh,001h,03Ah,04Dh,000h,047h,03Ah,04Fh
        db      000h,0A8h,0F2h,038h,001h,001h,0FFh,00Fh,009h,030h,001h,013h
        db      0CDh,098h,001h,0EBh,021h,000h,000h,0B7h,0EDh,052h,018h,003h
        db      0CDh,098h,001h,0EDh,05Bh,04Ah,000h,019h,022h,04Eh,000h,02Ah
        db      050h,000h,0EDh,05Bh,052h,000h,0B7h,0EDh,052h,0EDh,05Bh,048h
        db      000h,019h,022h,04Ch,000h,021h,056h,000h,034h,03Ah,046h,000h
        db      0BEh,0C2h,0C5h,000h,03Ah,056h,000h,0C9h,0CBh,07Ch,0C8h,0EBh
        db      021h,000h,000h,0B7h,0EDh,052h,0C9h,021h,000h,000h,0CBh,023h
        db      0CBh,012h,0CBh,023h,0CBh,012h,03Eh,00Eh,029h,0CBh,013h,0CBh
        db      012h,030h,004h,009h,030h,001h,013h,03Dh,020h,0F2h,0C9h,07Ch
        db      007h,007h,007h,0E6h,007h,0EBh,029h,029h,029h,0B5h,06Fh,0C9h
        db      07Ch,007h,007h,007h,007h,0E6h,00Fh,0EBh,029h,029h,029h,029h
        db      0B5h,06Fh,0C9h,03Eh,028h,032h,056h,000h,021h,0FBh,003h,006h
        db      020h,0AFh,077h,023h,010h,0FCh,03Eh,001h,032h,0FBh,003h,021h
        db      063h,008h,022h,068h,000h,03Ah,056h,000h,0FEh,001h,0CCh,019h
        db      002h,03Eh,0C7h,032h,055h,000h,021h,0FBh,003h,011h,01Bh,004h
        db      006h,020h,0B7h,07Eh,017h,012h,023h,013h,010h,0F9h,021h,0FBh
        db      003h,011h,01Bh,004h,006h,020h,01Ah,0AEh,077h,023h,013h,010h
        db      0F9h,03Ah,056h,000h,0FEh,001h,0CCh,019h,002h,021h,055h,000h
        db      035h,020h,0D3h,021h,056h,000h,035h,020h,0ABh,021h,0FBh,003h
        db      011h,000h,000h,006h,020h,07Eh,083h,05Fh,030h,001h,014h,023h
        db      010h,0F7h,0EDh,053h,038h,000h,0C3h,02Eh,000h,02Ah,068h,000h
        db      011h,077h,00Dh,0B7h,0EDh,052h,0D0h,0EDh,05Bh,068h,000h,021h
        db      0FBh,003h,001h,00Dh,000h,0EDh,0B0h,0EDh,053h,068h,000h,0C9h
        db      03Eh,008h,032h,056h,000h,021h,033h,010h,011h,034h,010h,001h
        db      0FEh,01Fh,036h,001h,0EDh,0B0h,021h,000h,000h,022h,05Bh,000h
        db      021h,033h,010h,07Eh,0B7h,028h,02Ah,0E5h,001h,033h,010h,0B7h
        db      0EDh,042h,029h,023h,023h,023h,0EBh,0E1h,0E5h,001h,032h,030h
        db      019h,038h,00Ch,0E5h,0B7h,0EDh,042h,0E1h,030h,005h,036h,000h
        db      019h,030h,0F4h,0E1h,0EDh,04Bh,05Bh,000h,003h,0EDh,043h,05Bh
        db      000h,023h,0E5h,001h,032h,030h,0B7h,0EDh,042h,0E1h,038h,0C7h
        db      021h,056h,000h,035h,020h,0ABh,02Ah,05Bh,000h,022h,038h,000h
        db      0C3h,02Eh,000h,021h,050h,0C3h,022h,05Dh,000h,021h,090h,001h
        db      022h,05Bh,000h,021h,000h,000h,022h,064h,000h,021h,077h,00Dh
        db      022h,068h,000h,02Ah,064h,000h,022h,06Ah,000h,02Ah,05Dh,000h
        db      022h,05Fh,000h,011h,002h,000h,02Ah,05Fh,000h,07Ch,04Dh,0CDh
        db      016h,003h,0E5h,067h,069h,0B7h,0EDh,052h,038h,015h,019h,0C1h
        db      078h,0B1h,020h,00Ch,022h,05Fh,000h,02Ah,064h,000h,023h,022h
        db      064h,000h,018h,0DEh,013h,018h,0DBh,0E1h,02Ah,064h,000h,023h
        db      022h,064h,000h,0EDh,05Bh,06Ah,000h,0B7h,0EDh,052h,07Dh,02Ah
        db      068h,000h,077h,023h,022h,068h,000h,02Ah,05Dh,000h,023h,022h
        db      05Dh,000h,02Ah,05Bh,000h,02Bh,022h,05Bh,000h,07Ch,0B5h,020h
        db      0A2h,02Ah,064h,000h,022h,038h,000h,0C3h,02Eh,000h,021h,000h
        db      000h,006h,010h,0CBh,021h,00Ch,017h,0EDh,06Ah,0EDh,052h,030h
        db      002h,019h,00Dh,010h,0F2h,0C9h,021h,007h,00Fh,022h,068h,000h
        db      0AFh,032h,06Ch,000h,021h,000h,000h,022h,062h,000h,022h,064h
        db      000h,022h,066h,000h,0EDh,04Bh,062h,000h,050h,059h,0CDh,06Fh
        db      001h,001h,000h,051h,0B7h,0E5h,060h,069h,0C1h,0EDh,042h,0E5h
        db      021h,025h,002h,0EDh,052h,0EBh,0E1h,0D5h,0E5h,0D9h,0E1h,0D1h
        db      0D9h,0CDh,0ABh,003h,0DDh,0E5h,0D1h,03Ah,06Ch,000h,0B7h,020h
        db      00Ch,02Ah,068h,000h,073h,023h,072h,023h,022h,068h,000h,03Eh
        db      028h,03Dh,032h,06Ch,000h,02Ah,064h,000h,019h,022h,064h,000h
        db      030h,007h,02Ah,066h,000h,023h,022h,066h,000h,02Ah,062h,000h
        db      023h,022h,062h,000h,011h,070h,017h,0B7h,0EDh,052h,038h,0A4h
        db      02Ah,064h,000h,022h,038h,000h,02Ah,066h,000h,022h,03Ah,000h
        db      0C3h,02Eh,000h,0DDh,021h,000h,000h,021h,000h,000h,0AFh,032h
        db      061h,000h,006h,010h,0D9h,029h,0CBh,013h,0CBh,012h,0D9h,0EDh
        db      06Ah,03Ah,061h,000h,017h,032h,061h,000h,0D9h,029h,0CBh,013h
        db      0CBh,012h,0D9h,0EDh,06Ah,03Ah,061h,000h,017h,032h,061h,000h
        db      0DDh,029h,0DDh,0E5h,0D1h,00Eh,000h,0CBh,023h,0CBh,012h,0CBh
        db      011h,0CBh,0C3h,0B7h,0EDh,052h,03Ah,061h,000h,099h,038h,007h
        db      032h,061h,000h,0DDh,023h,018h,001h,019h,010h,0BEh,0C9h,000h
Z80REL  LABEL   WORD
        dw      3,8,14,17,20,25,30,35,40,45,50,112
        dw      115,118,121,124,127,130,133,136,139,147,151,155
        dw      158,164,168,174,177,183,189,192,196,199,202,208
        dw      211,214,217,223,226,230,235,238,241,245,250,253
        dw      256,260,270,274,278,281,284,288,292,302,314,318
        dw      322,325,329,336,340,343,347,351,354,427,430,442
        dw      445,448,451,456,461,464,467,480,483,495,500,503
        dw      509,515,533,536,539,542,550,553,562,568,571,574
        dw      587,590,598,611,631,636,641,650,656,659,662,668
        dw      674,680,683,686,689,692,695,698,704,709,726,729
        dw      733,742,746,750,757,762,765,769,772,776,783,786
        dw      789,812,815,819,825,828,831,835,840,867,873,879
        dw      886,892,895,899,904,908,911,915,926,929,932,935
        dw      938,949,963,967,979,983,1004,1010

; Erzeugt von hw/gen_inc.py aus hw/dmvhw.def - nicht von Hand aendern.


; --- CPU-Umschaltung und Speicher
P_CPUSW          equ 0x0             ; OUT: Prozessorwechsel Z80 <-> 8088
P_RAMSEL         equ 0x10            ; OUT: RAM ueber 0000h-1FFFh zurueck
P_ROMSEL         equ 0x11            ; OUT: Mainboard-ROM (8 KB) ueber 0000h-1FFFh
ROMWIN_END       equ 0x2000          ; erste Adresse ausserhalb des ROM-Fensters
ROM_VEROFS       equ 0xFF9           ; ROM-Offset des Versionszaehlstrings / 'M' oder 'C'
ROM_MONO         equ 0x4D            ; 'M' an ROM_VEROFS: Mono
ROM_COLOR        equ 0x43            ; 'C' an ROM_VEROFS: Farbe
ROM_FWID         equ 0xFF7           ; ROM: Firmwarekennung, Zaehlbyte 8 + " C.07.00" (9 Byte)
FWID_COPY        equ 0x20F0          ; Kopie der Kennung fuer K234/K238 (Zielhardware 2a/2b)

; --- Firmware-Postfach, Segment 0
MB_SB8           equ 0xFE01          ; Status vom 8-Bit-Teil
MB_CB16          equ 0xFE04          ; Kommando an den 8088
MB_SB16          equ 0xFE05          ; Status zurueck
MB_MEM16         equ 0xFE06          ; Zahl der 64-KB-Erweiterungsbaenke
MB_HDBOOT        equ 0xFE07          ; 1 = von Platte gestartet, 0 = Diskette
MB_Z80A          equ 0xFE08          ; Z80-ROM: 1 = ID auf phys. Zyl. 2 von A: war nicht C=2 (Doppelschritt)
MB_Z80B          equ 0xFE09          ; Z80-ROM: N dieser Kennung (2 = 512 Byte)
BOOT_LOAD        equ 0x2000          ; Ladeadresse des Bootsektors (0000:2000)

; --- Startblock des MBR, Segment 0
SB_ADR           equ 0xFE20          ; Kennung 'DMSB', 28 Byte; nur gueltig bei MB_HDBOOT = 1
SB_VER           equ 0xFE24          ; Version, derzeit 1
SB_UNIT          equ 0xFE25          ; Einheit 0-2 (HD1-HD3)
SB_ENTRY         equ 0xFE26          ; Nummer des Partitionseintrags 0-3
SB_FROM          equ 0xFE27          ; Herkunft: 0 = DMV-MBR, 1 = Bootmanager
SB_SDH           equ 0xFE28          ; fertiger SDH-Grundwert: HD_SDHBASE + Einheit*8
SB_DL            equ 0xFE29          ; DL-Wert fuer den Lader: 80h + Einheit
SB_PART          equ 0xFE2A          ; Kopie des Partitionseintrags, 16 Byte
SB_SUM           equ 0xFE3A          ; Pruefsumme ueber SB_ADR..SB_PART+15
SB_LIFE          equ 60              ; Lebensdauer in Frames - danach liegt MSDOS.SYS darueber
BS_UNIT          equ 0x24            ; Einheitsbits (Bit 3-4) im Partitionsbootsektor, wie XFDisk; HD_SDHBASE kommt beim Lesen dazu
BS_DL            equ 0x25            ; dieselbe Einheit als Laufwerksnummer fuer den Lader; 80h kommt beim Lesen dazu

; --- Systemstatus Anschluss 13h
P_SYSSTAT        equ 0x13
SS_MOTOR         equ 0x1             ; Diskettenmotor laeuft
SS_NO16BIT       equ 0x2             ; 16-Bit-Prozessor vorhanden (negiert)
SS_READY         equ 0x4             ; Laufwerk bereit
SS_FDCINT        equ 0x8             ; Interrupt des uPD765 (live), Diskettenwechsel
SS_IRQ4          equ 0x10
SS_IRQ3          equ 0x20
SS_IRQ2          equ 0x40
SS_INDEX         equ 0x80            ; Indexloch

; --- Tastatur 8741
P_KBDDATA        equ 0x40
P_KBDCMD         equ 0x41            ; Kommando schreiben / Status lesen
KBD_COUNTRY      equ 0x1             ; Kommando: Landescode liefern (zugleich Init)
KBD_TONE         equ 0x6             ; Kommando: Ton
KBD_RDY          equ 0x1             ; Status: Zeichen da
KBD_IBF          equ 0x2             ; Status: Eingangspuffer voll
KBD_LANG         equ 0x80            ; Status: Zeichen ist der Landescode
KBD_LANGBAS      equ 0xE8            ; Landescode = Datenbyte - KBD_LANGBAS
KEY_ENTER        equ 0x88            ; Eingabetaste (nicht 0Dh)
KEY_CLR          equ 0x7F            ; CLR; mit Control KEY_CLRCTRL
KEY_CLRCTRL      equ 0x8C            ; Ctrl-CLR
KEY_FNCTRL       equ 0xA0            ; Ctrl-F1; F20 liegt bei KEY_FNCTRL + 19
KEY_PADCTRL      equ 0xB4            ; Ziffernblock mit Control, erster Code
KEY_FNSHIFT      equ 0xC0            ; Shift-F1
KEY_PADBAS       equ 0xD4            ; Ziffernblock mit Shift, erster Code (landesunabhaengig)
KEY_PADTOP       equ 0xDF            ; Ziffernblock mit Shift, letzter Code
KEY_FNBAS        equ 0xE0            ; F1
KEY_PADMINS      equ 0xF4            ; Minus des Blocks mit Shift; mit Control 0F5h
KEY_F20RST       equ 0xB3            ; Ctrl-F20: die Tastatur startet selbst neu, nicht abfangbar

; --- Interruptcontroller 8259 (nur K235) und Zeitgeber 8253
P_PICCMD         equ 0x90
P_PICDATA        equ 0x91            ; nach ICW: Maskenregister
PIC_ICW1         equ 0x17            ; flankengesteuert, Single, ICW4
PIC_ICW2         equ 0x8             ; Vektorbasis 8
PIC_ICW4         equ 0x3             ; AEOI, 8088
PIC_ICW1LVL      equ 0x1F            ; wie PIC_ICW1, aber pegelgesteuert (IRR zeigt den Eingang)
PIC_FWICW1       equ 0x1B            ; so hinterlaesst ihn das ROM 33473: pegelgesteuert, Single, ICW4
PIC_FWICW4       equ 0x1             ; ROM 33473: 8088, normales EOI (kein AEOI); Maske danach FFh
PIC_EOI          equ 0x20            ; unspezifisches EOI an 90h
PIC_OCW3IRR      equ 0xA             ; OCW3: naechstes Lesen von 90h liefert das IRR
P_PIT2           equ 0x82            ; 8253 Kanal 2 Daten
P_PITCTL         equ 0x83            ; 8253 Steuerwort
PIT_CTL2         equ 0xB6            ; Kanal 2, LSB+MSB, Modus 3
PIT_KHZ          equ 500             ; Eingangstakt in kHz
PIT_10MS         equ 5000            ; Teiler fuer 10 ms
PIT_PCTICK       equ 27463           ; Teiler fuer 18,2065 Hz
PIT_CTL2M0       equ 0xB0            ; Kanal 2, LSB+MSB, Modus 0 (OUT2 sofort 0, nach Zaehlende 1)
PIT_CTL2M2       equ 0xB4            ; Kanal 2, LSB+MSB, Modus 2 (kurzer 0-Puls je Periode)

; --- Grafik uPD7220
P_GDCSTAT        equ 0xA0            ; Status lesen
P_GDCPARAM       equ 0xA0            ; Parameter schreiben
P_GDCCMD         equ 0xA1            ; Kommando schreiben
P_GDCZOOM        equ 0xA2            ; Zoom-Latch (neuere Karten)
GDC_FIFULL       equ 0x2             ; Status: FIFO voll
GDC_FIEMPTY      equ 0x4             ; Status: FIFO leer
GDC_VSYNC        equ 0x20            ; Status: Bildruecklauf

; --- Zeichengenerator 76161, DMV-Umlautcodes
CH_MASK          equ 0x7F            ; ROM-Zugriff: Code AND CH_MASK
CH_AE_U          equ 0x80            ; Ae
CH_AELIG_U       equ 0x81            ; AE-Ligatur
CH_ARING_U       equ 0x82            ; A mit Ring
CH_POUND         equ 0x83            ; Pfund (Doppelbalken)
CH_EACUTE_U      equ 0x84            ; E mit Akut
CH_NTILDE_U      equ 0x85            ; N mit Tilde
CH_OE_U          equ 0x86            ; Oe
CH_OSLASH_U      equ 0x87            ; O mit Schraegstrich
CH_CCED_L        equ 0x88            ; c mit Cedille
CH_UE_U          equ 0x89            ; Ue
CH_AGRAVE_L      equ 0x8A            ; a mit Gravis
CH_EGRAVE_L      equ 0x8B            ; e mit Gravis
CH_ACUTE         equ 0x8C            ; Akut allein
CH_DEGREE        equ 0x8D            ; Gradzeichen
CH_UPARROW       equ 0x8E            ; Pfeil nach oben
CH_DIAERESIS     equ 0x8F            ; Trema allein
CH_AE_L          equ 0x90            ; ae
CH_AELIG_L       equ 0x91            ; ae-Ligatur
CH_ARING_L       equ 0x92            ; a mit Ring
CH_CURRENCY      equ 0x93            ; Waehrungszeichen
CH_EACUTE_L      equ 0x94            ; e mit Akut
CH_NTILDE_L      equ 0x95            ; n mit Tilde
CH_OE_L          equ 0x96            ; oe
CH_OSLASH_L      equ 0x97            ; o mit Schraegstrich
CH_OGRAVE_L      equ 0x98            ; o mit Gravis
CH_UE_L          equ 0x99            ; ue
CH_UGRAVE_L      equ 0x9A            ; u mit Gravis
CH_IGRAVE_L      equ 0x9B            ; i mit Gravis
CH_SECTION       equ 0x9C            ; Paragraph
CH_INVQUEST      equ 0x9D            ; umgedrehtes Fragezeichen
CH_SZ            equ 0x9E            ; sz
CH_INVEXCL       equ 0x9F            ; umgedrehtes Ausrufezeichen

; --- IFSEL-Haelften, Portbasis
IF_0A            equ 0x60            ; K210 / K212 / K213
IF_0B            equ 0x68
IF_1A            equ 0x70            ; K211 / K215
IF_1B            equ 0x78
IF_2A            equ 0x30            ; K806 Maus
IF_2B            equ 0x38            ; K804 IEEE 488
IF_3A            equ 0xB0
IF_3B            equ 0xB8            ; K600
IF_4A            equ 0xC0            ; Festplattencontroller
IF_4B            equ 0xC8            ; K803 Uhr

; --- Uhr K803 (MM58167) relativ zur Haelfte 4B
RTC_DATOFS       equ 4               ; Daten: Basis + 4 + (Adresse AND 3); Basis + 0 ist das Latch
RTC_INTEN        equ 0x80            ; Latch Bit 7: Uhr-Interrupt auf INT/ (nach MAME k803.cpp)
MM_SEC           equ 0x2             ; Sekunden (BCD)
MM_ISTAT         equ 0x10            ; Interrupt-Status, Lesen loescht
MM_ICTL          equ 0x11            ; Interrupt-Steuerung
MM_I10HZ         equ 0x2             ; Interrupt-Steuerung: zehnmal je Sekunde

; --- SCN2651/2661 relativ zur Haelfte
SIO_RDATA        equ 0               ; lesen: Daten
SIO_RSTAT        equ 1               ; lesen: Status
SIO_RMODE        equ 2               ; lesen: Moderegister
SIO_RCMD         equ 3               ; lesen: Befehlsregister
SIO_WDATA        equ 4               ; schreiben: Daten
SIO_WSYN         equ 5               ; schreiben: SYN1/SYN2/DLE
SIO_WMODE        equ 6               ; schreiben: Moderegister
SIO_WCMD         equ 7               ; schreiben: Befehlsregister
SIO_MR2_2651     equ 0x3E            ; Moderegister 2 zurueckgelesen beim 2651
SIO_MR2_2661     equ 0x7E            ; Moderegister 2 zurueckgelesen beim 2661
SIO_CMD_ON       equ 0x37            ; Vollduplex, NCRs Wert

; --- Maus K806 relativ zur Haelfte
MS_DATA          equ 0               ; Daten / Parameter
MS_CMDSTAT       equ 1               ; Befehl schreiben / Status lesen
MS_OBF           equ 0x1
MS_IBF           equ 0x2
MS_INT           equ 0x40            ; Interrupt steht an (active high)
MS_GETABS        equ 0x0
MS_ACKMOVE       equ 0x1
MS_INTOFF        equ 0x2
MS_INTCLR        equ 0x3             ; Leitung und INT-Bit freigeben
MS_SETPOS        equ 0x4
MS_SETMAX        equ 0x5
MS_ACKSW         equ 0x6
MS_ACKBOTH       equ 0x7
MS_ROMTEST       equ 0x8
MS_GETREL        equ 0x9
MS_SETMIN        equ 0xA

; --- Festplattencontroller WD1001
HD_DATA          equ 0xC0
HD_ERROR         equ 0xC1
HD_COUNT         equ 0xC2
HD_SECTOR        equ 0xC3
HD_CYLLO         equ 0xC4
HD_CYLHI         equ 0xC5
HD_SDH           equ 0xC6
HD_CMDSTAT       equ 0xC7
HD_SDHBASE       equ 0xA0            ; ECC, 512 Byte; Kopf Bit 0-2, Einheit Bit 3-4
HD_DESEL         equ 0x18            ; nach jedem Zugriff nach HD_SDH
HD_RESTORE       equ 0x10            ; + Schrittweite im unteren Nibble (0 = buffered)
HD_READ          equ 0x20
HD_WRITE         equ 0x30
HD_BUSY          equ 0x80
HD_READY         equ 0x40
HD_DRQ           equ 0x8
HD_ERR           equ 0x1
HD_MAXHEADS      equ 8
HD_MAXUNITS      equ 3               ; HD1-HD3

; --- DMVGEOM1-Block, Zylinder 0 Kopf 0 Sektor-ID 1 (Kopie ID 2)
GEO_SIG          equ 0               ; 8 Byte "DMVGEOM1"
GEO_CYLS         equ 8               ; Wort
GEO_HEADS        equ 10              ; Byte
GEO_SPT          equ 11              ; Byte
GEO_BPS          equ 12              ; Wort, 512
GEO_PRECOMP      equ 14              ; Wort
GEO_LZONE        equ 16              ; Wort
GEO_STEP         equ 18              ; Byte (wird nicht gelesen)
GEO_ILEAVE       equ 19              ; Byte
GEO_UNIT         equ 20              ; Byte
GEO_CSUM         equ 22              ; Wort: Summe der Bytes 0..21
GEO_CSUMLEN      equ 22

; --- Datentraeger-Kennungen und Partitionstypen
OEM_OFS          equ 3               ; Offset der OEM-Kennung im Bootsektor
PT_FAT12         equ 0x1
PT_FAT16         equ 0x4
PT_EXT           equ 0x5
FAT16_MIN        equ 32680           ; ab so vielen Sektoren FAT16
PT_TABLE         equ 0x1BE

; --- Diskettencontroller uPD765 und Steuerports
P_TC             equ 0x12            ; OUT: Transferende (TC) an den uPD765
P_FDMOTOR        equ 0x14            ; OUT: Diskettenmotor ein
P_FDCSTAT        equ 0x50            ; Hauptstatus
P_FDCDATA        equ 0x51            ; Daten
FDC_RQM          equ 0x80            ; Hauptstatus: bereit fuer Datenbyte
FDC_DIO          equ 0x40            ; Hauptstatus: Controller sendet
FDC_EXM          equ 0x20            ; Hauptstatus: Ausfuehrungsphase (nur ohne DMA)
FDC_CB           equ 0x10            ; Hauptstatus: Kommando laeuft
FDC_SPEC48       equ 0xD6            ; SPECIFY Schrittrate/HUT, 48 tpi (TEAC FD-55B)
FDC_SPEC96       equ 0xE6            ; SPECIFY Schrittrate/HUT, 96 tpi (TEAC FD-55F)
FDC_SPECDMA      equ 0x2             ; SPECIFY HLT, mit DMA
FDC_SPECPIO      equ 0x3             ; SPECIFY HLT, ohne DMA (ND)

; --- 68008-Karte K234
K234_IO          equ 0xFFF00         ; Basis: Port n liegt beim 68008 auf K234_IO + n
K234_RAMTOP      equ 0x80000         ; hoechstens 512 KB RAM linear ab 0
P_SNR            equ 0xDF            ; startet den 68008 (Z80 geht in HOLD); MAME nimmt D8h-DFh, echte Hardware DFh

; --- Uhr K803 (MM58167), IFSEL 4B
RTC_LATCH        equ 0xC8            ; OUT: Registergruppe/Adresslatch, Bit 7 = Interrupt auf INT/
RTC_SEC          equ 0xCE            ; IN: Sekundenregister (BCD) bei Registergruppe 0

; --- Firmware-Variablen, Segment 0
FW_CURX          equ 0xF800          ; Cursor Spalte (DISPLAY)
FW_CURY          equ 0xF801          ; Cursor Zeile (DISPLAY)
FW_INVFLG        equ 0xF90B          ; Attribut der Textausgabe
FW_ATTRNORM      equ 0xE8            ; gruen auf schwarz
FW_DISPSEG       equ 0xFF00          ; DISPLAY: CALL FW_DISPSEG:0000, DS:BX Zaehlstring
FWC_HOME         equ 0x1             ; DISPLAY: Cursor oben links - NUR K235 (33473)
FWC_CLS          equ 0xC             ; DISPLAY: Schirm loeschen - NUR K235; K230/K231 zeigt Zeichen
FWC_NEWL         equ 0x88            ; DISPLAY: neue Zeile - auf allen Karten
FWC_BS           equ 0x8B            ; DISPLAY: Rueckschritt - NUR K235

; --- Uebergabe Z80 <-> 16 Bit
Z80_HANDOFF      equ 0xF10C          ; Z80-ROM legt hier OUT 10h / OUT D0h / NOP / JP F111h ab
Z80_RESUME       equ 0xF111          ; hier laeuft der Z80 nach OUT D0h des 8088 weiter (RAM)
P_SWITCH16       equ 0xD0            ; OUT: Bus zwischen Z80 und 8088-Karte wechseln
K8_ROMSEG        equ 0xFF00          ; Segment des 8088-ROMs (K230 32167, K235 33473)
K8_LOOP          equ 0x18            ; Befehlsschleife im 8088-ROM: OUT 11h, OUT D0h, Befehl aus FE04h

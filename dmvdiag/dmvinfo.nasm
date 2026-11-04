; DMVINFO - Hardware-Uebersicht der NCR DMV (Diskette, ohne DOS).
; Derselbe Kern wie DMVDIAG, mit INFO gebaut: Seite 1 Uebersicht, Seite 2
; Steckplaetze und Interrupts, R Rohwertseiten, D sichert DMVINFO.TXT.
;   python3 diag/mkimg.py dmvinfo
%define INFO
%include "dmvdiag.nasm"

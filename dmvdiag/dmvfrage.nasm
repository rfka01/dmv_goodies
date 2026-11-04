; DMVFRAGE - kleine Testdiskette fuer die offenen Fragen von DMVDIAG 0.16
; (Bruecken S1/S2, 13h-Bit von Platz 2, Leitung von K211/K803, Interrupt
; der K211). Derselbe Kern wie DMVDIAG, mit FRAGE gebaut: nach dem Start
; eine Taste, ein Lauf, eine Seite zum Fotografieren. Kein Sichern.
;   python3 diag/mkimg.py dmvfrage
%define FRAGE
%include "dmvdiag.nasm"

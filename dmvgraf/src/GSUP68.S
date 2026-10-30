| GSUP68.S - DMVGRAF fuer CP/M-68K: Supervisorzustand fuer E/A
|
|   long gsup()        in den Supervisorzustand (BDOS 62); Ergebnis: das
|                      SR vorher, fuer gusr
|   gusr(long sr)      zurueck in den vorigen Zustand
|
| Die Anschluesse des DMV-Busses liegen beim 68008 auf FFF00h + Port
| (K234). Ob die Karte sie auch im Benutzerzustand durchlaesst, ist nicht
| gemessen; darum geht DMVGRAF fuer jeden Block von Zugriffen mit BDOS 62
| (Set Supervisor State, DRs CP/M-68K) in den Supervisorzustand. Danach
| zeigt SP auf den Systemstapel; gsup setzt SP auf den Benutzerstapel
| zurueck, damit C-Code dazwischen wie gewohnt laeuft, und merkt sich den
| Systemstapel. gusr gibt ihn zurueck und schaltet ueber das alte SR
| wieder in den Benutzerzustand. Lief das Programm schon im
| Supervisorzustand, tun beide nichts. Zwischen gsup und gusr keine
| BDOS-Aufrufe.
|
| Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT

        .text
        .global gsup
        .global gusr

gsup:   moveq   #0,d0
        move.w  sr,d0                   | 68000/68008: im Benutzerzustand erlaubt
        btst    #13,d0
        bne.s   gs9                     | schon Supervisor
        move.w  d0,gs_sr
        move.l  sp,gs_usp
        moveq   #62,d0                  | BDOS 62
        trap    #2
        move.l  sp,gs_ssp               | jetzt Supervisor, Systemstapel
        move.l  gs_usp,sp               | weiter auf dem Benutzerstapel
        moveq   #0,d0
        move.w  gs_sr,d0
gs9:    rts

gusr:   move.l  4(sp),d0
        btst    #13,d0
        bne.s   gu9                     | war schon Supervisor
        move.l  sp,a0
        move.l  a0,usp
        move.l  gs_ssp,sp               | Systemstapel zurueck
        move.w  d0,sr                   | Benutzerzustand, SP = USP
gu9:    rts

        .bss
        .even
gs_usp: .space  4
gs_ssp: .space  4
gs_sr:  .space  2

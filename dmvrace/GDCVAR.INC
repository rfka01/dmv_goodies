; GDCVAR.INC - Variablen
; gemeinsam fuer DMVDEMO und DMVRACE (per INCLUDE an derselben Stelle)

        EVEN
v_vbase dw      0
v_lpat  dw      0FFFFh
v_fx    dw      0
v_fy    dw      0
v_faddr dw      0
v_fn    dw      0
v_pram  dw      0
v_x0    dw      0
v_y0    dw      0
v_x1    dw      0
v_y1    dw      0
v_w     dw      0
v_h     dw      0
v_cx    dw      0
v_cy    dw      0
v_r     dw      0
v_tx    dw      0
v_ty    dw      0
v_tz    dw      1
v_sho   dw      2
v_patp  dw      offset PSOLID
v_hcnt  dw      0
v_npl   dw      3
v_frame dw      0
v_seed  dw      1234h
v_bx0   dw      0
v_bx1   dw      0
v_by    dw      0
v_fcx   dw      0
v_fcy   dw      0
v_fst   dw      4
v_fcnt  dw      0
v_mf    dw      0
v_slot  dw      0
MX      dw      8 dup (0)
MY      dw      8 dup (0)
MVX     dw      8 dup (0)
MVY     dw      8 dup (0)
HX      dw      TRAIL*8 dup (0)
HY      dw      TRAIL*8 dup (0)
HC      db      TRAIL*2 dup (0)
v_bcol  db      0
v_fcol  db      0
v_sc    dw      0
v_zx    dw      0
v_zy    dw      0
v_zr    dw      0
v_mj    dw      0
v_mr    dw      0
v_mi    dw      0
v_tpos  dw      0
v_tlen  dw      0
v_sa    dw      0
v_ca_   dw      0
v_t1    dw      0
v_t2    dw      0
v_t3    dw      0
v_x1r   dw      0
NLX     dw      NV dup (0)
NRX     dw      NV dup (0)
NY      dw      NV dup (0)
OLX     dw      NV dup (0)
ORX     dw      NV dup (0)
OY      dw      NV dup (0)
ROWB    dw      40 dup (0)
MCA     db      160 dup (0)
MCB     db      160 dup (0)
MCL     db      160 dup (0)
v_maxit dw      0
v_pmul  dw      5
v_s68   dw      0
v_fh    dw      0
v_kpl   dw      0
v_kpos  dw      0
v_kdir  dw      1
v_t0lo  dw      0
v_t0hi  dw      0
v_secs  dw      0
v_note  dw      0
v_sw    dw      0
v_tsec  dw      0
v_s68run db     0
v_eng   db      0
v_has87 db      0
v_lvl   db      7                       ; 6 = 68008, 7 = 8087, 8 = nur 8088/V20
v_tok   db      0
        EVEN
v_mpar  db      0
v_mpb   db      0
v_ang1  db      0
v_ang2  db      0
P_AX    dw      0
P_AY    dw      0
P_MX    dw      0
P_MY    dw      0
P_BX    dw      0
P_BY    dw      0
v_hoff  dw      0
v_pstep dw      0
v_pj    dw      0
v_pfy   dw      0
v_prow  dw      0
v_pa    dw      0
v_pb    dw      0
v_thr   dw      0
v_msk   dw      0
v_ar    dw      0
v_ag    dw      0
v_ab    dw      0
RG      dw      40 dup (0)
RR      dw      40 dup (0)
RB      dw      40 dup (0)
v_q0    db      0
v_q1    db      0
v_q2    db      0
v_q3    db      0
PBUF2   db      8 dup (0)
FIGP    db      0
FDC     dw      0
FD      dw      0
FD2     dw      0
FD1     dw      0
FDM     dw      0
v_fdot  db      0
v_fcmd  db      0
v_fcmdm db      0
v_col   db      7
v_dmode db      0
v_dsave db      0
v_fmode db      0
v_ca    db      0
v_cb    db      0
v_shc   db      0
v_hcol  db      0
v_mono  db      0
v_force db      0
v_single db     0
v_cur   db      0
v_key   db      0
v_zd    db      0
v_zg    db      0
AREA    db      8 dup (0)
PBUF    db      8 dup (0)

"""z80zeile.py - hwEin fuer DMVHWC.INC (Z80): ein Listeneintrag fuer eine Bitzeile.
Gibt drei Bloecke aus (Kopf, ganze Worte, allgemein), getrennt durch @@@.
Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT"""
import sys, os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from z80inl import pas
R=lambda n:('rel',n); W=lambda n:('w',n); WO=lambda n,o:('wo',n,o)
# hwK: 0 sh, 1 k, 2 n8, 3 letzt, 4 rest, 5 sh7, 6 oder, 7 X, 8 O, 9 M
K=lambda o: WO('hwK',o)
def srl_hl(n): return [([0xCB,0x3C,0xCB,0x1D],'SRL H / RR L')]*n
def body(mask):
    q=[([0xDD,0x7E,0x00],'LD A,(IX+0)'),([0xDD,0x23],'INC IX'),
       ([0x21,K(7)],'LD HL,hwK+7'),([0xAE],'XOR (HL)       X'),
       ([0xFD,0xA6,0x00],'AND (IY+0)     Maske'),([0xFD,0x23],'INC IY')]
    if mask: q+=[([0x21,K(3)],'LD HL,hwK+3'),([0xA6],'AND (HL)       letzt')]
    u='s%d'%mask; v='n%d'%mask
    q+=[([0x6F],'LD L,A'),([0x26,0x00],'LD H,0'),
        ([0x3A,K(5)],'LD A,(hwK+5)   sh7'),([0xB7],'OR A'),([0x28,R(v)],'JR Z,'+v),
        'L:'+u,([0x29],'ADD HL,HL'),([0x3D],'DEC A'),([0x20,R(u)],'JR NZ,'+u),
        'L:'+v,([0x7D],'LD A,L'),([0xB1],'OR C'),([0x4C],'LD C,H'),
        ([0x12],'LD (DE),A'),([0x13],'INC DE'),
        ([0x21,K(6)],'LD HL,hwK+6'),([0xB6],'OR (HL)'),([0x77],'LD (HL),A')]
    return q
p=[
 ([0xDD,0xE5],'PUSH IX'),
 # sh, k
 ([0x3A,W('hwSx')],'LD A,(hwSx)'),([0xE6,0x0F],'AND 15'),([0x32,K(0)],'LD (hwK+0),A  sh'),
 ([0x6F],'LD L,A'),([0x26,0x00],'LD H,0'),([0xED,0x5B,W('hwNb')],'LD DE,(hwNb)'),([0x19],'ADD HL,DE'),
 ([0x11,0x0F,0x00],'LD DE,15'),([0x19],'ADD HL,DE')]+srl_hl(4)+[
 ([0x7D],'LD A,L'),([0x87],'ADD A,A'),([0x32,K(1)],'LD (hwK+1),A  k'),
 ([0x3C],'INC A'),([0x32,WO('hwKopf',14)],'LD (hwKopf+14),A  1+k'),([0x32,WO('hwKurz',4)],'LD (hwKurz+4),A'),
 # n8
 ([0x2A,W('hwNb')],'LD HL,(hwNb)'),([0x11,0x07,0x00],'LD DE,7'),([0x19],'ADD HL,DE')]+srl_hl(3)+[
 ([0x7D],'LD A,L'),([0x32,K(2)],'LD (hwK+2),A  n8'),
 # letzt
 ([0x3A,W('hwNb')],'LD A,(hwNb)'),([0xE6,0x07],'AND 7'),([0x47],'LD B,A'),([0x3E,0xFF],'LD A,FFh'),
 ([0x28,R('lz')],'JR Z,lz'),([0xAF],'XOR A'),'L:lzs',([0x37],'SCF'),([0x17],'RLA'),([0x10,R('lzs')],'DJNZ lzs'),
 'L:lz',([0x32,K(3)],'LD (hwK+3),A  letzt'),
 # Kopf
 ([0x2A,W('hwAd')],'LD HL,(hwAd)'),([0x22,WO('hwKopf',2)],'LD (hwKopf+2),HL'),([0x22,WO('hwKurz',2)],'LD (hwKurz+2),HL'),
 ([0x3A,W('hwW')],'LD A,(hwW)'),([0x32,WO('hwKopf',15)],'LD (hwKopf+15),A'),([0x32,WO('hwKurz',5)],'LD (hwKurz+5),A'),
 ([0x2A,W('hwL')],'LD HL,(hwL)'),([0xED,0x5B,W('hwQn')],'LD DE,(hwQn)'),([0x19],'ADD HL,DE'),([0xEB],'EX DE,HL'),
 ([0x2A,W('hwTpl')],'LD HL,(hwTpl)  Kopf lang oder kurz'),([0xED,0x4B,W('hwTn')],'LD BC,(hwTn)'),([0xED,0xB0],'LDIR'),
 ([0x2A,W('hwQn')],'LD HL,(hwQn)'),([0xED,0x4B,W('hwHl')],'LD BC,(hwHl)'),([0x09],'ADD HL,BC'),([0x22,W('hwQe')],'LD (hwQe),HL'),
 ([0x23],'INC HL'),([0x23],'INC HL'),([0x22,W('hwQn')],'LD (hwQn),HL'),
  ([0xED,0x53,W('hwDst')],'LD (hwDst),DE'),([0xDD,0xE1],'POP IX')]
ganz=[
 ([0xED,0x5B,W('hwDst')],'LD DE,(hwDst)'),
 ([0x2A,W('hwSrc')],'LD HL,(hwSrc)'),([0x3A,K(1)],'LD A,(hwK+1)'),([0x4F],'LD C,A'),([0x06,0x00],'LD B,0'),([0xED,0xB0],'LDIR'),
 ([0x2A,W('hwQn')],'LD HL,(hwQn)'),([0x3A,K(1)],'LD A,(hwK+1)'),([0x5F],'LD E,A'),([0x16,0x00],'LD D,0'),
 ([0x19],'ADD HL,DE'),([0x22,W('hwQn')],'LD (hwQn),HL')]
lang=[([0xDD,0xE5],'PUSH IX'),([0xFD,0xE5],'PUSH IY'),([0xFD,0x2A,W('hwMsk')],'LD IY,(hwMsk)'),
 ([0xED,0x5B,W('hwDst')],'LD DE,(hwDst)'),
 ([0xAF],'XOR A'),([0x32,K(6)],'LD (hwK+6),A  oder'),([0x4F],'LD C,A        alt'),
 ([0x3A,K(1)],'LD A,(hwK+1)'),([0x47],'LD B,A'),
 ([0x3A,K(0)],'LD A,(hwK+0)'),([0xFE,0x08],'CP 8'),([0x38,R('nv')],'JR C,nv'),
 ([0xAF],'XOR A'),([0x12],'LD (DE),A'),([0x13],'INC DE'),([0x05],'DEC B'),
 'L:nv',([0x78],'LD A,B'),([0x32,K(4)],'LD (hwK+4),A  rest'),
 ([0x3A,K(0)],'LD A,(hwK+0)'),([0xE6,0x07],'AND 7'),([0x32,K(5)],'LD (hwK+5),A  sh7'),
 ([0x3A,K(2)],'LD A,(hwK+2)'),([0x47],'LD B,A        n8'),([0x3A,K(4)],'LD A,(hwK+4)'),
 ([0xB8],'CP B'),([0x30,R('vok')],'JR NC,vok'),
 ([0x47],'LD B,A'),([0x3E,0xFF],'LD A,FFh'),([0x32,K(3)],'LD (hwK+3),A'),([0x3A,K(4)],'LD A,(hwK+4)'),
 'L:vok',([0x90],'SUB B'),([0x32,K(4)],'LD (hwK+4),A'),
 ([0xDD,0x2A,W('hwSrc')],'LD IX,(hwSrc)'),
 ([0x05],'DEC B'),([0x28,R('le')],'JR Z,le'),
 'L:lp']+body(0)+[([0x10,R('lp')],'DJNZ lp'),'L:le']+body(1)+[
 # Rest
 ([0x3A,K(4)],'LD A,(hwK+4)'),([0xB7],'OR A'),([0x28,R('rf')],'JR Z,rf'),
 ([0x47],'LD B,A'),([0x79],'LD A,C'),([0x12],'LD (DE),A'),([0x13],'INC DE'),
 ([0x21,K(6)],'LD HL,hwK+6'),([0xB6],'OR (HL)'),([0x77],'LD (HL),A'),
 ([0x05],'DEC B'),([0x28,R('rf')],'JR Z,rf'),([0xAF],'XOR A'),
 'L:rz',([0x12],'LD (DE),A'),([0x13],'INC DE'),([0x10,R('rz')],'DJNZ rz'),
 'L:rf',
 ([0x3A,K(6)],'LD A,(hwK+6)'),([0xB7],'OR A'),([0x20,R('dazu')],'JR NZ,dazu'),
 ([0x3A,W('hwW')],'LD A,(hwW)'),([0xFE,0x20],'CP 20h'),([0x28,R('dazu')],'JR Z,dazu'),
 ([0x2A,W('hwQe')],'LD HL,(hwQe)  leer: zurueck'),([0xED,0x5B,W('hwHl')],'LD DE,(hwHl)'),([0xB7],'OR A'),([0xED,0x52],'SBC HL,DE'),
 ([0x22,W('hwQn')],'LD (hwQn),HL'),([0x21,0xFF,0xFF],'LD HL,-1'),([0x22,W('hwQe')],'LD (hwQe),HL'),
 ([0x18,R('ende')],'JR ende'),
 'L:dazu',([0x2A,W('hwQn')],'LD HL,(hwQn)'),([0x3A,K(1)],'LD A,(hwK+1)'),([0x5F],'LD E,A'),([0x16,0x00],'LD D,0'),
 ([0x19],'ADD HL,DE'),([0x22,W('hwQn')],'LD (hwQn),HL'),
 'L:ende',([0xFD,0xE1],'POP IY'),([0xDD,0xE1],'POP IX'),
]
print(pas(p)); print('@@@'); print(pas(ganz)); print('@@@'); print(pas(lang))

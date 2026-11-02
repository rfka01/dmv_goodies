"""z80zoom.py - ghzbits fuer DMVHWC.INC (Z80): Bits vervielfachen (Zoom) und ausschneiden.
Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT"""
import sys, os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from z80inl import pas
R=lambda n:('rel',n); W=lambda n:('w',n); WO=lambda n,o:('wo',n,o)
K=lambda o: WO('hwK',o)
p=[
 ([0xDD,0xE5],'PUSH IX'),
 ([0xDD,0x2A,W('hwDst')],'LD IX,(hwDst)'),
 ([0x2A,W('hwSx')],'LD HL,(hwSx)   off'),([0x7D],'LD A,L'),([0xE6,0x07],'AND 7'),([0x47],'LD B,A'),
 ([0x3E,0x01],'LD A,1'),([0x28,R('m0')],'JR Z,m0'),'L:m1',([0x07],'RLCA'),([0x10,R('m1')],'DJNZ m1'),
 'L:m0',([0x4F],'LD C,A        Quellmaske'),
 ([0xCB,0x3C,0xCB,0x1D],'SRL H / RR L'),([0xCB,0x3C,0xCB,0x1D],'SRL H / RR L'),([0xCB,0x3C,0xCB,0x1D],'SRL H / RR L'),
 ([0xED,0x5B,W('hwSrc')],'LD DE,(hwSrc)'),([0x19],'ADD HL,DE'),([0x56],'LD D,(HL)'),
 ([0x1E,0x00],'LD E,0'),([0x06,0x01],'LD B,1        Zielmaske'),
 'L:bit',([0x7A],'LD A,D'),([0xA1],'AND C'),([0x28,R('b0')],'JR Z,b0'),([0x3E,0xFF],'LD A,FFh'),
 'L:b0',([0x32,K(0)],'LD (hwK+0),A'),([0x3A,W('hwAa')],'LD A,(hwAa)   z1'),([0x32,K(1)],'LD (hwK+1),A'),
 'L:zl',([0x3A,K(0)],'LD A,(hwK+0)'),([0xA0],'AND B'),([0xB3],'OR E'),([0x5F],'LD E,A'),
 ([0xCB,0x00],'RLC B'),([0x30,R('nw')],'JR NC,nw'),
 ([0xDD,0x73,0x00],'LD (IX+0),E'),([0xDD,0x23],'INC IX'),([0x1E,0x00],'LD E,0'),
 'L:nw',([0xE5],'PUSH HL'),([0x2A,W('hwW')],'LD HL,(hwW)   m'),([0x2B],'DEC HL'),([0x22,W('hwW')],'LD (hwW),HL'),
 ([0x7C],'LD A,H'),([0xB5],'OR L'),([0xE1],'POP HL'),([0x28,R('fertig')],'JR Z,fertig'),
 ([0x3A,K(1)],'LD A,(hwK+1)'),([0x3D],'DEC A'),([0x32,K(1)],'LD (hwK+1),A'),([0x20,R('zl')],'JR NZ,zl'),
 ([0xCB,0x01],'RLC C'),([0x30,R('ns')],'JR NC,ns'),([0x23],'INC HL'),([0x56],'LD D,(HL)'),
 'L:ns',([0xE5],'PUSH HL'),([0x2A,W('hwNb')],'LD HL,(hwNb)  n'),([0x2B],'DEC HL'),([0x22,W('hwNb')],'LD (hwNb),HL'),
 ([0x7C],'LD A,H'),([0xB5],'OR L'),([0xE1],'POP HL'),([0x20,R('bit')],'JR NZ,bit'),
 'L:fertig',([0x78],'LD A,B'),([0xFE,0x01],'CP 1'),([0x28,R('raus')],'JR Z,raus'),([0xDD,0x73,0x00],'LD (IX+0),E'),
 'L:raus',([0xDD,0xE1],'POP IX'),
]
print(pas(p))

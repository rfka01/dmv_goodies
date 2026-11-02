"""z80list.py - ghlist fuer DMVHWC.INC (Z80): GDC-Liste ausgeben.
Aufruf: python3 z80list.py  (gibt den inline-Block aus)
Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT"""
import sys, os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from z80inl import pas
R=lambda n:('rel',n); W=lambda n:('w',n); WO=lambda n,o:('wo',n,o)
glist=[
 ([0x2A,W('hwSrc')],'LD HL,(hwSrc)'),
 'L:satz',
 ([0x7E],'LD A,(HL)      Anzahl'),
 ([0xB7],'OR A'),
 ([0x28,R('ende')],'JR Z,ende'),
 ([0x47],'LD B,A'),
 ([0x23],'INC HL'),
 'L:w1',
 ([0xDB,0xA0],'IN A,(A0h)'),
 ([0xE6,0x02],'AND 2          FIFO voll?'),
 ([0x20,R('w1')],'JR NZ,w1'),
 ([0x7E],'LD A,(HL)'),
 ([0xD3,0xA1],'OUT (A1h),A    Befehl'),
 ([0x23],'INC HL'),
 ([0x05],'DEC B'),
 ([0x28,R('satz')],'JR Z,satz'),
 'L:w2',
 ([0xDB,0xA0],'IN A,(A0h)'),
 ([0xE6,0x02],'AND 2'),
 ([0x20,R('w2')],'JR NZ,w2'),
 ([0x7E],'LD A,(HL)'),
 ([0xD3,0xA0],'OUT (A0h),A    Parameter'),
 ([0x23],'INC HL'),
 ([0x10,R('w2')],'DJNZ w2'),
 ([0x18,R('satz')],'JR satz'),
 'L:ende',
]
print(pas(glist))

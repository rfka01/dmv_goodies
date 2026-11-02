"""z80blit.py - hwBlitZ fuer DMVHWC.INC (Z80): ein Eintrag der Bildtabelle
fuer eine Zeile direkt an den 7220 (CURS ohne Punkt, WDAT, Worte), wie
HOPPLERs BlitAsm. Die Schleife ueber Zeilen und Eintraege macht ghblit in
Pascal. Eintrag ab hwXe (6 Worte): Ebenenadresse, WDAT-Befehl, Maske
(0 = aus der Zeile, sonst Adresse von FFh-Bytes), XOR, Versatz der Ebene,
1 = lauter Nullen. Zeile ab hwXp, Wortadresse hwXa, hwXw Worte, hwXm
maskiert das letzte Wort.
Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT"""
import sys, os; sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from z80inl import pas
R=lambda n:('rel',n); W=lambda n:('w',n); WO=lambda n,o:('wo',n,o)
zahl=[0]
def warte():
    zahl[0]+=1; l='w%d'%zahl[0]
    return ['L:'+l,([0xDB,0xA0],'IN A,(A0h)'),([0xE6,0x02],'AND 2'),([0x20,R(l)],'JR NZ,'+l)]
def par(r, code, name):                # OUT (C),r mit C = A0h
    return warte()+[([0xED,code],'OUT (C),'+name)]
def bef_a(cmt):                        # A nach A1h; A vorher in D sichern
    return [([0x57],'LD D,A')]+warte()+[([0x7A],'LD A,D'),([0xD3,0xA1],'OUT (A1h),A  '+cmt)]
def byte(n):                           # ein Byte: (Ebene xor X) and Maske
    q=[([0xDD,0x7E,0x00],'LD A,(IX+0)  Ebene'),([0xAB],'XOR E'),([0xA6],'AND (HL)     Maske'),
       ([0x57],'LD D,A'),
       ([0x78],'LD A,B'),([0x3D],'DEC A'),([0x20,R('k%d'%n)],'JR NZ,k%d'%n),
       ([0x3A,WO('hwXm',n)],'LD A,(hwXm+%d) letztes Wort'%n),([0xA2],'AND D'),([0x57],'LD D,A'),
       'L:k%d'%n]
    q+=par('D',0x51,'D')
    q+=[([0x23],'INC HL'),([0xDD,0x23],'INC IX')]
    return q
p=[([0xDD,0xE5],'PUSH IX'),([0xFD,0xE5],'PUSH IY'),
   ([0xFD,0x2A,W('hwXe')],'LD IY,(hwXe)  Eintrag'),
   ([0x0E,0xA0],'LD C,A0h')]
p+=[([0x3E,0x49],'LD A,49h')]+bef_a('CURS')
p+=[([0x2A,W('hwXa')],'LD HL,(hwXa)'),([0xFD,0x5E,0x00],'LD E,(IY+0)'),([0xFD,0x56,0x01],'LD D,(IY+1)'),
    ([0x19],'ADD HL,DE')]
p+=par('L',0x69,'L')+par('H',0x61,'H')
p+=[([0xFD,0x7E,0x02],'LD A,(IY+2)')]+bef_a('WDAT')
p+=[([0xFD,0x6E,0x04],'LD L,(IY+4)  Maske'),([0xFD,0x66,0x05],'LD H,(IY+5)'),
    ([0x7C],'LD A,H'),([0xB5],'OR L'),([0x20,R('mz')],'JR NZ,mz'),
    ([0x2A,W('hwXp')],'LD HL,(hwXp) Maske aus der Zeile'),'L:mz',
    ([0xDD,0x2A,W('hwXp')],'LD IX,(hwXp)'),
    ([0xFD,0x5E,0x08],'LD E,(IY+8)  Versatz'),([0xFD,0x56,0x09],'LD D,(IY+9)'),
    ([0xDD,0x19],'ADD IX,DE'),
    ([0xFD,0x5E,0x06],'LD E,(IY+6)  X'),
    ([0x3A,W('hwXw')],'LD A,(hwXw)'),([0x47],'LD B,A'),
    ([0xFD,0x7E,0x0A],'LD A,(IY+10) Nullen?'),([0xB7],'OR A'),([0x20,R('null')],'JR NZ,null'),
    'L:wort']
p+=byte(0)+byte(1)
p+=[([0x10,R('wort')],'DJNZ wort'),([0x18,R('ende')],'JR ende'),
    'L:null',([0x16,0x00],'LD D,0')]
p+=['L:nw']+par('D',0x51,'D')+par('D',0x51,'D')+[([0x10,R('nw')],'DJNZ nw'),
    'L:ende',([0xFD,0xE1],'POP IY'),([0xDD,0xE1],'POP IX')]
print(pas(p))

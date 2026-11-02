"""z80inl.py - kleiner Z80-Assembler fuer Turbo-Pascal-3-inline: ein
Programm ist eine Liste aus Marken ('L:name') und Eintraegen
(Bytes, Kommentar); ein Byte kann ('rel', marke) fuer einen relativen
Sprung, ('w', variable) oder ('wo', variable, versatz) fuer eine
Pascal-Variable sein (>name in TP3). pas() liefert den inline-Block.
Code: Claude (Anthropic), Idee und Prompts: rfka01. Lizenz: MIT"""
import sys
def asm(prog):
    # prog: list of items: 'L:name' label, or (list_of_tokens, comment)
    # token: int byte, ('rel','name') rel jump byte, ('w','pascalname') word ref, ('wo','name',off)
    pc=0; lab={}
    for it in prog:
        if isinstance(it,str): lab[it[2:]]=pc; continue
        for t in it[0]: pc += 2 if isinstance(t,tuple) and t[0] in('w','wo') else 1
    pc=0; out=[]
    for it in prog:
        if isinstance(it,str): out.append((None,'{ %s: }'%it[2:])); continue
        toks=[]
        for t in it[0]:
            if isinstance(t,int): toks.append('$%02X'%t); pc+=1
            elif t[0]=='rel':
                d=lab[t[1]]-(pc+1); assert -128<=d<=127; toks.append('$%02X'%(d&0xFF)); pc+=1
            elif t[0]=='w': toks.append('>'+t[1]); pc+=2
            elif t[0]=='wo': toks.append('>%s+%d'%(t[1],t[2])); pc+=2
        out.append(('/'.join(toks), it[1]))
    return out
def pas(prog, ind='  '):
    o=asm(prog); lines=[]
    toks=[x for x in o if x[0]]
    res=[ind+'inline(']
    for i,(t,c) in enumerate(o):
        if t is None: res.append(ind+'  '+c); continue
        last = (t,c)==toks[-1] and i==max(j for j,x in enumerate(o) if x[0])
        res.append(ind+'  %-34s{ %s }'%(t+('' if last else '/'), c))
    res.append(ind+');')
    return '\n'.join(res)

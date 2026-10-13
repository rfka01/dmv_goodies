"""Die langen Hilfetexte, die in der DMV-Fassung fehlen, als Klartext ausgeben."""
import re, sys

def dbtext(lines):
    out = ''
    for l in lines:
        m = re.match(r'\s*(\w+)?\s+db\s+(.*)', l)
        if not m:
            continue
        arg = m.group(2)
        # Argumente zerlegen: Strings und Zahlen/Namen
        for tok in re.findall(r"'(?:[^']|'')*'|[^,]+", arg):
            tok = tok.strip()
            if tok.startswith(';'):
                break
            if tok.startswith("'"):
                out += tok[1:-1].replace("''", "'")
            elif tok.lower() == 'cr':
                pass
            elif tok.lower() == 'lf':
                out += '\n'
            elif re.match(r'27h', tok):
                out += "'"
    return out.split('$')[0]

def block(f, label):
    L = open(f, errors='replace').read().split('\n')
    # die Fassung im else-Zweig (Original) nehmen: letzte Definition
    idx = [i for i, l in enumerate(L) if re.match(label + r'\s+db\b', l)]
    i = idx[-1]
    j = i + 1
    while j < len(L) and not L[j].startswith('endif'):
        if re.match(r'\w+\s+(db|dw|equ|label)\b', L[j]):
            break
        j += 1
    return dbtext([x for x in L[i:j] if not re.match(r'\s*(ifn?def|else|endif)', x)])

teile = [
    ('Commands ("?" at the MS-Kermit> prompt)', 'mssker.asm', 'tophlp'),
    ('SET ?', 'mssset.asm', 'sethlp'),
    ('SET TERMINAL ?', 'msyibm.asm', 'vthlp'),
    ('SET TERMINAL COLOR ?', 'msyibm.asm', 'colhlp'),
    ('REMOTE ?', 'mssser.asm', 'remhlp'),
    ('SET KEY ? (key)', 'msuibm.asm', 'dfhelp1'),
    ('SET KEY key ? (definition)', 'msuibm.asm', 'dfhelp2'),
]
for titel, f, lab in teile:
    print('=' * 72)
    print(titel)
    print('=' * 72)
    import os
    quelle = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src', f.upper())
    print(block(quelle, lab).strip('\n'))
    print()

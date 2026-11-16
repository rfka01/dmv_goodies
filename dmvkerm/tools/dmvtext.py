"""UTF-8-Text -> DOS-Text fuer die DMV.

Die Texte liegen im Repository als UTF-8 mit Umlauten. Auf der Diskette
brauchen sie die Codes der DMV-Konsole (NCR CRTTBL/GERMANY, DOS 2.11 und
DMVCON ab 1.4): ae 90h, oe 96h, ue 99h, Ae 80h, Oe 86h, Ue 89h, ss 9Eh.
Andere Zeichen ausserhalb von ASCII gibt es auf der DMV nicht; sie fuehren
zu einem Fehler statt zu einem falschen Zeichen auf der Diskette.
"""

DMV = {'ä': 0x90, 'ö': 0x96, 'ü': 0x99, 'Ä': 0x80, 'Ö': 0x86, 'Ü': 0x89, 'ß': 0x9E}


def dmv_bytes(s, name='Text'):
    """Zeichenkette -> Bytes im DMV-Zeichensatz."""
    out = bytearray()
    for nr, zeile in enumerate(s.split('\n'), 1):
        for ch in zeile:
            if ch in DMV:
                out.append(DMV[ch])
            elif ord(ch) < 128:
                out.append(ord(ch))
            else:
                raise ValueError('%s, Zeile %d: Zeichen %r gibt es auf der DMV nicht'
                                 % (name, nr, ch))
        out.append(10)
    return bytes(out[:-1])


def dostext(pfad, ctrlz=True):
    """UTF-8-Datei -> DOS-Text mit CRLF (und Ctrl-Z) im DMV-Zeichensatz."""
    s = open(pfad, 'rb').read().decode('utf-8').replace('\r', '')
    s = s.rstrip('\x1a')                  # ein vorhandenes Ctrl-Z nicht doppelt
    b = dmv_bytes(s, pfad).replace(b'\n', b'\r\n')
    return b + (b'\x1a' if ctrlz else b'')

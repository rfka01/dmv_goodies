#!/usr/bin/env python3
"""mktut.py FOTO OUT.PIC [vorschau.png] - King Tut (Mark Fischer, Wikimedia
Commons) als 8-Farben-Bild fuer die DMV: Ausschnitt wie das Amiga-Bild,
360 x 400 Punkte in der Mitte von 640 x 400, Floyd-Steinberg auf die acht
Farben der DMV. Ausgabe wie KITT.PIC: 4 Ebenen zu 32000 Bytes (gruen, rot,
blau, mono), je Zeile 40 Worte, Bit 0 = linker Punkt."""
import sys
from PIL import Image, ImageEnhance
src, out = sys.argv[1], sys.argv[2]
im = Image.open(src).convert('RGB')
w, h = im.size; s = w / 4000
c = im.crop((int(1380*s), int(140*s), int(2560*s), int(1600*s))).resize((360, 400), Image.LANCZOS)
c = ImageEnhance.Color(c).enhance(1.1)
c = ImageEnhance.Contrast(c).enhance(1.15)
# Lapis-Streifen sind im Foto fast schwarz: blaeuliche Punkte aufhellen
def lapis(p):
    r, g, b = p
    if b > r + 8 and b > g:
        k = 1.9
        return (int(min(255, r*1.2)), int(min(255, g*1.3)), int(min(255, b*k)))
    return p
c = Image.eval(c, lambda v: int(255 * (v/255) ** 0.85))
c.putdata([lapis(p) for p in c.getdata()])
W, H = 640, 400; X0 = 140
px = [[(0.0, 0.0, 0.0)] * W for _ in range(H)]
for y in range(H):
    for x in range(360):
        px[y][X0 + x] = tuple(float(v) for v in c.getpixel((x, y)))
pal = [(r*255, g*255, b*255) for b in (0, 1) for g in (0, 1) for r in (0, 1)]  # Index = R|G<<1|B<<2
idx = [[0] * W for _ in range(H)]
ALLOW = (0, 1, 3, 7, 4, 6)          # schwarz, rot, gelb, weiss, blau, cyan - kein Gruen/Magenta-Rauschen
ATT = 0.85                          # Fehler etwas daempfen
for y in range(H):
    for x in range(W):
        r, g, b = px[y][x]
        best = min(ALLOW, key=lambda i: (pal[i][0]-r)**2*0.30 + (pal[i][1]-g)**2*0.59 + (pal[i][2]-b)**2*0.11)
        idx[y][x] = best
        er, eg, eb = (r - pal[best][0])*ATT, (g - pal[best][1])*ATT, (b - pal[best][2])*ATT
        for dx, dy, f in ((1, 0, 7/16), (-1, 1, 3/16), (0, 1, 5/16), (1, 1, 1/16)):
            xx, yy = x + dx, y + dy
            if 0 <= xx < W and yy < H:
                q = px[yy][xx]; px[yy][xx] = (q[0]+er*f, q[1]+eg*f, q[2]+eb*f)
# Mono: Helligkeit, Floyd-Steinberg schwarz/weiss
gray = [[0.0]*W for _ in range(H)]
for y in range(H):
    for x in range(360):
        r, g, b = c.getpixel((x, y)); gray[y][X0+x] = 0.30*r + 0.59*g + 0.11*b
mono = [[0]*W for _ in range(H)]
for y in range(H):
    for x in range(W):
        v = gray[y][x]; o = 255 if v >= 128 else 0; mono[y][x] = 1 if o else 0
        e = v - o
        for dx, dy, f in ((1, 0, 7/16), (-1, 1, 3/16), (0, 1, 5/16), (1, 1, 1/16)):
            xx, yy = x + dx, y + dy
            if 0 <= xx < W and yy < H: gray[yy][xx] += e*f
def plane(get):
    b = bytearray()
    for y in range(H):
        for wx in range(40):
            v = 0
            for k in range(16):
                if get(y, wx*16 + k): v |= 1 << k
            b += bytes((v & 255, v >> 8))
    return b
data = plane(lambda y, x: idx[y][x] & 2) + plane(lambda y, x: idx[y][x] & 1) + plane(lambda y, x: idx[y][x] & 4) + plane(lambda y, x: mono[y][x])
open(out, 'wb').write(data)
if len(sys.argv) > 3:
    p = Image.new('RGB', (W, H))
    for y in range(H):
        for x in range(W): p.putpixel((x, y), tuple(int(v) for v in pal[idx[y][x]]))
    p.save(sys.argv[3])
    m = Image.new('L', (W, H))
    for y in range(H):
        for x in range(W): m.putpixel((x, y), 255*mono[y][x])
    m.save(sys.argv[3].replace('.png', '_mono.png'))
print(out, len(data))

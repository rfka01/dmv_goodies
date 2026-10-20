# Build a 360 KB FAT12 floppy image (40 tracks, 2 sides, 9 sectors of 512 bytes, media FDh)
# usage: mkimg.py image.img label file...
import os, struct, sys, time

SECTOR = 512
TOTAL = 720
SPC = 2              # sectors per cluster
RESERVED = 1
NFATS = 2
ROOT_ENTRIES = 112
FAT_SECTORS = 2
MEDIA = 0xFD

root_sectors = ROOT_ENTRIES * 32 // SECTOR
data_start = RESERVED + NFATS * FAT_SECTORS + root_sectors
clusters = (TOTAL - data_start) // SPC


def dos_name(path):
    base = os.path.basename(path).upper()
    name, _, ext = base.partition('.')
    assert 1 <= len(name) <= 8 and len(ext) <= 3, base
    return name.ljust(8).encode('ascii') + ext.ljust(3).encode('ascii')


def dos_datetime(t):
    lt = time.localtime(t)
    d = ((lt.tm_year - 1980) << 9) | (lt.tm_mon << 5) | lt.tm_mday
    tm = (lt.tm_hour << 11) | (lt.tm_min << 5) | (lt.tm_sec // 2)
    return tm, d


def main():
    out, label, files = sys.argv[1], sys.argv[2], sys.argv[3:]
    img = bytearray(TOTAL * SECTOR)

    # boot sector with BPB (not bootable)
    bs = bytearray(SECTOR)
    bs[0:3] = b'\xeb\x3c\x90'
    bs[3:11] = b'DMV-TEST'
    struct.pack_into('<HBHBHHBHHHII', bs, 11, SECTOR, SPC, RESERVED, NFATS, ROOT_ENTRIES, TOTAL,
                     MEDIA, FAT_SECTORS, 9, 2, 0, 0)
    struct.pack_into('<BBBI', bs, 36, 0, 0, 0x29, 0x19861010)
    bs[43:54] = label.upper().ljust(11).encode('ascii')[:11]
    bs[54:62] = b'FAT12   '
    msg = b'\r\nThis is not a system disk.\r\n$'
    bs[62:62 + len(msg)] = msg
    bs[62 + len(msg):62 + len(msg) + 2] = b'\xf4\xeb'      # hlt / jmp (never executed)
    bs[510:512] = b'\x55\xaa'
    img[0:SECTOR] = bs

    fat = [0] * (clusters + 2)
    fat[0] = 0xF00 | MEDIA
    fat[1] = 0xFFF
    root = bytearray(root_sectors * SECTOR)
    entry = 0
    now = time.time()

    # volume label
    root[0:11] = label.upper().ljust(11).encode('ascii')[:11]
    root[11] = 0x08
    tm, dt = dos_datetime(now)
    struct.pack_into('<HH', root, 22, tm, dt)
    entry = 1

    next_cluster = 2
    for path in files:
        data = open(path, 'rb').read()
        n = (len(data) + SPC * SECTOR - 1) // (SPC * SECTOR)
        first = next_cluster if n else 0
        for k in range(n):
            c = next_cluster + k
            fat[c] = (c + 1) if k < n - 1 else 0xFFF
            off = (data_start + (c - 2) * SPC) * SECTOR
            chunk = data[k * SPC * SECTOR:(k + 1) * SPC * SECTOR]
            img[off:off + len(chunk)] = chunk
        next_cluster += n
        assert next_cluster <= clusters + 2, 'disk full'
        e = bytearray(32)
        e[0:11] = dos_name(path)
        e[11] = 0x20
        tm, dt = dos_datetime(os.path.getmtime(path))
        struct.pack_into('<HHHI', e, 22, tm, dt, first, len(data))
        root[entry * 32:entry * 32 + 32] = e
        entry += 1

    # pack FAT12
    fb = bytearray(FAT_SECTORS * SECTOR)
    for i in range(0, len(fat), 2):
        a = fat[i]
        b = fat[i + 1] if i + 1 < len(fat) else 0
        o = i * 3 // 2
        if o + 2 < len(fb):
            fb[o] = a & 0xff
            fb[o + 1] = ((a >> 8) & 0x0f) | ((b & 0x0f) << 4)
            fb[o + 2] = (b >> 4) & 0xff
    for k in range(NFATS):
        off = (RESERVED + k * FAT_SECTORS) * SECTOR
        img[off:off + len(fb)] = fb
    off = (RESERVED + NFATS * FAT_SECTORS) * SECTOR
    img[off:off + len(root)] = root

    open(out, 'wb').write(img)
    used = (next_cluster - 2) * SPC * SECTOR
    print('%s: %d files, %d bytes used, %d bytes free' % (out, len(files), used, (clusters + 2 - next_cluster) * SPC * SECTOR))


main()

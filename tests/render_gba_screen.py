#!/usr/bin/env python3
# Renders dumps from test_gba_screen (mode 0 BG0, 4bpp tiles in charblock 0, map in screenblock 4) to 3x scaled PNGs.
# No dependencies, writes the PNG itself.
import sys, struct, zlib
def rgb(c): return ((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31)
def render(src, dst, scale=3):
    d = open(src, 'rb').read()
    vram, pal = d[:0x3000], struct.unpack('<256H', d[0x3000:0x3200])
    W, H = 240, 160
    rows = []
    for y in range(H):
        row = bytearray()
        for x in range(W):
            e = struct.unpack_from('<H', vram, 0x2000 + ((y//8)*32 + x//8)*2)[0]
            tile, palno = e & 0x3FF, e >> 12
            b = vram[tile*32 + (y%8)*4 + (x%8)//2]
            idx = (b >> 4) if x % 2 else (b & 15)
            c = rgb(pal[0] if idx == 0 else pal[palno*16 + idx])
            row += bytes(c) * scale
        for _ in range(scale): rows.append(b'\x00' + bytes(row))
    def chunk(t, data): return struct.pack('>I', len(data)) + t + data + struct.pack('>I', zlib.crc32(t + data) & 0xffffffff)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W*scale, H*scale, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(b''.join(rows))) + chunk(b'IEND', b'')
    open(dst, 'wb').write(png)
for f in sys.argv[1:]: render(f, f.replace('.bin', '.png'))

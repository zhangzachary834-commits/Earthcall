#!/usr/bin/env python3
"""Write a placeholder 1024x1024 AppIcon PNG until Zach supplies artwork.

A deep-blue sphere under a thin gold ring of light, on transparency. Pure
standard library (zlib + struct), so a fresh clone needs nothing installed.
Replace by dropping real artwork at scripts/macos/AppIcon.png.

Usage: make_placeholder_icon.py <out.png>
"""
import math
import struct
import sys
import zlib

N = 1024


def pixel(x: float, y: float) -> tuple[int, int, int, int]:
    cx = cy = N / 2
    dx, dy = (x - cx) / (N * 0.40), (y - cy) / (N * 0.40)
    r = math.hypot(dx, dy)
    ring = abs(math.hypot((x - cx) / (N * 0.46), (y - cy) / (N * 0.46)) - 1.0)
    if r <= 1.0:
        z = math.sqrt(max(0.0, 1.0 - r * r))
        light = max(0.0, 0.35 * -dx + 0.55 * -dy + 0.75 * z)
        return (int(18 + 60 * light), int(52 + 110 * light), int(120 + 125 * light), 255)
    if ring < 0.012:
        a = int(255 * (1.0 - ring / 0.012))
        return (238, 196, 92, a)
    return (0, 0, 0, 0)


def main() -> int:
    out = sys.argv[1]
    rows = bytearray()
    for y in range(N):
        rows.append(0)
        for x in range(N):
            rows.extend(pixel(x + 0.5, y + 0.5))

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", N, N, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    png += chunk(b"IEND", b"")
    with open(out, "wb") as f:
        f.write(png)
    return 0


if __name__ == "__main__":
    sys.exit(main())

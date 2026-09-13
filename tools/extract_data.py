#!/usr/bin/env python3
"""
Экстрактор основного архива ресурсов Data.dat игры "Великолепный Вовочка".
Порт udatadat.c (c) CTPAX-X Team 2011,2022.

Использование:
    python tools/extract_data.py [Data.dat] [outdir] [--xorkey xorkey.c]

Таблица xorkey не вшита: читается из xorkey.c рядом со скриптом (или --xorkey),
"""
import argparse
import re
import struct
import sys
from pathlib import Path

M = 0xFFFFFFFF
XORKEY_MIN = 1058          # максимальный используемый индекс: 802 + 255 = 1057


def load_xorkey(path: Path):
    """Парсит массив uint32 из xorkey.c; склеивает литералы, побитые пробелами."""
    if not path.is_file():
        sys.exit(f"Error: xorkey table not found: {path}\n"
                 f"Put xorkey.c next to this script or pass --xorkey.")
    text = path.read_text(encoding="utf-8", errors="ignore")
    toks = re.findall(r"0x[0-9A-Fa-f]+(?: [0-9A-Fa-f]+)?", text)
    key = [int(t.replace(" ", ""), 16) & M for t in toks]
    if len(key) < XORKEY_MIN:
        sys.exit(f"Error: xorkey.c gave {len(key)} values, need >= {XORKEY_MIN}.")
    return key


def decrypt1(buf: bytearray, key) -> None:
    """Порт vv1decrypt1: блочный шифр по парам uint32, 16 раундов, mod 2^32."""
    nwords = len(buf) // 4
    if nwords < 2:
        return
    w = list(struct.unpack_from("<%dI" % nwords, buf, 0))
    for i in range(nwords // 2):
        bi = 2 * i
        ai = bi + 1
        a = w[ai]
        b = w[bi]
        for j in range(17, 1, -1):
            b = (b ^ key[j]) & M
            c = key[34 + ((b >> 24) & 0xFF)]
            c = (c + key[290 + ((b >> 16) & 0xFF)]) & M
            c = (c ^ key[546 + ((b >> 8) & 0xFF)]) & M
            c = (c + key[802 + (b & 0xFF)]) & M
            a = (a ^ c) & M
            a, b = b, a
        a, b = b, a
        a = (a ^ key[1]) & M
        b = (b ^ key[0]) & M
        w[ai] = a
        w[bi] = b
    struct.pack_into("<%dI" % nwords, buf, 0, *w)


def main() -> int:
    ap = argparse.ArgumentParser(description="Vovochka Data.dat extractor")
    ap.add_argument("archive", nargs="?", default="Data.dat")
    ap.add_argument("outdir", nargs="?", default=".")
    ap.add_argument("--xorkey", default=None)
    args = ap.parse_args()

    key = load_xorkey(Path(args.xorkey) if args.xorkey
                      else Path(__file__).resolve().parent / "xorkey.c")

    data = Path(args.archive).read_bytes()
    if len(data) < 8:
        print('Error: can\'t open / too small archive file.')
        return 1

    # 1. Размер индекса НЕ зашифрован (в отличие от movi.dat)
    (idx_size,) = struct.unpack_from("<I", data, 0)
    if 4 + idx_size > len(data):
        print("Error: bad index size.")
        return 2

    index = bytearray(data[4:4 + idx_size])
    decrypt1(index, key)

    # 2. FAT/TOC: DWORD count(+1); затем WORD len, char[len] name, DWORD size, DWORD offs
    (count,) = struct.unpack_from("<I", index, 0)
    base = 4 + idx_size          # offs считается от конца FAT/TOC
    out = Path(args.outdir)

    j = 4
    i = count
    extracted = 0
    while i > 1 and j + 2 <= len(index):
        (namelen,) = struct.unpack_from("<H", index, j)
        name_b = index[j + 2: j + 2 + namelen]
        j += 2 + namelen
        (size,) = struct.unpack_from("<I", index, j)
        j += 4
        (offs,) = struct.unpack_from("<I", index, j)
        j += 4

        name = name_b.decode("cp866", errors="replace").replace("\\", "/")
        chunk = bytearray(data[base + offs: base + offs + size])
        if len(chunk) < size:
            print(f"{name} - truncated in archive, skipped")
            i -= 1
            continue

        decrypt1(chunk, key)

        # 3. Убираем паддинг: реальный размер = size - (last_byte + 1)
        real = size - (chunk[size - 1] + 1) if size else 0

        dst = out / name
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes(bytes(chunk[:real]))
        print(f"{name} ({real} bytes) OK")
        extracted += 1
        i -= 1

    print(f"\ndone: {extracted} files -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
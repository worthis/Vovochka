#!/usr/bin/env python3
"""
Экстрактор видео-архива movi.dat игры "Великолепный Вовочка".
Порт movi_rip.c (c) CTPAX-X Team: индекс оффсетов в начале файла,
каждый байт архива зашифрован вычитанием 169.

Использование:
    python tools/extract_movi.py [путь_к_movi.dat] [папка_назначения]
По умолчанию:
    python tools/extract_movi.py movi.dat .
"""
import struct
import sys
from pathlib import Path

KEY = 169

# Шифр — побайтовая подстановка (b - 169) mod 256:
# строим таблицу один раз и декодируем через bytes.translate (C-скорость)
_DECRYPT_TABLE = bytes((b - KEY) % 256 for b in range(256))


def decrypt(data: bytes) -> bytes:
    return data.translate(_DECRYPT_TABLE)


def main() -> int:
    in_path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("movi.dat")
    out_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else Path(".")

    if not in_path.is_file():
        print(f'Error: can\'t open "{in_path}" archive file.')
        return 1

    out_dir.mkdir(parents=True, exist_ok=True)

    with open(in_path, "rb") as f:
        # 1. Размер индекса: первые 4 байта, зашифрованы
        head = decrypt(f.read(4))
        if len(head) < 4:
            print("Error: file too small, not an archive.")
            return 2
        (index_size,) = struct.unpack("<I", head)

        # 2. Весь индекс лежит с начала файла
        f.seek(0)
        index = decrypt(f.read(index_size))
        offsets = struct.unpack("<%dI" % (index_size // 4), index)

        # 3. Файлы: размер = разница соседних оффсетов
        count = index_size // 4 - 1
        extracted = 0

        for i in range(count):
            ofs1, ofs2 = offsets[i], offsets[i + 1]
            if ofs1 > ofs2:          # хвост индекса (filesize, 0, 0, 0...)
                break

            size = ofs2 - ofs1
            f.seek(ofs1)
            chunk = decrypt(f.read(size))

            name = "MOVI%04d.AVI" % (i + 1)
            (out_dir / name).write_bytes(chunk)

            # Самопроверка: расшифрованный AVI обязан начинаться с RIFF....AVI
            ok = chunk[:4] == b"RIFF" and chunk[8:12] == b"AVI "
            print(f"{name} ({size} bytes) ... {'OK' if ok else 'WARNING: not a RIFF/AVI!'}")
            extracted += 1

    print(f"\ndone: {extracted} files -> {out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
dump_data_tree.py — дамп полной структуры папки data игры в txt.

Использование:
  python dump_data_tree.py [DATA_DIR] [OUT_TXT] [--md5] [--no-size]

По умолчанию (из D:\\Projects\\GitHub\\Vovochka):
  python tools\\dump_data_tree.py data data_structure.txt --md5
"""

import argparse
import hashlib
import os
import sys
import time


def human_size(n: int) -> str:
    if n < 1024:
        return f"{n} B"
    for unit in ("KB", "MB", "GB"):
        n /= 1024.0
        if n < 1024 or unit == "GB":
            return f"{n:.1f} {unit}"
    return f"{n:.1f} GB"


def md5_of(path: str, chunk: int = 1 << 20) -> str:
    h = hashlib.md5()
    try:
        with open(path, "rb") as f:
            for b in iter(lambda: f.read(chunk), b""):
                h.update(b)
        return h.hexdigest()
    except OSError:
        return "read-error"


def main():
    ap = argparse.ArgumentParser(description="Dump game data folder tree to txt")
    ap.add_argument("data_dir", nargs="?", default="data",
                    help="корневая папка data (по умолчанию ./data)")
    ap.add_argument("out_txt", nargs="?", default="data_structure.txt",
                    help="выходной txt (по умолчанию data_structure.txt)")
    ap.add_argument("--md5", action="store_true",
                    help="считать MD5 для файлов <= 32 МБ (полезно для сверки ассетов)")
    ap.add_argument("--no-size", action="store_true", help="не выводить размеры")
    args = ap.parse_args()

    root = os.path.abspath(args.data_dir)
    if not os.path.isdir(root):
        print(f"[ERROR] папка не найдена: {root}")
        sys.exit(1)

    lines = [
        "# Структура папки data игры Vovochka",
        f"# Root: {root}",
        f"# Date: {time.strftime('%Y-%m-%d %H:%M:%S')}",
        "",
        f"{os.path.basename(root)}/",
    ]
    stats = {"dirs": 0, "files": 0, "total": 0}

    def walk(dirpath: str, prefix: str):
        try:
            entries = sorted(
                os.scandir(dirpath),
                key=lambda e: (not e.is_dir(), e.name.lower()),
            )
        except OSError as e:
            lines.append(f"{prefix}├── <error: {e}>")
            return

        for i, entry in enumerate(entries):
            last = i == len(entries) - 1
            conn = "└── " if last else "├── "
            child_prefix = prefix + ("    " if last else "│   ")

            if entry.is_dir():
                stats["dirs"] += 1
                lines.append(f"{prefix}{conn}{entry.name}/")
                walk(entry.path, child_prefix)
            else:
                stats["files"] += 1
                try:
                    size = entry.stat().st_size
                except OSError:
                    size = -1
                if size >= 0:
                    stats["total"] += size

                extra = ""
                if not args.no_size and size >= 0:
                    extra = f"  [{human_size(size)}]"
                if args.md5 and 0 <= size <= 32 * 1024 * 1024:
                    extra += f"  md5={md5_of(entry.path)}"

                lines.append(f"{prefix}{conn}{entry.name}{extra}")

    walk(root, "")
    lines += [
        "",
        f"# Итого: папок={stats['dirs']}, файлов={stats['files']}, "
        f"объём={human_size(stats['total'])}",
    ]

    with open(args.out_txt, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

    print(f"OK: {args.out_txt} "
          f"(папок={stats['dirs']}, файлов={stats['files']}, {human_size(stats['total'])})")


if __name__ == "__main__":
    main()
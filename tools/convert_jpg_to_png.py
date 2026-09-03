#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Конвертирует все JPG/JPEG в data/ в PNG рядом с оригиналом.
Оригинальные .JPG остаются на месте (лоадер сам перейдёт на .png)."""
import pathlib
import sys

from PIL import Image

def main():
    src = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "data")
    n = 0
    for p in sorted(src.rglob("*")):
        if not p.is_file() or p.suffix.lower() not in (".jpg", ".jpeg"):
            continue
        out = p.with_suffix(".png")          # BG1.JPG -> BG1.png
        try:
            with Image.open(p) as im:
                im.convert("RGB").save(out, "PNG")
            n += 1
            print(f"  [ok] {p} -> {out}")
        except Exception as e:
            print(f"  [fail] {p}: {e}")
    print(f"Done: {n} file(s) converted")

if __name__ == "__main__":
    main()
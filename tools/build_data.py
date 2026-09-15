#!/usr/bin/env python3
"""
Сборщик папки data из архивов gameassets/Data.dat и gameassets/movi.dat.
Запуск из корня проекта (где лежит папка gameassets и tools):
    python tools/build_data.py
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

# --- Конфигурация путей ---
ROOT_DIR = Path(__file__).resolve().parent.parent  # Корень проекта
TOOLS_DIR = Path(__file__).resolve().parent        # Папка tools
ASSETS_DIR = ROOT_DIR / "gameassets"               # Входная папка с архивами
DATA_DIR = ROOT_DIR / "data"                       # Выходная папка
VIDEO_DIR = DATA_DIR / "VIDEO"                     # Папка для видео

# Файлы для удаления из MENUSOUNDS
UNUSED_SOUNDS = [
    "Down2.wav", "Down3.wav", "Down4.wav",
    "Move2.wav", "Move3.wav", "Move4.wav", "Move5.wav", "Move6.wav", "Move7.wav"
]

def run_script(script_name, args=None):
    """Запускает python-скрипт из папки tools."""
    script_path = TOOLS_DIR / script_name
    if not script_path.exists():
        print(f"[ERROR] Script not found: {script_path}")
        sys.exit(1)
    
    cmd = [sys.executable, str(script_path)] + (args or [])
    print(f"\n>>> Starting: {' '.join(cmd)}")
    
    result = subprocess.run(cmd, cwd=ROOT_DIR)
    if result.returncode != 0:
        print(f"[ERROR] Script {script_name} ended with error (code {result.returncode})")
        sys.exit(1)

def main():
    print(f"=== Build Vovochka ===")
    print(f"Root: {ROOT_DIR}")
    print(f"Assets: {ASSETS_DIR}")
    print(f"Result: {DATA_DIR}")

    # 1. Проверка входных файлов
    data_dat = ASSETS_DIR / "Data.dat"
    movi_dat = ASSETS_DIR / "movi.dat"
    
    if not data_dat.exists():
        print(f"[ERROR] Not found {data_dat}")
        sys.exit(1)
    if not movi_dat.exists():
        print(f"[ERROR] Not found {movi_dat}")
        sys.exit(1)

    # 2. Создание/очистка выходной папки
    if DATA_DIR.exists():
        print(f"\n[INFO] Deleting old folder {DATA_DIR}...")
        shutil.rmtree(DATA_DIR)
    DATA_DIR.mkdir(parents=True)
    VIDEO_DIR.mkdir(parents=True, exist_ok=True)
    print(f"[OK] Created folder {DATA_DIR}")

    # 3. Распаковка Data.dat
    print("\n--- Step 1: Unpacking Data.dat ---")
    run_script("extract_data.py", [str(data_dat), str(DATA_DIR)])

    # 4. Распаковка movi.dat
    print("\n--- Step 2: Unpacking movi.dat ---")
    run_script("extract_movi.py", [str(movi_dat), str(VIDEO_DIR)])

    # 5. Конвертация видео (AVI -> MPG)
    print("\n--- Step 3: Converting video ---")
    # convert_videos.py по умолчанию берет src из папки скрипта, 
    # но нам нужно указать явно src=data/VIDEO и out=data/VIDEO
    run_script("convert_videos.py", [
        "--src", str(VIDEO_DIR),
        "--out", str(VIDEO_DIR)
    ])

    # 6. Удаление исходных AVI и лишних MP3
    print("\n--- Step 4: Cleaning video ---")
    avi_count = 0
    mp3_count = 0
    for f in VIDEO_DIR.iterdir():
        if f.suffix.lower() == ".avi":
            f.unlink()
            avi_count += 1
        elif f.suffix.lower() == ".mp3":
            f.unlink()
            mp3_count += 1
    print(f"[OK] Deleted AVI: {avi_count}, MP3: {mp3_count}")

    # 7. Конвертация JPG -> PNG
    print("\n--- Step 5: Converting JPG -> PNG ---")
    run_script("convert_jpg_to_png.py", [str(DATA_DIR)])
    
    # Удаление исходных JPG (скрипт convert_jpg_to_png.py их не удаляет, делаем сами)
    jpg_count = 0
    for p in DATA_DIR.rglob("*"):
        if p.is_file() and p.suffix.lower() in (".jpg", ".jpeg"):
            p.unlink()
            jpg_count += 1
    print(f"[OK] Deleted JPG: {jpg_count}")

    # 8. Удаление лишних звуков меню
    print("\n--- Step 6: Cleaning MENUSOUNDS ---")
    menusounds_dir = DATA_DIR / "COMMON" / "MENUSOUNDS"
    removed_sounds = 0
    if menusounds_dir.exists():
        for name in UNUSED_SOUNDS:
            f = menusounds_dir / name
            if f.exists():
                f.unlink()
                removed_sounds += 1
                print(f"  [del] {name}")
    print(f"[OK] Deleted unused sounds: {removed_sounds}")

    print("\n=== Building completed! ===")
    print(f"Result: {DATA_DIR}")

if __name__ == "__main__":
    main()
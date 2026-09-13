#!/usr/bin/env python3
"""
Конвертер распакованных роликов MOVIxxxx.AVI (DivX5 + MP3) в MPEG-1 Program Stream
для плеера pl_mpeg + извлечение родной MP3-дорожки без перекодирования.

Использование:
    python tools/convert_videos.py [опции]

Значения по умолчанию:
    src = папка скрипта (там лежат MOVIxxxx.AVI)
    out = <папка скрипта>/../data/VIDEO
"""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def run(cmd) -> int:
    """Запуск ffmpeg с прозрачным выводом в консоль (stats видны)."""
    return subprocess.run(cmd).returncode


def main() -> int:
    script_dir = Path(__file__).resolve().parent

    ap = argparse.ArgumentParser(description="MOVIxxxx.AVI -> videoN.mpg + audioN.mp3")
    ap.add_argument("--src", type=Path, default=script_dir,
                    help="папка с MOVIxxxx.AVI (по умолчанию — папка скрипта)")
    ap.add_argument("--out", type=Path, default=script_dir.parent / "data" / "VIDEO",
                    help="папка результатов (по умолчанию ../data/VIDEO от скрипта)")
    ap.add_argument("--video-br", default="2500k", help="битрейт MPEG-1 video")
    ap.add_argument("--audio-br", default="128k", help="битрейт MP2 audio")
    ap.add_argument("--start", type=int, default=1, help="первый номер ролика")
    ap.add_argument("--count", type=int, default=12, help="последний номер ролика (включительно)")
    ap.add_argument("--pause", action="store_true",
                    help="ждать Enter в конце (удобно при запуске двойным кликом)")
    args = ap.parse_args()

    # where ffmpeg >nul
    if shutil.which("ffmpeg") is None:
        print("[ERROR] ffmpeg not found in PATH.")
        print("Download a build from https://www.gyan.dev/ffmpeg/builds/")
        print("and add its bin\\ folder to PATH, then retry.")
        if args.pause:
            input("Press Enter to exit...")
        return 1

    args.out.mkdir(parents=True, exist_ok=True)

    ok, skipped, failed = [], [], []

    for i in range(args.start, args.count + 1):
        src = args.src / f"MOVI{i:04d}.AVI"
        if not src.is_file():
            print(f"[SKIP] MOVI{i:04d}.AVI not found")
            skipped.append(i)
            continue

        mpg = args.out / f"video{i}.mpg"
        print()
        print(f"[{i}/{args.count}] {src.name} -> {mpg}")
        rc = run([
            "ffmpeg", "-y", "-hide_banner", "-stats", "-i", str(src),
            "-c:v", "mpeg1video",
            "-b:v", args.video_br, "-maxrate", args.video_br, "-bufsize", "1000k",
            "-r", "25", "-s", "512x384", "-g", "25",
            "-c:a", "mp2", "-b:a", args.audio_br, "-ar", "44100", "-ac", "2",
            "-f", "mpeg", str(mpg),
        ])
        if rc != 0:
            print(f"[WARN] ffmpeg failed on {src.name}")
            failed.append(i)
            continue
        ok.append(i)

        mp3 = args.out / f"audio{i}.mp3"
        print(f"       + {mp3.name}")
        rc = run([
            "ffmpeg", "-y", "-hide_banner", "-loglevel", "error",
            "-i", str(src), "-vn", "-acodec", "copy", str(mp3),
        ])
        if rc != 0:
            print(f"[WARN] ffmpeg (audio copy) failed on {src.name}")
            failed.append(i)

    print()
    print(f"Done. Output dir: {args.out}")
    print(f"  ok: {len(ok)}, skipped: {len(skipped)}, failed: {len(failed)}")
    if args.pause:
        input("Press Enter to exit...")
    return 0 if not failed else 2


if __name__ == "__main__":
    sys.exit(main())
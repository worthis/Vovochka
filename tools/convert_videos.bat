@echo off
setlocal enabledelayedexpansion

where ffmpeg >nul 2>nul
if errorlevel 1 (
    echo [ERROR] ffmpeg not found in PATH.
    pause
    exit /b 1
)

set "SRC_DIR=%~dp0"
set "OUT_DIR=%~dp0..\data\VIDEO"
set "VIDEO_BR=2500k"
set "AUDIO_BR=128k"

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

set /a i=1
:loop
if %i% gtr 12 goto done
set "num=000%i%"
set "num=!num:~-4!"
set "src=%SRC_DIR%MOVI!num!.AVI"

if not exist "!src!" (
    echo [SKIP] MOVI!num!.AVI not found
    goto next
)

echo.
echo [%i%/12] MOVI!num!.AVI -^> video%i%.mpg
ffmpeg -y -hide_banner -stats -i "!src!" -c:v mpeg1video -b:v %VIDEO_BR% -maxrate %VIDEO_BR% -bufsize 1000k -r 25 -s 512x384 -g 25 -c:a mp2 -b:a %AUDIO_BR% -ar 44100 -ac 2 -f mpeg "%OUT_DIR%\video%i%.mpg"
if errorlevel 1 echo [WARN] ffmpeg failed on MOVI!num!.AVI

echo        + audio%i%.mp3
ffmpeg -y -hide_banner -loglevel error -i "!src!" -vn -acodec copy "%OUT_DIR%\audio%i%.mp3"

:next
set /a i+=1
goto loop

:done
echo.
echo Done. Output dir: %OUT_DIR%
pause
endlocal
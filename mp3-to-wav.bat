@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

:: Klasör ayarları
set "input_folder=input/mp3/"
set "output_folder=output/wav/"
set "ffmpeg_path=ffmpeg.exe"

:: Klasörleri oluştur
if not exist "%input_folder%" mkdir "%input_folder%"
if not exist "%output_folder%" mkdir "%output_folder%"

:: FFmpeg kontrolü
where ffmpeg >nul 2>&1
if %errorlevel% neq 0 (
    echo FFmpeg bulunamadı! Lütfen ffmpeg.exe'yi bu klasöre kopyalayın.
    echo İndirme: https://ffmpeg.org/download.html
    pause
    exit /b
)

:: Dönüştürme işlemi
set "count=0"
for %%f in ("%input_folder%\*.mp3") do (
    set /a "count+=1"
    set "filename=%%~nf"
    set "output_file=%output_folder%\!filename!.wav"
    
    echo [!count!] Dönüştürülüyor: %%~nxf
    %ffmpeg_path% -i "%%f" -ar 44100 -ac 1 -c:a pcm_s16le "!output_file!" -hide_banner -loglevel error
    
    if exist "!output_file!" (
        echo [+] Başarılı: !filename!.wav
    ) else (
        echo [-] Hata: %%f
    )
)

echo.
echo Toplam %count% dosya dönüştürüldü.
echo Çıktı klasörü: %cd%\%output_folder%\
pause
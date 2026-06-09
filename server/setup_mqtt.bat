@echo off
:: ========================================================
:: Auto-Setup Mosquitto MQTT untuk CNC ESP32
:: Run as Administrator
:: ========================================================

echo ========================================================
echo Meminta hak akses Administrator (UAC)...
echo ========================================================
:: Cek privileges
net session >nul 2>&1
if %errorLevel% == 0 (
    echo [OK] Sudah berjalan sebagai Administrator.
) else (
    echo Membuka ulang dengan hak akses Administrator...
    powershell -Command "Start-Process '%~0' -Verb RunAs"
    exit /b
)

echo.
echo ========================================================
echo 1. Menginstal Mosquitto MQTT Broker (via Winget)
echo ========================================================
:: Cek apakah Mosquitto sudah terinstall di Program Files
if exist "C:\Program Files\mosquitto\mosquitto.exe" (
    echo [OK] Mosquitto sudah terinstal! Melewati proses download.
) else (
    echo Mengunduh dan menginstal Mosquitto...
    winget install EclipseFoundation.Mosquitto --silent --accept-source-agreements --accept-package-agreements
    if %errorLevel% neq 0 (
        echo [ERROR] Gagal menginstal Mosquitto. Pastikan koneksi internet aktif.
        pause
        exit /b
    )
)

echo.
echo ========================================================
echo 2. Mengkonfigurasi mosquitto.conf
echo ========================================================
cd /d "C:\Program Files\mosquitto"

:: Cek apakah file conf sudah di-backup, jika belum, backup
if not exist "mosquitto.conf.backup" (
    copy mosquitto.conf mosquitto.conf.backup >nul
    echo [OK] Backup mosquitto.conf.backup berhasil dibuat.
)

:: Mengecek apakah konfigurasi listener sudah ada agar tidak duplikat
findstr /C:"listener 1883 0.0.0.0" mosquitto.conf >nul
if %errorLevel% == 0 (
    echo [OK] Konfigurasi sudah ada di mosquitto.conf.
) else (
    echo Menambahkan listener 1883 dan allow_anonymous ke mosquitto.conf...
    echo.>> mosquitto.conf
    echo # Ditambahkan otomatis oleh Auto-Setup CNC>> mosquitto.conf
    echo listener 1883 0.0.0.0>> mosquitto.conf
    echo allow_anonymous true>> mosquitto.conf
    echo [OK] Konfigurasi berhasil ditambahkan.
)

echo.
echo ========================================================
echo 3. Membuka Port 1883 di Windows Firewall
echo ========================================================
netsh advfirewall firewall show rule name="Mosquitto MQTT" >nul
if %errorLevel% == 0 (
    echo [OK] Rule Firewall "Mosquitto MQTT" sudah ada.
) else (
    netsh advfirewall firewall add rule name="Mosquitto MQTT" dir=in action=allow protocol=TCP localport=1883 >nul
    echo [OK] Port TCP 1883 berhasil diizinkan di Windows Firewall.
)

echo.
echo ========================================================
echo 4. Restart Service Mosquitto
echo ========================================================
echo Menghentikan service...
net stop mosquitto
echo Memulai service...
net start mosquitto

echo.
echo ========================================================
echo SETUP SELESAI!
echo ESP32 sekarang seharusnya bisa terhubung ke MQTT Broker.
echo ========================================================
pause

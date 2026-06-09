@echo off
:: ================================================================
:: start_all.bat — Jalankan semua service CNC Project sekaligus
:: Buka jendela terpisah untuk: Mosquitto, Backend, Dashboard
:: ================================================================

echo ================================================================
echo  CNC IoT — Menjalankan Semua Service...
echo ================================================================
echo.

:: 1. Mosquitto MQTT Broker
echo [1/3] Menghidupkan Mosquitto MQTT Broker...
net start mosquitto > nul 2>&1
if %errorLevel% == 0 (
    echo      [OK] Mosquitto service berjalan.
) else (
    echo      [OK] Mosquitto sudah berjalan / sudah aktif.
)

:: 2. Backend Node.js — buka di jendela baru
echo [2/3] Membuka Backend (Node.js)...
start "CNC Backend" cmd /k "cd /d %~dp0server\backend && node index.js"

:: Tunggu sebentar agar backend sempat start
timeout /t 2 /nobreak > nul

:: 3. Dashboard Vite — buka di jendela baru
echo [3/3] Membuka Dashboard (Vite + React)...
start "CNC Dashboard" cmd /k "cd /d %~dp0server\dashboard && npm run dev"

echo.
echo ================================================================
echo  Semua service sedang dimulai!
echo  - Backend  : http://localhost:3001/api
echo  - Dashboard : http://localhost:5173
echo  - MQTT Broker : localhost:1883
echo ================================================================
echo.

:: Tunggu sebentar lalu buka browser otomatis
timeout /t 5 /nobreak > nul
echo Membuka Dashboard di browser...
start "" http://localhost:5173

pause

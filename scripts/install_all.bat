@echo off
echo ================================================================
echo  CNC IoT — Menginstal Semua Dependensi...
echo ================================================================
echo.

cd /d %~dp0..

echo [1/3] Menginstal dependensi root...
call npm install
echo [OK] Dependensi root selesai diinstal.
echo.

echo [2/3] Menginstal dependensi Backend (Node.js)...
cd server\backend
call npm install
cd ..\..
echo [OK] Dependensi Backend selesai diinstal.
echo.

echo [3/3] Menginstal dependensi Dashboard (Vite + React)...
cd server\dashboard
call npm install
cd ..\..
echo [OK] Dependensi Dashboard selesai diinstal.
echo.

echo ================================================================
echo  Semua dependensi berhasil diinstal!
echo  Anda sekarang bisa menjalankan proyek dengan:
echo  1. start_all.bat
echo     (atau "npm start" di terminal)
echo ================================================================
pause

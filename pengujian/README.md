# Pengujian Histeresis End-to-End — CNC Monitoring

Skrip otomasi untuk menguji perilaku resume relay pada 3 zona nilai arus dan suhu,
sepenuhnya otomatis tanpa perlu arus fisik atau power supply terkendali.

Lihat isi lengkap skenario pengujian di [pengujian.py](pengujian.py).

## Prasyarat

- Firmware ESP32 sudah diupdate dengan command `test_hold` / `test_clear` /
  `test_hold_temp` / `test_clear_temp` (lihat `diff_test_hold_firmware.patch`),
  sudah di-compile dan di-upload.
- Backend server sudah berjalan dan dapat diakses (sesuai `BASE_URL` di
  `pengujian.py`).
- Python 3.10+ terpasang di komputer yang menjalankan skrip ini.

## Instalasi

Dari folder `pengujian/`, buat virtual environment dan install dependensi:

```powershell
# Windows (PowerShell)
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

```bash
# macOS/Linux
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Konfigurasi

Sebelum menjalankan, sesuaikan konstanta di bagian atas [pengujian.py](pengujian.py):

- `BASE_URL` — alamat backend (sesuai `HTTP_PORT` di `.env` backend).
- `DEVICE_ID` — sesuai `MQTT_ID` di `credentials.h` firmware ESP32.
- `AMBANG_ALARM`, `HISTERESIS`, `AMBANG_ALARM_SUHU`, `HISTERESIS_SUHU` — sesuai
  nilai di `SensorConfig.h` firmware.

## Menjalankan

Pastikan venv sudah aktif (lihat langkah instalasi), lalu:

```powershell
python pengujian.py
```

Skrip akan meminta konfirmasi bahwa firmware sudah diupdate dan mesin aman
untuk diuji sebelum mulai. Hasil pengujian disimpan ke:

- `hasil_histeresis_end_to_end.csv` — hasil pengujian kanal arus.
- `hasil_histeresis_suhu_end_to_end.csv` — hasil pengujian kanal suhu.

## Keluar dari virtual environment

```powershell
deactivate
```

# CNC IoT Monitor

Sistem monitoring arus dan suhu mesin CNC berbasis ESP32 + MQTT.

## Struktur Proyek

```
cnc-iot-monitor/
├── firmware/          → Kode ESP32 (PlatformIO)
└── server/
    ├── broker/        → Konfigurasi Mosquitto MQTT
    ├── backend/       → Node.js bridge MQTT ↔ WebSocket + REST
    └── dashboard/     → React SCADA dashboard
```

## Cara Menjalankan

### 1. MQTT Broker
```bash
mosquitto -c server/broker/mosquitto.conf
```

### 2. Backend
```bash
cd server/backend
npm install
npm run dev
```

### 3. Dashboard
```bash
cd server/dashboard
npm install
npm run dev
```

### 4. Firmware
Buka folder `firmware/` di VS Code + PlatformIO, lalu:
1. Edit `lib/SensorConfig/SensorConfig.h` sesuai pin hardware
2. Edit `WIFI_SSID`, `WIFI_PASS`, `MQTT_HOST` di `src/main.cpp`
3. Build & Upload

## Alur Data

```
ESP32 → MQTT Broker → Backend (Node.js) → WebSocket → Dashboard (React)
                              ↕
                          REST API
                              ↕
                     Dashboard (kontrol relay)
```

## Kalibrasi Sensor (via Serial Monitor)
```
c  = auto offset (mesin MATI)
C  = auto scale  (masukkan arus referensi)
s  = simpan ke NVS
p  = pilih sensor
z  = lihat status
```

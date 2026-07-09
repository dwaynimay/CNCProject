#include "SerialCLI.h"
#include "SensorConfig.h"

SerialCLI::SerialCLI(Calibration& cal, ACS712* sensors, uint8_t n, EventLog& log)
    : _cal(cal), _sensors(sensors), _n(n), _active(0), _log(log), _showLive(false) {
    _cmdBuf.reserve(32);
}

void SerialCLI::begin()  { printHelp(); }

void SerialCLI::setSelfTestCallback(SelfTestCallback cb) { _selfTestCb = cb; }

void SerialCLI::loop() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (_cmdBuf.length() > 0) {
                handleCommand(_cmdBuf);
                _cmdBuf = "";
            }
        } else {
            _cmdBuf += c;
        }
    }
}

void SerialCLI::handleCommand(String cmd) {
    cmd.trim();
    if (cmd.length() == 0) return;

    char action = cmd.charAt(0);
    int arg = -1;
    
    // Ambil angka argumen jika ada (misal "c 1" -> arg=1)
    if (cmd.length() > 1) {
        arg = cmd.substring(1).toInt();
    }

    switch (action) {
        case 'c': 
            if (arg >= 0 && arg < _n) _active = arg;
            doOffset(_active);
            break;
        case 'z': 
            printStatus();  
            break;
        case 'p': 
            if (arg >= 0 && arg < _n) {
                _active = arg;
                Serial.printf(">> Aktif: [%d] %s\n", _active, CURRENT_NAMES[_active]);
            } else {
                Serial.println(">> Pilih sensor dengan format: p <angka>");
                for (int i = 0; i < _n; i++)
                    Serial.printf("   %d: %s\n", i, CURRENT_NAMES[i]);
            }
            break;
        case 's': 
            _cal.save();    
            break;
        case 'l': 
            _cal.load();    
            break;
        case 'r': 
            _cal.reset();   
            Serial.println(">> Reset default"); 
            break;
        case 'e': 
            _log.print();   
            break;
        case 'v':
            _showLive = !_showLive;
            Serial.println(_showLive ? ">> Live Data ON" : ">> Live Data OFF");
            break;
        case 't':
            if (_selfTestCb) {
                Serial.println(">> Menjalankan self-test...");
                _selfTestCb();
                Serial.println(">> Self-test selesai. Hasil dipublish ke MQTT (selftest_result).");
            } else {
                Serial.println(">> Self-test belum tersedia.");
            }
            break;
        case '?': 
            printHelp();    
            break;
        default:
            Serial.println(">> Perintah tidak dikenal. Ketik ? untuk bantuan.");
            break;
    }
}

void SerialCLI::doOffset(int idx) {
    Serial.printf("\n>> Mengkalibrasi Offset [%d] %s...\n", idx, CURRENT_NAMES[idx]);
    _cal.autoOffset(_sensors[idx]);
    Serial.printf("   vMid = %.3f mV  — ketik 's' untuk simpan\n", _cal.getData(idx).vMid);
}

void SerialCLI::printStatus() {
    Serial.println("\n>> KALIBRASI:");
    for (int i = 0; i < _n; i++) {
        const auto& d = _cal.getData(i);
        Serial.printf("   [%d]%s %-14s vMid=%-8.2f mVpA=%-7.2f [%s]\n",
            i, i==_active?"◄":" ", CURRENT_NAMES[i], d.vMid, d.mVpA,
            d.offsetDone?"OFF✓":"OFF✗");
    }
}

void SerialCLI::printHelp() {
    Serial.println("\n============== MENU BANTUAN CLI ==============");
    Serial.println("Format penulisan perintah: <huruf> [angka]");
    Serial.println("");
    Serial.println("KALIBRASI SENSOR:");
    Serial.println("  c <n> : Kalibrasi titik nol (offset) sensor ke-n.");
    Serial.println("          Contoh: 'c 1' -> Kalibrasi Sensor 1.");
    Serial.println("          (Pastikan mesin CNC DALAM KEADAAN MATI saat kalibrasi)");
    Serial.println("  p <n> : Pilih sensor ke-n sebagai sensor aktif (fokus saat ini).");
    Serial.println("          Contoh: 'p 1' -> Fokus ke Sensor 1.");
    Serial.println("  z     : Tampilkan status & nilai kalibrasi semua sensor saat ini.");
    Serial.println("");
    Serial.println("PENYIMPANAN DATA:");
    Serial.println("  s     : Simpan (Save) hasil kalibrasi ke memori internal (NVS).");
    Serial.println("  l     : Muat ulang (Load) data kalibrasi terakhir dari memori.");
    Serial.println("  r     : Hapus (Reset) semua data kalibrasi ke pengaturan pabrik.");
    Serial.println("");
    Serial.println("MONITORING & DIAGNOSTIK:");
    Serial.println("  v     : Hidupkan/matikan (Toggle) tampilan data pembacaan sensor secara live.");
    Serial.println("  e     : Tampilkan riwayat log error (Event Log) terbaru.");
    Serial.println("  t     : Jalankan self-test diagnostik lengkap (hasil dipublish ke MQTT).");
    Serial.println("  ?     : Tampilkan menu bantuan ini.");
    Serial.println("==============================================");
}

void SerialCLI::printLive(const CurrentReading* r, uint8_t n) {
    if (!_showLive) return;
    Serial.println("==========================================");
    for (int i = 0; i < n; i++)
        Serial.printf("[%d] %-14s %.4f A %s\n",
            i, CURRENT_NAMES[i], r[i].ampere, r[i].alarm?"⚠ ALARM":"");
}

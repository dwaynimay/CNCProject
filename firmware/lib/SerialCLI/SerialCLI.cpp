#include "SerialCLI.h"

SerialCLI::SerialCLI(Calibration& cal, ACS712* sensors, uint8_t n, EventLog& log)
    : _cal(cal), _sensors(sensors), _n(n), _active(0), _log(log) {}

void SerialCLI::begin()  { printHelp(); }
void SerialCLI::loop()   { if (Serial.available()) handleChar(Serial.read()); }

void SerialCLI::handleChar(char c) {
    switch (c) {
        case 'c': doOffset();     break;
        case 'z': printStatus();  break;
        case 'p': selectSensor(); break;
        case 's': _cal.save();    break;
        case 'l': _cal.load();    break;
        case 'r': _cal.reset();   Serial.println(">> Reset default"); break;
        case 'e': _log.print();   break;
        case '?': printHelp();    break;
    }
}

void SerialCLI::doOffset() {
    Serial.printf("\n>> Offset [%d] %s — mesin MATI, tekan Enter\n",
        _active, CURRENT_NAMES[_active]);
    while (!Serial.available()) delay(100);
    Serial.read();
    _cal.autoOffset(_sensors[_active]);
    Serial.printf("   vMid = %.3f mV  — ketik 's' untuk simpan\n",
        _cal.getData(_active).vMid);
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
    Serial.println("-------------------------------------------");
    Serial.println("c=offset  z=status  p=sensor  e=eventlog");
    Serial.println("s=simpan  l=load   r=reset   ?=help");
    Serial.println("-------------------------------------------");
}

void SerialCLI::selectSensor() {
    Serial.println(">> Pilih sensor (0-4):");
    for (int i = 0; i < _n; i++)
        Serial.printf("   %d: %s\n", i, CURRENT_NAMES[i]);
    unsigned long t = millis();
    while (!Serial.available() && millis()-t < 5000);
    if (Serial.available()) {
        int idx = Serial.read() - '0';
        if (idx >= 0 && idx < _n) {
            _active = idx;
            Serial.printf(">> Aktif: [%d] %s\n", _active, CURRENT_NAMES[_active]);
        }
    }
}

void SerialCLI::printLive(const CurrentReading* r, uint8_t n) {
    Serial.println("==========================================");
    for (int i = 0; i < n; i++)
        Serial.printf("[%d] %-14s %.4f A %s\n",
            i, CURRENT_NAMES[i], r[i].ampere, r[i].alarm?"⚠ ALARM":"");
}

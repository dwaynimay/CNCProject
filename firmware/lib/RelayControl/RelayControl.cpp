#include "RelayControl.h"

RelayControl::RelayControl() : _state(false) {}

void RelayControl::begin() { off(); }

void RelayControl::on() {
    if (RELAY_ACTIVE_LOW) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW);
    } else {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, HIGH);
    }
    _state = true;
}

void RelayControl::off() {
    if (RELAY_ACTIVE_LOW) {
        // Trik khusus untuk ESP32 (3.3V) ke Relay 5V
        // Ubah jadi INPUT (High Impedance) agar arus dari VCC 5V benar-benar terputus
        pinMode(RELAY_PIN, INPUT); 
    } else {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW);
    }
    _state = false;
}

bool RelayControl::isOn()  const { return _state; }

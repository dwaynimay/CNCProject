#include "RelayControl.h"

RelayControl::RelayControl() : _state(false) {}

void RelayControl::begin() {
    pinMode(ESTOP_PIN, OUTPUT);
    digitalWrite(ESTOP_PIN, LOW);
    off();
}

void RelayControl::on() {
    if (RELAY_ACTIVE_LOW) {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, LOW);
    } else {
        pinMode(RELAY_PIN, OUTPUT);
        digitalWrite(RELAY_PIN, HIGH);
    }
    digitalWrite(ESTOP_PIN, HIGH);   // sinyal E-Stop ke CNC Shield
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
    digitalWrite(ESTOP_PIN, LOW);    // E-Stop clear
    _state = false;
}

bool RelayControl::isOn()  const { return _state; }

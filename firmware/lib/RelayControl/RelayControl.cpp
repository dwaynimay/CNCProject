#include "RelayControl.h"

RelayControl::RelayControl() : _state(false) {}

void RelayControl::begin() { pinMode(RELAY_PIN, OUTPUT); off(); }
void RelayControl::on()    { digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? LOW : HIGH);  _state = true; }
void RelayControl::off()   { digitalWrite(RELAY_PIN, RELAY_ACTIVE_LOW ? HIGH : LOW); _state = false; }
bool RelayControl::isOn()  const { return _state; }

#include <Arduino.h>
#include <Wire.h>
void test_wire1() {
    Wire1.begin();
    Wire1.beginTransmission(0x60);
    Wire1.endTransmission();
}

/**
 * @file radio_module.cpp
 * @brief Implementation of the TEA5767 I2C FM Stereo Radio driver.
 */

#include "radio_module.h"
#include <Wire.h>
#include "config.h"

RadioTEA5767::RadioTEA5767()
    : currentFrequency_(kDefaultRadioFreqMhz),
      isMuted_(true) {}

void RadioTEA5767::begin() {
    Wire.begin();
    isMuted_ = true;
    setFrequency(currentFrequency_);
}

void RadioTEA5767::setFrequency(float frequencyMhz) {
    // Frequenz auf das gültige UKW-Rundfunkband (87.5 - 108.0 MHz) begrenzen
    if (frequencyMhz < kMinRadioFreqMhz) {
        frequencyMhz = kMinRadioFreqMhz;
    } else if (frequencyMhz > kMaxRadioFreqMhz) {
        frequencyMhz = kMaxRadioFreqMhz;
    }

    currentFrequency_ = frequencyMhz;

    // Gerundete PLL-Berechnung für High-Side Injection:
    // PLL = 4 * (f_RF * 1e6 + f_IF) / f_ref
    const float pllFloat =
        (4.0F * ((frequencyMhz * 1000000.0F) + kTea5767IfFreqHz)) / kTea5767RefCrystalHz;
    const uint16_t pll = static_cast<uint16_t>(pllFloat + 0.5F);

    uint8_t transmissionData[5];

    // Byte 1: Mute-Bit (Bit 7: 1 = stumm, 0 = aktiv) | PLL[13:8]
    transmissionData[0] = static_cast<uint8_t>((isMuted_ ? 0x80U : 0x00U) | ((pll >> 8U) & 0x3FU));

    // Byte 2: PLL[7:0]
    transmissionData[1] = static_cast<uint8_t>(pll & 0xFFU);

    // Byte 3: HLSI=1 (High Side Injection), Stereo-Betrieb, Suchlauf deaktiviert
    transmissionData[2] = 0x10U;

    // Byte 4: XTAL=1 (32.768 kHz Quarz), SMUTE=0, HCC=1 (High Cut Control), SNC=1 (Stereo Noise Cancelling)
    transmissionData[3] = 0x16U;

    // Byte 5: PLLREF=0, DTC=0
    transmissionData[4] = 0x00U;

    (void)sendData(transmissionData);
}

void RadioTEA5767::setMute(bool muteState) {
    isMuted_ = muteState;
    // Der TEA5767 benötigt alle 5 Register-Bytes, um das Mute-Bit zu aktualisieren
    setFrequency(currentFrequency_);
}

bool RadioTEA5767::isMuted() const {
    return isMuted_;
}

float RadioTEA5767::getFrequency() const {
    return currentFrequency_;
}

bool RadioTEA5767::sendData(const uint8_t transmissionData[5]) {
    Wire.beginTransmission(kTea5767I2cAddr);
    for (size_t i = 0; i < 5U; ++i) {
        Wire.write(transmissionData[i]);
    }
    const uint8_t result = Wire.endTransmission();

    if (result != 0U) {
        Serial.print("I2C-Fehler beim TEA5767! Code: ");
        Serial.println(result);
        return false;
    }

    Serial.println("TEA5767: I2C-Daten erfolgreich gesendet.");
    return true;
}

void performI2cDiagnosticScan() {
    Serial.println("--- Starte I2C Scan ---");
    Wire.begin();
    uint8_t foundCount = 0U;

    for (uint8_t addr = 1U; addr < 120U; ++addr) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0U) {
            Serial.print("I2C-Geraet gefunden auf 0x");
            if (addr < 16U) {
                Serial.print('0');
            }
            Serial.println(addr, HEX);
            ++foundCount;
        }
    }

    if (foundCount == 0U) {
        Serial.println("Keine I2C-Geraete gefunden.");
    }
    Serial.println("--- I2C Scan Ende ---");
}

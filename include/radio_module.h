/**
 * @file radio_module.h
 * @brief Driver interface for the Philips/NXP TEA5767 FM Stereo Radio tuner via I2C.
 */

#ifndef RADIO_MODULE_H
#define RADIO_MODULE_H

#pragma once

#include <Arduino.h>

/**
 * @class RadioTEA5767
 * @brief Controls frequency tuning and hardware mute state of the TEA5767 I2C FM receiver.
 */
class RadioTEA5767 {
public:
    /**
     * @brief Construct a new RadioTEA5767 driver with default frequency (105.2 MHz) and muted output.
     */
    RadioTEA5767();

    /**
     * @brief Initialize the I2C bus and apply initial muted state and default frequency.
     */
    void begin();

    /**
     * @brief Tune the TEA5767 receiver to a target FM frequency in MHz.
     *
     * Automatically clamps the frequency to the valid FM broadcast band (87.5 - 108.0 MHz)
     * and computes the rounded 14-bit high-side injection PLL word.
     *
     * @param frequencyMhz Target frequency in MHz (e.g., 105.2F).
     */
    void setFrequency(float frequencyMhz);

    /**
     * @brief Enable or disable the TEA5767 hardware audio mute.
     *
     * @param muteState `true` to mute audio output, `false` to unmute (play radio).
     */
    void setMute(bool muteState);

    /**
     * @brief Query whether the radio is currently muted.
     *
     * @return true if muted, false if playing audio.
     */
    bool isMuted() const;

    /**
     * @brief Query the currently tuned FM frequency in MHz.
     *
     * @return Current frequency in MHz.
     */
    float getFrequency() const;

private:
    /**
     * @brief Transmit a 5-byte control register frame to the TEA5767 over I2C (`0x60`).
     *
     * @param transmissionData Array of 5 configuration bytes.
     * @return true if I2C transmission succeeded (`endTransmission() == 0`).
     */
    bool sendData(const uint8_t transmissionData[5]);

    float currentFrequency_; ///< Currently tuned frequency in MHz
    bool  isMuted_;          ///< Current hardware mute state
};

/**
 * @brief Perform a diagnostic I2C address scan (`0x01`..`0x77`) and log discovered devices to `Serial`.
 */
void performI2cDiagnosticScan();

#endif // RADIO_MODULE_H

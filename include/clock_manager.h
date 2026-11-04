/**
 * @file clock_manager.h
 * @brief Hardware RTC (DS1307) abstraction with drift-free local time interpolation.
 *
 * Avoids 1 Hz I2C bus polling (which induces audible interference on the shared
 * TEA5767 analog audio lines) by interpolating elapsed time from `millis()`
 * relative to a periodic hardware RTC synchronization baseline.
 */

#ifndef CLOCK_MANAGER_H
#define CLOCK_MANAGER_H

#pragma once

#include <Arduino.h>
#include <RTClib.h>

/**
 * @class ClockManager
 * @brief Manages real-time clock synchronization and drift-free time queries.
 */
class ClockManager {
public:
    /**
     * @brief Construct a new ClockManager instance.
     */
    ClockManager();

    /**
     * @brief Initialize the DS1307 RTC over the primary I2C bus (`Wire`).
     *
     * If the DS1307 is not detected or not running, falls back gracefully
     * to the firmware compile timestamp (`__DATE__`, `__TIME__`).
     *
     * @param currentMillis Current system timestamp from `millis()`.
     * @return true if the physical DS1307 RTC responded on the I2C bus.
     */
    bool begin(unsigned long currentMillis);

    /**
     * @brief Perform periodic maintenance and hourly hardware RTC synchronization.
     *
     * @param currentMillis Current system timestamp from `millis()`.
     */
    void update(unsigned long currentMillis);

    /**
     * @brief Compute the exact current DateTime without triggering I2C traffic.
     *
     * Calculates `baseRtcTime_ + TimeSpan(elapsedSeconds)` using `millis()` delta
     * so blocking operations (such as TLS handshakes) never cause clock drift.
     *
     * @param currentMillis Current system timestamp from `millis()`.
     * @return Current interpolated `DateTime`.
     */
    DateTime now(unsigned long currentMillis) const;

    /**
     * @brief Manually set the current time of day (updates both RAM baseline and DS1307).
     *
     * @param hour          Hour in 24-hour format (0..23).
     * @param minute        Minute (0..59).
     * @param second        Second (0..59).
     * @param currentMillis Current system timestamp from `millis()`.
     * @return true if the provided time parameters are valid.
     */
    bool setTime(uint8_t hour, uint8_t minute, uint8_t second, unsigned long currentMillis);

    /**
     * @brief Format the current time as a null-terminated `"HH:MM:SS"` string.
     *
     * @param currentMillis Current system timestamp from `millis()`.
     * @param buffer        Destination character buffer (at least 9 bytes).
     * @param bufferSize    Size of the destination buffer in bytes.
     */
    void formatTime(unsigned long currentMillis, char *buffer, size_t bufferSize) const;

    /**
     * @brief Check whether the physical DS1307 RTC is available on the I2C bus.
     *
     * @return true if hardware RTC is online, false if running on software fallback.
     */
    bool isHardwareRtcAvailable() const;

private:
    RTC_DS1307    rtc_;                 ///< Underlying Adafruit RTClib DS1307 driver
    DateTime      baseRtcTime_;         ///< Reference timestamp at last sync point
    unsigned long baseSyncMillis_;      ///< `millis()` value corresponding to `baseRtcTime_`
    bool          rtcHardwarePresent_;  ///< True if DS1307 responded during `begin()`
};

#endif // CLOCK_MANAGER_H

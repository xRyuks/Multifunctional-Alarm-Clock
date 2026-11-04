/**
 * @file clock_manager.cpp
 * @brief Implementation of hardware RTC management and drift-free local time interpolation.
 */

#include "clock_manager.h"
#include <Wire.h>
#include "config.h"

ClockManager::ClockManager()
    : rtc_(),
      baseRtcTime_(DateTime(F(__DATE__), F(__TIME__))),
      baseSyncMillis_(0UL),
      rtcHardwarePresent_(false) {}

bool ClockManager::begin(unsigned long currentMillis) {
    baseSyncMillis_ = currentMillis;

    if (!rtc_.begin(&Wire)) {
        Serial.println("WARNING: Hardware RTC (DS1307) not found! Falling back to software clock (compile time).");
        rtcHardwarePresent_ = false;
        baseRtcTime_ = DateTime(F(__DATE__), F(__TIME__));
        return false;
    }

    rtcHardwarePresent_ = true;
    if (!rtc_.isrunning()) {
        Serial.println("RTC was not running! Initializing with compile timestamp...");
        rtc_.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    baseRtcTime_ = rtc_.now();
    Serial.println("RTC (DS1307) initialized successfully.");
    return true;
}

void ClockManager::update(unsigned long currentMillis) {
    if ((currentMillis - baseSyncMillis_) >= kRtcSyncIntervalMs) {
        if (rtcHardwarePresent_) {
            // Re-synchronize with physical DS1307 hardware once per hour
            baseRtcTime_ = rtc_.now();
            baseSyncMillis_ = currentMillis;
        } else {
            // Roll software baseline forward to prevent 32-bit delta overflow
            baseRtcTime_ = now(currentMillis);
            baseSyncMillis_ = currentMillis;
        }
    }
}

DateTime ClockManager::now(unsigned long currentMillis) const {
    const unsigned long elapsedMs = currentMillis - baseSyncMillis_;
    const int32_t elapsedSeconds = static_cast<int32_t>(elapsedMs / 1000UL);
    return baseRtcTime_ + TimeSpan(elapsedSeconds);
}

bool ClockManager::setTime(uint8_t hour, uint8_t minute, uint8_t second, unsigned long currentMillis) {
    if (hour >= 24U || minute >= 60U || second >= 60U) {
        return false;
    }

    const DateTime current = now(currentMillis);
    const DateTime updatedTime(
        current.year(),
        current.month(),
        current.day(),
        hour,
        minute,
        second
    );

    baseRtcTime_ = updatedTime;
    baseSyncMillis_ = currentMillis;

    if (rtcHardwarePresent_) {
        rtc_.adjust(updatedTime);
    }

    return true;
}

void ClockManager::formatTime(unsigned long currentMillis, char *buffer, size_t bufferSize) const {
    if (buffer == nullptr || bufferSize == 0U) {
        return;
    }
    const DateTime current = now(currentMillis);
    snprintf(
        buffer,
        bufferSize,
        "%02u:%02u:%02u",
        static_cast<unsigned int>(current.hour()),
        static_cast<unsigned int>(current.minute()),
        static_cast<unsigned int>(current.second())
    );
}

bool ClockManager::isHardwareRtcAvailable() const {
    return rtcHardwarePresent_;
}

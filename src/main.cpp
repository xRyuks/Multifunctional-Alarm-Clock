/**
 * @file main.cpp
 * @brief Main application entry point for the Multifunctional Radio Alarm Clock firmware.
 *
 * Coordinates hardware initialization (`setup()`) and non-blocking cooperative
 * scheduling (`loop()`) across all functional domain modules:
 * - `ClockManager`      (DS1307 hardware RTC + drift-free `millis()` interpolation)
 * - `AlarmController`   (Daily wake-up alarm state machine with Snooze & Stop)
 * - `RadioTEA5767`      (I2C FM Stereo Tuner on `0x60`)
 * - `DisplayUI`         (ILI9341 320x240 8-bit parallel TFT UI)
 * - `NetworkController` (ESP32-S3 Wi-Fi + HTTPS JSONP live Virtual RDS)
 * - `InputHandler`      (Non-blocking USB-Serial CLI)
 */

#include <Arduino.h>

#include "alarm_controller.h"
#include "clock_manager.h"
#include "config.h"
#include "display_ui.h"
#include "input_handler.h"
#include "network_controller.h"
#include "radio_module.h"

namespace {
RadioTEA5767    gRadio;
ClockManager    gClockManager;
AlarmController gAlarmController;

unsigned long gLastClockUpdateMs = 0UL;
unsigned long gLastRdsFetchMs    = 0UL;
} // namespace

void setup() {
    Serial.begin(kSerialBaudRate);
    delay(1000);
    Serial.println("=================================================");
    Serial.println("Multifunctional Radio Alarm Clock - Booting");
    Serial.println("=================================================");

    // 1. Diagnostic I2C bus scan (verifies TEA5767 @ 0x60 and DS1307 @ 0x68)
    performI2cDiagnosticScan();

    // 2. Initialize FM Radio (TEA5767 starts muted at 105.2 MHz)
    Serial.println("Initializing TEA5767 FM Radio...");
    gRadio.begin();

    // 3. Initialize Real-Time Clock (DS1307) and drift-free time baseline
    (void)gClockManager.begin(millis());

    // 4. Initialize TFT Display (ILI9341) and draw static UI frame
    (void)initDisplayUi(gRadio.getFrequency());
    renderNowPlaying("bigFM (105.2 MHz)", "Waiting for Virtual RDS...");

    // 5. Connect to Wi-Fi dynamically for live Virtual RDS updates
    if (!connectToWifiDynamic()) {
        Serial.println("Continuing in offline mode: Virtual RDS disabled until Wi-Fi is available.");
        renderNowPlaying("Offline Mode", "FM Radio Ready (105.2 MHz)");
    } else {
        fetchVirtualRDS();
        gLastRdsFetchMs = millis();
    }

    // Initial clock footer render
    const unsigned long bootNow = millis();
    gLastClockUpdateMs = bootNow;
    char timeStr[16];
    char alarmStr[16];
    gClockManager.formatTime(bootNow, timeStr, sizeof(timeStr));
    gAlarmController.formatAlarmText(alarmStr, sizeof(alarmStr));
    renderClock(timeStr, alarmStr);

    Serial.println("=================================================");
    Serial.println("SYSTEM READY.");
    Serial.println("IMPORTANT: Configure your Serial Monitor line ending to 'Newline (LF)'!");
    printSerialCommandHelp();
}

void loop() {
    const unsigned long now = millis();

    // 1. Poll and dispatch incoming USB-Serial commands non-blockingly
    processSerialInput(gClockManager, gAlarmController, gRadio, now);

    // 2. Poll live Virtual RDS metadata over HTTPS every 15 seconds
    if ((now - gLastRdsFetchMs) >= kRdsFetchIntervalMs) {
        gLastRdsFetchMs = now;
        fetchVirtualRDS();
    }

    // 3. Update clock, evaluate alarm state machine, and refresh footer UI at 1 Hz
    const unsigned long updatedNow = millis();
    if ((updatedNow - gLastClockUpdateMs) >= kClockUpdateIntervalMs) {
        gLastClockUpdateMs = updatedNow;

        // Check hourly hardware RTC synchronization
        gClockManager.update(updatedNow);

        const DateTime currentRealTime = gClockManager.now(updatedNow);

        // Evaluate alarm trigger and snooze conditions
        if (gAlarmController.update(currentRealTime, gRadio)) {
            renderHeaderStatus(gRadio.getFrequency(), gRadio.isMuted());
        }

        // Render time and alarm state flicker-free on the TFT footer
        char timeStr[16];
        char alarmStr[16];
        gClockManager.formatTime(updatedNow, timeStr, sizeof(timeStr));
        gAlarmController.formatAlarmText(alarmStr, sizeof(alarmStr));
        renderClock(timeStr, alarmStr);
    }
}

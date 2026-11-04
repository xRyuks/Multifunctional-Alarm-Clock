/**
 * @file main.cpp
 * @brief Main application entry point for the Multifunktionaler Radiowecker firmware.
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
    Serial.println("Multifunktionaler Radiowecker - Systemstart");
    Serial.println("=================================================");

    // 1. Diagnostischer I2C-Bus-Scan (prüft TEA5767 @ 0x60 und DS1307 @ 0x68)
    performI2cDiagnosticScan();

    // 2. FM-Radio (TEA5767) initialisieren (startet stummgeschaltet auf 105.2 MHz)
    Serial.println("Initialisiere TEA5767 FM-Radio...");
    gRadio.begin();

    // 3. Echtzeituhr (DS1307) und driftfreie Zeitbasis initialisieren
    (void)gClockManager.begin(millis());

    // 4. TFT-Display (ILI9341) initialisieren und statisches Layout zeichnen
    (void)initDisplayUi(gRadio.getFrequency());
    renderNowPlaying("bigFM (105.2 MHz)", "Warte auf Virtual RDS...");

    // 5. Dynamische WLAN-Verbindung für Virtual RDS aufbauen
    if (!connectToWifiDynamic()) {
        Serial.println("Start im Offline-Modus: Virtual RDS bleibt deaktiviert, bis WLAN verfuegbar ist.");
        renderNowPlaying("Offline-Modus", "UKW-Radio bereit (105.2 MHz)");
    } else {
        fetchVirtualRDS();
        gLastRdsFetchMs = millis();
    }

    // Initiales Rendern der Uhrzeit
    const unsigned long bootNow = millis();
    gLastClockUpdateMs = bootNow;
    char timeStr[16];
    char alarmStr[16];
    gClockManager.formatTime(bootNow, timeStr, sizeof(timeStr));
    gAlarmController.formatAlarmText(alarmStr, sizeof(alarmStr));
    renderClock(timeStr, alarmStr);

    Serial.println("=================================================");
    Serial.println("SYSTEM BEREIT.");
    Serial.println("WICHTIG: Stelle den Seriellen Monitor auf 'Zeilenumbruch' (Newline / LF) ein!");
    printSerialCommandHelp();
}

void loop() {
    const unsigned long now = millis();

    // 1. Serielle Benutzereingaben nicht-blockierend auswerten
    processSerialInput(gClockManager, gAlarmController, gRadio, now);

    // 2. Virtual RDS über HTTPS alle 15 Sekunden aktualisieren
    if ((now - gLastRdsFetchMs) >= kRdsFetchIntervalMs) {
        gLastRdsFetchMs = now;
        fetchVirtualRDS();
    }

    // 3. Uhrzeit, Alarmlogik und Footer-UI im Sekundentakt aktualisieren
    const unsigned long updatedNow = millis();
    if ((updatedNow - gLastClockUpdateMs) >= kClockUpdateIntervalMs) {
        gLastClockUpdateMs = updatedNow;

        // Stündliche RTC-Resynchronisation prüfen
        gClockManager.update(updatedNow);

        const DateTime currentRealTime = gClockManager.now(updatedNow);

        // Alarm-Zustandsautomat auswerten
        if (gAlarmController.update(currentRealTime, gRadio)) {
            renderHeaderStatus(gRadio.getFrequency(), gRadio.isMuted());
        }

        // Uhrzeit und Alarmstatus flackerfrei auf dem TFT ausgeben
        char timeStr[16];
        char alarmStr[16];
        gClockManager.formatTime(updatedNow, timeStr, sizeof(timeStr));
        gAlarmController.formatAlarmText(alarmStr, sizeof(alarmStr));
        renderClock(timeStr, alarmStr);
    }
}

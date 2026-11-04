/**
 * @file input_handler.cpp
 * @brief Implementation of the non-blocking Serial command line interface (CLI).
 */

#include "input_handler.h"
#include <ctype.h>
#include <stdlib.h>
#include "config.h"
#include "display_ui.h"

namespace {
char   gSerialBuffer[kSerialInputBufferSize];
size_t gSerialBufIdx   = 0U;
bool   gLineOverflowed = false;

/**
 * @brief Strictly parse a non-empty string of decimal digits into an integer.
 */
bool parseStrictInt(const String &str, int &outValue) {
    if (str.length() == 0U) {
        return false;
    }
    for (unsigned int i = 0U; i < str.length(); ++i) {
        if (!isdigit(static_cast<unsigned char>(str.charAt(i)))) {
            return false;
        }
    }
    outValue = str.toInt();
    return true;
}

/**
 * @brief Parse `"HH:MM"` or `"HH:MM:SS"` strictly into hour, minute, and optional second.
 */
bool parseTimeComponents(const String &timeArg, int &outHour, int &outMin, int &outSec) {
    const int firstColon = timeArg.indexOf(':');
    if (firstColon <= 0) {
        return false;
    }

    const int secondColon = timeArg.indexOf(':', firstColon + 1);
    const String hourPart = timeArg.substring(0, firstColon);
    String minPart;
    String secPart = "0";

    if (secondColon == -1) {
        minPart = timeArg.substring(firstColon + 1);
    } else {
        minPart = timeArg.substring(firstColon + 1, secondColon);
        secPart = timeArg.substring(secondColon + 1);
    }

    if (!parseStrictInt(hourPart, outHour) ||
        !parseStrictInt(minPart, outMin) ||
        !parseStrictInt(secPart, outSec)) {
        return false;
    }

    return (outHour >= 0 && outHour < 24 &&
            outMin >= 0 && outMin < 60 &&
            outSec >= 0 && outSec < 60);
}

void executeSerialCommand(
    const String &rawCommand,
    ClockManager &clockManager,
    AlarmController &alarmController,
    RadioTEA5767 &radio,
    unsigned long currentMillis
) {
    String input = rawCommand;
    input.trim();
    input.toUpperCase();

    if (input.length() == 0U) {
        return;
    }

    if (input == "HELP" || input == "?") {
        printSerialCommandHelp();
    } else if (input == "ALARM OFF") {
        alarmController.disableAlarm(radio);
        renderHeaderStatus(radio.getFrequency(), radio.isMuted());
        Serial.println("Alarm disabled (FM radio muted).");
    } else if (input.startsWith("ALARM ")) {
        const String timeArg = input.substring(6);
        int h = -1;
        int m = -1;
        int s = 0;
        if ( timeArg.indexOf(':', timeArg.indexOf(':') + 1) == -1 &&
             parseTimeComponents(timeArg, h, m, s) &&
             alarmController.setAlarm(h, m) ) {
            char msgBuf[64];
            snprintf(msgBuf, sizeof(msgBuf), "Alarm set to %02d:%02d", h, m);
            Serial.println(msgBuf);
        } else {
            Serial.println("Invalid time format. Use ALARM HH:MM (e.g., ALARM 07:30)");
        }
    } else if (input == "STOP") {
        alarmController.dismissRinging(radio);
        renderHeaderStatus(radio.getFrequency(), radio.isMuted());
        Serial.println("Active alarm stopped (remains armed for tomorrow).");
    } else if (input == "SNOOZE") {
        const DateTime now = clockManager.now(currentMillis);
        if (alarmController.snooze(radio, now, kDefaultSnoozeMinutes)) {
            renderHeaderStatus(radio.getFrequency(), radio.isMuted());
            Serial.print("Snooze activated for ");
            Serial.print(kDefaultSnoozeMinutes);
            Serial.println(" minutes.");
        } else {
            Serial.println("Cannot snooze: alarm is not currently ringing.");
        }
    } else if (input.startsWith("SET TIME ")) {
        const String timeArg = input.substring(9);
        int h = -1;
        int m = -1;
        int s = 0;
        if (parseTimeComponents(timeArg, h, m, s) &&
            clockManager.setTime(static_cast<uint8_t>(h), static_cast<uint8_t>(m), static_cast<uint8_t>(s), currentMillis)) {
            char msgBuf[64];
            snprintf(msgBuf, sizeof(msgBuf), "System & RTC time updated to %02d:%02d:%02d", h, m, s);
            Serial.println(msgBuf);
        } else {
            Serial.println("Invalid time format. Use SET TIME HH:MM or SET TIME HH:MM:SS");
        }
    } else if (input == "RADIO ON") {
        radio.setMute(false);
        renderHeaderStatus(radio.getFrequency(), radio.isMuted());
        Serial.println("FM radio unmuted (Audio ON).");
    } else if (input == "RADIO OFF" || input == "MUTE") {
        radio.setMute(true);
        renderHeaderStatus(radio.getFrequency(), radio.isMuted());
        Serial.println("FM radio muted (Audio OFF).");
    } else if (input.startsWith("FREQ ")) {
        const String freqArg = input.substring(5);
        char *endPtr = nullptr;
        const float freq = strtof(freqArg.c_str(), &endPtr);
        if (endPtr != freqArg.c_str() && *endPtr == '\0' && freq >= kMinRadioFreqMhz && freq <= kMaxRadioFreqMhz) {
            radio.setFrequency(freq);
            renderHeaderStatus(radio.getFrequency(), radio.isMuted());
            Serial.print("Tuned FM frequency to ");
            Serial.print(radio.getFrequency(), 1);
            Serial.println(" MHz.");
        } else {
            Serial.println("Invalid frequency. Allowed range: 87.5 to 108.0 MHz (e.g., FREQ 105.2)");
        }
    } else {
        Serial.print("Unknown command: '");
        Serial.print(input);
        Serial.println("'. Type HELP for a list of available commands.");
    }
}
} // namespace

void printSerialCommandHelp() {
    Serial.println("=================================================");
    Serial.println("AVAILABLE SERIAL COMMANDS:");
    Serial.println("  ALARM HH:MM        -> Set daily wake-up time (e.g., ALARM 07:30)");
    Serial.println("  ALARM OFF          -> Disable alarm completely & mute radio");
    Serial.println("  STOP               -> Silence ringing alarm (stays armed for tomorrow)");
    Serial.println("  SNOOZE             -> Snooze ringing alarm for 5 minutes");
    Serial.println("  SET TIME HH:MM:SS  -> Set clock time manually (e.g., SET TIME 14:30:00)");
    Serial.println("  RADIO ON / OFF     -> Manually unmute / mute FM radio");
    Serial.println("  FREQ <MHz>         -> Tune FM frequency (87.5 - 108.0 MHz)");
    Serial.println("  HELP               -> Display this command reference");
    Serial.println("=================================================");
}

void processSerialInput(
    ClockManager &clockManager,
    AlarmController &alarmController,
    RadioTEA5767 &radio,
    unsigned long currentMillis
) {
    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());

        if (c == '\n' || c == '\r') {
            if (gLineOverflowed) {
                Serial.println("ERROR: Command line too long (max 63 chars). Input discarded.");
                gLineOverflowed = false;
                gSerialBufIdx = 0U;
                continue;
            }

            if (gSerialBufIdx > 0U) {
                gSerialBuffer[gSerialBufIdx] = '\0';
                executeSerialCommand(
                    String(gSerialBuffer),
                    clockManager,
                    alarmController,
                    radio,
                    currentMillis
                );
                gSerialBufIdx = 0U;
            }
        } else {
            if (gSerialBufIdx < (sizeof(gSerialBuffer) - 1U)) {
                gSerialBuffer[gSerialBufIdx++] = c;
            } else {
                gLineOverflowed = true;
            }
        }
    }
}

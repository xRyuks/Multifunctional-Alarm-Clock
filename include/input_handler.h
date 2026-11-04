/**
 * @file input_handler.h
 * @brief Non-blocking USB-Serial command line interface (CLI) parser and dispatcher.
 *
 * Supported Commands:
 * - `ALARM HH:MM`       : Arm daily wake-up alarm (strict 24h numeric validation)
 * - `ALARM OFF`         : Disable alarm and mute radio if ringing
 * - `STOP`              : Silence ringing alarm while keeping daily alarm armed
 * - `SNOOZE`            : Snooze ringing alarm for 5 minutes
 * - `SET TIME HH:MM[:SS]`: Manually set real-time clock (updates RAM & DS1307 hardware)
 * - `RADIO ON`          : Manually unmute FM radio
 * - `RADIO OFF`         : Manually mute FM radio
 * - `FREQ <MHz>`        : Tune FM radio to frequency (87.5 .. 108.0 MHz)
 * - `HELP`              : Display command reference
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#pragma once

#include <Arduino.h>
#include "alarm_controller.h"
#include "clock_manager.h"
#include "radio_module.h"

/**
 * @brief Print the interactive serial command reference banner to `Serial`.
 */
void printSerialCommandHelp();

/**
 * @brief Poll `Serial` non-blockingly and execute completed newline-terminated commands.
 *
 * Includes overflow protection (lines exceeding `kSerialInputBufferSize - 1` are safely
 * discarded until the next newline rather than executing truncated prefixes).
 *
 * @param clockManager    Reference to the system `ClockManager`.
 * @param alarmController Reference to the system `AlarmController`.
 * @param radio           Reference to the `RadioTEA5767` driver.
 * @param currentMillis   Current timestamp from `millis()`.
 */
void processSerialInput(
    ClockManager &clockManager,
    AlarmController &alarmController,
    RadioTEA5767 &radio,
    unsigned long currentMillis
);

#endif // INPUT_HANDLER_H

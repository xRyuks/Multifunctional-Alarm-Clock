/**
 * @file alarm_controller.h
 * @brief Finite state machine for daily alarm scheduling, triggering, snoozing, and dismissal.
 */

#ifndef ALARM_CONTROLLER_H
#define ALARM_CONTROLLER_H

#pragma once

#include <Arduino.h>
#include <RTClib.h>
#include "config.h"
#include "radio_module.h"

/**
 * @enum AlarmState
 * @brief Operational states of the alarm controller state machine.
 */
enum class AlarmState : uint8_t {
    DISABLED = 0, ///< No alarm configured (`--:--`)
    ARMED,        ///< Alarm set and waiting for target `HH:MM`
    RINGING,      ///< Alarm actively triggered (radio unmuted)
    SNOOZED       ///< Alarm temporarily muted and waiting for snooze timer expiration
};

/**
 * @class AlarmController
 * @brief Manages alarm time configuration, state transitions, and radio wake-up control.
 */
class AlarmController {
public:
    /**
     * @brief Construct a new AlarmController in the `DISABLED` state.
     */
    AlarmController();

    /**
     * @brief Configure and arm the daily wake-up alarm.
     *
     * @param hour   Target hour in 24-hour format (0..23).
     * @param minute Target minute (0..59).
     * @return true if the time is valid and the alarm was armed.
     */
    bool setAlarm(int hour, int minute);

    /**
     * @brief Completely disable the alarm and mute the radio if the alarm was ringing.
     *
     * @param radio Reference to the `RadioTEA5767` driver to mute if active.
     */
    void disableAlarm(RadioTEA5767 &radio);

    /**
     * @brief Silence a currently ringing or snoozed alarm while keeping the daily alarm armed for tomorrow.
     *
     * @param radio Reference to the `RadioTEA5767` driver to mute.
     */
    void dismissRinging(RadioTEA5767 &radio);

    /**
     * @brief Snooze a currently ringing alarm for a specified number of minutes.
     *
     * Mutes the radio immediately and schedules re-triggering at `currentTime + snoozeMinutes`.
     *
     * @param radio         Reference to the `RadioTEA5767` driver to mute.
     * @param currentTime   Current `DateTime` from `ClockManager`.
     * @param snoozeMinutes Duration of snooze in minutes (defaults to `kDefaultSnoozeMinutes`).
     * @return true if the alarm was ringing and is now snoozed.
     */
    bool snooze(RadioTEA5767 &radio, const DateTime &currentTime, uint8_t snoozeMinutes = kDefaultSnoozeMinutes);

    /**
     * @brief Evaluate alarm trigger conditions against the current time.
     *
     * Should be called once per second in the main loop. Automatically resets the
     * daily trigger latch once the alarm minute rolls over so the alarm repeats next day.
     *
     * @param currentTime Current `DateTime` from `ClockManager`.
     * @param radio       Reference to the `RadioTEA5767` driver to unmute upon trigger.
     * @return true if the alarm transitioned into `RINGING` on this tick.
     */
    bool update(const DateTime &currentTime, RadioTEA5767 &radio);

    /**
     * @brief Format the alarm status text for the TFT footer (e.g. `"07:30"`, `"SNZ 07:35"`, `"--:--"`).
     *
     * @param buffer     Destination character buffer.
     * @param bufferSize Size of destination buffer in bytes.
     */
    void formatAlarmText(char *buffer, size_t bufferSize) const;

    /**
     * @brief Get the current state of the alarm state machine.
     *
     * @return Current `AlarmState`.
     */
    AlarmState getState() const;

    /**
     * @brief Get the configured alarm hour (`-1` if disabled).
     */
    int getAlarmHour() const;

    /**
     * @brief Get the configured alarm minute (`-1` if disabled).
     */
    int getAlarmMinute() const;

private:
    AlarmState state_;                ///< Current state machine status
    int        alarmHour_;            ///< Configured wake-up hour (0..23, or -1 if unset)
    int        alarmMinute_;          ///< Configured wake-up minute (0..59, or -1 if unset)
    bool       triggeredThisMinute_;  ///< Latch preventing re-trigger within the same minute
    DateTime   snoozeUntilTime_;      ///< Target wake-up timestamp when in `SNOOZED` state
};

#endif // ALARM_CONTROLLER_H

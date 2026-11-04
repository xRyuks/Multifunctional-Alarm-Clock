/**
 * @file alarm_controller.cpp
 * @brief Implementation of the daily alarm finite state machine.
 */

#include "alarm_controller.h"

AlarmController::AlarmController()
    : state_(AlarmState::DISABLED),
      alarmHour_(-1),
      alarmMinute_(-1),
      triggeredThisMinute_(false),
      snoozeUntilTime_() {}

bool AlarmController::setAlarm(int hour, int minute) {
    if (hour < 0 || hour >= 24 || minute < 0 || minute >= 60) {
        return false;
    }

    alarmHour_ = hour;
    alarmMinute_ = minute;
    state_ = AlarmState::ARMED;
    triggeredThisMinute_ = false;
    return true;
}

void AlarmController::disableAlarm(RadioTEA5767 &radio) {
    if (state_ == AlarmState::RINGING) {
        radio.setMute(true);
    }
    alarmHour_ = -1;
    alarmMinute_ = -1;
    state_ = AlarmState::DISABLED;
    triggeredThisMinute_ = false;
}

void AlarmController::dismissRinging(RadioTEA5767 &radio) {
    if (state_ == AlarmState::RINGING || state_ == AlarmState::SNOOZED) {
        radio.setMute(true);
        if (alarmHour_ >= 0 && alarmMinute_ >= 0) {
            state_ = AlarmState::ARMED;
            triggeredThisMinute_ = true; // Prevent re-triggering within the same minute
        } else {
            state_ = AlarmState::DISABLED;
        }
    }
}

bool AlarmController::snooze(RadioTEA5767 &radio, const DateTime &currentTime, uint8_t snoozeMinutes) {
    if (state_ != AlarmState::RINGING) {
        return false;
    }

    radio.setMute(true);
    snoozeUntilTime_ = currentTime + TimeSpan(0, 0, static_cast<int8_t>(snoozeMinutes), 0);
    state_ = AlarmState::SNOOZED;
    triggeredThisMinute_ = true;
    return true;
}

bool AlarmController::update(const DateTime &currentTime, RadioTEA5767 &radio) {
    if (state_ == AlarmState::DISABLED || alarmHour_ < 0 || alarmMinute_ < 0) {
        return false;
    }

    const int currentHour = static_cast<int>(currentTime.hour());
    const int currentMin  = static_cast<int>(currentTime.minute());

    // Once the alarm minute rolls over, clear the latch so the alarm repeats the next day
    if (currentHour != alarmHour_ || currentMin != alarmMinute_) {
        triggeredThisMinute_ = false;
    }

    if (state_ == AlarmState::ARMED) {
        if (!triggeredThisMinute_ && currentHour == alarmHour_ && currentMin == alarmMinute_) {
            triggeredThisMinute_ = true;
            state_ = AlarmState::RINGING;
            Serial.println("ALARM TRIGGERED! Unmuting FM radio.");
            radio.setMute(false);
            return true;
        }
    } else if (state_ == AlarmState::SNOOZED) {
        if (currentTime.unixtime() >= snoozeUntilTime_.unixtime()) {
            state_ = AlarmState::RINGING;
            Serial.println("SNOOZE EXPIRED! Unmuting FM radio.");
            radio.setMute(false);
            return true;
        }
    }

    return false;
}

void AlarmController::formatAlarmText(char *buffer, size_t bufferSize) const {
    if (buffer == nullptr || bufferSize == 0U) {
        return;
    }

    switch (state_) {
        case AlarmState::DISABLED:
            snprintf(buffer, bufferSize, "--:--");
            break;
        case AlarmState::ARMED:
            snprintf(buffer, bufferSize, "%02d:%02d", alarmHour_, alarmMinute_);
            break;
        case AlarmState::RINGING:
            snprintf(buffer, bufferSize, "RING!");
            break;
        case AlarmState::SNOOZED:
            snprintf(
                buffer,
                bufferSize,
                "S%02u:%02u",
                static_cast<unsigned int>(snoozeUntilTime_.hour()),
                static_cast<unsigned int>(snoozeUntilTime_.minute())
            );
            break;
    }
}

AlarmState AlarmController::getState() const {
    return state_;
}

int AlarmController::getAlarmHour() const {
    return alarmHour_;
}

int AlarmController::getAlarmMinute() const {
    return alarmMinute_;
}

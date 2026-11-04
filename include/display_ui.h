/**
 * @file display_ui.h
 * @brief Graphical UI manager for the 320x240 ILI9341 TFT color display.
 *
 * Implements three strictly partitioned, non-overlapping screen viewports:
 * 1. Header Banner (`y = 0..35`): Station name and tuned FM frequency.
 * 2. Now Playing Body (`y = 36..171`): Live Virtual RDS Artist & Song information.
 * 3. Clock & Alarm Footer (`y = 173..239`): Flicker-free 1 Hz digital clock and alarm indicator.
 */

#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#pragma once

#include <Arduino.h>
#include "config.h"

/**
 * @brief Initialize the 8-bit parallel data bus and ILI9341 TFT controller.
 *
 * Sets landscape orientation (`rotation = 1`) and draws the static UI frame.
 *
 * @param initialFreqMhz Initial FM frequency to display in the header banner.
 * @return true if the TFT display controller initialized successfully.
 */
bool initDisplayUi(float initialFreqMhz = kDefaultRadioFreqMhz);

/**
 * @brief Draw the static outer frame, header banner, and viewport divider line.
 *
 * @param frequencyMhz Current FM frequency in MHz shown in the right header area.
 */
void drawStaticUiFrame(float frequencyMhz = kDefaultRadioFreqMhz);

/**
 * @brief Update the tuned FM frequency badge in the header bar without redrawing the whole screen.
 *
 * @param frequencyMhz Current FM frequency in MHz.
 * @param isMuted      Current radio mute status.
 */
void renderHeaderStatus(float frequencyMhz, bool isMuted);

/**
 * @brief Render the "Now Playing" Virtual RDS metadata inside the middle viewport (`y = 42..168`).
 *
 * Strictly clips clearing and text rendering above `kDividerY` so the clock footer is never erased.
 *
 * @param artist Null-terminated artist name string.
 * @param song   Null-terminated song title string.
 */
void renderNowPlaying(const char *artist, const char *song);

/**
 * @brief Render the current time and alarm status in the bottom footer viewport (`y = 178..234`).
 *
 * Uses opaque background glyph rendering (`setTextColor(fg, bg)`) with fixed-width padding
 * instead of full-region `fillRect()` clearing, eliminating 1 Hz screen flicker.
 *
 * @param timeStr  Null-terminated `"HH:MM:SS"` string.
 * @param alarmStr Null-terminated alarm status string (e.g., `"07:30"`, `"--:--"`).
 */
void renderClock(const char *timeStr, const char *alarmStr);

#endif // DISPLAY_UI_H

/**
 * @file config.h
 * @brief Central hardware pinout, timing, network, and UI configuration constants.
 *
 * Target Hardware: Arduino UNO R4 WiFi (Renesas RA4M1 + ESP32-S3)
 */

#ifndef CONFIG_H
#define CONFIG_H

#pragma once

#include <Arduino.h>

// ============================================================================
// Hardware Pin Mapping — TFT Display (ILI9341 via 8-Bit Software Parallel Bus)
// ============================================================================
inline constexpr uint8_t kPinTftDc  = A2; ///< Data/Command (RS) control pin
inline constexpr uint8_t kPinTftCs  = A3; ///< Chip Select control pin (Active Low)
inline constexpr uint8_t kPinTftWr  = A1; ///< Write strobe control pin (Active Low)
inline constexpr uint8_t kPinTftRd  = A0; ///< Read strobe control pin (Active Low)
inline constexpr uint8_t kPinTftRst = 10; ///< Hardware Reset pin (Active Low)

inline constexpr uint8_t kPinTftD0  = 8;  ///< 8-bit parallel data bus bit 0
inline constexpr uint8_t kPinTftD1  = 9;  ///< 8-bit parallel data bus bit 1
inline constexpr uint8_t kPinTftD2  = 2;  ///< 8-bit parallel data bus bit 2
inline constexpr uint8_t kPinTftD3  = 3;  ///< 8-bit parallel data bus bit 3
inline constexpr uint8_t kPinTftD4  = 4;  ///< 8-bit parallel data bus bit 4
inline constexpr uint8_t kPinTftD5  = 5;  ///< 8-bit parallel data bus bit 5
inline constexpr uint8_t kPinTftD6  = 6;  ///< 8-bit parallel data bus bit 6
inline constexpr uint8_t kPinTftD7  = 7;  ///< 8-bit parallel data bus bit 7

// ============================================================================
// I2C Peripheral & FM Radio Constants
// ============================================================================
inline constexpr uint8_t kTea5767I2cAddr      = 0x60;   ///< TEA5767 FM tuner I2C address
inline constexpr uint8_t kRtcDs1307I2cAddr    = 0x68;   ///< DS1307 Real-Time Clock I2C address

inline constexpr float   kDefaultRadioFreqMhz = 105.2F; ///< Default station (bigFM, 105.2 MHz)
inline constexpr float   kMinRadioFreqMhz     = 87.5F;  ///< Lower FM broadcast band limit (MHz)
inline constexpr float   kMaxRadioFreqMhz     = 108.0F; ///< Upper FM broadcast band limit (MHz)
inline constexpr float   kTea5767IfFreqHz     = 225000.0F; ///< TEA5767 intermediate frequency (Hz)
inline constexpr float   kTea5767RefCrystalHz = 32768.0F;  ///< TEA5767 32.768 kHz crystal frequency

// ============================================================================
// Real-Time Clock, Alarm & Scheduler Timing Constants
// ============================================================================
inline constexpr unsigned long kSerialBaudRate        = 115200UL;  ///< USB-Serial monitor baud rate
inline constexpr unsigned long kClockUpdateIntervalMs = 1000UL;    ///< 1 Hz UI clock refresh period
inline constexpr unsigned long kRtcSyncIntervalMs     = 3600000UL; ///< Hourly hardware RTC resync (1h)
inline constexpr uint8_t       kDefaultSnoozeMinutes  = 5U;        ///< Default alarm snooze duration

// ============================================================================
// Wi-Fi & Virtual RDS (HTTPS JSONP) Constants
// ============================================================================
inline constexpr char          kRdsHost[]                   = "core-search.radioplayer.cloud";
inline constexpr uint16_t      kRdsPort                     = 443U;
inline constexpr char          kRdsPath[]                   = "/276/qp/v3/onair?rpIds=34";

inline constexpr unsigned long kRdsFetchIntervalMs          = 15000UL; ///< Poll live RDS every 15s
inline constexpr unsigned long kWifiConnectAttemptTimeoutMs = 10000UL; ///< Per-attempt Wi-Fi timeout
inline constexpr unsigned long kUserInputTimeoutMs          = 30000UL; ///< Serial prompt timeout (30s)
inline constexpr unsigned long kHttpReadTimeoutMs           = 5000UL;  ///< HTTPS socket read timeout
inline constexpr unsigned long kDhcpTimeoutMs               = 10000UL; ///< DHCP IPv4 lease timeout
inline constexpr size_t        kSerialInputBufferSize       = 64U;     ///< Max UART command line length
inline constexpr int           kWifiMaxRetries              = 3;       ///< Max Wi-Fi connect attempts

// ============================================================================
// Display UI Layout & RGB565 Color Palette (320x240 Landscape)
// ============================================================================
inline constexpr uint16_t kColorBlack  = 0x0000; ///< Background color (Black)
inline constexpr uint16_t kColorWhite  = 0xFFFF; ///< Primary text & border color (White)
inline constexpr uint16_t kColorHeader = 0xF800; ///< Header banner fill color (Red)
inline constexpr uint16_t kColorAccent = 0x07E0; ///< Alarm active indicator color (Green)
inline constexpr uint16_t kColorMuted  = 0xC618; ///< Secondary label color (Light Gray)

inline constexpr int16_t  kHeaderHeight = 36;    ///< Top banner height in pixels (y = 0..35)
inline constexpr int16_t  kDividerY     = 172;   ///< Horizontal divider separating RDS and Clock
inline constexpr int16_t  kUiPadding    = 6;     ///< Standard inner viewport margin in pixels

#endif // CONFIG_H

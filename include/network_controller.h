/**
 * @file network_controller.h
 * @brief Wi-Fi provisioning and HTTPS JSONP Virtual RDS client for the UNO R4 WiFi (ESP32-S3).
 */

#ifndef NETWORK_CONTROLLER_H
#define NETWORK_CONTROLLER_H

#pragma once

#include <Arduino.h>

/**
 * @brief Scan for nearby Wi-Fi networks, prompt the user over `Serial`, and connect via WPA2/DHCP.
 *
 * Enforces a 30-second user input timeout so the alarm clock boots into offline mode
 * automatically if no serial terminal is attached.
 *
 * @return true if Wi-Fi connected and a valid non-zero DHCP IPv4 address was assigned.
 */
bool connectToWifiDynamic();

/**
 * @brief Fetch live "Now Playing" track metadata (Virtual RDS) via HTTPS JSONP and update the TFT UI.
 *
 * Queries `core-search.radioplayer.cloud` using `WiFiSSLClient` and filters the JSONP
 * stream in-place via `ArduinoJson` v7. Redraws the "Now Playing" UI viewport only when
 * the song or broadcast state changes.
 */
void fetchVirtualRDS();

#endif // NETWORK_CONTROLLER_H

/**
 * @file network_controller.cpp
 * @brief Implementation of dynamic Wi-Fi provisioning and HTTPS JSONP Virtual RDS polling.
 */

#include "network_controller.h"

#include <ArduinoJson.h>
#include <WiFiS3.h>

#include "config.h"
#include "display_ui.h"

namespace {
WiFiSSLClient gWifiClient;
String        gLastSongTitle = ""; // Caching-Variable zur Vermeidung von Display-Flackern

void printWifiStatus() {
    Serial.print("WiFi status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "connected" : "not connected");

    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

void clearTrailingNewlines() {
    while (Serial.available() > 0) {
        const int c = Serial.peek();
        if (c == '\n' || c == '\r') {
            (void)Serial.read();
        } else {
            break;
        }
    }
}

bool readLineWithTimeout(char *buffer, const size_t bufferSize, const unsigned long timeoutMs) {
    if (buffer == nullptr || bufferSize == 0U) {
        return false;
    }

    size_t index = 0U;
    const unsigned long startMs = millis();

    while ((millis() - startMs) < timeoutMs) {
        while (Serial.available() > 0) {
            const char c = static_cast<char>(Serial.read());

            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                buffer[index] = '\0';
                clearTrailingNewlines();
                return true;
            }

            if (index < (bufferSize - 1U)) {
                buffer[index++] = c;
            }
        }
        delay(1);
    }

    buffer[0] = '\0';
    return false;
}

bool skipHttpHeaders(WiFiClient &client) {
    const unsigned long startMs = millis();
    uint8_t matchCount = 0U;

    while ((millis() - startMs) < kHttpReadTimeoutMs) {
        while (client.available() > 0) {
            const char c = static_cast<char>(client.read());
            if ((matchCount == 0U || matchCount == 2U) && c == '\r') {
                ++matchCount;
            } else if ((matchCount == 1U || matchCount == 3U) && c == '\n') {
                ++matchCount;
                if (matchCount == 4U) {
                    return true;
                }
            } else {
                matchCount = 0U;
            }
        }

        if (!client.connected()) {
            break;
        }

        delay(1);
    }

    return false;
}
} // namespace

bool connectToWifiDynamic() {
    Serial.println("Scanning WiFi networks...");
    const int networkCount = WiFi.scanNetworks();

    if (networkCount <= 0) {
        Serial.println("No WiFi networks found.");
        Serial.print("Final WiFi status/error code: ");
        Serial.println(WiFi.status());
        return false;
    }

    for (int i = 0; i < networkCount; ++i) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (RSSI ");
        Serial.print(WiFi.RSSI(i));
        Serial.println(" dBm)");
    }

    char inputBuffer[kSerialInputBufferSize] = {0};
    Serial.println("Type the network number and press Enter (30s timeout):");
    if (!readLineWithTimeout(inputBuffer, sizeof(inputBuffer), kUserInputTimeoutMs)) {
        Serial.println("Timed out waiting for network selection.");
        Serial.print("Final WiFi status/error code: ");
        Serial.println(WiFi.status());
        return false;
    }

    char *parseEnd = nullptr;
    const long selectedNumber = strtol(inputBuffer, &parseEnd, 10);
    if (parseEnd == inputBuffer || *parseEnd != '\0' || selectedNumber < 1 || selectedNumber > networkCount) {
        Serial.println("Invalid network number.");
        Serial.print("Final WiFi status/error code: ");
        Serial.println(WiFi.status());
        return false;
    }

    const int selectedIndex = static_cast<int>(selectedNumber - 1);
    const String selectedSsid = WiFi.SSID(selectedIndex);

    Serial.print("Selected SSID: ");
    Serial.println(selectedSsid);
    Serial.println("Type WPA2 password and press Enter (30s timeout):");

    char passwordBuffer[kSerialInputBufferSize] = {0};
    if (!readLineWithTimeout(passwordBuffer, sizeof(passwordBuffer), kUserInputTimeoutMs)) {
        Serial.println("Timed out waiting for password input.");
        Serial.print("Final WiFi status/error code: ");
        Serial.println(WiFi.status());
        return false;
    }

    int finalStatus = WL_IDLE_STATUS;
    for (int attempt = 1; attempt <= kWifiMaxRetries; ++attempt) {
        Serial.print("WiFi connect attempt ");
        Serial.print(attempt);
        Serial.print("/");
        Serial.println(kWifiMaxRetries);

        WiFi.disconnect();
        delay(300);
        (void)WiFi.begin(selectedSsid.c_str(), passwordBuffer);

        const unsigned long attemptStartMs = millis();
        while ((millis() - attemptStartMs) < kWifiConnectAttemptTimeoutMs) {
            finalStatus = WiFi.status();
            if (finalStatus == WL_CONNECTED) {
                Serial.println("WiFi connected.");
                printWifiStatus();

                Serial.println("Waiting for DHCP IP assignment...");
                const unsigned long dhcpStartMs = millis();
                while ((millis() - dhcpStartMs) < kDhcpTimeoutMs) {
                    const IPAddress localIp = WiFi.localIP();
                    const IPAddress invalidIp(0, 0, 0, 0);
                    if (localIp != invalidIp) {
                        Serial.println("\nDHCP IP assignment successful.");
                        Serial.print("Final WiFi status/error code: ");
                        Serial.println(finalStatus);
                        return true;
                    }
                    delay(500);
                    Serial.print('~');
                }

                Serial.println("\nDHCP timeout: IP address still 0.0.0.0 after 10 seconds.");
                Serial.println("WiFi connection failed due to DHCP.");
                break;
            }
            delay(250);
            Serial.print('.');
        }
        Serial.println();
        Serial.print("Attempt failed with status/error code: ");
        Serial.println(finalStatus);
    }

    Serial.println("WiFi connection failed after retries.");
    Serial.print("Final WiFi status/error code: ");
    Serial.println(finalStatus);
    return false;
}

void fetchVirtualRDS() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("RDS fetch skipped: WiFi not connected.");
        return;
    }

    gWifiClient.stop();
    gWifiClient.setTimeout(kHttpReadTimeoutMs);

    if (!gWifiClient.connect(kRdsHost, kRdsPort)) {
        Serial.println("RDS fetch failed: HTTP connection could not be established.");
        return;
    }

    gWifiClient.print("GET ");
    gWifiClient.print(kRdsPath);
    gWifiClient.println(" HTTP/1.1");
    gWifiClient.print("Host: ");
    gWifiClient.println(kRdsHost);
    gWifiClient.println("User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/16.4 Safari/605.1.15");
    gWifiClient.println("Accept: application/json, text/plain, */*");
    gWifiClient.println("Accept-Encoding: identity");
    gWifiClient.println("Connection: close");
    gWifiClient.println();

    if (!gWifiClient.connected()) {
        Serial.println("RDS fetch failed: connection dropped after request.");
        gWifiClient.stop();
        return;
    }

    if (!skipHttpHeaders(gWifiClient)) {
        Serial.println("RDS fetch failed: HTTP headers could not be skipped.");
        gWifiClient.stop();
        return;
    }

    if (!gWifiClient.find("(")) {
        Serial.println("RDS fetch failed: JSONP prefix not found.");
        gWifiClient.stop();
        return;
    }

    JsonDocument filter;
    filter["results"]["34"][0]["song"] = true;
    filter["results"]["34"][0]["artistName"] = true;
    filter["results"]["34"][0]["name"] = true;

    JsonDocument doc;
    const DeserializationError error = deserializeJson(
        doc, gWifiClient, DeserializationOption::Filter(filter)
    );

    if (error) {
        Serial.print("RDS JSON parse failed: ");
        Serial.println(error.c_str());
        gWifiClient.stop();
        return;
    }

    const bool isSong = doc["results"]["34"][0]["song"] | false;
    const char *artist = doc["results"]["34"][0]["artistName"] | "Unbekannter Kuenstler";
    const char *song = doc["results"]["34"][0]["name"] | "Unbekannter Song";

    if (isSong) {
        const String currentSong = String(song);
        // Display-Aktualisierung nur bei echtem Songwechsel (verhindert Flackern)
        if (currentSong != gLastSongTitle) {
            gLastSongTitle = currentSong;

            Serial.println("\n>>> NOW PLAYING auf bigFM <<<");
            Serial.print("Artist: ");
            Serial.println(artist);
            Serial.print("Song:   ");
            Serial.println(song);
            Serial.println("-----------------------------\n");

            renderNowPlaying(artist, song);
        }
    } else {
        if (gLastSongTitle != "__MODERATION__") {
            gLastSongTitle = "__MODERATION__";
            renderNowPlaying("bigFM", "Werbung / Moderation");
        }
    }

    gWifiClient.stop();
}

/**
 * @file display_ui.cpp
 * @brief Implementation of the ILI9341 320x240 TFT user interface renderer.
 */

#include "display_ui.h"
#include <Arduino_GFX_Library.h>

namespace {
// 8-Bit Software Parallel Bus und ILI9341 Display-Instanz (gekapselt im UI-Modul)
Arduino_DataBus *gDataBus = nullptr;
Arduino_GFX     *gTft     = nullptr;

void ensureDisplayAllocated() {
    if (gDataBus == nullptr) {
        gDataBus = new Arduino_SWPAR8(
            kPinTftDc, kPinTftCs, kPinTftWr, kPinTftRd,
            kPinTftD0, kPinTftD1, kPinTftD2, kPinTftD3,
            kPinTftD4, kPinTftD5, kPinTftD6, kPinTftD7
        );
    }
    if (gTft == nullptr) {
        gTft = new Arduino_ILI9341(gDataBus, kPinTftRst);
    }
}
} // namespace

bool initDisplayUi(float initialFreqMhz) {
    ensureDisplayAllocated();

    if (!gTft->begin()) {
        Serial.println("FEHLER: TFT Init fehlgeschlagen!");
        return false;
    }

    Serial.println("TFT (ILI9341) erfolgreich initialisiert.");
    gTft->setRotation(1); // 320x240 Landscape
    drawStaticUiFrame(initialFreqMhz);
    return true;
}

void drawStaticUiFrame(float frequencyMhz) {
    ensureDisplayAllocated();

    const int16_t width       = gTft->width();
    const int16_t height      = gTft->height();
    const int16_t headerTextY = (kHeaderHeight - 16) / 2;

    gTft->fillScreen(kColorBlack);

    // 1. Header-Banner (y = 0 .. kHeaderHeight - 1)
    gTft->fillRect(0, 0, width, kHeaderHeight, kColorHeader);
    gTft->drawRect(0, 0, width, kHeaderHeight, kColorWhite);

    // 2. Hauptrahmen & horizontale Trennlinie zwischen RDS-Bereich und Uhren-Footer
    gTft->drawRect(0, kHeaderHeight, width, height - kHeaderHeight, kColorWhite);
    gTft->drawFastHLine(1, kDividerY, width - 2, kColorWhite);

    gTft->setTextWrap(false);
    gTft->setTextColor(kColorWhite, kColorHeader);
    gTft->setTextSize(2);
    gTft->setCursor(kUiPadding, headerTextY);
    gTft->print("bigFM Radio");

    renderHeaderStatus(frequencyMhz, true);
}

void renderHeaderStatus(float frequencyMhz, bool isMuted) {
    ensureDisplayAllocated();

    const int16_t headerTextY = (kHeaderHeight - 16) / 2;
    char freqBuf[20];
    snprintf(
        freqBuf,
        sizeof(freqBuf),
        "%5.1f MHz %s",
        static_cast<double>(frequencyMhz),
        isMuted ? "[M]" : "[ON]"
    );

    gTft->setTextWrap(false);
    gTft->setTextColor(kColorWhite, kColorHeader);
    gTft->setTextSize(2);
    gTft->fillRect(160, 2, gTft->width() - 162, kHeaderHeight - 4, kColorHeader);
    gTft->setCursor(164, headerTextY);
    gTft->print(freqBuf);
}

void renderNowPlaying(const char *artist, const char *song) {
    ensureDisplayAllocated();

    const int16_t width = gTft->width();
    const int16_t textX = kUiPadding;
    const int16_t textY = kHeaderHeight + kUiPadding;
    const int16_t textW = width - (kUiPadding * 2);
    // Strikt oberhalb der Trennlinie (kDividerY) begrenzen, damit die Uhrzeit nie überschrieben wird!
    const int16_t textH = kDividerY - textY - 2;

    gTft->fillRect(textX, textY, textW, textH, kColorBlack);
    gTft->setTextWrap(true);
    gTft->setTextColor(kColorWhite, kColorBlack);
    gTft->setTextSize(2);

    gTft->setCursor(textX, textY);
    gTft->print("Artist: ");
    gTft->println(artist != nullptr ? artist : "-");
    gTft->println();
    gTft->print("Song: ");
    gTft->println(song != nullptr ? song : "-");
}

void renderClock(const char *timeStr, const char *alarmStr) {
    ensureDisplayAllocated();

    const int16_t textX = kUiPadding + 2;
    const int16_t textY = kDividerY + 20;

    // Flackerfreies Rendering durch opake Hintergrundfarbe (kein fillRect im Sekundentakt nötig)
    gTft->setTextWrap(false);
    gTft->setTextColor(kColorWhite, kColorBlack);

    // Links: Aktuelle Uhrzeit HH:MM:SS (Textgröße 3 -> 8 Zeichen * 18px = 144px Breite)
    gTft->setTextSize(3);
    gTft->setCursor(textX, textY);
    gTft->print(timeStr != nullptr ? timeStr : "00:00:00");

    // Rechts: Alarmstatus mit fester Breite gepaddet, um Restzeichen sauber zu überschreiben
    char alarmField[16];
    const char *effectiveAlarm = (alarmStr != nullptr && alarmStr[0] != '\0') ? alarmStr : "--:--";
    snprintf(alarmField, sizeof(alarmField), "Alarm:%-6s", effectiveAlarm);

    gTft->setTextSize(2);
    gTft->setCursor(textX + 154, textY + 6);
    gTft->print(alarmField);
}

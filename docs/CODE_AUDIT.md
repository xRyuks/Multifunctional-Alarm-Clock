# Technical Code & Feasibility Audit — Multifunktionaler Radiowecker

**Audit Date:** November 4, 2026  
**Target Platform:** Arduino UNO R4 WiFi (Renesas RA4M1 ARM Cortex-M4 @ 48 MHz + ESP32-S3 Wi-Fi Coprocessor)  
**Framework & Build System:** Arduino Framework via PlatformIO (`platform = renesas-ra`, `board = uno_r4_wifi`)  
**Language Standard:** C++ (using C++17 `inline constexpr` features)

---

## 1. System Inventory & Hardware Peripherals

### 1.1 Microcontroller & Build Environment
- **Primary MCU:** Renesas RA4M1 (`R7FA4M1AB3CFM`, 48 MHz ARM Cortex-M4, 32 KB SRAM, 256 KB Flash).
- **Network Coprocessor:** Onboard ESP32-S3-MINI-1 (`WiFiS3` library) handling Wi-Fi 802.11 b/g/n and hardware TLS/SSL sockets (`WiFiSSLClient`).
- **External Libraries (`platformio.ini`):**
  - `bblanchon/ArduinoJson @ ^7.0.4` — JSON deserialization with filter projection.
  - `moononournation/Arduino_GFX @ v1.6.5` — Graphic display HAL and `Arduino_ILI9341` driver.
  - `adafruit/RTClib @ ^2.1.4` — Real-Time Clock driver (`RTC_DS1307`, `DateTime`, `TimeSpan`).

### 1.2 Hardware Peripherals & Pin Mapping
| Peripheral | Controller / IC | Interface / Bus | Pins Used (UNO R4 WiFi) | Function |
| :--- | :--- | :--- | :--- | :--- |
| **FM Stereo Radio** | Philips/NXP `TEA5767` | Primary I2C (`Wire`, `0x60`) | `A4` (SDA), `A5` (SCL) | FM tuner (fixed/default `105.2 MHz`, hardware mute control) |
| **Real-Time Clock** | Maxim `DS1307` | Primary I2C (`Wire`, `0x68`) | `A4` (SDA), `A5` (SCL) | Battery-backed hardware timekeeping |
| **TFT Color Display** | `ILI9341` (320×240) | 8-bit Parallel (`Arduino_SWPAR8`) | `A2` (DC/RS), `A3` (CS), `A1` (WR), `A0` (RD), `10` (RST)<br>Data: `8` (D0), `9` (D1), `2`..`7` (D2..D7) | Graphical UI for header, Now Playing (Virtual RDS), Clock & Alarm |
| **Wi-Fi / Virtual RDS** | Onboard `ESP32-S3` | Internal UART Bridge (`WiFiS3`) | Internal PCB routing | Fetches live bigFM RDS metadata via HTTPS JSONP endpoint |
| **User Terminal** | USB-CDC / UART | `Serial` (`115200` baud) | `0` (RX), `1` (TX) / USB | Interactive Wi-Fi provisioning & runtime alarm/clock commands |

---

## 2. What Currently Works Well

1. **Audio Popping Mitigation Strategy (`src/main.cpp:140-147`):**
   - Polling the shared I2C bus (`Wire`) every second causes audible clicking/popping on the analog output of poorly decoupled TEA5767 modules. Keeping a local `DateTime currentRealTime` variable in RAM and syncing with the hardware DS1307 once per hour (`3600000UL` ms) is a smart hardware-aware workaround.
2. **Low-Memory JSONP Parsing (`src/NetworkController.cpp:237-257`):**
   - Using `wifiClient.find("(")` to strip the JSONP wrapper together with `ArduinoJson` v7 `DeserializationOption::Filter` avoids buffering the massive bigFM API payload in the RA4M1's 32 KB SRAM.
3. **Display Flicker Prevention for RDS (`src/NetworkController.cpp:265-277`):**
   - Caching `lastSongTitle` prevents redrawing the "Now Playing" region every 15 seconds when the song has not changed.
4. **Modern C++ Constants (`include/Config.h`):**
   - Using `inline constexpr` with explicit types (`uint16_t`, `unsigned long`, `size_t`) instead of untyped `#define` macros is clean and type-safe.
5. **Defensive Serial Line Reader in Wi-Fi Provisioning (`src/NetworkController.cpp:34-64`):**
   - `readLineWithTimeout()` properly bounds buffer writes (`index < bufferSize - 1U`), strips CR/LF, and enforces a 30-second timeout so the device boots offline if no user is connected to the serial terminal.

---

## 3. Technical Audit Findings & Bugs

### 3.1 Compilation, Linker & Structural Issues

| ID | Severity | Location | Issue Description |
| :--- | :--- | :--- | :--- |
| **C-01** | **Medium** | `src/test_wire1.cpp:1-7` | **Dead test code inside `src/` compiled into production binary:** PlatformIO automatically compiles all `.cpp` files under `src/`. `test_wire1.cpp` defines an unused global function `test_wire1()` accessing `Wire1` (the Qwiic bus). While it currently compiles without a duplicate `setup()`/`loop()`, it pollutes the translation units and global symbol table. |
| **C-02** | **Medium** | `include/RadioTEA5767.h:6-78` | **Header-only hardware driver implementation:** `TEA5767` places all I2C (`Wire`) and `Serial` debug logic directly inside the header. If included in multiple `.cpp` files, the compiler must duplicate or inline I2C/Serial routines across translation units, increasing flash footprint and coupling headers to hardware buses. |
| **C-03** | **Medium** | `src/DisplayUI.cpp:5-12`<br>`include/DisplayUI.h:5-6` | **Static Initialization Order Fiasco & Raw Global Pointers:** `bus` and `tft` are allocated via `new` at dynamic global initialization time before `main()` / `initVariant()` configures MCU clocks and GPIO registers. Furthermore, exposing raw `extern Arduino_DataBus *bus` and `extern Arduino_GFX *tft` breaks encapsulation. |
| **C-04** | **Low** | `src/main.cpp:72, 82, 84` | **Redundant `Wire.begin()` initializations:** `Wire.begin()` is called three times during `setup()` (manual I2C scan, `radio.begin()`, and `rtc.begin(&Wire)`). |
| **C-05** | **Low** | Root Directory | **Repository clutter:** `test_tea.py` (a Python scraper script) and `desktop.ini` sit in the project root. |

---

### 3.2 Real-Time, Timing & Concurrency Bugs

| ID | Severity | Location | Issue Description |
| :--- | :--- | :--- | :--- |
| **RT-01** | **CRITICAL** | `src/main.cpp:137-147` | **Severe Cumulative Clock Drift (~6 Minutes Lost per Hour):** In `loop()`, when `(now - lastClockUpdateMs) >= 1000`, the code sets `lastClockUpdateMs = now;` and increments `currentRealTime` by **hardcoded `+1 second`** (`TimeSpan(0, 0, 0, 1)`). However, `fetchVirtualRDS()` runs synchronously every 15 seconds on the same thread and blocks for **2 to 15+ seconds** (TLS handshake + HTTP headers + JSON stream read). After a 3-second HTTPS call, `now - lastClockUpdateMs` is `3000 ms`, yet the clock only advances by `1 second`—losing `2 seconds` on every RDS fetch! Over one hour (240 RDS fetches), the software clock falls behind by **5 to 8 minutes** before jumping forward at the hourly RTC sync. |
| **RT-02** | **High** | `src/NetworkController.cpp:200-287`<br>`src/main.cpp:131-134` | **Main-Loop Freezing During Synchronous HTTPS / TLS Polling:** `fetchVirtualRDS()` performs blocking DNS resolution, TCP connect, TLS 1.2/1.3 handshake (`wifiClient.connect(kRdsHost, 443)`), blocking header skipping (`skipHttpHeaders` up to 5000 ms), and `wifiClient.find("(")` (up to 5000 ms). While `fetchVirtualRDS()` executes, the 1-second UI clock freezes, serial commands are unresponsive, and alarm evaluation is delayed. |
| **RT-03** | **Low** | N/A (No ISRs used) | **ISR & `volatile` Audit:** The codebase currently uses cooperative polling in `loop()` and does not register hardware or timer Interrupt Service Routines (`attachInterrupt` / `FspTimer`), so there are no missing `volatile` bugs or ISR I2C deadlocks at present. |

---

### 3.3 Logic, UI & Edge-Case Bugs

| ID | Severity | Location | Issue Description |
| :--- | :--- | :--- | :--- |
| **L-01** | **CRITICAL** | `src/main.cpp:50-53, 160-166` | **Ringing Alarm Cannot Be Silenced / Stopped (`ALARM OFF` Bug):** When the alarm triggers (`main.cpp:164`), `radio.setMute(false)` unmutes the TEA5767 radio. When the user enters `ALARM OFF` (`main.cpp:50-53`), the code sets `alarmHour = -1; alarmMinute = -1;` but **never calls `radio.setMute(true)`** and never resets `alarmTriggered = false`. Once triggered, the alarm plays indefinitely until hardware reset. |
| **L-02** | **High** | `src/DisplayUI.cpp:31-50, 52-75` | **Display Region Collision (`renderNowPlaying` Erases the Clock):** On the 320×240 screen (`rotation = 1`), `renderNowPlaying()` clears `y = 42` through `y = 234` (`textH = 240 - 36 - 12 = 192`). However, `renderClock()` renders at `y = 180` through `y = 230` (`height - 60`). Every time the RDS song changes, `renderNowPlaying()` wipes out the clock display, and if a song title wraps onto multiple lines, the clock overwrites the song text! |
| **L-03** | **High** | `src/main.cpp:37-49` | **Invalid Alarm Input Silently Sets Alarm to Midnight (`00:00`):** `input.substring(6, sep).toInt()` and `input.substring(sep + 1).toInt()` use Arduino's `String::toInt()`, which returns `0` when given non-digits (e.g., `ALARM ab:cd` or `ALARM :`). Since `0 >= 0 && 0 < 24`, any malformed `ALARM` command containing a colon silently sets a valid alarm for `00:00`! |
| **L-04** | **High** | `src/main.cpp:54-56` | **`SET TIME` Command Is an Empty Stub:** `else if (input.startsWith("SET TIME "))` has an empty body (`// Option für manuelle Zeiteinstellung per Terminal`). Users cannot adjust the RTC time without reflashing the firmware. |
| **L-05** | **Medium** | `src/main.cpp:84-96, 144-147` | **Unhandled RTC Hardware Failure Corrupts Software Clock:** If `rtc.begin(&Wire)` fails at boot, `currentRealTime` is left at the default `DateTime()` constructor (`2000-01-01 00:00:00`) instead of falling back to compile time, and after 1 hour (`main.cpp:144`), `rtc.now()` is called unconditionally on an uninitialized RTC bus, overwriting `currentRealTime` with invalid I2C data. |
| **L-06** | **Medium** | `src/main.cpp:160-166` | **Single-Shot Alarm Lockout (Does Not Rearm for Next Day):** Once `alarmTriggered` is set to `true`, it is never reset when the alarm minute rolls over (`currentRealTime.minute() != alarmMinute`), meaning a recurring daily alarm will only ever ring on the first day. |
| **L-07** | **Medium** | `src/DisplayUI.cpp:59-60` | **Per-Second Full-Region `fillRect` Causes Clock Flicker:** `renderClock()` executes `tft->fillRect(textX, textY, 308, 50, kColorBlack)` over software bit-banged 8-bit parallel GPIO every second before printing fixed-width strings (`HH:MM:SS` and `Alarm: HH:MM`). Since `setTextColor(kColorWhite, kColorBlack)` already draws opaque background pixels per glyph, clearing 15,400 pixels every second causes visible blinking. |
| **L-08** | **Low** | `include/RadioTEA5767.h:36-62` | **TEA5767 PLL Truncation & Missing Frequency Bounds:** `setFrequency(float)` does not clamp frequencies to the FM band (`87.5–108.0 MHz`) and casts floating-point PLL values via `static_cast<unsigned int>(...)` without rounding (`+ 0.5F`), introducing a slight downward frequency offset. |

---

## 4. Recommended Fixes for Phase 2

1. **Fix Timekeeping Drift (RT-01 & L-05):**
   - Store the baseline `DateTime rtcBaseTime` and `unsigned long lastRtcSyncMs` when syncing with the DS1307 (or compile-time fallback if RTC is absent).
   - Compute `currentRealTime = rtcBaseTime + TimeSpan((now - lastRtcSyncMs) / 1000UL)` on each UI tick and advance `lastClockUpdateMs` cleanly so blocking HTTPS calls in `fetchVirtualRDS()` never cause the clock to lose seconds.
   - Track `rtcAvailable` so hourly hardware syncs only query `rtc.now()` when the DS1307 is actually present on the I2C bus.
2. **Fix Alarm State Machine & Commands (L-01, L-03, L-04, L-06):**
   - Make `ALARM OFF` (and a dedicated `STOP` / `MUTE` / `SNOOZE` command) properly mute the radio (`radio.setMute(true)`) and clear the active ringing state.
   - Auto-rearm the daily alarm once the minute rolls past `alarmMinute` after dismissal, and add snooze support.
   - Strictly validate numeric digits in `ALARM HH:MM` before accepting the time.
   - Implement `SET TIME HH:MM[:SS]` so users can update both `currentRealTime` and the hardware DS1307 RTC via Serial.
   - Add manual radio control commands (`RADIO ON`, `RADIO OFF`, `FREQ <MHz>`) for testing and everyday use.
3. **Fix Display Layout & Flicker (L-02 & L-07):**
   - Partition the 320×240 screen into non-overlapping zones:
     - **Header Zone:** `y = 0..35` (`kHeaderHeight = 36`)
     - **Now Playing (RDS) Zone:** `y = 42..165` (with a horizontal divider line at `y = 170`)
     - **Clock & Alarm Footer Zone:** `y = 176..233`
   - Remove the per-second `fillRect` in `renderClock()` and rely on fixed-width padded strings with opaque text background `setTextColor(kColorWhite, kColorBlack)` (and `setTextWrap(false)`) for zero-flicker 1 Hz updates.
4. **Modular Architecture Refactoring (C-01 through C-05, L-08):**
   - Split the project cleanly into domain modules under `include/` and `src/`:
     - `config.h` — Hardware pins, timing constants, UI geometry, and tuner limits.
     - `clock_manager.h / .cpp` — DS1307 RTC driver wrapper + drift-free software time interpolation.
     - `alarm_controller.h / .cpp` — Alarm state machine (Armed, Ringing, Snoozed, Disabled).
     - `radio_module.h / .cpp` — TEA5767 I2C driver with frequency clamping (`87.5–108.0 MHz`) and rounded PLL calculation.
     - `network_controller.h / .cpp` — Wi-Fi provisioning and HTTPS JSONP Virtual RDS client.
     - `display_ui.h / .cpp` — Flicker-free ILI9341 UI renderer with non-overlapping viewports.
     - `input_handler.h / .cpp` — Non-blocking UART command parser (`ALARM HH:MM`, `ALARM OFF`, `SNOOZE`, `SET TIME HH:MM:SS`, `RADIO ON/OFF`, `FREQ <MHz>`).
     - `main.cpp` — Clean system orchestrator (`setup()` and `loop()`).
   - Move scratch files (`src/test_wire1.cpp`, `test_tea.py`) out of the active build tree.

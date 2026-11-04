# System Architecture & Hardware Pinout Reference

This document details the hardware architecture, pin assignments, I2C bus topology, and software module boundaries for the **Multifunktionaler Radiowecker** firmware targeting the **Arduino UNO R4 WiFi**.

---

## 1. Hardware Architecture & Pinout Reference

### 1.1 Complete Pin Assignment Table (Arduino UNO R4 WiFi)

| Component | Signal / Pin Name | UNO R4 WiFi Pin | Bus / Interface | Function & Electrical Notes |
| :--- | :--- | :--- | :--- | :--- |
| **ILI9341 TFT** | `LCD_RD` | `A0` | 8-Bit Parallel Control | Read strobe (Active Low) |
| **ILI9341 TFT** | `LCD_WR` | `A1` | 8-Bit Parallel Control | Write strobe (Active Low) |
| **ILI9341 TFT** | `LCD_RS` (`DC`) | `A2` | 8-Bit Parallel Control | Data/Command register select |
| **ILI9341 TFT** | `LCD_CS` | `A3` | 8-Bit Parallel Control | Chip Select (Active Low) |
| **ILI9341 TFT** | `LCD_RST` | `D10` | 8-Bit Parallel Control | Hardware Reset (Active Low) |
| **ILI9341 TFT** | `LCD_D0` | `D8` | 8-Bit Parallel Data `[0]` | Parallel data bit 0 |
| **ILI9341 TFT** | `LCD_D1` | `D9` | 8-Bit Parallel Data `[1]` | Parallel data bit 1 |
| **ILI9341 TFT** | `LCD_D2` | `D2` | 8-Bit Parallel Data `[2]` | Parallel data bit 2 |
| **ILI9341 TFT** | `LCD_D3` | `D3` | 8-Bit Parallel Data `[3]` | Parallel data bit 3 |
| **ILI9341 TFT** | `LCD_D4` | `D4` | 8-Bit Parallel Data `[4]` | Parallel data bit 4 |
| **ILI9341 TFT** | `LCD_D5` | `D5` | 8-Bit Parallel Data `[5]` | Parallel data bit 5 |
| **ILI9341 TFT** | `LCD_D6` | `D6` | 8-Bit Parallel Data `[6]` | Parallel data bit 6 |
| **ILI9341 TFT** | `LCD_D7` | `D7` | 8-Bit Parallel Data `[7]` | Parallel data bit 7 |
| **TEA5767 FM** | `SDA` / `SCL` | `A4` / `A5` | Primary I2C (`Wire`, `0x60`) | 5-byte PLL & hardware mute control register |
| **DS1307 RTC** | `SDA` / `SCL` | `A4` / `A5` | Primary I2C (`Wire`, `0x68`) | Battery-backed real-time clock |
| **ESP32-S3** | Internal Bridge | Internal UART | `WiFiS3` Coprocessor | 2.4 GHz Wi-Fi & TLS/HTTPS socket offload |
| **Host PC CLI** | `USB-CDC` / `UART` | `D0` / `D1` (USB) | `Serial` (`115200 8N1`) | Interactive Wi-Fi setup & runtime command parser |

---

## 2. Software Module Responsibilities

| Module | Header | Implementation | Responsibility |
| :--- | :--- | :--- | :--- |
| **Configuration** | `include/config.h` | *(Header-only `constexpr`)* | Central single source of truth for GPIO pin mappings, I2C addresses, timeouts, and UI geometry. |
| **Clock Manager** | `include/clock_manager.h` | `src/clock_manager.cpp` | Manages the DS1307 hardware RTC and computes drift-free local time via `millis()` delta interpolation to avoid 1 Hz I2C audio-popping on the TEA5767. |
| **Alarm Controller** | `include/alarm_controller.h` | `src/alarm_controller.cpp` | Implements the 4-state alarm state machine (`DISABLED`, `ARMED`, `RINGING`, `SNOOZED`) with automatic daily re-arming and snooze timer calculation. |
| **Radio Module** | `include/radio_module.h` | `src/radio_module.cpp` | Encapsulates the TEA5767 I2C register protocol, high-side injection PLL calculation with rounding, FM band clamping (`87.5–108.0 MHz`), and hardware mute. |
| **Display UI** | `include/display_ui.h` | `src/display_ui.cpp` | Drives the 320×240 ILI9341 TFT display with three strictly isolated viewports (Header, Virtual RDS body, and flicker-free 1 Hz Clock/Alarm footer). |
| **Network Controller** | `include/network_controller.h` | `src/network_controller.cpp` | Handles interactive serial Wi-Fi provisioning and polls live bigFM track metadata over HTTPS JSONP using `ArduinoJson` v7 stream filtering. |
| **Input Handler** | `include/input_handler.h` | `src/input_handler.cpp` | Non-blocking UART command line interface supporting `ALARM HH:MM`, `ALARM OFF`, `STOP`, `SNOOZE`, `SET TIME`, `RADIO ON/OFF`, `FREQ`, and `HELP`. |
| **Main Orchestrator** | — | `src/main.cpp` | Coordinates peripheral initialization in `setup()` and cooperative non-blocking scheduling in `loop()`. |

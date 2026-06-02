# Circuit Diagram — Smart Tool Management System

## Wiring Reference

### OLED SSD1306 (I2C) → XIAO ESP32-C3

| OLED Pin | ESP32-C3 Pin | Notes |
|----------|-------------|-------|
| GND | GND | Common ground |
| VCC | 3V3 | 3.3V power only |
| SDA | GPIO 4 | I2C Data |
| SCL | GPIO 5 | I2C Clock |

**I2C Address:** `0x3C` (default for most SSD1306 modules)

---

### RC522 RFID Reader (SPI) → XIAO ESP32-C3

| RC522 Pin | ESP32-C3 Pin | Notes |
|-----------|-------------|-------|
| SDA / SS / CS | GPIO 10 | SPI Chip Select |
| SCK | GPIO 6 | SPI Clock |
| MOSI | GPIO 7 | SPI Master Out |
| MISO | GPIO 2 | SPI Master In |
| RST | GPIO 3 | Reset |
| GND | GND | Common ground |
| VCC | 3V3 | ⚠️ 3.3V ONLY — NEVER 5V |

> **Warning:** The RC522 is a 3.3V module. Connecting VCC to 5V will permanently damage the chip.

---

### Active Buzzer → XIAO ESP32-C3

| Buzzer Pin | ESP32-C3 Pin | Notes |
|------------|-------------|-------|
| + (positive / signal) | GPIO 19 | Digital output HIGH = ON |
| - (negative / ground) | GND | Common ground |

**Type:** Active buzzer (generates its own tone — no PWM needed)

---

## Power Supply

| Rail | Source | Components Powered |
|------|--------|-------------------|
| 3.3V | ESP32-C3 onboard regulator | OLED, RFID RC522 |
| GPIO | ESP32-C3 digital output | Buzzer |
| USB 5V | USB-C cable | ESP32-C3 input |

---

## Notes

- All components share a **common GND** rail
- The ESP32-C3 is powered via **USB-C** from a computer or USB power bank
- SPI bus: SCK=6, MOSI=7, MISO=2, SS=10 (custom SPI init in firmware)
- I2C bus: SDA=4, SCL=5 (custom Wire.begin in firmware)
- No external pull-up resistors needed — ESP32 internal pull-ups used for I2C

---

## Circuit Diagram Image

> See `circuit_diagram.png` in this folder for the full visual wiring diagram.
> 
> Tools used for circuit design: **Fritzing** / **EasyEDA** / **KiCad**

---

## Quick Test Checklist

- [ ] OLED powers on and shows boot message
- [ ] RFID RC522 recognized (check `rfid.PCD_DumpVersionToSerial()` output in Thonny)
- [ ] Buzzer beeps on startup
- [ ] WiFi connects successfully
- [ ] Firebase authentication succeeds
- [ ] Scanning RFID card prints UID in Thonny Shell

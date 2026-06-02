# Datasheets — Smart Tool Management System

This folder contains (or links to) datasheets for all components used in the project.

---

## Component Datasheets

### 1. XIAO ESP32-C3 (Seeed Studio)

| Property | Value |
|---|---|
| Manufacturer | Seeed Studio |
| Chip | Espressif ESP32-C3 (RISC-V) |
| Flash | 4MB |
| SRAM | 400KB |
| WiFi | 2.4GHz 802.11 b/g/n |
| Bluetooth | BLE 5.0 |
| GPIO | 11 programmable pins |
| Operating Voltage | 3.3V (5V via USB) |
| Dimensions | 21 × 17.5 mm |

**Download Datasheet:**
- [XIAO ESP32-C3 Wiki](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- [ESP32-C3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-c3_technical_reference_manual_en.pdf)
- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)

---

### 2. RC522 RFID Reader Module

| Property | Value |
|---|---|
| Manufacturer | NXP (MFRC522 chip) |
| Operating Frequency | 13.56 MHz |
| Interface | SPI (up to 10 Mbps) |
| Operating Voltage | **3.3V** (CRITICAL — not 5V) |
| Current Consumption | 13–26 mA |
| Read Distance | ~3–5 cm (card), ~1–3 cm (tag) |
| Supported Cards | ISO/IEC 14443A, Mifare 1K, Mifare 4K |
| Card UID Length | 4 or 5 bytes |

**Download Datasheet:**
- [MFRC522 Datasheet (NXP)](https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf)
- [RC522 Module Pinout Guide](https://components101.com/wireless/rc522-rfid-module)

---

### 3. SSD1306 OLED Display (0.96 inch)

| Property | Value |
|---|---|
| Manufacturer | Solomon Systech (SSD1306 driver) |
| Display Size | 0.96 inch diagonal |
| Resolution | 128 × 64 pixels |
| Interface | I2C (default address 0x3C or 0x3D) |
| Operating Voltage | 3.3V – 5V |
| Current Consumption | 9 mA typical |
| Color | Monochrome (blue or white) |
| Viewing Angle | > 160° |

**Download Datasheet:**
- [SSD1306 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)
- [SSD1306 Application Note](https://www.solomon-systech.com/en/product/advanced-display/oled-display-driver-ic/ssd1306/)

---

### 4. Active Buzzer (3.3V)

| Property | Value |
|---|---|
| Type | Active (internal oscillator) |
| Operating Voltage | 3–5V |
| Current Consumption | 25–35 mA |
| Frequency | ~2.3–2.7 kHz (fixed internal) |
| Sound Level | ~85 dB at 10 cm |
| Control | Simple HIGH/LOW digital signal |
| Dimensions | 12mm diameter |

**Note:** This project uses an **active** buzzer (generates its own tone when HIGH). Do not substitute with a **passive** buzzer (requires PWM tone generation).

---

### 5. Mifare 1K RFID Cards / Tags

| Property | Value |
|---|---|
| Standard | ISO/IEC 14443A |
| Frequency | 13.56 MHz |
| Memory | 1KB (EEPROM, 16 sectors × 4 blocks) |
| UID | 4 or 5 bytes (read-only unique ID) |
| Read Range | Up to 5 cm with RC522 |
| Data Retention | > 10 years |
| Write Endurance | 100,000 cycles |

**Download Datasheet:**
- [Mifare 1K Datasheet (NXP)](https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf)

---

## How to Add Datasheets to This Folder

1. Download the PDF from the links above
2. Rename to match the filenames below
3. Upload to this folder on GitHub

| Filename | Component |
|---|---|
| `ESP32-C3_Datasheet.pdf` | Espressif ESP32-C3 chip |
| `XIAO_ESP32C3_Pinout.pdf` | Seeed Studio XIAO ESP32-C3 |
| `RC522_MFRC522_Datasheet.pdf` | NXP MFRC522 RFID chip |
| `SSD1306_OLED_Datasheet.pdf` | Solomon Systech SSD1306 |
| `Mifare_1K_Datasheet.pdf` | NXP Mifare 1K card |
| `Active_Buzzer_Spec.pdf` | Active buzzer specifications |

---

## MicroPython Libraries Used

| Library | Version | Source |
|---|---|---|
| `mfrc522.py` | latest | [github.com/cefn/micropython-mfrc522](https://github.com/cefn/micropython-mfrc522) |
| `ssd1306.py` | built-in | MicroPython firmware |
| `urequests.py` | latest | [github.com/micropython/micropython-lib](https://github.com/micropython/micropython-lib) |
| `ntptime.py` | built-in | MicroPython firmware |
| Firebase SDK | 9.22.1 | [gstatic.com CDN](https://www.gstatic.com/firebasejs/9.22.1/) |

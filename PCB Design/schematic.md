# Schematic Reference — Smart Tool Management System

## Signal Connections

```
                    ┌─────────────────────────────┐
                    │        XIAO ESP32-C3         │
                    │                             │
USB-C ─────────────►│ VIN/USB                     │
                    │                             │
                    │ 3V3 ────────────────────────┼──► RC522 VCC
                    │ 3V3 ────────────────────────┼──► OLED VCC
                    │                             │
                    │ GND ────────────────────────┼──► RC522 GND
                    │ GND ────────────────────────┼──► OLED GND
                    │ GND ────────────────────────┼──► Buzzer GND
                    │                             │
                    │ GPIO4  (SDA) ───────────────┼──► OLED SDA
                    │ GPIO5  (SCL) ───────────────┼──► OLED SCL
                    │                             │
                    │ GPIO10 (SS)  ───────────────┼──► RC522 SDA/CS
                    │ GPIO6  (SCK) ───────────────┼──► RC522 SCK
                    │ GPIO7  (MOSI)───────────────┼──► RC522 MOSI
                    │ GPIO2  (MISO)───────────────┼──► RC522 MISO
                    │ GPIO3  (RST) ───────────────┼──► RC522 RST
                    │                             │
                    │ GPIO19       ───────────────┼──► Buzzer +
                    │                             │
                    └─────────────────────────────┘
```

## Power Budget

| Component | Voltage | Current (typical) | Current (peak) |
|---|---|---|---|
| XIAO ESP32-C3 | 5V via USB | 80mA | 500mA (WiFi TX) |
| RC522 RFID | 3.3V | 13mA | 26mA |
| SSD1306 OLED | 3.3V | 9mA | 15mA |
| Active Buzzer | 3.3V | 30mA | 35mA |
| **Total** | | **~132mA** | **~576mA** |

A standard **500mA USB port** or **1A USB power bank** is sufficient.

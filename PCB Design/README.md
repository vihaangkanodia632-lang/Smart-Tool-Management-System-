# PCB Design — Smart Tool Management System

## Overview

This folder contains PCB design files for the Smart Tool Management System.
The current prototype was built on a **perfboard (perf board / veroboard)** 
for rapid prototyping. A proper PCB can be designed using EasyEDA or KiCad
based on the schematic provided.

---

## Current Prototype

The prototype uses:
- **Perfboard** — 5cm × 7cm copper-dot board
- **Pin headers** — for connecting XIAO ESP32-C3 and modules
- **Jumper wires** — for OLED and RFID connections

---

## Schematic Reference

### Components List (BOM)

| Ref | Component | Value / Part No. | Qty |
|-----|-----------|-----------------|-----|
| U1 | Microcontroller | XIAO ESP32-C3 | 1 |
| U2 | RFID Reader | RC522 SPI module | 1 |
| U3 | OLED Display | SSD1306 0.96" I2C | 1 |
| BZ1 | Buzzer | Active 3.3V buzzer | 1 |
| J1 | Connector | USB-C (on ESP32 board) | 1 |
| — | RFID Cards | Mifare 1K 13.56MHz | N |
| — | RFID Tags | Mifare 1K key fob/sticker | N |

---

## PCB Design Software

Recommended tools for PCB design:

| Tool | Link | Notes |
|------|------|-------|
| EasyEDA | https://easyeda.com | Online, free, integrates with JLCPCB |
| KiCad | https://kicad.org | Open source, professional |
| Fritzing | https://fritzing.org | Good for prototyping breadboard layouts |

---

## Design Guidelines

When designing the PCB:

1. **Keep RC522 SPI traces short** — high-frequency SPI signals are noise-sensitive
2. **Decouple VCC pins** — add 100nF capacitor close to each IC VCC pin
3. **Common ground plane** — use solid ground pour on bottom layer
4. **Separate digital/RF areas** — keep RFID antenna area clear of other traces
5. **OLED I2C** — 4.7kΩ pull-up resistors on SDA and SCL lines
6. **USB-C footprint** — route 5V directly to ESP32-C3 VIN pin

---

## PCB Ordering

Once design is complete, export Gerber files and order from:
- [JLCPCB](https://jlcpcb.com) — affordable, fast shipping to India
- [PCBWay](https://pcbway.com) — good quality
- [Aisler](https://aisler.net) — European option

Minimum order is typically **5 PCBs**.

---

## Files in This Folder

| File | Description |
|------|-------------|
| `schematic.md` | Component connections reference |
| `*.kicad_pcb` | KiCad PCB file (when designed) |
| `*.kicad_sch` | KiCad schematic file (when designed) |
| `gerbers/` | Gerber files for PCB fabrication |
| `BOM.csv` | Bill of Materials for ordering |

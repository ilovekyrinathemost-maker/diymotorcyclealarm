# DIY Motorcycle Alarm System

A custom-built, two-way motorcycle alarm and GPS tracking system designed for a 2007 Yamaha Serow (XT250).

## Features

- 🔒 **Two-way communication** — LoRa P2P fob (2-15km range) + SMS alerts to phone
- 📍 **GPS tracking** — Real-time position via Google Maps links
- 🛡️ **Comprehensive sensors** — Motion, tilt, proximity radar, ignition tamper, battery disconnect
- 🔐 **Encrypted** — AES-128-CTR + HMAC-SHA256 + anti-replay protection
- ⚡ **Low power** — ULP coprocessor monitors sensors at ~50µA during deep sleep
- 🔋 **Backup battery** — 18650 cell provides 7-15 days of stealth tracking if main power cut
- 🏍️ **Tamper-proof** — Stealth install + visible decoy siren
- 📱 **Fully local** — No cloud server, no subscriptions (just ~$3-5/month for IoT SIM)

## Architecture

```
Bike Unit (ESP32-S3 + SX1262 + SIM7080G)
    │ LoRa P2P              │ SMS (LTE-M)
    ▼                       ▼
Fob (ESP32-C3 + SX1262)    Phone (SMS + BLE config)
```

## Hardware

| Component | Bike Unit | Fob Unit |
|-----------|-----------|----------|
| MCU | ESP32-S3-WROOM-1 (N16R8) | ESP32-C3-MINI-1 |
| LoRa | Ebyte E22-900M30S (+30dBm) | Ebyte E22-900M22S (+22dBm) |
| Cellular+GPS | SIM7080G (LTE-M + GNSS) | — |
| IMU | LSM6DSO (6-axis) | — |
| Display | — | SSD1306 0.96" OLED |
| Proximity | RCWL-0516 (microwave radar) | — |

## Cost

- **Total build cost:** ~$145
- **Monthly operating:** ~$3-5 (IoT SIM for SMS)

## Project Structure

```
├── docs/specs/          # Design specification
├── hardware/            # KiCad schematics & PCB layouts
│   ├── bike-unit/
│   └── fob-unit/
├── firmware/            # ESP-IDF firmware source
│   ├── bike-unit/
│   └── fob-unit/
├── enclosures/          # 3D printable STL/STEP files
│   ├── bike-unit/
│   └── fob-unit/
└── docs/                # Documentation
```

## Status

🟡 **Design phase** — Specification complete, implementation planning in progress.

## Documentation

- [Full Design Specification](docs/specs/2026-04-29-motorcycle-alarm-design.md)

## License

MIT

# 2-Way Motorcycle Alarm System — Design Specification

**Project:** Custom GPS-tracked anti-theft alarm for 2007 Yamaha Serow (XT250)  
**Date:** 2026-04-29  
**Version:** 1.0  
**Status:** Approved for implementation  

---

## Table of Contents

1. [Overview](#1-overview)
2. [System Architecture](#2-system-architecture)
3. [Bike Unit Hardware](#3-bike-unit-hardware)
4. [Fob Unit Hardware](#4-fob-unit-hardware)
5. [Communication Protocol & Encryption](#5-communication-protocol--encryption)
6. [Firmware Architecture](#6-firmware-architecture)
7. [Phone Communication (Fully Local)](#7-phone-communication-fully-local)
8. [Physical Design & Installation](#8-physical-design--installation)
9. [Bill of Materials](#9-bill-of-materials)
10. [Project Timeline](#10-project-timeline)

---

## 1. Overview

### 1.1 Purpose

A custom-built, two-way motorcycle alarm and tracking system providing:
- Instant alerts to a handheld fob via LoRa P2P (2-15km range)
- SMS-based alerts and commands to the owner's phone (global, fully local, no cloud)
- GPS tracking with live position reporting
- Comprehensive sensor coverage (motion, tilt, proximity, tamper, voltage)
- Tamper-proof stealth installation with backup battery
- Two-way control (arm, disarm, siren, locate, sensitivity, status)

### 1.2 Target Vehicle

- **Motorcycle:** 2007 Yamaha Serow (XT250)
- **Characteristics:** Single-cylinder dual-sport, air-cooled, 12V electrical system, significant vibration profile, plastic bodywork (RF transparent), ample hiding space under seat and in frame triangle

### 1.3 Design Priorities

1. Reliability and quality (no budget constraints)
2. Low power consumption (weeks of backup battery life)
3. Tamper resistance (stealth + detection)
4. Range and responsiveness (sub-second LoRa alerts)
5. Simplicity of operation (minimal ongoing costs, no cloud dependency)

### 1.4 Architecture Decision

**Chosen: ESP32-S3 + Modular Stack with ULP coprocessor**

- ESP32-S3 as main MCU (dual-core, ULP-RISC-V, WiFi+BLE, deep sleep)
- SX1262 LoRa for P2P fob communication
- SIM7080G for cellular (SMS) + GNSS (GPS/GLONASS/BeiDou/Galileo)
- Fully local communication (SMS + BLE, no cloud server)

---

## 2. System Architecture

### 2.1 System Components

The system consists of 3 main components communicating across 2 channels:

```
┌─────────────────────────────────────────────────────────────────┐
│                        BIKE UNIT                                 │
│  ESP32-S3 + SX1262 (LoRa) + SIM7080G (LTE-M + GNSS)           │
│  Sensors: IMU, Radar, Tamper, Voltage                           │
│  Actuators: Siren, LED strobe                                   │
│  Power: 12V Buck + 18650 LiPo backup                           │
└─────────────────────────────────────────────────────────────────┘
         │ LoRa P2P (868/915MHz)              │ LTE-M (SMS)
         │ <15km range                        │ Global
         │ Encrypted, <500ms latency          │
         ▼                                    ▼
┌─────────────────────┐          ┌──────────────────────┐
│      FOB UNIT       │          │    OWNER'S PHONE     │
│  ESP32-C3 + SX1262  │          │  (SMS + BLE config)  │
│  OLED + Buttons     │          │                      │
│  500mAh LiPo       │          │                      │
└─────────────────────┘          └──────────────────────┘
```

### 2.2 Communication Channels

| Channel | Link | Protocol | Range | Latency | Use Case |
|---------|------|----------|-------|---------|----------|
| LoRa P2P | Bike ↔ Fob | Custom encrypted packets | 2-15km (LOS) | <500ms | Instant alerts, arm/disarm, locate |
| SMS | Bike → Phone | LTE-M SMS via SIM7080G | Global | 1-5s | GPS tracking, alerts, remote commands |
| BLE | Bike ↔ Phone | BLE 5.0 (ESP32-S3) | <10m | Instant | Setup, config, data download, OTA |

### 2.3 Operating Modes

| Mode | Power Draw | Active Systems | Description |
|------|-----------|----------------|-------------|
| Armed - Idle | ~5mA from 12V | ULP monitors IMU + tamper, LoRa listens | Normal parked state |
| Armed - Alert | ~150mA | Full system, siren, GPS fix, transmit | Triggered state |
| Disarmed | ~3mA | LoRa listen only, no sensors | Riding or maintenance |
| Backup Power | ~8mA | Same as Armed-Idle, cellular periodic check-in | 12V disconnected |

### 2.4 Backup Battery Life

- 18650 cell (3000mAh) at 8mA idle = ~15 days stealth tracking after main power cut
- With periodic GPS check-ins every 30min: ~7-10 days

---

## 3. Bike Unit Hardware

### 3.1 Core Components

| Component | Specific Part | Role | Interface |
|-----------|--------------|------|----------|
| MCU | ESP32-S3-WROOM-1 (N16R8) | Main brain, 16MB flash, 8MB PSRAM, ULP | — |
| LoRa | Ebyte E22-900M30S (SX1262) | P2P to fob, +30dBm TX, -148dBm RX | SPI |
| Cellular+GPS | SIM7080G | LTE-M/NB-IoT + Multi-GNSS combo | UART |
| IMU | LSM6DSO | 6-axis accel+gyro, hardware interrupt | I²C |
| Proximity | RCWL-0516 | Microwave doppler radar, 5-7m | Digital GPIO |
| Siren | Piezo 120dB module | Deterrent, 12V driven | MOSFET GPIO |
| LED Strobe | High-power LED module | Visual alert | MOSFET GPIO |
| Backup Battery | Samsung 30Q 18650 (3000mAh) | Power when 12V cut | Battery holder |
| Charger | BQ24075 | LiPo charge + power path management | — |
| Buck Converter | TPS563200 | 12V→3.3V, 93% efficiency, 3A | — |
| MOSFETs | AO3400 (N-ch), AO3401 (P-ch) | Switching siren, LEDs, power path | GPIO |
| Optocoupler | PC817 | Ignition line isolation | GPIO |
| Reed Switch | NC magnetic | Enclosure tamper detection | GPIO |

### 3.2 Pin Assignment (ESP32-S3)

| GPIO | Function | Interface | Notes |
|------|----------|-----------|-------|
| 1-4 | SX1262 LoRa | SPI (MOSI, MISO, SCK, CS) | HSPI bus |
| 5 | SX1262 DIO1 (IRQ) | Digital In | Packet received/sent interrupt |
| 6 | SX1262 BUSY | Digital In | Module busy flag |
| 7 | SX1262 RST | Digital Out | Module reset |
| 17, 18 | SIM7080G | UART1 TX/RX | AT commands, 115200 baud |
| 8 | SIM7080G PWRKEY | Digital Out | Power on/off toggle |
| 9 | SIM7080G STATUS | Digital In | Module status |
| 10, 11 | LSM6DSO IMU | I²C (SDA, SCL) | Address 0x6A |
| 12 | LSM6DSO INT1 | Digital In (interrupt) | Wake-on-motion → ULP |
| 13 | RCWL-0516 | Digital In | Proximity trigger |
| 14 | Siren MOSFET | Digital Out | 120dB piezo drive |
| 15 | LED Flasher MOSFET | Digital Out | High-power LED strobe |
| 16 | Horn Relay | Digital Out | Optional horn tap |
| 38 | 12V ADC Monitor | ADC1 | Via voltage divider (12V→3.3V) |
| 39 | Backup Battery ADC | ADC1 | LiPo voltage monitor |
| 40 | Tamper Switch | Digital In (pullup) | Enclosure open detection |
| 41 | Ignition Sense | Digital In | Via optocoupler |
| 42 | Power Path Control | Digital Out | Switch between 12V and backup |

### 3.3 Power Architecture

```
     12V Motorcycle Battery
            │
            ├──[Fuse 3A]──┬──[TPS563200 Buck]──→ 3.3V Rail (Main System)
            │              │
            │              └──[BQ24075 Charger]──→ 18650 LiPo (Backup)
            │                                          │
            │                                          ▼
            │                              [Power Path MOSFET]
            │                                          │
            └──[Voltage Divider]──→ ADC (12V Monitor)  │
                                                       ▼
                                              3.3V Rail (if 12V lost)
```

- Normal: 12V present → Buck provides 3.3V, charger tops up LiPo
- Tamper: 12V cut → System detects via ADC drop, LiPo takes over seamlessly
- Auto-switchover time: <10ms (P-MOSFET body diode provides instant bridge)

### 3.4 Sensor Details

#### Motion/Tilt (LSM6DSO)
- Accelerometer range: ±2g (high sensitivity for parking)
- Gyroscope: detects rotation (bike being wheeled away)
- Hardware interrupts: wake-on-motion threshold programmable via I²C
- Tilt detection: compare gravity vector angle, trigger if >15° change
- ULP coprocessor reads IMU over I²C while main CPU sleeps

#### Proximity (RCWL-0516)
- Microwave radar at 3.18GHz — works through plastic/3D printed enclosure
- Detection range: adjustable via resistor (default ~5m, reduce to 2-3m for parking)
- Output: HIGH for 2-3 seconds when motion detected
- Current: ~3mA active (power-gated when disarmed)

#### Voltage/Tamper
- 12V monitor: 100K/33K resistor divider → ~2.97V at ADC for 12V input
- Rapid voltage drop (>2V/100ms) = wire cut detection
- Enclosure tamper: magnetic reed switch, NC (normally closed)
- Ignition sense: optocoupler isolates ignition line, detects hot-wire attempts

### 3.5 Antenna Strategy

| Antenna | Type | Placement |
|---------|------|----------|
| LoRa (868/915MHz) | SMA pigtail to external whip | Vertical, along frame tube, hidden in bodywork |
| LTE-M | FPC flexible antenna | Inside enclosure, away from metal |
| GNSS | 25x25mm ceramic patch | Under seat cowl (plastic = RF transparent), facing sky |

---

## 4. Fob Unit Hardware

### 4.1 Core Components

| Component | Specific Part | Role | Interface |
|-----------|--------------|------|----------|
U | ESP32-C3-MINI-1 (N4) | Main brain, RISC-V, BLE 5.0 | — |
| LoRa | Ebyte E22-900M22S (SX1262) | P2P to bike, +22dBm TX | SPI |
| Display | SSD1306 0.96" OLED (128x64) | Status, alerts, UI | I²C |
| Vibration | ERM coin motor (10mm, 3V) | Haptic feedback | GPIO + transistor |
| Buzzer | Passive piezo | Multi-tone alerts | PWM GPIO |
| Buttons | 3x tactile (6x6mm) | Arm/Disarm, Locate, Panic | GPIO + pullup |
| LED | WS2812B RGB | Status indicator | Digital GPIO |
| Battery | 502535 LiPo (500mAh) | Main power | — |
| Charger | TP4056 USB-C | Charging circuit | — |
| Antenna | Spring helical (868/915MHz) | LoRa antenna | Soldered to PCB |

### 4.2 Pin Assignment (ESP32-C3)

| GPIO | Function | Interface |
|------|----------|----------|
| 0 | Button 1 (Arm/Disarm) | Digital In + pullup |
| 1 | Button 2 (Locate) | Digital In + pullup |
| 2 | Button 3 (Panic/Status) | Digital In + pullup |
| 3 | Vibration Motor | Digital Out (via transistor) |
| 4 | Buzzer PWM | PWM Out |
| 5 | RGB LED Data | Digital Out |
| 6, 7 | SSD1306 OLED | I²C (SDA, SCL) |
| 8-10, 18 | SX1262 LoRa | SPI (MOSI, MISO, SCK, CS) |
| 19 | SX1262 DIO1 (IRQ) | Digital In (interrupt) |
| 20 | SX1262 BUSY | Digital In |
| 21 | SX1262 RST | Digital Out |
| ADC1 | Battery voltage | ADC via divider |

### 4.3 Power Budget

| Mode | Current Draw | Duration |
|------|-------------|----------|
| Deep Sleep (LoRa duty cycle listen) | ~2.5mA avg | 99% of time |
| Wake on alert | ~45mA | 2-5 seconds |
| Active use (button press) | ~35mA | <3 seconds |

- Estimated battery life (standby): 7-10 days on 500mAh
- With 5 alerts/day: 6-8 days

### 4.4 User Interface

#### Button Actions

| Button | Short Press | Long Press (3s) |
|--------|------------|------------------|
| ARM | Toggle arm/disarm | Sensitivity cycle (Low→Med→High) |
| LOCATE | Chirp + flash bike | Request full status update |
| PANIC | Trigger siren immediately | Silent alarm (cellular alert only) |

#### Alert Feedback

| Alert Level | Vibration | Buzzer | LED | OLED |
|-------------|-----------|--------|-----|------|
| Warning (proximity) | 2 short pulses | Soft beep | Yellow flash | "Movement nearby" |
| Alert (motion/tilt) | Continuous pulse | Alarm tone | Red flash | "BIKE DISTURBED" |
| Critical (GPS moving) | Aggressive pulse | Loud siren tone | Red strobe | "BIKE MOVING!" |
| Confirmation | 1 short buzz | Chirp | Green flash | Action confirmed |

### 4.5 Physical Design

- Size target: 65mm × 38mm × 16mm (pocket/keychain size)
- Material: PETG (impact resistant, easy to print)
- Features: Lanyard hole, sealed USB-C port, snap-fit + M2 screw assembly

---

## 5. Communication Protocol & Encryption

### 5.1 LoRa Radio Parameters

| Parameter | Value | Rationale |
|-----------|-------|----------|
| Frequency | 868 MHz (EU) / 915 MHz (US/AU) | ISM band, license-free |
| Spreading Factor | SF9 (default), SF12 (long range) | Good balance of range/speed |
| Bandwidth | 125 kHz | Standard, good sensitivity |
| Coding Rate | 4/5 | Minimal overhead |
| TX Power (Bike) | +30 dBm (1W) | Maximum range for alerts |
| TX Power (Fob) | +22 dBm (158mW) | Battery-friendly |
| Sync Word | 0x34 (private) | Distinguish from LoRaWAN traffic |

### 5.2 Packet Format (32 bytes max)

```
| PKT_TYPE | DEVICE_ID | SEQ_NUM  | PAYLOAD     | HMAC    | FLAGS |
| 1 byte   | 2 bytes   | 4 bytes  | 20 bytes    | 4 bytes | 1 byte|
|          |           | (counter)| (encrypted) | (trunc) |       |
```

### 5.3 Encryption

| Layer | Algorithm | Purpose |
|-------|-----------|--------|
| Confidentiality | AES-128-CTR | Encrypt payload |
| Integrity | HMAC-SHA256 (truncated 4B) | Verify packet integrity |
| Anti-replay | Sequence number (uint32) | Reject old/replayed packets |
| Key derivation | HKDF-SHA256 | Derive session keys from master key |

Key hierarchy:
- Master Key (256-bit, generated during pairing)
  - → ENC_KEY (128-bit) for AES-128-CTR
  - → HMAC_KEY (256-bit) for HMAC-SHA256

### 5.4 Anti-Replay Protection

- Each device maintains a TX counter (uint32, stored in NVS flash)
- RX window accepts only packets where SEQ > last_received_seq
- Counter persists across reboots (NVS write every 100 increments)
- Accepts SEQ within last_seq+1 to last_seq+256 (window for packet loss)

### 5.5 Pairing Procedure

1. Hold PAIR button on bike unit (5s) → enters pairing mode
2. Hold ARM+LOCATE on fob (5s) → enters pairing mode
3. Bike broadcasts PAIR_REQUEST with random challenge
4. Fob displays 6-digit code on OLED derived from challenge
5. User confirms code matches LED blink pattern on bike unit
6. Diffie-Hellman key exchange (Curve25519) over LoRa
7. Master key derived, stored in ESP32 NVS (encrypted partition)
8. Both devices store each other's DEVICE_ID
9. Confirmation chirp + OLED "Paired ✓"

### 5.6 Message Types

| PKT_TYPE | Direction | Name | Payload |
|----------|-----------|------|--------|
| 0x01 | Bike → Fob | ALERT | alert_type, severity, sensor_data, gps, speed, heading, battery |
| 0x02 | Fob → Bike | COMMAND | cmd_type, parameters |
| 0x03 | Both | ACK | acked_seq, status |
| 0x04 | Bike → Fob | STATUS | bike_batt, gps_fix, lat, lon, temp, signal, armed |
| 0x05 | Bike → Fob | HEARTBEAT | bike_batt, armed, signal |
| 0x06 | Both | PAIRING | pairing protocol data |

### 5.7 Command Types (Fob → Bike)

| CMD_TYPE | Command | Parameters |
|----------|---------|------------|
| 0x10 | ARM | — |
| 0x11 | DISARM | — |
| 0x20 | SIREN_ON | duration(2B) |
| 0x21 | SIREN_OFF | — |
| 0x30 | LED_FLASH | pattern, duration |
| 0x40 | LOCATE | mode: chirp/flash/both |
| 0x50 | SENSITIVITY | level: low/med/high |
| 0x60 | STATUS_REQ | — |

### 5.8 Alert Types (Bike → Fob)

| ALERT_TYPE | Meaning | Severity |
|------------|---------|----------|
| 0x01 | Motion detected | WARNING |
| 0x02 | Tilt/tip over | ALERT |
| 0x03 | Proximity trigger | WARNING |
| 0x04 | Ignition tamper | CRITICAL |
| 0x05 | Battery disconnect | CRITICAL |
| 0x06 | Geofence breach | ALERT |
| 0x07 | GPS moving | CRITICAL |
| 0x08 | Enclosure tamper | CRITICAL |

---

## 6. Firmware Architecture

### 6.1 Framework & Toolchain

| Item | Choice | Rationale |
|------|--------|----------|
| Framework | ESP-IDF v5.x | Full control over ULP, power, partitions, OTA |
| RTOS | FreeRTOS (built-in) | Task-based concurrency |
| ULP Programming | ULP-RISC-V | C-programmable coprocessor |
| Fob Framework | ESP-IDF (ESP32-C3) | Same ecosystem |

### 6.2 Bike Unit Task Architecture

| Task | Priority | Core | Stack | Role |
|------|----------|------|-------|------|
| Alert Manager | 7 (highest) | 1 | 4KB | Coordinates alarm responses, state transitions |
| Sensor Task | 6 | 0 | 4KB | Processes ULP wake events, reads sensors, filters false positives |
| LoRa Task | 5 | 0 | 8KB | All LoRa TX/RX, encrypt/decrypt, fob communication |
| Cellular Task | 4 | 1 | 8KB | SMS send/receive, AT commands to SIM7080G |
| GPS Task | 3 | 0 | 4KB | GNSS fix, geofence calculation, position logging |
| OTA Task | 2 | 1 | 8KB | Firmware update over BLE |
| Logging Task | 1 (lowest) | 0 | 4KB | Flash storage of events, GPS breadcrumbs |

### 6.3 ULP-RISC-V Coprocessor (Always-On Monitor)

Runs at ~10Hz during deep sleep:
1. Read IMU via I²C (accel X/Y/Z)
2. Calculate magnitude change from baseline
3. Check motion threshold (configurable sensitivity)
4. Check tilt (gravity vector shift >15°)
5. Check tamper GPIO (NC reed switch)
6. Sample voltage every 10th cycle (1Hz) — detect 12V disconnect
7. Wake main CPU if any threshold exceeded

ULP Power Budget: ~50µA at 10Hz polling

### 6.4 State Machine

```
BOOT/INIT → (paired?) → DISARMED
                       ↓ arm command
                     ARMED (IDLE) — ULP active, deep sleep
                       ↓ sensor trigger
                     WARNING — full wake, verify (5s window)
                       ↓ confirmed threat    ↓ false positive → back to ARMED
                     ALERT — siren ON, GPS track, LoRa alert, SMS alert
                       ↓ GPS moving
                     TRACKING — siren OFF (stealth), continuous GPS log, SMS reports
                       ↓ user dismiss → back to ARMED
```

### 6.5 Boot Sequence

1. Power on / wake from deep sleep
2. Determine wake reason (ULP trigger / LoRa IRQ / timer)
3. Route to appropriate task based on wake reason
4. Normal boot (first power on): Init NVS → peripherals → LoRa → cellular → GPS → enter last state → start ULP → deep sleep

Boot time (wake from deep sleep): ~80ms to first instruction, ~300ms to full task ready

### 6.6 Flash Partitions

| Partition | Size | Purpose |
|-----------|------|--------|
| nvs | 24KB | Config, keys, calibration, state, seq counters |
| ota_0 | 2MB | Active firmware |
| ota_1 | 2MB | OTA update staging |
| event_log | 1MB | Circular event buffer (~10,000 events) |
| gps_log | 2MB | GPS breadcrumb trail (~100,000 points) |
| spiffs | 1MB | Config files, certificates |

### 6.7 OTA Updates

- Firmware transferred over BLE when near bike
- Writes to ota_1 partition with SHA256 verification
- Reboots, self-tests (LoRa, cellular, sensors), marks valid or rolls back
- Firmware images signed with Ed25519

### 6.8 Error Handling

| Mechanism | Action |
|-----------|--------|
| Hardware WDT | 10s timeout → hard reset |
| Task WDT | 5s per task → restart task |
| LoRa fail | 3 retries with backoff |
| Cellular fail | Reconnect exponential backoff (5s→15s→60s→5min) |
| GPS no fix | 120s timeout → report last known position |
| Sensor fail | Disable sensor, alert user, continue with remaining |

---

## 7. Phone Communication (Fully Local)

### 7.1 Architecture (No Cloud Server)

- Bike unit communicates with owner's phone via SMS (through SIM7080G)
- BLE used for local setup, configuration, and data download when near bike
- No server, no database, no monthly hosting costs
- Total ongoing cost: $2-5/month for IoT SIM with SMS

### 7.2 SMS Alert Format (Bike → Phone)

```
🚨 MOTO ALERT
Type: MOTION DETECTED
Severity: HIGH
Time: 14:32:05
Battery: 12.4V | Backup: 92%
GPS: https://maps.google.com/?q=-33.8688,151.2093
Reply: STOP to silence | STATUS for info
```

### 7.3 SMS Commands (Phone → Bike)

| SMS Text | Action | Response |
|----------|--------|----------|
| ARM | Arm the alarm | "✅ Armed. Sensors active." |
| DISARM | Disarm the alarm | "🔓 Disarmed." |
| STATUS | Request full status | Battery, GPS link, armed state, signal, temp |
| LOCATE | Chirp + GPS reply | GPS Google Maps link + chirp/flash |
| SIREN ON | Activate siren | "🔊 Siren activated." |
| SIREN OFF | Stop siren | "🔇 Siren off." |
| TRACK | Start continuous tracking | GPS link every 30s until STOP |
| STOP | Stop siren + tracking | "⏹ All alerts silenced." |
| SENS HIGH/MED/LOW | Change sensitivity | "Sensitivity set to HIGH" |
| GPS | One-time GPS fix | Google Maps link |

### 7.4 SMS Security

- Only respond to pre-registered phone number(s)
- Optional 4-digit PIN prefix: "1234 ARM"
- SIM7080G validates sender number at modem level
- Phone number stored during initial BLE setup

### 7.5 BLE Interface (Setup & Data Download)

| Function | Purpose |
|----------|--------|
| Initial setup | Register phone number, set PIN, calibrate sensors |
| Download event log | Pull full event history to phone |
| Download GPS trail | Export breadcrumbs as GPX file |
| Configuration | Sensitivity, geofence, timers, auto-arm |
| Firmware update | OTA over BLE |
| Diagnostics | Live sensor readings, signal strength, battery |

### 7.6 Reliability & Fallback

| Scenario | Behavior |
|----------|----------|
| Fob out of LoRa range | Alert goes via SMS to phone only |
| No cellular coverage | LoRa alert to fob + events stored in flash for later |
| Both down | Siren activates locally, GPS logs stored |
| Fob battery dead | SMS path still works |
| Bike backup battery low | Reduces check-in frequency, sends low-battery SMS |

---

## 8. Physical Design & Installation

### 8.1 Component Placement (Yamaha Serow)

| Component | Location | Mounting |
|-----------|----------|----------|
| Main unit | Under seat, in frame triangle / behind tool tray | VHB tape + zip ties |
| GPS antenna | Under seat cowl (plastic, RF transparent) | Adhesive, facing sky |
| LoRa antenna | Along frame tube, inside fairings | Heat shrink + cable tie |
| LTE antenna | FPC inside main enclosure | PCB mounted |
| Proximity sensor | Behind headlight cowl, forward-facing | Aimed at approach |
| Decoy siren | Visible under tail/rear fender | Security Torx + epoxy |
| 18650 backup | Inside main unit enclosure | Foam-lined holder |
| Power tap | Battery/fuse box, hidden in loom | Soldered + heat shrink, fused |

### 8.2 Main Unit Enclosure

- Dimensions: 90mm × 55mm × 25mm (deck of cards size)
- Material: ASA (UV resistant, heat resistant, impact resistant)
- Gasket: silicone O-ring (IP65)
- Mounting: VHB tape + cable ties (no drilling)
- Connectors: JST-XH internal, IP67 gland for external wires
- Tamper switch: magnetic reed (detects opening)
- Color: matte black (blends with frame)

### 8.3 Vibration Management

| Layer | Method |
|-------|--------|
| Enclosure mount | VHB tape (acts as damper) + soft foam pad |
| PCB mount | Rubber standoffs (M3 grommets) |
| Battery | Foam-lined holder |
| Connectors | Strain-relieved with silicone |
| IMU calibration | Baseline vibration profile learned; firmware filters engine vibration |

### 8.4 Decoy Siren Unit

- 120dB piezo siren + flashing RED LED + "GPS TRACKED" sticker
- Size: 70mm × 40mm × 30mm
- Mount: Security Torx + epoxy
- Wire to main unit (if cut → tamper detected)

### 8.5 Wiring

- 12V tap from battery positive via 3A inline fuse
- All wires wrapped in OEM-matching cloth harness tape
- Routed along existing wiring harness
- Connections: solder + adhesive heat shrink
- Ground to frame

### 8.6 Installation Checklist

1. Remove seat + side panels
2. Identify power tap point (battery or fuse box)
3. Mount main unit (VHB + zip ties)
4. Route power wire along existing harness
5. Install GPS antenna under seat cowl
6. Install LoRa antenna along frame
7. Mount proximity sensor behind headlight
8. Tap ignition wire (solder + optocoupler)
9. Mount decoy siren under tail
10. Install LED strobe near tail light
11. Connect all, wrap with loom tape
12. Reassemble panels
13. Power on, BLE setup (pair phone, calibrate, set geofence)
14. Pair fob
15. Test all triggers

---

## 9. Bill of Materials

### 9.1 Bike Unit BOM

| # | Component | Part | Qty | ~USD |
|---|-----------|------|-----|------|
| 1 | MCU Module | ESP32-S3-WROOM-1-N16R8 | 1 | $4.50 |
| 2 | LoRa Module | Ebyte E22-900M30S (SX1262, +30dBm) | 1 | $8.00 |
| 3 | Cellular+GNSS | SIM7080G | 1 | $12.00 |
| 4 | SIM Holder | Nano SIM push-push | 1 | $0.30 |
| 5 | IoT SIM Card | Hologram / 1NCE / prepaid | 1 | $5.00 |
| 6 | IMU | LSM6DSO (LGA-14) | 1 | $3.50 |
| 7 | Proximity Radar | RCWL-0516 module | 1 | $1.50 |
| 8 | Buck Converter | TPS563200 | 1 | $1.20 |
| 9 | Buck passives | 4.7µH inductor + 22µF caps | 1 set | $1.00 |
| 10 | Charger IC | BQ24075 | 1 | $2.50 |
| 11 | 18650 Cell | Samsung 30Q (3000mAh) | 1 | $5.00 |
| 12 | 18650 Holder | Spring clip, PCB mount | 1 | $0.50 |
| 13 | Siren | 120dB piezo, 12V | 1 | $5.00 |
| 14 | LED Strobe | High-power module | 1 | $3.00 |
| 15 | MOSFETs N-ch | AO3400 (SOT-23) | 4 | $0.40 |
| 16 | MOSFETs P-ch | AO3401 (SOT-23) | 2 | $0.30 |
| 17 | Optocoupler | PC817 | 1 | $0.20 |
| 18 | Reed Switch | NC magnetic | 1 | $0.50 |
| 19 | LoRa Antenna | SMA whip + U.FL pigtail | 1 | $3.00 |
| 20 | LTE Antenna | FPC flexible (U.FL) | 1 | $2.00 |
| 21 | GPS Antenna | 25x25mm ceramic patch (U.FL) | 1 | $3.00 |
| 22 | U.FL Connectors | PCB mount | 3 | $0.60 |
| 23 | Resistors | Voltage dividers + misc (0603) | assorted | $0.50 |
| 24 | Capacitors | 100nF + 10µF (0603) | 20 | $0.50 |
| 25 | JST-XH Connectors | 2/3/4 pin | 6 | $1.00 |
| 26 | Cable Glands | PG7 IP67 | 2 | $1.00 |
| 27 | Fuse + holder | Blade type, 3A inline | 1 | $1.50 |
| 28 | Wire | Silicone 22AWG assorted | 2m | $2.00 |
| 29 | Heat shrink | Adhesive-lined assortment | 1 set | $3.00 |
| 30 | Loom tape | Cloth harness tape | 1 roll | $4.00 |
| 31 | PCB | 2-layer, HASL, ~90x55mm (5pcs) | 1 order | $8.00 |
| 32 | 3D Print (ASA) | Enclosure (~50g) | 1 | $2.00 |
| 33 | Silicone O-ring | Enclosure seal | 1 | $0.50 |
| 34 | Mounting | VHB tape + zip ties | 1 set | $3.00 |
| 35 | Security bolts | Torx T10/T15 with pin | 4 | $2.00 |
| | **SUBTOTAL** | | | **~$85** |

### 9.2 Fob Unit BOM

| # | Component | Part | Qty | ~USD |
|---|-----------|------|-----|------|
| 1 | MCU Module | ESP32-C3-MINI-1-N4 | 1 | $2.50 |
| 2 | LoRa Module | Ebyte E22-900M22S (SX1262, +22dBm) | 1 | $6.00 |
| 3 | OLED Display | SSD1306 0.96" 128x64 I²C | 1 | $2.50 |
| 4 | Vibration Motor | Coin ERM 10mm 3V | 1 | $0.80 |
| 5 | Buzzer | Passive piezo 3.3V | 1 | $0.30 |
| 6 | RGB LED | WS2812B (5050) | 1 | $0.20 |
| 7 | Buttons | 6x6mm tactile | 3 | $0.30 |
| 8 | Battery | 502535 LiPo 500mAh | 1 | $4.00 |
| 9 | Charger | TP4056 USB-C | 1 | $0.80 |
| 10 | USB-C Connector | 16-pin SMD | 1 | $0.30 |
| 11 | Antenna | Spring helical 868/915MHz | 1 | $0.50 |
| 12 | Transistor | S8050 SOT-23 | 1 | $0.10 |
| 13 | Passives | Resistors, caps | assorted | $0.30 |
| 14 | PCB | 2-layer, ~60x35mm (5pcs) | 1 order | $5.00 |
| 15 | 3D Print (PETG) | Fob enclosure (~15g) | 1 | $0.50 |
| 16 | Hardware | M2 screws + inserts | 2 | $0.30 |
| | **SUBTOTAL** | | | **~$28** |

### 9.3 Total Cost Summary

| Category | Cost |
|----------|------|
| Bike unit components | ~$85 |
| Fob unit components | ~$28 |
| PCB fabrication (both) | ~$13 |
| 3D print filament | ~$3 |
| Mounting hardware & wiring | ~$15 |
| **Total one-time build** | **~$145** |
| Monthly (IoT SIM, SMS) | ~$3-5/month |

### 9.4 PCB Design Notes

- Layers: 2-layer, 1oz copper, 1.6mm thickness
- Min trace/space: 0.2mm/0.2mm
- Surface finish: HASL lead-free or ENIG
- Soldermask: Black
- EDA Tool: KiCad 8 (free)
- Manufacturer: JLCPCB ($2 for 5 boards + $8 shipping)
- 50Ω microstrip for LoRa antenna trace
- U.FL connectors on board edge
- Ground pour both layers
- IMU center of board
- Mounting holes M3 in corners with rubber grommets

---

## 10. Project Timeline

### 10.1 Phase Schedule

| Phase | Duration | Deliverables |
|-------|----------|-------------|
| 1. Schematic Design | 1-2 weeks | KiCad schematic, component library |
| 2. PCB Layout | 1-2 weeks | Gerber files, BOM for JLCPCB |
| 3. Order & Wait | 1-2 weeks | PCBs + components ship |
| 4. Assembly | 1 week | Solder both boards, power test |
| 5. Firmware — Core | 2-3 weeks | LoRa P2P, ULP sensor monitor, state machine |
| 6. Firmware — Cellular | 1-2 weeks | SIM7080G AT commands, SMS, GPS |
| 7. Firmware — Fob | 1-2 weeks | LoRa RX/TX, OLED UI, buttons |
| 8. Encryption & Security | 1 week | AES-128, HMAC, pairing protocol |
| 9. 3D Print Enclosures | 3-5 days | Design, print, fit test |
| 10. Integration & Test | 1-2 weeks | Full system test, range test, tuning |
| 11. Install on Bike | 1 day | Wire, mount, calibrate, final test |
| **Total** | **10-14 weeks** | **Working system installed** |

### 10.2 Development Order

```
Week 1-2:  Schematic + PCB design → order boards
Week 3:    While waiting: Start firmware on dev boards
           (ESP32-S3-DevKitC + SX1262 breakout + SIM7080G EVB)
Week 4:    PCBs arrive → assemble, verify power + basic comms
Week 5-6:  LoRa P2P working (bike ↔ fob, cleartext first)
Week 7:    Add encryption, pairing, anti-replay
Week 8:    Cellular: SMS send/receive + GPS fix
Week 9:    ULP coprocessor: IMU monitoring during deep sleep
Week 10:   Fob UI: OLED screens, buttons, alert feedback
Week 11:   Full integration: state machine, alert escalation
Week 12:   Range testing, sensitivity tuning, false alarm elimination
Week 13:   3D print final enclosures, install on Serow
Week 14:   Final testing, edge cases, polish
```

### 10.3 Dev Board Setup (Firmware Before Custom PCBs)

| Dev Board | Use | ~Price |
|-----------|-----|--------|
| ESP32-S3-DevKitC-1 | Bike firmware dev | $8 |
| ESP32-C3-DevKitM-1 | Fob firmware dev | $5 |
| Ebyte E22-900M30S test boards (×2) | LoRa testing | $16 |
| SIM7080G EVB | Cellular/GPS testing | $15 |
| LSM6DSO breakout | IMU testing | $10 |
| SSD1306 OLED module | Fob display testing | $3 |
| Breadboard + jumpers | Prototyping | $5 |
| **Dev kit total** | | **~$62** |

---

## Appendix A: Sensitivity Levels

| Level | IMU Threshold | Proximity Range | Debounce Time | Use Case |
|-------|--------------|-----------------|---------------|----------|
| LOW | 500mg | 2m | 3s | Windy conditions, busy parking |
| MEDIUM | 200mg | 4m | 2s | Normal parking (default) |
| HIGH | 100mg | 6m | 1s | Secure storage, low traffic |

## Appendix B: Geofence Configuration

- Stored on device (NVS): up to 5 geofences
- Each defined by: center lat/lon + radius (meters)
- Configured via BLE setup
- Checked on every GPS fix
- Breach triggers ALERT with SMS notification

## Appendix C: Auto-Arm Feature

- Optional: auto-arm after configurable timeout (e.g., 10 min with no motion)
- Detects engine off + no movement → countdown → arm
- Can also use BLE proximity: arm when phone leaves BLE range
- Disable via BLE config if not wanted

---

*End of specification document.*

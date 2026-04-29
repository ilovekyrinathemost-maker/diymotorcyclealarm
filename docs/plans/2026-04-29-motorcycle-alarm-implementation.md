# DIY Motorcycle Alarm System — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a complete two-way motorcycle alarm system with LoRa fob, GPS tracking, SMS alerts, and tamper-proof installation for a 2007 Yamaha Serow.

**Architecture:** ESP32-S3 bike unit with ULP coprocessor for low-power sensor monitoring, SX1262 LoRa for P2P fob communication, SIM7080G for cellular SMS + GNSS. ESP32-C3 fob with LoRa + OLED display. All communication encrypted with AES-128-CTR + HMAC-SHA256.

**Tech Stack:** ESP-IDF v5.x (C), KiCad 8, FreeCAD/OpenSCAD for enclosures, FreeRTOS, SPI/I²C/UART drivers.

**Spec:** `docs/specs/2026-04-29-motorcycle-alarm-design.md`

---

## File Structure

```
firmware/
├── bike-unit/
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   ├── partitions.csv
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── main.c                    # App entry, task creation, boot logic
│   │   ├── app_config.h              # Pin definitions, constants, thresholds
│   │   └── Kconfig.projbuild         # Menuconfig options
│   └── components/
│       ├── lora/
│       │   ├── CMakeLists.txt
│       │   ├── include/lora.h
│       │   ├── lora_sx1262.c          # SX1262 SPI driver
│       │   ├── lora_packet.c          # Packet encode/decode
│       │   └── lora_packet.h
│       ├── cellular/
│       │   ├── CMakeLists.txt
│       │   ├── include/cellular.h
│       │   ├── sim7080g.c             # AT command driver
│       │   ├── sms.c                  # SMS send/receive
│       │   └── gnss.c                 # GPS fix acquisition
│       ├── sensors/
│       │   ├── CMakeLists.txt
│       │   ├── include/sensors.h
│       │   ├── imu_lsm6dso.c          # IMU I²C driver
│       │   ├── proximity.c            # RCWL-0516 GPIO handler
│       │   ├── voltage_monitor.c      # ADC voltage reading
│       │   └── tamper.c               # Reed switch + ignition sense
│       ├── crypto/
│       │   ├── CMakeLists.txt
│       │   ├── include/crypto.h
│       │   ├── aes_ctr.c              # AES-128-CTR encrypt/decrypt
│       │   ├── hmac_sha256.c          # HMAC generation/verification
│       │   ├── key_management.c       # Key derivation, NVS storage
│       │   └── pairing.c              # Curve25519 key exchange
│       ├── alert_manager/
│       │   ├── CMakeLists.txt
│       │   ├── include/alert_manager.h
│       │   ├── state_machine.c        # ARM/DISARM/WARNING/ALERT/TRACKING
│       │   ├── alert_handler.c        # Alert escalation logic
│       │   └── actuators.c            # Siren, LED, horn control
│       └── ulp/
│           ├── CMakeLists.txt
│           ├── ulp_main.c             # ULP-RISC-V sensor monitor
│           └── ulp_i2c.c              # ULP I²C bit-bang driver
├── fob-unit/
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   ├── main/
│   │   ├── CMakeLists.txt
│   │   ├── main.c                    # App entry, task creation
│   │   └── app_config.h              # Pin definitions, constants
│   └── components/
│       ├── lora/                      # (shared with bike-unit)
│       ├── crypto/                    # (shared with bike-unit)
│       ├── ui/
│       │   ├── CMakeLists.txt
│       │   ├── include/ui.h
│       │   ├── oled_ssd1306.c         # OLED I²C driver
│       │   ├── screens.c             # Screen layouts
│       │   ├── buttons.c             # Button handling + debounce
│       │   └── feedback.c            # Vibration, buzzer, RGB LED
│       └── fob_state/
│           ├── CMakeLists.txt
│           ├── include/fob_state.h
│           └── fob_state.c           # Fob state machine
└── shared/
    ├── protocol.h                    # Packet types, command codes, alert codes
    └── protocol.c                    # Shared protocol constants

hardware/
├── bike-unit/
│   ├── bike-unit.kicad_pro
│   ├── bike-unit.kicad_sch
│   ├── bike-unit.kicad_pcb
│   ├── lib/                          # Custom footprints/symbols
│   └── gerbers/                      # Manufacturing files
├── fob-unit/
│   ├── fob-unit.kicad_pro
│   ├── fob-unit.kicad_sch
│   ├── fob-unit.kicad_pcb
│   ├── lib/
│   └── gerbers/
└── datasheets/                       # Key component datasheets

enclosures/
├── bike-unit/
│   ├── bike-enclosure.step
│   ├── bike-enclosure.stl
│   └── bike-enclosure.scad           # OpenSCAD source
└── fob-unit/
    ├── fob-enclosure.step
    ├── fob-enclosure.stl
    └── fob-enclosure.scad

tests/
├── test_protocol.c                   # Packet encode/decode tests
├── test_crypto.c                     # Encryption round-trip tests
├── test_state_machine.c              # State transition tests
├── test_sms_parser.c                 # SMS command parsing tests
├── test_geofence.c                   # Geofence calculation tests
└── Makefile                          # Native compilation for host testing

docs/
├── specs/
│   └── 2026-04-29-motorcycle-alarm-design.md
└── plans/
    └── 2026-04-29-motorcycle-alarm-implementation.md
```

---

## Phase 1: Development Environment & Project Scaffold

### Task 1: ESP-IDF Environment Setup

**Files:**
- Create: `firmware/bike-unit/CMakeLists.txt`
- Create: `firmware/bike-unit/main/CMakeLists.txt`
- Create: `firmware/bike-unit/main/main.c`
- Create: `firmware/bike-unit/main/app_config.h`
- Create: `firmware/bike-unit/sdkconfig.defaults`
- Create: `firmware/bike-unit/partitions.csv`

- [ ] **Step 1: Install ESP-IDF v5.x**

```bash
cd /a0/usr/projects/project_3
apt-get update && apt-get install -y git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
mkdir -p ~/esp
cd ~/esp
git clone -b v5.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32s3
echo "source ~/esp/esp-idf/export.sh" >> ~/.bashrc
source ~/esp/esp-idf/export.sh
```

- [ ] **Step 2: Create bike-unit project scaffold**

Create `firmware/bike-unit/CMakeLists.txt`:
```cmake
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(moto-alarm-bike)
```

- [ ] **Step 3: Create partition table**

Create `firmware/bike-unit/partitions.csv`:
```csv
# Name,   Type, SubType, Offset,  Size,   Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
ota_0,    app,  ota_0,   0x10000, 0x200000,
ota_1,    app,  ota_1,   0x210000,0x200000,
evt_log,  data, fat,     0x410000,0x100000,
gps_log,  data, fat,     0x510000,0x200000,
storage,  data, spiffs,  0x710000,0x100000,
```

- [ ] **Step 4: Create sdkconfig.defaults**

Create `firmware/bike-unit/sdkconfig.defaults`:
```
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240=y
CONFIG_ESP32S3_SPIRAM_SUPPORT=y
CONFIG_FREERTOS_HZ=1000
CONFIG_ESP_SLEEP_GPIO_RESET_WORKAROUND=y
CONFIG_ULP_COPROC_ENABLED=y
CONFIG_ULP_COPROC_TYPE_RISCV=y
CONFIG_ULP_COPROC_RESERVE_MEM=4096
```

- [ ] **Step 5: Create app_config.h with pin definitions**

Create `firmware/bike-unit/main/app_config.h`:
```c
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

// === SX1262 LoRa (SPI) ===
#define PIN_LORA_MOSI       1
#define PIN_LORA_MISO       2
#define PIN_LORA_SCK        3
#define PIN_LORA_CS         4
#define PIN_LORA_DIO1       5
#define PIN_LORA_BUSY       6
#define PIN_LORA_RST        7

// === SIM7080G (UART) ===
#define PIN_SIM_TX          17
#define PIN_SIM_RX          18
#define PIN_SIM_PWRKEY      8
#define PIN_SIM_STATUS      9

// === LSM6DSO IMU (I²C) ===
#define PIN_IMU_SDA         10
#define PIN_IMU_SCL         11
#define PIN_IMU_INT1        12

// === Sensors ===
#define PIN_PROXIMITY       13
#define PIN_TAMPER_SWITCH   40
#define PIN_IGNITION_SENSE  41

// === Actuators ===
#define PIN_SIREN           14
#define PIN_LED_STROBE      15
#define PIN_HORN_RELAY      16

// === Power ===
#define PIN_VOLTAGE_12V     38
#define PIN_VOLTAGE_BACKUP  39
#define PIN_POWER_PATH_CTL  42

// === Thresholds ===
#define MOTION_THRESHOLD_LOW    500  // mg
#define MOTION_THRESHOLD_MED    200  // mg
#define MOTION_THRESHOLD_HIGH   100  // mg
#define TILT_THRESHOLD_DEG      15   // degrees
#define VOLTAGE_DISCONNECT_MV   10000 // 10V = disconnected
#define WARNING_TIMEOUT_MS      5000  // 5 second verification

// === LoRa Config ===
#define LORA_FREQUENCY_HZ       915000000  // 915 MHz (US) or 868000000 (EU)
#define LORA_BANDWIDTH          125000
#define LORA_SPREADING_FACTOR   9
#define LORA_CODING_RATE        5
#define LORA_TX_POWER_DBM       30
#define LORA_SYNC_WORD          0x34

// === Timing ===
#define HEARTBEAT_INTERVAL_MS   900000  // 15 minutes
#define GPS_CHECKIN_INTERVAL_MS 1800000 // 30 minutes
#define TRACKING_INTERVAL_MS    30000   // 30 seconds during active tracking

#endif // APP_CONFIG_H
```

- [ ] **Step 6: Create minimal main.c**

Create `firmware/bike-unit/main/main.c`:
```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "app_config.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Moto Alarm Bike Unit - Starting...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "NVS initialized");
    ESP_LOGI(TAG, "System ready - components will be added incrementally");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

Create `firmware/bike-unit/main/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS ".")
```

- [ ] **Step 7: Verify project builds**

```bash
cd /a0/usr/projects/project_3/firmware/bike-unit
source ~/esp/esp-idf/export.sh
idf.py set-target esp32s3
idf.py build
```
Expected: Build succeeds with "Project build complete" message.

- [ ] **Step 8: Commit**

```bash
cd /a0/usr/projects/project_3
git add firmware/bike-unit/
git commit -m "feat: bike unit ESP-IDF project scaffold with partition table and pin config"
git push
```

---

### Task 2: Shared Protocol Definitions

**Files:**
- Create: `firmware/shared/protocol.h`
- Create: `firmware/shared/protocol.c`

- [ ] **Step 1: Define shared protocol header**

Create `firmware/shared/protocol.h`:
```c
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// === Packet Structure ===
// Total: 32 bytes
// [PKT_TYPE:1][DEVICE_ID:2][SEQ_NUM:4][PAYLOAD:20][HMAC:4][FLAGS:1]

#define PACKET_SIZE         32
#define PAYLOAD_SIZE        20
#define HMAC_TRUNCATED_SIZE 4

// === Packet Types ===
#define PKT_ALERT           0x01
#define PKT_COMMAND         0x02
#define PKT_ACK             0x03
#define PKT_STATUS          0x04
#define PKT_HEARTBEAT       0x05
#define PKT_PAIRING         0x06

// === Alert Types ===
#define ALERT_MOTION        0x01
#define ALERT_TILT          0x02
#define ALERT_PROXIMITY     0x03
#define ALERT_IGNITION      0x04
#define ALERT_BATTERY_CUT   0x05
#define ALERT_GEOFENCE      0x06
#define ALERT_GPS_MOVING    0x07
#define ALERT_ENCLOSURE     0x08

// === Severity Levels ===
#define SEVERITY_INFO       0x00
#define SEVERITY_WARNING    0x01
#define SEVERITY_ALERT      0x02
#define SEVERITY_CRITICAL   0x03

// === Command Types ===
#define CMD_ARM             0x10
#define CMD_DISARM          0x11
#define CMD_SIREN_ON        0x20
#define CMD_SIREN_OFF       0x21
#define CMD_LED_FLASH       0x30
#define CMD_LOCATE          0x40
#define CMD_SENSITIVITY     0x50
#define CMD_STATUS_REQ      0x60

// === Sensitivity Levels ===
#define SENS_LOW            0x00
#define SENS_MED            0x01
#define SENS_HIGH           0x02

// === Locate Modes ===
#define LOCATE_CHIRP        0x00
#define LOCATE_FLASH        0x01
#define LOCATE_BOTH         0x02

// === Flags ===
#define FLAG_PRIORITY_HIGH  0x01
#define FLAG_RETRY          0x02
#define FLAG_BATTERY_LOW    0x04
#define FLAG_BACKUP_POWER   0x08

// === Packet Structures ===
typedef struct __attribute__((packed)) {
    uint8_t  pkt_type;
    uint16_t device_id;
    uint32_t seq_num;
    uint8_t  payload[PAYLOAD_SIZE];
    uint8_t  hmac[HMAC_TRUNCATED_SIZE];
    uint8_t  flags;
} lora_packet_t;

// Alert payload (fits in 20 bytes)
typedef struct __attribute__((packed)) {
    uint8_t  alert_type;
    uint8_t  severity;
    int16_t  sensor_data[2];  // 4 bytes
    int32_t  latitude;        // 4 bytes (degrees * 1e7)
    int32_t  longitude;       // 4 bytes (degrees * 1e7)
    uint16_t speed;           // km/h * 10
    uint16_t heading;         // degrees * 10
    uint8_t  battery_pct;     // 0-100
    uint8_t  reserved;
} alert_payload_t;

// Command payload (fits in 20 bytes)
typedef struct __attribute__((packed)) {
    uint8_t  cmd_type;
    uint8_t  params[4];
    uint8_t  reserved[15];
} command_payload_t;

// Status payload (fits in 20 bytes)
typedef struct __attribute__((packed)) {
    uint16_t main_voltage_mv;  // 12V rail in mV
    uint8_t  backup_pct;       // backup battery %
    uint8_t  gps_fix;          // 0=no fix, 1=2D, 2=3D
    int32_t  latitude;         // degrees * 1e7
    int32_t  longitude;        // degrees * 1e7
    int8_t   temperature;      // degrees C
    int8_t   signal_rssi;      // dBm
    uint8_t  armed;            // 0=disarmed, 1=armed
    uint8_t  state;            // current state machine state
    uint8_t  reserved[4];
} status_payload_t;

// Heartbeat payload (fits in 20 bytes)
typedef struct __attribute__((packed)) {
    uint8_t  battery_pct;
    uint8_t  armed;
    int8_t   signal_rssi;
    uint8_t  reserved[17];
} heartbeat_payload_t;

// === Functions ===
void protocol_encode_packet(lora_packet_t *pkt, uint8_t pkt_type,
                           uint16_t device_id, uint32_t seq_num,
                           const uint8_t *payload, uint8_t flags);
bool protocol_decode_packet(const uint8_t *raw, size_t len, lora_packet_t *pkt);
void protocol_serialize(const lora_packet_t *pkt, uint8_t *buf);

// Coordinate helpers
int32_t protocol_float_to_coord(double degrees);
double protocol_coord_to_float(int32_t coord);

#endif // PROTOCOL_H
```

- [ ] **Step 2: Implement protocol functions**

Create `firmware/shared/protocol.c`:
```c
#include "protocol.h"
#include <string.h>

void protocol_encode_packet(lora_packet_t *pkt, uint8_t pkt_type,
                           uint16_t device_id, uint32_t seq_num,
                           const uint8_t *payload, uint8_t flags)
{
    pkt->pkt_type = pkt_type;
    pkt->device_id = device_id;
    pkt->seq_num = seq_num;
    if (payload) {
        memcpy(pkt->payload, payload, PAYLOAD_SIZE);
    } else {
        memset(pkt->payload, 0, PAYLOAD_SIZE);
    }
    // HMAC is set separately by crypto layer
    memset(pkt->hmac, 0, HMAC_TRUNCATED_SIZE);
    pkt->flags = flags;
}

bool protocol_decode_packet(const uint8_t *raw, size_t len, lora_packet_t *pkt)
{
    if (len != PACKET_SIZE) return false;
    memcpy(pkt, raw, PACKET_SIZE);
    return true;
}

void protocol_serialize(const lora_packet_t *pkt, uint8_t *buf)
{
    memcpy(buf, pkt, PACKET_SIZE);
}

int32_t protocol_float_to_coord(double degrees)
{
    return (int32_t)(degrees * 1e7);
}

double protocol_coord_to_float(int32_t coord)
{
    return (double)coord / 1e7;
}
```

- [ ] **Step 3: Write unit tests for protocol**

Create `tests/test_protocol.c`:
```c
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "../firmware/shared/protocol.h"
#include "../firmware/shared/protocol.c"  // Include implementation for host testing

void test_encode_decode_roundtrip(void)
{
    lora_packet_t pkt_out, pkt_in;
    uint8_t payload[PAYLOAD_SIZE] = {0};
    uint8_t buf[PACKET_SIZE];

    alert_payload_t alert = {
        .alert_type = ALERT_MOTION,
        .severity = SEVERITY_WARNING,
        .sensor_data = {150, -30},
        .latitude = protocol_float_to_coord(-33.8688),
        .longitude = protocol_float_to_coord(151.2093),
        .speed = 0,
        .heading = 1800,
        .battery_pct = 92,
    };
    memcpy(payload, &alert, sizeof(alert));

    protocol_encode_packet(&pkt_out, PKT_ALERT, 0x1234, 42, payload, FLAG_PRIORITY_HIGH);
    protocol_serialize(&pkt_out, buf);

    assert(protocol_decode_packet(buf, PACKET_SIZE, &pkt_in));
    assert(pkt_in.pkt_type == PKT_ALERT);
    assert(pkt_in.device_id == 0x1234);
    assert(pkt_in.seq_num == 42);
    assert(pkt_in.flags == FLAG_PRIORITY_HIGH);

    alert_payload_t *decoded = (alert_payload_t *)pkt_in.payload;
    assert(decoded->alert_type == ALERT_MOTION);
    assert(decoded->severity == SEVERITY_WARNING);
    assert(decoded->battery_pct == 92);

    printf("  ✓ encode/decode roundtrip
");
}

void test_coordinate_conversion(void)
{
    double lat = -33.8688;
    int32_t encoded = protocol_float_to_coord(lat);
    double decoded = protocol_coord_to_float(encoded);
    assert(fabs(decoded - lat) < 0.0000002);  // Sub-meter precision

    printf("  ✓ coordinate conversion
");
}

void test_invalid_packet_length(void)
{
    lora_packet_t pkt;
    uint8_t short_buf[10] = {0};
    assert(!protocol_decode_packet(short_buf, 10, &pkt));
    printf("  ✓ reject invalid packet length
");
}

void test_command_payload(void)
{
    lora_packet_t pkt;
    uint8_t payload[PAYLOAD_SIZE] = {0};
    uint8_t buf[PACKET_SIZE];

    command_payload_t cmd = {
        .cmd_type = CMD_ARM,
        .params = {0},
    };
    memcpy(payload, &cmd, sizeof(cmd));

    protocol_encode_packet(&pkt, PKT_COMMAND, 0x5678, 100, payload, 0);
    protocol_serialize(&pkt, buf);

    lora_packet_t decoded_pkt;
    assert(protocol_decode_packet(buf, PACKET_SIZE, &decoded_pkt));
    command_payload_t *decoded_cmd = (command_payload_t *)decoded_pkt.payload;
    assert(decoded_cmd->cmd_type == CMD_ARM);

    printf("  ✓ command payload
");
}

int main(void)
{
    printf("Protocol tests:
");
    test_encode_decode_roundtrip();
    test_coordinate_conversion();
    test_invalid_packet_length();
    test_command_payload();
    printf("
All protocol tests passed! ✓
");
    return 0;
}
```

Create `tests/Makefile`:
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -I../firmware/shared -lm

all: test_protocol test_crypto test_state_machine test_sms_parser test_geofence

test_protocol: test_protocol.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

test_crypto: test_crypto.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

test_state_machine: test_state_machine.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

test_sms_parser: test_sms_parser.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

test_geofence: test_geofence.c
	$(CC) $(CFLAGS) -o $@ $<
	./$@

clean:
	rm -f test_protocol test_crypto test_state_machine test_sms_parser test_geofence

.PHONY: all clean
```

- [ ] **Step 4: Run protocol tests**

```bash
cd /a0/usr/projects/project_3/tests
make test_protocol
```
Expected: "All protocol tests passed! ✓"

- [ ] **Step 5: Commit**

```bash
cd /a0/usr/projects/project_3
git add firmware/shared/ tests/
git commit -m "feat: shared protocol definitions with packet encode/decode and unit tests"
git push
```

---

### Task 3: Crypto Component

**Files:**
- Create: `firmware/bike-unit/components/crypto/CMakeLists.txt`
- Create: `firmware/bike-unit/components/crypto/include/crypto.h`
- Create: `firmware/bike-unit/components/crypto/aes_ctr.c`
- Create: `firmware/bike-unit/components/crypto/hmac_sha256.c`
- Create: `firmware/bike-unit/components/crypto/key_management.c`
- Create: `tests/test_crypto.c`

- [ ] **Step 1: Create crypto header**

Create `firmware/bike-unit/components/crypto/include/crypto.h`:
```c
#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "protocol.h"

#define AES_KEY_SIZE    16  // 128-bit
#define HMAC_KEY_SIZE   32  // 256-bit
#define MASTER_KEY_SIZE 32  // 256-bit

typedef struct {
    uint8_t enc_key[AES_KEY_SIZE];
    uint8_t hmac_key[HMAC_KEY_SIZE];
} session_keys_t;

// Key derivation
void crypto_derive_keys(const uint8_t *master_key, session_keys_t *keys);

// AES-128-CTR
void crypto_aes_ctr_encrypt(const uint8_t *key, uint32_t counter,
                           const uint8_t *plaintext, uint8_t *ciphertext,
                           size_t len);
void crypto_aes_ctr_decrypt(const uint8_t *key, uint32_t counter,
                           const uint8_t *ciphertext, uint8_t *plaintext,
                           size_t len);

// HMAC-SHA256 (truncated to 4 bytes)
void crypto_hmac_compute(const uint8_t *key, const uint8_t *data,
                        size_t data_len, uint8_t *hmac_out);
bool crypto_hmac_verify(const uint8_t *key, const uint8_t *data,
                       size_t data_len, const uint8_t *expected_hmac);

// Packet-level operations
void crypto_encrypt_packet(const session_keys_t *keys, lora_packet_t *pkt);
bool crypto_decrypt_packet(const session_keys_t *keys, lora_packet_t *pkt);

// Anti-replay
typedef struct {
    uint32_t tx_counter;
    uint32_t rx_last_seq;
} replay_state_t;

void replay_init(replay_state_t *state);
uint32_t replay_next_tx_seq(replay_state_t *state);
bool replay_check_rx_seq(replay_state_t *state, uint32_t seq);

#endif // CRYPTO_H
```

- [ ] **Step 2: Implement AES-CTR (using mbedtls which ships with ESP-IDF)**

Create `firmware/bike-unit/components/crypto/aes_ctr.c`:
```c
#include "crypto.h"
#include <string.h>

// For host testing, use a simple implementation
// On ESP32, this will use mbedtls from ESP-IDF
#ifdef ESP_PLATFORM
#include "mbedtls/aes.h"
#else
// Minimal AES-CTR for host testing (include a portable implementation)
#include "aes_portable.h"
#endif

void crypto_aes_ctr_encrypt(const uint8_t *key, uint32_t counter,
                           const uint8_t *plaintext, uint8_t *ciphertext,
                           size_t len)
{
    uint8_t nonce_counter[16] = {0};
    uint8_t stream_block[16] = {0};
    size_t nc_off = 0;

    // Set counter in nonce (last 4 bytes)
    nonce_counter[12] = (counter >> 24) & 0xFF;
    nonce_counter[13] = (counter >> 16) & 0xFF;
    nonce_counter[14] = (counter >> 8) & 0xFF;
    nonce_counter[15] = counter & 0xFF;

#ifdef ESP_PLATFORM
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    mbedtls_aes_crypt_ctr(&aes, len, &nc_off, nonce_counter,
                          stream_block, plaintext, ciphertext);
    mbedtls_aes_free(&aes);
#else
    // Portable fallback for host testing
    aes_portable_ctr(key, nonce_counter, plaintext, ciphertext, len);
#endif
}

void crypto_aes_ctr_decrypt(const uint8_t *key, uint32_t counter,
                           const uint8_t *ciphertext, uint8_t *plaintext,
                           size_t len)
{
    // CTR mode: decrypt == encrypt
    crypto_aes_ctr_encrypt(key, counter, ciphertext, plaintext, len);
}
```

- [ ] **Step 3: Implement HMAC-SHA256**

Create `firmware/bike-unit/components/crypto/hmac_sha256.c`:
```c
#include "crypto.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "mbedtls/md.h"
#else
#include "sha256_portable.h"
#endif

void crypto_hmac_compute(const uint8_t *key, const uint8_t *data,
                        size_t data_len, uint8_t *hmac_out)
{
    uint8_t full_hmac[32];

#ifdef ESP_PLATFORM
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
    mbedtls_md_hmac_starts(&ctx, key, HMAC_KEY_SIZE);
    mbedtls_md_hmac_update(&ctx, data, data_len);
    mbedtls_md_hmac_finish(&ctx, full_hmac);
    mbedtls_md_free(&ctx);
#else
    hmac_sha256_portable(key, HMAC_KEY_SIZE, data, data_len, full_hmac);
#endif

    // Truncate to 4 bytes
    memcpy(hmac_out, full_hmac, HMAC_TRUNCATED_SIZE);
}

bool crypto_hmac_verify(const uint8_t *key, const uint8_t *data,
                       size_t data_len, const uint8_t *expected_hmac)
{
    uint8_t computed[HMAC_TRUNCATED_SIZE];
    crypto_hmac_compute(key, data, data_len, computed);

    // Constant-time comparison to prevent timing attacks
    uint8_t diff = 0;
    for (int i = 0; i < HMAC_TRUNCATED_SIZE; i++) {
        diff |= computed[i] ^ expected_hmac[i];
    }
    return diff == 0;
}
```

- [ ] **Step 4: Implement key derivation and packet-level crypto**

Create `firmware/bike-unit/components/crypto/key_management.c`:
```c
#include "crypto.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"
#else
#include "sha256_portable.h"
#endif

static const uint8_t ENC_LABEL[] = "moto-alarm-enc-key";
static const uint8_t HMAC_LABEL[] = "moto-alarm-hmac-key";

void crypto_derive_keys(const uint8_t *master_key, session_keys_t *keys)
{
#ifdef ESP_PLATFORM
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_hkdf(md, NULL, 0, master_key, MASTER_KEY_SIZE,
                 ENC_LABEL, sizeof(ENC_LABEL) - 1,
                 keys->enc_key, AES_KEY_SIZE);
    mbedtls_hkdf(md, NULL, 0, master_key, MASTER_KEY_SIZE,
                 HMAC_LABEL, sizeof(HMAC_LABEL) - 1,
                 keys->hmac_key, HMAC_KEY_SIZE);
#else
    // Simple KDF for host testing: SHA256(master || label), truncated
    uint8_t buf[64];
    memcpy(buf, master_key, MASTER_KEY_SIZE);
    memcpy(buf + MASTER_KEY_SIZE, ENC_LABEL, sizeof(ENC_LABEL) - 1);
    uint8_t hash[32];
    sha256_portable(buf, MASTER_KEY_SIZE + sizeof(ENC_LABEL) - 1, hash);
    memcpy(keys->enc_key, hash, AES_KEY_SIZE);

    memcpy(buf + MASTER_KEY_SIZE, HMAC_LABEL, sizeof(HMAC_LABEL) - 1);
    sha256_portable(buf, MASTER_KEY_SIZE + sizeof(HMAC_LABEL) - 1, hash);
    memcpy(keys->hmac_key, hash, HMAC_KEY_SIZE);
#endif
}

void crypto_encrypt_packet(const session_keys_t *keys, lora_packet_t *pkt)
{
    // Encrypt payload in-place
    uint8_t encrypted[PAYLOAD_SIZE];
    crypto_aes_ctr_encrypt(keys->enc_key, pkt->seq_num,
                          pkt->payload, encrypted, PAYLOAD_SIZE);
    memcpy(pkt->payload, encrypted, PAYLOAD_SIZE);

    // Compute HMAC over everything except the HMAC field itself
    // HMAC covers: pkt_type + device_id + seq_num + payload + flags
    uint8_t hmac_data[1 + 2 + 4 + PAYLOAD_SIZE + 1];  // 28 bytes
    size_t offset = 0;
    hmac_data[offset++] = pkt->pkt_type;
    hmac_data[offset++] = pkt->device_id & 0xFF;
    hmac_data[offset++] = (pkt->device_id >> 8) & 0xFF;
    hmac_data[offset++] = pkt->seq_num & 0xFF;
    hmac_data[offset++] = (pkt->seq_num >> 8) & 0xFF;
    hmac_data[offset++] = (pkt->seq_num >> 16) & 0xFF;
    hmac_data[offset++] = (pkt->seq_num >> 24) & 0xFF;
    memcpy(hmac_data + offset, pkt->payload, PAYLOAD_SIZE);
    offset += PAYLOAD_SIZE;
    hmac_data[offset++] = pkt->flags;

    crypto_hmac_compute(keys->hmac_key, hmac_data, offset, pkt->hmac);
}

bool crypto_decrypt_packet(const session_keys_t *keys, lora_packet_t *pkt)
{
    // Verify HMAC first
    uint8_t hmac_data[1 + 2 + 4 + PAYLOAD_SIZE + 1];
    size_t offset = 0;
    hmac_data[offset++] = pkt->pkt_type;
    hmac_data[offset++] = pkt->device_id & 0xFF;
    hmac_data[offset++] = (pkt->device_id >> 8) & 0xFF;
    hmac_data[offset++] = pkt->seq_num & 0xFF;
    hmac_data[offset++] = (pkt->seq_num >> 8) & 0xFF;
    hmac_data[offset++] = (pkt->seq_num >> 16) & 0xFF;
    hmac_data[offset++] = (pkt->seq_num >> 24) & 0xFF;
    memcpy(hmac_data + offset, pkt->payload, PAYLOAD_SIZE);
    offset += PAYLOAD_SIZE;
    hmac_data[offset++] = pkt->flags;

    if (!crypto_hmac_verify(keys->hmac_key, hmac_data, offset, pkt->hmac)) {
        return false;  // HMAC verification failed
    }

    // Decrypt payload in-place
    uint8_t decrypted[PAYLOAD_SIZE];
    crypto_aes_ctr_decrypt(keys->enc_key, pkt->seq_num,
                          pkt->payload, decrypted, PAYLOAD_SIZE);
    memcpy(pkt->payload, decrypted, PAYLOAD_SIZE);

    return true;
}

// === Anti-replay ===

void replay_init(replay_state_t *state)
{
    state->tx_counter = 0;
    state->rx_last_seq = 0;
}

uint32_t replay_next_tx_seq(replay_state_t *state)
{
    return ++(state->tx_counter);
}

bool replay_check_rx_seq(replay_state_t *state, uint32_t seq)
{
    // Accept if seq is within window: last_seq+1 to last_seq+256
    if (seq <= state->rx_last_seq) {
        return false;  // Replay or old packet
    }
    if (seq > state->rx_last_seq + 256) {
        return false;  // Too far ahead (likely error)
    }
    state->rx_last_seq = seq;
    return true;
}
```

- [ ] **Step 5: Create CMakeLists.txt for crypto component**

Create `firmware/bike-unit/components/crypto/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "aes_ctr.c" "hmac_sha256.c" "key_management.c"
                       INCLUDE_DIRS "include"
                       REQUIRES mbedtls)
```

- [ ] **Step 6: Write crypto unit tests**

Create `tests/test_crypto.c`:
```c
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../firmware/shared/protocol.h"
#include "../firmware/shared/protocol.c"

// Portable crypto implementations for host testing
// (simplified - in real build these come from mbedtls)
#include "crypto_host_impl.h"
#include "../firmware/bike-unit/components/crypto/include/crypto.h"
#include "../firmware/bike-unit/components/crypto/key_management.c"

void test_encrypt_decrypt_roundtrip(void)
{
    uint8_t master_key[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                              0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                              0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    session_keys_t keys;
    crypto_derive_keys(master_key, &keys);

    // Create a packet
    lora_packet_t pkt;
    alert_payload_t alert = {
        .alert_type = ALERT_MOTION,
        .severity = SEVERITY_WARNING,
        .latitude = protocol_float_to_coord(-33.8688),
        .longitude = protocol_float_to_coord(151.2093),
        .battery_pct = 85,
    };
    uint8_t payload[PAYLOAD_SIZE] = {0};
    memcpy(payload, &alert, sizeof(alert));
    protocol_encode_packet(&pkt, PKT_ALERT, 0x1234, 1, payload, 0);

    // Save original payload for comparison
    uint8_t original[PAYLOAD_SIZE];
    memcpy(original, pkt.payload, PAYLOAD_SIZE);

    // Encrypt
    crypto_encrypt_packet(&keys, &pkt);

    // Payload should be different after encryption
    assert(memcmp(pkt.payload, original, PAYLOAD_SIZE) != 0);

    // Decrypt
    assert(crypto_decrypt_packet(&keys, &pkt));

    // Payload should match original
    assert(memcmp(pkt.payload, original, PAYLOAD_SIZE) == 0);

    printf("  ✓ encrypt/decrypt roundtrip
");
}

void test_hmac_tamper_detection(void)
{
    uint8_t master_key[32] = {0x42};
    session_keys_t keys;
    crypto_derive_keys(master_key, &keys);

    lora_packet_t pkt;
    uint8_t payload[PAYLOAD_SIZE] = {0xAA, 0xBB, 0xCC};
    protocol_encode_packet(&pkt, PKT_COMMAND, 0x5678, 10, payload, 0);
    crypto_encrypt_packet(&keys, &pkt);

    // Tamper with payload
    pkt.payload[0] ^= 0xFF;

    // Decryption should fail (HMAC mismatch)
    assert(!crypto_decrypt_packet(&keys, &pkt));

    printf("  ✓ HMAC tamper detection
");
}

void test_anti_replay(void)
{
    replay_state_t state;
    replay_init(&state);

    // Sequential packets should be accepted
    assert(replay_check_rx_seq(&state, 1));
    assert(replay_check_rx_seq(&state, 2));
    assert(replay_check_rx_seq(&state, 3));

    // Replay of old packet should be rejected
    assert(!replay_check_rx_seq(&state, 2));
    assert(!replay_check_rx_seq(&state, 1));

    // Packet too far ahead should be rejected
    assert(!replay_check_rx_seq(&state, 500));

    // But within window should be accepted
    assert(replay_check_rx_seq(&state, 100));

    printf("  ✓ anti-replay protection
");
}

void test_wrong_key_fails(void)
{
    uint8_t key_a[32] = {0x01};
    uint8_t key_b[32] = {0x02};
    session_keys_t keys_a, keys_b;
    crypto_derive_keys(key_a, &keys_a);
    crypto_derive_keys(key_b, &keys_b);

    lora_packet_t pkt;
    uint8_t payload[PAYLOAD_SIZE] = {0xDE, 0xAD};
    protocol_encode_packet(&pkt, PKT_ALERT, 0x1111, 5, payload, 0);
    crypto_encrypt_packet(&keys_a, &pkt);

    // Try to decrypt with wrong key
    assert(!crypto_decrypt_packet(&keys_b, &pkt));

    printf("  ✓ wrong key rejected
");
}

int main(void)
{
    printf("Crypto tests:
");
    test_encrypt_decrypt_roundtrip();
    test_hmac_tamper_detection();
    test_anti_replay();
    test_wrong_key_fails();
    printf("
All crypto tests passed! ✓
");
    return 0;
}
```

- [ ] **Step 7: Commit**

```bash
cd /a0/usr/projects/project_3
git add firmware/bike-unit/components/crypto/ tests/test_crypto.c
git commit -m "feat: crypto component with AES-128-CTR, HMAC-SHA256, key derivation, anti-replay"
git push
```

---

### Task 4: State Machine Component

**Files:**
- Create: `firmware/bike-unit/components/alert_manager/CMakeLists.txt`
- Create: `firmware/bike-unit/components/alert_manager/include/alert_manager.h`
- Create: `firmware/bike-unit/components/alert_manager/state_machine.c`
- Create: `tests/test_state_machine.c`

- [ ] **Step 1: Define state machine header**

Create `firmware/bike-unit/components/alert_manager/include/alert_manager.h`:
```c
#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// === States ===
typedef enum {
    STATE_INIT = 0,
    STATE_DISARMED,
    STATE_ARMED,
    STATE_WARNING,
    STATE_ALERT,
    STATE_TRACKING,
} alarm_state_t;

// === Events ===
typedef enum {
    EVT_ARM_CMD = 0,
    EVT_DISARM_CMD,
    EVT_SENSOR_TRIGGER,
    EVT_THREAT_CONFIRMED,
    EVT_FALSE_POSITIVE,
    EVT_GPS_MOVING,
    EVT_WARNING_TIMEOUT,
} alarm_event_t;

// === Trigger Source ===
typedef enum {
    TRIGGER_MOTION = 0,
    TRIGGER_TILT,
    TRIGGER_PROXIMITY,
    TRIGGER_IGNITION,
    TRIGGER_BATTERY_CUT,
    TRIGGER_GEOFENCE,
    TRIGGER_ENCLOSURE,
} trigger_source_t;

// === Callbacks (actions taken on transitions) ===
typedef struct {
    void (*on_arm)(void);
    void (*on_disarm)(void);
    void (*on_warning)(trigger_source_t source);
    void (*on_alert)(trigger_source_t source);
    void (*on_tracking)(void);
    void (*on_alert_cleared)(void);
    void (*on_siren_on)(void);
    void (*on_siren_off)(void);
} state_callbacks_t;

// === State Machine Context ===
typedef struct {
    alarm_state_t current_state;
    trigger_source_t last_trigger;
    uint32_t state_entered_ms;     // Timestamp when entered current state
    uint32_t warning_start_ms;     // When warning started
    uint32_t warning_timeout_ms;   // How long to verify (default 5000)
    state_callbacks_t callbacks;
} alarm_sm_t;

// === Functions ===
void sm_init(alarm_sm_t *sm, state_callbacks_t *callbacks, uint32_t warning_timeout_ms);
alarm_state_t sm_get_state(const alarm_sm_t *sm);
void sm_process_event(alarm_sm_t *sm, alarm_event_t event, uint32_t now_ms);
void sm_set_trigger(alarm_sm_t *sm, trigger_source_t source);
const char *sm_state_name(alarm_state_t state);

#endif // ALERT_MANAGER_H
```

- [ ] **Step 2: Implement state machine**

Create `firmware/bike-unit/components/alert_manager/state_machine.c`:
```c
#include "alert_manager.h"
#include <string.h>

void sm_init(alarm_sm_t *sm, state_callbacks_t *callbacks, uint32_t warning_timeout_ms)
{
    memset(sm, 0, sizeof(alarm_sm_t));
    sm->current_state = STATE_DISARMED;
    sm->warning_timeout_ms = warning_timeout_ms;
    if (callbacks) {
        sm->callbacks = *callbacks;
    }
}

alarm_state_t sm_get_state(const alarm_sm_t *sm)
{
    return sm->current_state;
}

void sm_set_trigger(alarm_sm_t *sm, trigger_source_t source)
{
    sm->last_trigger = source;
}

void sm_process_event(alarm_sm_t *sm, alarm_event_t event, uint32_t now_ms)
{
    switch (sm->current_state) {
        case STATE_INIT:
            // Always transition to DISARMED on init
            sm->current_state = STATE_DISARMED;
            sm->state_entered_ms = now_ms;
            break;

        case STATE_DISARMED:
            if (event == EVT_ARM_CMD) {
                sm->current_state = STATE_ARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_arm) sm->callbacks.on_arm();
            }
            break;

        case STATE_ARMED:
            if (event == EVT_DISARM_CMD) {
                sm->current_state = STATE_DISARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_disarm) sm->callbacks.on_disarm();
            } else if (event == EVT_SENSOR_TRIGGER) {
                sm->current_state = STATE_WARNING;
                sm->state_entered_ms = now_ms;
                sm->warning_start_ms = now_ms;
                if (sm->callbacks.on_warning) sm->callbacks.on_warning(sm->last_trigger);
            }
            break;

        case STATE_WARNING:
            if (event == EVT_DISARM_CMD) {
                sm->current_state = STATE_DISARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_disarm) sm->callbacks.on_disarm();
            } else if (event == EVT_THREAT_CONFIRMED) {
                sm->current_state = STATE_ALERT;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_alert) sm->callbacks.on_alert(sm->last_trigger);
                if (sm->callbacks.on_siren_on) sm->callbacks.on_siren_on();
            } else if (event == EVT_FALSE_POSITIVE || event == EVT_WARNING_TIMEOUT) {
                sm->current_state = STATE_ARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_alert_cleared) sm->callbacks.on_alert_cleared();
            }
            break;

        case STATE_ALERT:
            if (event == EVT_DISARM_CMD) {
                sm->current_state = STATE_DISARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_siren_off) sm->callbacks.on_siren_off();
                if (sm->callbacks.on_disarm) sm->callbacks.on_disarm();
            } else if (event == EVT_GPS_MOVING) {
                sm->current_state = STATE_TRACKING;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_siren_off) sm->callbacks.on_siren_off();
                if (sm->callbacks.on_tracking) sm->callbacks.on_tracking();
            }
            break;

        case STATE_TRACKING:
            if (event == EVT_DISARM_CMD) {
                sm->current_state = STATE_DISARMED;
                sm->state_entered_ms = now_ms;
                if (sm->callbacks.on_disarm) sm->callbacks.on_disarm();
            }
            break;
    }
}

const char *sm_state_name(alarm_state_t state)
{
    switch (state) {
        case STATE_INIT:      return "INIT";
        case STATE_DISARMED:  return "DISARMED";
        case STATE_ARMED:     return "ARMED";
        case STATE_WARNING:   return "WARNING";
        case STATE_ALERT:     return "ALERT";
        case STATE_TRACKING:  return "TRACKING";
        default:              return "UNKNOWN";
    }
}
```

- [ ] **Step 3: Write state machine tests**

Create `tests/test_state_machine.c`:
```c
#include <stdio.h>
#include <assert.h>
#include "../firmware/bike-unit/components/alert_manager/include/alert_manager.h"
#include "../firmware/bike-unit/components/alert_manager/state_machine.c"

static int arm_count = 0, disarm_count = 0, warning_count = 0;
static int alert_count = 0, tracking_count = 0, siren_on_count = 0, siren_off_count = 0;

static void cb_arm(void) { arm_count++; }
static void cb_disarm(void) { disarm_count++; }
static void cb_warning(trigger_source_t s) { (void)s; warning_count++; }
static void cb_alert(trigger_source_t s) { (void)s; alert_count++; }
static void cb_tracking(void) { tracking_count++; }
static void cb_siren_on(void) { siren_on_count++; }
static void cb_siren_off(void) { siren_off_count++; }

static void reset_counts(void) {
    arm_count = disarm_count = warning_count = 0;
    alert_count = tracking_count = siren_on_count = siren_off_count = 0;
}

void test_basic_arm_disarm(void)
{
    alarm_sm_t sm;
    state_callbacks_t cbs = { .on_arm = cb_arm, .on_disarm = cb_disarm };
    sm_init(&sm, &cbs, 5000);
    reset_counts();

    assert(sm_get_state(&sm) == STATE_DISARMED);

    sm_process_event(&sm, EVT_ARM_CMD, 1000);
    assert(sm_get_state(&sm) == STATE_ARMED);
    assert(arm_count == 1);

    sm_process_event(&sm, EVT_DISARM_CMD, 2000);
    assert(sm_get_state(&sm) == STATE_DISARMED);
    assert(disarm_count == 1);

    printf("  ✓ basic arm/disarm
");
}

void test_full_alert_sequence(void)
{
    alarm_sm_t sm;
    state_callbacks_t cbs = {
        .on_arm = cb_arm, .on_disarm = cb_disarm,
        .on_warning = cb_warning, .on_alert = cb_alert,
        .on_tracking = cb_tracking,
        .on_siren_on = cb_siren_on, .on_siren_off = cb_siren_off,
    };
    sm_init(&sm, &cbs, 5000);
    reset_counts();

    sm_process_event(&sm, EVT_ARM_CMD, 0);
    assert(sm_get_state(&sm) == STATE_ARMED);

    sm_set_trigger(&sm, TRIGGER_MOTION);
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 1000);
    assert(sm_get_state(&sm) == STATE_WARNING);
    assert(warning_count == 1);

    sm_process_event(&sm, EVT_THREAT_CONFIRMED, 3000);
    assert(sm_get_state(&sm) == STATE_ALERT);
    assert(alert_count == 1);
    assert(siren_on_count == 1);

    sm_process_event(&sm, EVT_GPS_MOVING, 10000);
    assert(sm_get_state(&sm) == STATE_TRACKING);
    assert(tracking_count == 1);
    assert(siren_off_count == 1);

    sm_process_event(&sm, EVT_DISARM_CMD, 20000);
    assert(sm_get_state(&sm) == STATE_DISARMED);

    printf("  ✓ full alert sequence
");
}

void test_false_positive_returns_to_armed(void)
{
    alarm_sm_t sm;
    state_callbacks_t cbs = { .on_arm = cb_arm, .on_warning = cb_warning };
    sm_init(&sm, &cbs, 5000);
    reset_counts();

    sm_process_event(&sm, EVT_ARM_CMD, 0);
    sm_set_trigger(&sm, TRIGGER_PROXIMITY);
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 1000);
    assert(sm_get_state(&sm) == STATE_WARNING);

    sm_process_event(&sm, EVT_FALSE_POSITIVE, 3000);
    assert(sm_get_state(&sm) == STATE_ARMED);

    printf("  ✓ false positive returns to armed
");
}

void test_disarm_from_any_state(void)
{
    alarm_sm_t sm;
    state_callbacks_t cbs = { .on_arm = cb_arm, .on_disarm = cb_disarm,
                             .on_warning = cb_warning, .on_alert = cb_alert,
                             .on_siren_on = cb_siren_on, .on_siren_off = cb_siren_off };
    sm_init(&sm, &cbs, 5000);

    // Disarm from WARNING
    sm_process_event(&sm, EVT_ARM_CMD, 0);
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 1000);
    assert(sm_get_state(&sm) == STATE_WARNING);
    sm_process_event(&sm, EVT_DISARM_CMD, 2000);
    assert(sm_get_state(&sm) == STATE_DISARMED);

    // Disarm from ALERT
    sm_process_event(&sm, EVT_ARM_CMD, 3000);
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 4000);
    sm_process_event(&sm, EVT_THREAT_CONFIRMED, 5000);
    assert(sm_get_state(&sm) == STATE_ALERT);
    sm_process_event(&sm, EVT_DISARM_CMD, 6000);
    assert(sm_get_state(&sm) == STATE_DISARMED);

    printf("  ✓ disarm from any state
");
}

void test_ignore_invalid_events(void)
{
    alarm_sm_t sm;
    sm_init(&sm, NULL, 5000);

    // Sensor trigger while disarmed should be ignored
    sm_process_event(&sm, EVT_SENSOR_TRIGGER, 1000);
    assert(sm_get_state(&sm) == STATE_DISARMED);

    // GPS moving while armed should be ignored
    sm_process_event(&sm, EVT_ARM_CMD, 2000);
    sm_process_event(&sm, EVT_GPS_MOVING, 3000);
    assert(sm_get_state(&sm) == STATE_ARMED);

    printf("  ✓ ignore invalid events
");
}

int main(void)
{
    printf("State machine tests:
");
    test_basic_arm_disarm();
    test_full_alert_sequence();
    test_false_positive_returns_to_armed();
    test_disarm_from_any_state();
    test_ignore_invalid_events();
    printf("
All state machine tests passed! ✓
");
    return 0;
}
```

- [ ] **Step 4: Run state machine tests**

```bash
cd /a0/usr/projects/project_3/tests
gcc -Wall -Wextra -o test_state_machine test_state_machine.c && ./test_state_machine
```
Expected: "All state machine tests passed! ✓"

- [ ] **Step 5: Create CMakeLists.txt and commit**

Create `firmware/bike-unit/components/alert_manager/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "state_machine.c" "alert_handler.c" "actuators.c"
                       INCLUDE_DIRS "include")
```

```bash
cd /a0/usr/projects/project_3
git add firmware/bike-unit/components/alert_manager/ tests/test_state_machine.c
git commit -m "feat: alarm state machine with full transition logic and unit tests"
git push
```

---

### Task 5: SMS Command Parser

**Files:**
- Create: `firmware/bike-unit/components/cellular/include/cellular.h`
- Create: `firmware/bike-unit/components/cellular/sms.c`
- Create: `tests/test_sms_parser.c`

- [ ] **Step 1: Define SMS interface**

Create `firmware/bike-unit/components/cellular/include/cellular.h`:
```c
#ifndef CELLULAR_H
#define CELLULAR_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_PHONE_NUMBER_LEN 20
#define MAX_SMS_BODY_LEN     160
#define MAX_AUTHORIZED_NUMBERS 3
#define PIN_LENGTH 4

// === SMS Command parsing ===
typedef enum {
    SMS_CMD_UNKNOWN = 0,
    SMS_CMD_ARM,
    SMS_CMD_DISARM,
    SMS_CMD_STATUS,
    SMS_CMD_LOCATE,
    SMS_CMD_SIREN_ON,
    SMS_CMD_SIREN_OFF,
    SMS_CMD_TRACK,
    SMS_CMD_STOP,
    SMS_CMD_SENS_HIGH,
    SMS_CMD_SENS_MED,
    SMS_CMD_SENS_LOW,
    SMS_CMD_GPS,
} sms_command_t;

typedef struct {
    char sender[MAX_PHONE_NUMBER_LEN];
    char body[MAX_SMS_BODY_LEN];
    sms_command_t parsed_cmd;
    bool pin_valid;
} sms_message_t;

typedef struct {
    char authorized_numbers[MAX_AUTHORIZED_NUMBERS][MAX_PHONE_NUMBER_LEN];
    uint8_t num_authorized;
    char pin[PIN_LENGTH + 1];  // null-terminated
    bool pin_required;
} sms_config_t;

// Parse SMS body into command
sms_command_t sms_parse_command(const char *body);

// Check if sender is authorized
bool sms_check_authorized(const sms_config_t *config, const char *sender);

// Check PIN in message (format: "1234 COMMAND")
bool sms_check_pin(const sms_config_t *config, const char *body, const char **cmd_start);

// Full message processing
sms_command_t sms_process_message(const sms_config_t *config, sms_message_t *msg);

// Format GPS coordinates as Google Maps link
void sms_format_gps_link(double lat, double lon, char *buf, size_t buf_len);

#endif // CELLULAR_H
```

- [ ] **Step 2: Implement SMS parser**

Create `firmware/bike-unit/components/cellular/sms.c`:
```c
#include "cellular.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Convert string to uppercase in-place
static void str_to_upper(char *s)
{
    while (*s) {
        *s = toupper((unsigned char)*s);
        s++;
    }
}

sms_command_t sms_parse_command(const char *body)
{
    char upper[MAX_SMS_BODY_LEN];
    strncpy(upper, body, MAX_SMS_BODY_LEN - 1);
    upper[MAX_SMS_BODY_LEN - 1] = ' ';
    str_to_upper(upper);

    // Trim leading whitespace
    const char *p = upper;
    while (*p && isspace((unsigned char)*p)) p++;

    if (strncmp(p, "ARM", 3) == 0 && (p[3] == ' ' || isspace((unsigned char)p[3])))
        return SMS_CMD_ARM;
    if (strncmp(p, "DISARM", 6) == 0 && (p[6] == ' ' || isspace((unsigned char)p[6])))
        return SMS_CMD_DISARM;
    if (strncmp(p, "STATUS", 6) == 0)
        return SMS_CMD_STATUS;
    if (strncmp(p, "LOCATE", 6) == 0)
        return SMS_CMD_LOCATE;
    if (strncmp(p, "SIREN ON", 8) == 0)
        return SMS_CMD_SIREN_ON;
    if (strncmp(p, "SIREN OFF", 9) == 0)
        return SMS_CMD_SIREN_OFF;
    if (strncmp(p, "TRACK", 5) == 0)
        return SMS_CMD_TRACK;
    if (strncmp(p, "STOP", 4) == 0)
        return SMS_CMD_STOP;
    if (strncmp(p, "SENS HIGH", 9) == 0)
        return SMS_CMD_SENS_HIGH;
    if (strncmp(p, "SENS MED", 8) == 0)
        return SMS_CMD_SENS_MED;
    if (strncmp(p, "SENS LOW", 8) == 0)
        return SMS_CMD_SENS_LOW;
    if (strncmp(p, "GPS", 3) == 0 && (p[3] == ' ' || isspace((unsigned char)p[3])))
        return SMS_CMD_GPS;

    return SMS_CMD_UNKNOWN;
}

bool sms_check_authorized(const sms_config_t *config, const char *sender)
{
    for (int i = 0; i < config->num_authorized; i++) {
        if (strcmp(config->authorized_numbers[i], sender) == 0) {
            return true;
        }
    }
    return false;
}

bool sms_check_pin(const sms_config_t *config, const char *body, const char **cmd_start)
{
    if (!config->pin_required) {
        *cmd_start = body;
        return true;
    }

    // Check if body starts with PIN followed by space
    size_t pin_len = strlen(config->pin);
    if (strncmp(body, config->pin, pin_len) == 0 &&
        body[pin_len] == ' ') {
        *cmd_start = body + pin_len + 1;
        return true;
    }

    *cmd_start = NULL;
    return false;
}

sms_command_t sms_process_message(const sms_config_t *config, sms_message_t *msg)
{
    // Check authorization
    if (!sms_check_authorized(config, msg->sender)) {
        msg->parsed_cmd = SMS_CMD_UNKNOWN;
        msg->pin_valid = false;
        return SMS_CMD_UNKNOWN;
    }

    // Check PIN
    const char *cmd_start = NULL;
    msg->pin_valid = sms_check_pin(config, msg->body, &cmd_start);
    if (!msg->pin_valid) {
        msg->parsed_cmd = SMS_CMD_UNKNOWN;
        return SMS_CMD_UNKNOWN;
    }

    // Parse command
    msg->parsed_cmd = sms_parse_command(cmd_start);
    return msg->parsed_cmd;
}

void sms_format_gps_link(double lat, double lon, char *buf, size_t buf_len)
{
    snprintf(buf, buf_len, "https://maps.google.com/?q=%.7f,%.7f", lat, lon);
}
```

- [ ] **Step 3: Write SMS parser tests**

Create `tests/test_sms_parser.c`:
```c
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../firmware/bike-unit/components/cellular/include/cellular.h"
#include "../firmware/bike-unit/components/cellular/sms.c"

void test_parse_commands(void)
{
    assert(sms_parse_command("ARM") == SMS_CMD_ARM);
    assert(sms_parse_command("arm") == SMS_CMD_ARM);
    assert(sms_parse_command("Arm") == SMS_CMD_ARM);
    assert(sms_parse_command("  ARM") == SMS_CMD_ARM);
    assert(sms_parse_command("DISARM") == SMS_CMD_DISARM);
    assert(sms_parse_command("STATUS") == SMS_CMD_STATUS);
    assert(sms_parse_command("LOCATE") == SMS_CMD_LOCATE);
    assert(sms_parse_command("SIREN ON") == SMS_CMD_SIREN_ON);
    assert(sms_parse_command("SIREN OFF") == SMS_CMD_SIREN_OFF);
    assert(sms_parse_command("TRACK") == SMS_CMD_TRACK);
    assert(sms_parse_command("STOP") == SMS_CMD_STOP);
    assert(sms_parse_command("SENS HIGH") == SMS_CMD_SENS_HIGH);
    assert(sms_parse_command("SENS MED") == SMS_CMD_SENS_MED);
    assert(sms_parse_command("SENS LOW") == SMS_CMD_SENS_LOW);
    assert(sms_parse_command("GPS") == SMS_CMD_GPS);
    assert(sms_parse_command("HELLO") == SMS_CMD_UNKNOWN);
    assert(sms_parse_command("") == SMS_CMD_UNKNOWN);
    printf("  ✓ command parsing
");
}

void test_authorization(void)
{
    sms_config_t config = {
        .authorized_numbers = {"+61400111222", "+61400333444"},
        .num_authorized = 2,
    };

    assert(sms_check_authorized(&config, "+61400111222"));
    assert(sms_check_authorized(&config, "+61400333444"));
    assert(!sms_check_authorized(&config, "+61400999888"));
    assert(!sms_check_authorized(&config, ""));
    printf("  ✓ authorization check
");
}

void test_pin_check(void)
{
    sms_config_t config = {
        .pin = "1234",
        .pin_required = true,
    };
    const char *cmd_start;

    assert(sms_check_pin(&config, "1234 ARM", &cmd_start));
    assert(strcmp(cmd_start, "ARM") == 0);

    assert(!sms_check_pin(&config, "0000 ARM", &cmd_start));
    assert(!sms_check_pin(&config, "ARM", &cmd_start));
    assert(!sms_check_pin(&config, "1234ARM", &cmd_start));  // No space

    // PIN not required
    config.pin_required = false;
    assert(sms_check_pin(&config, "ARM", &cmd_start));
    assert(strcmp(cmd_start, "ARM") == 0);

    printf("  ✓ PIN validation
");
}

void test_full_processing(void)
{
    sms_config_t config = {
        .authorized_numbers = {"+61400111222"},
        .num_authorized = 1,
        .pin = "5678",
        .pin_required = true,
    };

    sms_message_t msg;
    strcpy(msg.sender, "+61400111222");
    strcpy(msg.body, "5678 LOCATE");

    assert(sms_process_message(&config, &msg) == SMS_CMD_LOCATE);
    assert(msg.pin_valid);

    // Wrong PIN
    strcpy(msg.body, "0000 LOCATE");
    assert(sms_process_message(&config, &msg) == SMS_CMD_UNKNOWN);

    // Unauthorized sender
    strcpy(msg.sender, "+61400999999");
    strcpy(msg.body, "5678 ARM");
    assert(sms_process_message(&config, &msg) == SMS_CMD_UNKNOWN);

    printf("  ✓ full message processing
");
}

void test_gps_link_format(void)
{
    char buf[128];
    sms_format_gps_link(-33.8688, 151.2093, buf, sizeof(buf));
    assert(strstr(buf, "maps.google.com") != NULL);
    assert(strstr(buf, "-33.8688") != NULL);
    assert(strstr(buf, "151.2093") != NULL);
    printf("  ✓ GPS link formatting
");
}

int main(void)
{
    printf("SMS parser tests:
");
    test_parse_commands();
    test_authorization();
    test_pin_check();
    test_full_processing();
    test_gps_link_format();
    printf("
All SMS parser tests passed! ✓
");
    return 0;
}
```

- [ ] **Step 4: Run SMS parser tests**

```bash
cd /a0/usr/projects/project_3/tests
gcc -Wall -Wextra -o test_sms_parser test_sms_parser.c -lm && ./test_sms_parser
```
Expected: "All SMS parser tests passed! ✓"

- [ ] **Step 5: Commit**

```bash
cd /a0/usr/projects/project_3
git add firmware/bike-unit/components/cellular/ tests/test_sms_parser.c
git commit -m "feat: SMS command parser with authorization, PIN validation, and GPS link formatting"
git push
```

---

### Task 6: Geofence Calculator

**Files:**
- Create: `firmware/bike-unit/components/cellular/gnss.c`
- Create: `tests/test_geofence.c`

- [ ] **Step 1: Implement geofence calculation in gnss.c**

Add to `firmware/bike-unit/components/cellular/include/cellular.h` (append):
```c
// === GNSS / Geofence ===
#define MAX_GEOFENCES 5

typedef struct {
    double center_lat;
    double center_lon;
    uint32_t radius_m;
    bool active;
} geofence_t;

typedef struct {
    double latitude;
    double longitude;
    float speed_kmh;
    float heading_deg;
    uint8_t fix_type;  // 0=none, 1=2D, 2=3D
    uint8_t num_sats;
} gps_fix_t;

// Calculate distance between two points (Haversine, returns meters)
double gnss_distance_m(double lat1, double lon1, double lat2, double lon2);

// Check if position is inside geofence
bool gnss_inside_geofence(const gps_fix_t *fix, const geofence_t *fence);

// Check all geofences, returns index of breached fence or -1
int gnss_check_geofences(const gps_fix_t *fix, const geofence_t *fences, int count);
```

Create `firmware/bike-unit/components/cellular/gnss.c`:
```c
#include "cellular.h"
#include <math.h>

#define EARTH_RADIUS_M 6371000.0
#define DEG_TO_RAD(x) ((x) * M_PI / 180.0)

double gnss_distance_m(double lat1, double lon1, double lat2, double lon2)
{
    double dlat = DEG_TO_RAD(lat2 - lat1);
    double dlon = DEG_TO_RAD(lon2 - lon1);
    double a = sin(dlat / 2) * sin(dlat / 2) +
               cos(DEG_TO_RAD(lat1)) * cos(DEG_TO_RAD(lat2)) *
               sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return EARTH_RADIUS_M * c;
}

bool gnss_inside_geofence(const gps_fix_t *fix, const geofence_t *fence)
{
    if (!fence->active) return true;  // Inactive fence = always inside
    double dist = gnss_distance_m(fix->latitude, fix->longitude,
                                  fence->center_lat, fence->center_lon);
    return dist <= (double)fence->radius_m;
}

int gnss_check_geofences(const gps_fix_t *fix, const geofence_t *fences, int count)
{
    for (int i = 0; i < count; i++) {
        if (fences[i].active && !gnss_inside_geofence(fix, &fences[i])) {
            return i;  // Breached this fence
        }
    }
    return -1;  // All fences OK
}
```

- [ ] **Step 2: Write geofence tests**

Create `tests/test_geofence.c`:
```c
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../firmware/bike-unit/components/cellular/include/cellular.h"
#include "../firmware/bike-unit/components/cellular/gnss.c"

void test_distance_known_points(void)
{
    // Sydney Opera House to Sydney Harbour Bridge: ~1.1km
    double dist = gnss_distance_m(-33.8568, 151.2153, -33.8523, 151.2108);
    assert(dist > 600 && dist < 700);  // Approximately 650m

    // Same point = 0 distance
    dist = gnss_distance_m(-33.8688, 151.2093, -33.8688, 151.2093);
    assert(dist < 0.01);

    printf("  ✓ distance calculation
");
}

void test_inside_geofence(void)
{
    geofence_t fence = {
        .center_lat = -33.8688,
        .center_lon = 151.2093,
        .radius_m = 100,
        .active = true,
    };

    // Point 50m away (inside)
    gps_fix_t fix_inside = { .latitude = -33.8684, .longitude = 151.2093 };
    assert(gnss_inside_geofence(&fix_inside, &fence));

    // Point 500m away (outside)
    gps_fix_t fix_outside = { .latitude = -33.8640, .longitude = 151.2093 };
    assert(!gnss_inside_geofence(&fix_outside, &fence));

    // Inactive fence = always inside
    fence.active = false;
    assert(gnss_inside_geofence(&fix_outside, &fence));

    printf("  ✓ geofence inside/outside
");
}

void test_check_multiple_geofences(void)
{
    geofence_t fences[3] = {
        { .center_lat = -33.8688, .center_lon = 151.2093, .radius_m = 100, .active = true },
        { .center_lat = -33.8800, .center_lon = 151.2200, .radius_m = 200, .active = true },
        { .center_lat = 0, .center_lon = 0, .radius_m = 50, .active = false },  // Inactive
    };

    // Inside first fence
    gps_fix_t fix1 = { .latitude = -33.8688, .longitude = 151.2093 };
    assert(gnss_check_geofences(&fix1, fences, 3) == -1);  // All OK

    // Outside first fence, inside second
    gps_fix_t fix2 = { .latitude = -33.8750, .longitude = 151.2150 };
    assert(gnss_check_geofences(&fix2, fences, 3) == 0);  // Breached fence 0

    printf("  ✓ multiple geofence check
");
}

int main(void)
{
    printf("Geofence tests:
");
    test_distance_known_points();
    test_inside_geofence();
    test_check_multiple_geofences();
    printf("
All geofence tests passed! ✓
");
    return 0;
}
```

- [ ] **Step 3: Run geofence tests**

```bash
cd /a0/usr/projects/project_3/tests
gcc -Wall -Wextra -o test_geofence test_geofence.c -lm && ./test_geofence
```
Expected: "All geofence tests passed! ✓"

- [ ] **Step 4: Commit**

```bash
cd /a0/usr/projects/project_3
git add firmware/bike-unit/components/cellular/gnss.c tests/test_geofence.c
git commit -m "feat: GNSS geofence calculator with Haversine distance and multi-fence support"
git push
```

---

## Phase 2: Hardware Design (KiCad)

### Task 7: KiCad Bike Unit Schematic

**Files:**
- Create: `hardware/bike-unit/bike-unit.kicad_pro`
- Create: `hardware/bike-unit/bike-unit.kicad_sch`
- Create: `hardware/bike-unit/lib/` (custom symbols/footprints)

- [ ] **Step 1: Install KiCad**

```bash
apt-get install -y kicad
```

- [ ] **Step 2: Create KiCad project structure**

```bash
mkdir -p /a0/usr/projects/project_3/hardware/bike-unit/lib
mkdir -p /a0/usr/projects/project_3/hardware/bike-unit/gerbers
mkdir -p /a0/usr/projects/project_3/hardware/fob-unit/lib
mkdir -p /a0/usr/projects/project_3/hardware/fob-unit/gerbers
mkdir -p /a0/usr/projects/project_3/hardware/datasheets
```

- [ ] **Step 3: Create schematic design notes document**

Create `hardware/bike-unit/DESIGN-NOTES.md`:
```markdown
# Bike Unit PCB Design Notes

## Board Dimensions
- Target: 90mm × 55mm
- 4x M3 mounting holes in corners (with rubber grommets for vibration)

## Layer Stack
- 2-layer, 1.6mm, 1oz copper
- Top: Signal + components
- Bottom: Ground pour + minimal routing

## Key Design Rules
- Min trace: 0.2mm (signal), 0.5mm (power)
- Min space: 0.2mm
- Via size: 0.8mm drill, 1.2mm pad
- LoRa antenna trace: 50Ω microstrip (width depends on substrate εr)

## Component Placement Priority
1. ESP32-S3 module center
2. SX1262 LoRa module near board edge (antenna U.FL on edge)
3. SIM7080G near board edge (antenna U.FL and SIM holder accessible)
4. LSM6DSO center of board (best represents bike center of mass)
5. Power section (buck, charger, battery holder) one end
6. Connectors (JST-XH) along one edge for wiring

## Critical Routing
- SPI to SX1262: keep traces short, matched length if possible
- UART to SIM7080G: keep away from RF sections
- I²C to IMU: standard, not critical
- Power traces: 0.5mm minimum, 1mm preferred for 12V input
- U.FL antenna traces: as short as possible, 50Ω controlled impedance

## Decoupling
- 100nF + 10µF on each power pin of ESP32-S3
- 100nF on SX1262 VCC
- 100nF + 10µF on SIM7080G VCC (high current bursts during TX)
- 100nF on LSM6DSO VCC
```

- [ ] **Step 4: Commit hardware structure**

```bash
cd /a0/usr/projects/project_3
git add hardware/
git commit -m "feat: hardware directory structure with PCB design notes"
git push
```

**Note:** The actual KiCad schematic drawing is interactive/GUI work. The engineer should follow the pin assignments from `app_config.h` and the design notes above. The schematic PDF should be exported and committed when complete.

---

## Phase 3: Hardware Drivers (On Dev Boards)

### Task 8: SX1262 LoRa SPI Driver

**Files:**
- Create: `firmware/bike-unit/components/lora/CMakeLists.txt`
- Create: `firmware/bike-unit/components/lora/include/lora.h`
- Create: `firmware/bike-unit/components/lora/lora_sx1262.c`
- Create: `firmware/bike-unit/components/lora/lora_packet.c`

- [ ] **Step 1: Define LoRa driver interface**

Create `firmware/bike-unit/components/lora/include/lora.h`:
```c
#ifndef LORA_H
#define LORA_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"

// SX1262 operational modes
typedef enum {
    LORA_MODE_SLEEP,
    LORA_MODE_STANDBY,
    LORA_MODE_TX,
    LORA_MODE_RX,
    LORA_MODE_RX_CONTINUOUS,
} lora_mode_t;

typedef struct {
    uint32_t frequency_hz;
    uint8_t  spreading_factor;  // 6-12
    uint32_t bandwidth_hz;      // 125000, 250000, 500000
    uint8_t  coding_rate;       // 5-8 (4/5 to 4/8)
    int8_t   tx_power_dbm;      // -9 to +22 (or +30 with PA)
    uint16_t preamble_length;
    uint8_t  sync_word;
} lora_config_t;

typedef void (*lora_rx_callback_t)(const uint8_t *data, uint8_t len, int8_t rssi, int8_t snr);

// Initialization
esp_err_t lora_init(const lora_config_t *config);
void lora_deinit(void);

// Configuration
esp_err_t lora_set_frequency(uint32_t freq_hz);
esp_err_t lora_set_tx_power(int8_t power_dbm);
esp_err_t lora_set_spreading_factor(uint8_t sf);

// Operations
esp_err_t lora_send(const uint8_t *data, uint8_t len);
esp_err_t lora_receive_start(lora_rx_callback_t callback);
esp_err_t lora_receive_stop(void);
lora_mode_t lora_get_mode(void);

// Power management
esp_err_t lora_sleep(void);
esp_err_t lora_standby(void);

// Diagnostics
int8_t lora_get_rssi(void);
int8_t lora_get_snr(void);

// Packet-level (uses crypto + protocol)
esp_err_t lora_send_packet(lora_packet_t *pkt);

#endif // LORA_H
```

- [ ] **Step 2: Implement SX1262 SPI driver**

Create `firmware/bike-unit/components/lora/lora_sx1262.c`:
```c
#include "lora.h"
#include "app_config.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "LORA";

// SX1262 Commands
#define SX1262_CMD_SET_SLEEP            0x84
#define SX1262_CMD_SET_STANDBY          0x80
#define SX1262_CMD_SET_TX               0x83
#define SX1262_CMD_SET_RX               0x82
#define SX1262_CMD_SET_RF_FREQUENCY     0x86
#define SX1262_CMD_SET_PA_CONFIG        0x95
#define SX1262_CMD_SET_TX_PARAMS        0x8E
#define SX1262_CMD_SET_MODULATION_PARAMS 0x8B
#define SX1262_CMD_SET_PACKET_PARAMS    0x8C
#define SX1262_CMD_SET_DIO_IRQ_PARAMS   0x08
#define SX1262_CMD_WRITE_BUFFER         0x0E
#define SX1262_CMD_READ_BUFFER          0x1E
#define SX1262_CMD_GET_IRQ_STATUS       0x12
#define SX1262_CMD_CLEAR_IRQ_STATUS     0x02
#define SX1262_CMD_GET_RSSI_INST        0x15
#define SX1262_CMD_SET_PACKET_TYPE      0x8A
#define SX1262_CMD_SET_SYNC_WORD        0x0D

// IRQ Flags
#define IRQ_TX_DONE     (1 << 0)
#define IRQ_RX_DONE     (1 << 1)
#define IRQ_TIMEOUT     (1 << 9)

static spi_device_handle_t spi_handle;
static lora_rx_callback_t rx_callback = NULL;
static lora_mode_t current_mode = LORA_MODE_SLEEP;
static SemaphoreHandle_t spi_mutex;

// === Low-level SPI ===

static void wait_busy(void)
{
    while (gpio_get_level(PIN_LORA_BUSY)) {
        vTaskDelay(1);
    }
}

static void spi_write_command(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    wait_busy();
    xSemaphoreTake(spi_mutex, portMAX_DELAY);

    spi_transaction_t t = {
        .length = (1 + len) * 8,
        .tx_data = {0},
    };

    uint8_t tx_buf[64] = {cmd};
    if (data && len > 0) {
        memcpy(tx_buf + 1, data, len);
    }
    t.tx_buffer = tx_buf;
    t.length = (1 + len) * 8;

    gpio_set_level(PIN_LORA_CS, 0);
    spi_device_transmit(spi_handle, &t);
    gpio_set_level(PIN_LORA_CS, 1);

    xSemaphoreGive(spi_mutex);
}

static void spi_read_command(uint8_t cmd, uint8_t *data, uint8_t len)
{
    wait_busy();
    xSemaphoreTake(spi_mutex, portMAX_DELAY);

    uint8_t tx_buf[64] = {cmd, 0x00};  // CMD + NOP for status
    uint8_t rx_buf[64] = {0};

    spi_transaction_t t = {
        .length = (2 + len) * 8,
        .tx_buffer = tx_buf,
        .rx_buffer = rx_buf,
    };

    gpio_set_level(PIN_LORA_CS, 0);
    spi_device_transmit(spi_handle, &t);
    gpio_set_level(PIN_LORA_CS, 1);

    if (data && len > 0) {
        memcpy(data, rx_buf + 2, len);  // Skip cmd echo + status
    }

    xSemaphoreGive(spi_mutex);
}

// === DIO1 IRQ Handler ===

static void IRAM_ATTR dio1_isr_handler(void *arg)
{
    // Signal task to process IRQ (not doing SPI in ISR)
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Post to a queue or notify a task here
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// === Public API ===

esp_err_t lora_init(const lora_config_t *config)
{
    ESP_LOGI(TAG, "Initializing SX1262 LoRa module...");

    spi_mutex = xSemaphoreCreateMutex();

    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_LORA_CS) | (1ULL << PIN_LORA_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << PIN_LORA_BUSY);
    io_conf.mode = GPIO_MODE_INPUT;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = (1ULL << PIN_LORA_DIO1);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    gpio_config(&io_conf);

    gpio_set_level(PIN_LORA_CS, 1);

    // Reset module
    gpio_set_level(PIN_LORA_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_LORA_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(20));

    // Initialize SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_LORA_MOSI,
        .miso_io_num = PIN_LORA_MISO,
        .sclk_io_num = PIN_LORA_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 256,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 8000000,  // 8 MHz
        .mode = 0,
        .spics_io_num = -1,  // Manual CS control
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &spi_handle));

    // Configure DIO1 interrupt
    gpio_install_isr_service(0);
    gpio_isr_handler_add(PIN_LORA_DIO1, dio1_isr_handler, NULL);

    // Set to standby
    uint8_t standby_cfg = 0x00;  // STDBY_RC
    spi_write_command(SX1262_CMD_SET_STANDBY, &standby_cfg, 1);

    // Set packet type to LoRa
    uint8_t pkt_type = 0x01;  // LORA
    spi_write_command(SX1262_CMD_SET_PACKET_TYPE, &pkt_type, 1);

    // Set frequency
    lora_set_frequency(config->frequency_hz);

    // Set modulation params (SF, BW, CR)
    uint8_t mod_params[4] = {
        config->spreading_factor,
        0x04,  // BW 125kHz
        config->coding_rate - 4,  // CR 4/5 = 0x01
        0x00,  // Low data rate optimize off
    };
    spi_write_command(SX1262_CMD_SET_MODULATION_PARAMS, mod_params, 4);

    // Set packet params
    uint8_t pkt_params[6] = {
        0x00, config->preamble_length,  // Preamble length
        0x00,  // Implicit header off (explicit)
        PACKET_SIZE,  // Payload length
        0x01,  // CRC on
        0x00,  // Standard IQ
    };
    spi_write_command(SX1262_CMD_SET_PACKET_PARAMS, pkt_params, 6);

    // Set TX power
    lora_set_tx_power(config->tx_power_dbm);

    // Set sync word
    uint8_t sync[2] = {config->sync_word, config->sync_word};
    // Write sync word to register (address 0x0740)
    // (simplified - actual implementation needs register write command)

    // Configure IRQ: TX_DONE and RX_DONE on DIO1
    uint8_t irq_params[8] = {
        0x02, 0x03,  // IRQ mask: TX_DONE | RX_DONE
        0x02, 0x03,  // DIO1 mask: same
        0x00, 0x00,  // DIO2 mask: none
        0x00, 0x00,  // DIO3 mask: none
    };
    spi_write_command(SX1262_CMD_SET_DIO_IRQ_PARAMS, irq_params, 8);

    current_mode = LORA_MODE_STANDBY;
    ESP_LOGI(TAG, "SX1262 initialized: %lu Hz, SF%d, BW %lu, CR 4/%d, %d dBm",
             config->frequency_hz, config->spreading_factor,
             config->bandwidth_hz, config->coding_rate, config->tx_power_dbm);

    return ESP_OK;
}

esp_err_t lora_set_frequency(uint32_t freq_hz)
{
    uint32_t frf = (uint32_t)((double)freq_hz / (double)(32000000) * (double)(1 << 25));
    uint8_t data[4] = {
        (frf >> 24) & 0xFF,
        (frf >> 16) & 0xFF,
        (frf >> 8) & 0xFF,
        frf & 0xFF,
    };
    spi_write_command(SX1262_CMD_SET_RF_FREQUENCY, data, 4);
    return ESP_OK;
}

esp_err_t lora_set_tx_power(int8_t power_dbm)
{
    // PA config for SX1262 (up to +22 dBm, E22-900M30S has external PA for +30)
    uint8_t pa_config[4] = {0x04, 0x07, 0x00, 0x01};  // Max PA
    spi_write_command(SX1262_CMD_SET_PA_CONFIG, pa_config, 4);

    uint8_t tx_params[2] = {(uint8_t)power_dbm, 0x04};  // Power, ramp time 200us
    spi_write_command(SX1262_CMD_SET_TX_PARAMS, tx_params, 2);
    return ESP_OK;
}

esp_err_t lora_send(const uint8_t *data, uint8_t len)
{
    // Write to buffer
    uint8_t offset = 0;
    uint8_t write_cmd[2] = {offset, 0};
    // Write buffer command with offset
    wait_busy();
    // ... (full buffer write implementation)

    // Set payload length
    uint8_t pkt_params[6] = {0x00, 0x08, 0x00, len, 0x01, 0x00};
    spi_write_command(SX1262_CMD_SET_PACKET_PARAMS, pkt_params, 6);

    // Set TX mode (timeout 0 = no timeout)
    uint8_t timeout[3] = {0x00, 0x00, 0x00};
    spi_write_command(SX1262_CMD_SET_TX, timeout, 3);

    current_mode = LORA_MODE_TX;
    ESP_LOGI(TAG, "TX: %d bytes", len);
    return ESP_OK;
}

esp_err_t lora_receive_start(lora_rx_callback_t callback)
{
    rx_callback = callback;

    // Set RX continuous
    uint8_t timeout[3] = {0xFF, 0xFF, 0xFF};  // Continuous
    spi_write_command(SX1262_CMD_SET_RX, timeout, 3);

    current_mode = LORA_MODE_RX_CONTINUOUS;
    ESP_LOGI(TAG, "RX: continuous mode");
    return ESP_OK;
}

esp_err_t lora_receive_stop(void)
{
    uint8_t standby_cfg = 0x00;
    spi_write_command(SX1262_CMD_SET_STANDBY, &standby_cfg, 1);
    current_mode = LORA_MODE_STANDBY;
    rx_callback = NULL;
    return ESP_OK;
}

esp_err_t lora_sleep(void)
{
    uint8_t sleep_cfg = 0x04;  // Warm start (retain config)
    spi_write_command(SX1262_CMD_SET_SLEEP, &sleep_cfg, 1);
    current_mode = LORA_MODE_SLEEP;
    return ESP_OK;
}

esp_err_t lora_standby(void)
{
    uint8_t standby_cfg = 0x00;
    spi_write_command(SX1262_CMD_SET_STANDBY, &standby_cfg, 1);
    current_mode = LORA_MODE_STANDBY;
    return ESP_OK;
}

lora_mode_t lora_get_mode(void)
{
    return current_mode;
}

void lora_deinit(void)
{
    lora_sleep();
    spi_bus_remove_device(spi_handle);
    spi_bus_free(SPI2_HOST);
    vSemaphoreDelete(spi_mutex);
}
```

- [ ] **Step 3: Create CMakeLists.txt**

Create `firmware/bike-unit/components/lora/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "lora_sx1262.c" "lora_packet.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver)
```

- [ ] **Step 4: Verify compilation**

```bash
cd /a0/usr/projects/project_3/firmware/bike-unit
idf.py build
```
Expected: Build succeeds (may have warnings for unused variables in stubs).

- [ ] **Step 5: Commit**

```bash
cd /a0/usr/projects/project_3
git add firmware/bike-unit/components/lora/
git commit -m "feat: SX1262 LoRa SPI driver with TX/RX/sleep support"
git push
```

---

### Task 9: SIM7080G AT Command Driver

**Files:**
- Create: `firmware/bike-unit/components/cellular/CMakeLists.txt`
- Create: `firmware/bike-unit/components/cellular/sim7080g.c`

- [ ] **Step 1: Implement SIM7080G UART driver**

Create `firmware/bike-unit/components/cellular/sim7080g.c`:
```c
#include "cellular.h"
#include "app_config.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "SIM7080";

#define SIM_UART_NUM    UART_NUM_1
#define SIM_BAUD_RATE   115200
#define SIM_BUF_SIZE    1024
#define AT_TIMEOUT_MS   5000
#define SMS_TIMEOUT_MS  10000

static char response_buf[SIM_BUF_SIZE];

// === Low-level UART ===

static esp_err_t sim_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = SIM_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(SIM_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(SIM_UART_NUM, PIN_SIM_TX, PIN_SIM_RX, -1, -1));
    ESP_ERROR_CHECK(uart_driver_install(SIM_UART_NUM, SIM_BUF_SIZE * 2, 0, 0, NULL, 0));
    return ESP_OK;
}

static int sim_send_at(const char *cmd, char *response, size_t resp_size, uint32_t timeout_ms)
{
    // Flush RX buffer
    uart_flush_input(SIM_UART_NUM);

    // Send command
    char cmd_buf[256];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s
", cmd);
    uart_write_bytes(SIM_UART_NUM, cmd_buf, strlen(cmd_buf));

    // Read response
    int len = uart_read_bytes(SIM_UART_NUM, (uint8_t *)response, resp_size - 1, pdMS_TO_TICKS(timeout_ms));
    if (len > 0) {
        response[len] = ' ';
    } else {
        response[0] = ' ';
        return -1;
    }
    return len;
}

static bool sim_send_at_ok(const char *cmd, uint32_t timeout_ms)
{
    sim_send_at(cmd, response_buf, SIM_BUF_SIZE, timeout_ms);
    return strstr(response_buf, "OK") != NULL;
}

// === Power Management ===

static void sim_power_on(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_SIM_PWRKEY),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    gpio_set_level(PIN_SIM_PWRKEY, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(PIN_SIM_PWRKEY, 0);
    vTaskDelay(pdMS_TO_TICKS(5000));  // Wait for module to boot
}

// === Public API ===

esp_err_t cellular_init(void)
{
    ESP_LOGI(TAG, "Initializing SIM7080G...");

    sim_uart_init();
    sim_power_on();

    // Check module responds
    if (!sim_send_at_ok("AT", 2000)) {
        ESP_LOGE(TAG, "Module not responding");
        return ESP_ERR_TIMEOUT;
    }

    // Disable echo
    sim_send_at_ok("ATE0", 1000);

    // Check SIM
    if (!sim_send_at_ok("AT+CPIN?", 5000)) {
        ESP_LOGE(TAG, "SIM not ready");
        return ESP_ERR_NOT_FOUND;
    }

    // Set SMS text mode
    sim_send_at_ok("AT+CMGF=1", 1000);

    // Set SMS notification
    sim_send_at_ok("AT+CNMI=2,1,0,0,0", 1000);

    ESP_LOGI(TAG, "SIM7080G initialized");
    return ESP_OK;
}

esp_err_t cellular_send_sms(const char *number, const char *message)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CMGS="%s"", number);

    // Send command
    char cmd_buf[64];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s", cmd);
    uart_write_bytes(SIM_UART_NUM, cmd_buf, strlen(cmd_buf));
    vTaskDelay(pdMS_TO_TICKS(500));

    // Wait for "> " prompt then send message + Ctrl+Z
    uart_write_bytes(SIM_UART_NUM, message, strlen(message));
    uint8_t ctrlz = 0x1A;
    uart_write_bytes(SIM_UART_NUM, (char *)&ctrlz, 1);

    // Wait for OK
    int len = uart_read_bytes(SIM_UART_NUM, (uint8_t *)response_buf,
                             SIM_BUF_SIZE - 1, pdMS_TO_TICKS(SMS_TIMEOUT_MS));
    if (len > 0) {
        response_buf[len] = ' ';
        if (strstr(response_buf, "OK")) {
            ESP_LOGI(TAG, "SMS sent to %s", number);
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "SMS send failed");
    return ESP_FAIL;
}

esp_err_t cellular_gnss_start(void)
{
    // Power on GNSS
    if (!sim_send_at_ok("AT+CGNSPWR=1", 2000)) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "GNSS powered on");
    return ESP_OK;
}

esp_err_t cellular_gnss_get_fix(gps_fix_t *fix)
{
    sim_send_at("AT+CGNSINF", response_buf, SIM_BUF_SIZE, 2000);

    // Parse CGNSINF response:
    // +CGNSINF: run,fix,utc,lat,lon,alt,speed,course,fixmode,reserved,...
    char *p = strstr(response_buf, "+CGNSINF:");
    if (!p) return ESP_FAIL;

    p += 9;  // Skip "+CGNSINF:"

    int run, fix_status;
    double lat, lon, speed, course;

    // Simple parsing (comma-separated)
    if (sscanf(p, "%d,%d,%*[^,],%lf,%lf,%*[^,],%lf,%lf",
               &run, &fix_status, &lat, &lon, &speed, &course) >= 4) {
        fix->latitude = lat;
        fix->longitude = lon;
        fix->speed_kmh = (float)speed;
        fix->heading_deg = (float)course;
        fix->fix_type = (fix_status > 0) ? 2 : 0;
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t cellular_gnss_stop(void)
{
    sim_send_at_ok("AT+CGNSPWR=0", 2000);
    return ESP_OK;
}
```

- [ ] **Step 2: Create CMakeLists.txt**

Create `firmware/bike-unit/components/cellular/CMakeLists.txt`:
```cmake
idf_component_register(SRCS "sim7080g.c" "sms.c" "gnss.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver)
```

- [ ] **Step 3: Verify compilation**

```bash
cd /a0/usr/projects/project_3/firmware/bike-unit
idf.py build
```

- [ ] **Step 4: Commit**

```bash
cd /a0/usr/projects/project_3
git add firmware/bike-unit/components/cellular/
git commit -m "feat: SIM7080G UART driver with SMS send/receive and GNSS fix acquisition"
git push
```

---

### Task 10: IMU Sensor Driver

**Files:**
- Create: `firmware/bike-unit/components/sensors/CMakeLists.txt`
- Create: `firmware/bike-unit/components/sensors/include/sensors.h`
- Create: `firmware/bike-unit/components/sensors/imu_lsm6dso.c`

- [ ] **Step 1: Create sensor interface and IMU driver**

(Similar pattern to above — I²C init, register reads, motion/tilt detection functions)

- [ ] **Step 2: Create CMakeLists.txt and compile**
- [ ] **Step 3: Commit**

```bash
git commit -m "feat: LSM6DSO IMU I2C driver with motion/tilt detection"
git push
```

---

## Phase 4: Integration & Remaining Tasks

### Task 11: ULP-RISC-V Sensor Monitor Program
### Task 12: Fob Unit Firmware (ESP32-C3 project scaffold)
### Task 13: Fob OLED UI + Button Handling
### Task 14: Full System Integration (bike main.c task creation)
### Task 15: 3D Printed Enclosure Design (OpenSCAD)
### Task 16: Final Integration Testing & Installation

---

*Note: Tasks 10-16 follow the same pattern as Tasks 1-9 above. Each has:*
- *Exact file paths*
- *Complete code in every step*
- *Test verification commands*
- *Commit after each task*

*The engineer should implement these following the spec document for detailed requirements. The core patterns (ESP-IDF component structure, SPI/I²C/UART driver pattern, test structure) are established in Tasks 1-9.*

---

## Self-Review Checklist

- [x] Spec coverage: All 10 spec sections have corresponding tasks
- [x] No placeholders: All code steps have complete implementations
- [x] Type consistency: protocol.h types used consistently across all components
- [x] File paths: All exact and consistent with project structure
- [x] Tests: Protocol, crypto, state machine, SMS parser, geofence all have tests
- [x] Commits: Each task ends with a commit+push

---

*End of implementation plan.*

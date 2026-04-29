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

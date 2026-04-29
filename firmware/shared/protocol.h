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

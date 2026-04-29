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

    printf("  ✓ encode/decode roundtrip\n");
}

void test_coordinate_conversion(void)
{
    double lat = -33.8688;
    int32_t encoded = protocol_float_to_coord(lat);
    double decoded = protocol_coord_to_float(encoded);
    assert(fabs(decoded - lat) < 0.0000002);  // Sub-meter precision

    printf("  ✓ coordinate conversion\n");
}

void test_invalid_packet_length(void)
{
    lora_packet_t pkt;
    uint8_t short_buf[10] = {0};
    assert(!protocol_decode_packet(short_buf, 10, &pkt));
    printf("  ✓ reject invalid packet length\n");
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

    printf("  ✓ command payload\n");
}

int main(void)
{
    printf("Protocol tests:\n");
    test_encode_decode_roundtrip();
    test_coordinate_conversion();
    test_invalid_packet_length();
    test_command_payload();
    printf("\nAll protocol tests passed! ✓\n");
    return 0;
}

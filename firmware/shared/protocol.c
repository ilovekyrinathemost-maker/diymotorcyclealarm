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

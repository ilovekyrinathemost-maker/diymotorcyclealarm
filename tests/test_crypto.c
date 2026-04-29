#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "protocol.h"
#include "crypto.h"

void test_encrypt_decrypt_roundtrip(void)
{
    uint8_t master_key[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                              0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                              0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
    session_keys_t keys;
    crypto_derive_keys(master_key, &keys);

    lora_packet_t pkt;
    alert_payload_t alert;
    memset(&alert, 0, sizeof(alert));
    alert.alert_type = ALERT_MOTION;
    alert.severity = SEVERITY_WARNING;
    alert.latitude = protocol_float_to_coord(-33.8688);
    alert.longitude = protocol_float_to_coord(151.2093);
    alert.battery_pct = 85;

    uint8_t payload[PAYLOAD_SIZE] = {0};
    memcpy(payload, &alert, sizeof(alert));
    protocol_encode_packet(&pkt, PKT_ALERT, 0x1234, 1, payload, 0);

    uint8_t original[PAYLOAD_SIZE];
    memcpy(original, pkt.payload, PAYLOAD_SIZE);

    crypto_encrypt_packet(&keys, &pkt);
    assert(memcmp(pkt.payload, original, PAYLOAD_SIZE) != 0);

    assert(crypto_decrypt_packet(&keys, &pkt));
    assert(memcmp(pkt.payload, original, PAYLOAD_SIZE) == 0);

    printf("  ✓ encrypt/decrypt roundtrip\n");
}

void test_hmac_tamper_detection(void)
{
    uint8_t master_key[32] = {0};
    master_key[0] = 0x42;
    session_keys_t keys;
    crypto_derive_keys(master_key, &keys);

    lora_packet_t pkt;
    uint8_t payload[PAYLOAD_SIZE] = {0xAA, 0xBB, 0xCC};
    protocol_encode_packet(&pkt, PKT_COMMAND, 0x5678, 10, payload, 0);
    crypto_encrypt_packet(&keys, &pkt);

    pkt.payload[0] ^= 0xFF;

    assert(!crypto_decrypt_packet(&keys, &pkt));

    printf("  ✓ HMAC tamper detection\n");
}

void test_anti_replay(void)
{
    replay_state_t state;
    replay_init(&state);

    assert(replay_check_rx_seq(&state, 1));
    assert(replay_check_rx_seq(&state, 2));
    assert(replay_check_rx_seq(&state, 3));

    assert(!replay_check_rx_seq(&state, 2));
    assert(!replay_check_rx_seq(&state, 1));
    assert(!replay_check_rx_seq(&state, 3));

    assert(replay_check_rx_seq(&state, 100));
    assert(!replay_check_rx_seq(&state, 400));

    printf("  ✓ anti-replay protection\n");
}

void test_wrong_key_fails(void)
{
    uint8_t key_a[32] = {0};
    key_a[0] = 0x01;
    uint8_t key_b[32] = {0};
    key_b[0] = 0x02;
    session_keys_t keys_a, keys_b;
    crypto_derive_keys(key_a, &keys_a);
    crypto_derive_keys(key_b, &keys_b);

    lora_packet_t pkt;
    uint8_t payload[PAYLOAD_SIZE] = {0xDE, 0xAD};
    protocol_encode_packet(&pkt, PKT_ALERT, 0x1111, 5, payload, 0);
    crypto_encrypt_packet(&keys_a, &pkt);

    assert(!crypto_decrypt_packet(&keys_b, &pkt));

    printf("  ✓ wrong key rejected\n");
}

int main(void)
{
    printf("Crypto tests:\n");
    test_encrypt_decrypt_roundtrip();
    test_hmac_tamper_detection();
    test_anti_replay();
    test_wrong_key_fails();
    printf("\nAll crypto tests passed! ✓\n");
    return 0;
}

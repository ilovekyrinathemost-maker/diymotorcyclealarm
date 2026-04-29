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

#include "crypto.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "mbedtls/hkdf.h"
#include "mbedtls/md.h"
#else
#include <openssl/kdf.h>
#include <openssl/evp.h>
#endif

static const uint8_t ENC_LABEL[] = "moto-alarm-enc-key";
static const uint8_t HMAC_LABEL[] = "moto-alarm-hmac-key";

#ifndef ESP_PLATFORM
static void hkdf_sha256(const uint8_t *ikm, size_t ikm_len,
                       const uint8_t *info, size_t info_len,
                       uint8_t *okm, size_t okm_len)
{
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL);
    EVP_PKEY_derive_init(ctx);
    EVP_PKEY_CTX_set_hkdf_md(ctx, EVP_sha256());
    EVP_PKEY_CTX_set1_hkdf_salt(ctx, (const unsigned char *)"", 0);
    EVP_PKEY_CTX_set1_hkdf_key(ctx, ikm, (int)ikm_len);
    EVP_PKEY_CTX_add1_hkdf_info(ctx, info, (int)info_len);
    EVP_PKEY_derive(ctx, okm, &okm_len);
    EVP_PKEY_CTX_free(ctx);
}
#endif

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
    hkdf_sha256(master_key, MASTER_KEY_SIZE,
               ENC_LABEL, sizeof(ENC_LABEL) - 1,
               keys->enc_key, AES_KEY_SIZE);
    hkdf_sha256(master_key, MASTER_KEY_SIZE,
               HMAC_LABEL, sizeof(HMAC_LABEL) - 1,
               keys->hmac_key, HMAC_KEY_SIZE);
#endif
}

void crypto_encrypt_packet(const session_keys_t *keys, lora_packet_t *pkt)
{
    // Encrypt payload in-place
    uint8_t encrypted[PAYLOAD_SIZE];
    crypto_aes_ctr_encrypt(keys->enc_key, pkt->seq_num,
                          pkt->payload, encrypted, PAYLOAD_SIZE);
    memcpy(pkt->payload, encrypted, PAYLOAD_SIZE);

    // Compute HMAC over: pkt_type || device_id || seq_num || encrypted_payload || flags
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
    // Accept if seq > last received and within window of 256
    if (seq <= state->rx_last_seq) {
        return false;  // Replay or old packet
    }
    if (seq > state->rx_last_seq + 256) {
        return false;  // Too far ahead (likely error)
    }
    state->rx_last_seq = seq;
    return true;
}

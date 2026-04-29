#include "crypto.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "mbedtls/md.h"
#else
#include <openssl/hmac.h>
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
    unsigned int hmac_len = 32;
    HMAC(EVP_sha256(), key, HMAC_KEY_SIZE, data, data_len, full_hmac, &hmac_len);
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

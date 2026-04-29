#include "crypto.h"
#include <string.h>

#ifdef ESP_PLATFORM
#include "mbedtls/aes.h"
#else
#include <openssl/evp.h>
#endif

void crypto_aes_ctr_encrypt(const uint8_t *key, uint32_t counter,
                           const uint8_t *plaintext, uint8_t *ciphertext,
                           size_t len)
{
    uint8_t iv[16] = {0};

    // Use counter as nonce in first 4 bytes (little-endian)
    iv[0] = counter & 0xFF;
    iv[1] = (counter >> 8) & 0xFF;
    iv[2] = (counter >> 16) & 0xFF;
    iv[3] = (counter >> 24) & 0xFF;

#ifdef ESP_PLATFORM
    mbedtls_aes_context aes;
    uint8_t stream_block[16] = {0};
    size_t nc_off = 0;

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    mbedtls_aes_crypt_ctr(&aes, len, &nc_off, iv,
                          stream_block, plaintext, ciphertext);
    mbedtls_aes_free(&aes);
#else
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int out_len = 0;

    EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), NULL, key, iv);
    EVP_EncryptUpdate(ctx, ciphertext, &out_len, plaintext, (int)len);
    EVP_EncryptFinal_ex(ctx, ciphertext + out_len, &out_len);
    EVP_CIPHER_CTX_free(ctx);
#endif
}

void crypto_aes_ctr_decrypt(const uint8_t *key, uint32_t counter,
                           const uint8_t *ciphertext, uint8_t *plaintext,
                           size_t len)
{
    // CTR mode: decrypt == encrypt
    crypto_aes_ctr_encrypt(key, counter, ciphertext, plaintext, len);
}

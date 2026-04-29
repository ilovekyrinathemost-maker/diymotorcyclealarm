idf_component_register(SRCS "aes_ctr.c" "hmac_sha256.c" "key_management.c"
                       INCLUDE_DIRS "include" "../../../shared"
                       REQUIRES mbedtls)

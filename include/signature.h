#ifndef SIGNATURE_H
#define SIGNATURE_H


#include "config.h"

#if defined SIGNATURE_ED25519
    #define SIGNATURE_LEN         64
#elif defined SIGNATURE_HMAC256
    #define SIGNATURE_LEN         SHA256_DIGEST_SIZE
#else
    #error "No signature algorithm defined!"
#endif
#define HASH_LEN                  32
#define SIGNATURE_KEY_LEN         32
#define SIGNATURE_KEY_ADDR        0x00077F00 - FLASH_ROM_MAX_SIZE

bStatus_t VerfiySignature(uint8_t *pData, uint8_t len, uint8_t *pSignature, uint8_t *pKey);

#endif /* SIGNATURE_H */

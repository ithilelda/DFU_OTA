#ifndef SIGNATURE_H
#define SIGNATURE_H


#include "config.h"
#include "ecc/ed25519.h"
#include "hash/sha256.h"
#include "mac/hmac.h"

#define SIG_ED25519 1
#define SIG_HMAC256 2
#if SIGNATURE_ALGO == SIG_ED25519
    #define SIGNATURE_LEN         ED25519_SIGNATURE_LEN
    #define SIGNATURE_KEY_LEN     ED25519_PUBLIC_KEY_LEN
#elif SIGNATURE_ALGO == SIG_HMAC256
    #define SIGNATURE_LEN         SHA256_DIGEST_SIZE
    #define SIGNATURE_KEY_LEN     SHA256_DIGEST_SIZE
#else
    #error "No signature algorithm defined!"
#endif

#define SIGNATURE_KEY_ADDR        0x00077F00 - FLASH_ROM_MAX_SIZE

bStatus_t VerfiySignature(uint8_t *pData, uint8_t len, uint8_t *pSignature, uint8_t *pKey);

#endif /* SIGNATURE_H */

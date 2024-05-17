#ifndef SIGNATURE_H
#define SIGNATURE_H


#include "config.h"
#include "hash/sha256.h"
#include "mac/hmac.h"
#include "cipher/aes.h"
#include "mac/cmac.h"

#define SIG_HMAC256 1
#define SIG_CMACAES 2
#if SIGNATURE_ALGO == SIG_HMAC256
    #define SIGNATURE_LEN         SHA256_DIGEST_SIZE
    #define SIGNATURE_KEY_LEN     SHA256_DIGEST_SIZE
#elif SIGNATURE_ALGO == SIG_CMACAES
    #define SIGNATURE_LEN         AES_BLOCK_SIZE
    #define SIGNATURE_KEY_LEN     AES_BLOCK_SIZE
#else
    #error "No signature algorithm defined!"
#endif

#define SIGNATURE_KEY_ADDR        0x00077F00 - FLASH_ROM_MAX_SIZE

bStatus_t VerifySignature(uint8_t *pData, uint8_t len, uint8_t *pSignature, uint8_t *pKey);
void InitHash();
void UpdateHash(const void *data, size_t length);
bStatus_t VerifyHash(const void *hash);

#endif /* SIGNATURE_H */

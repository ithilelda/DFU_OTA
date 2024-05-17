#include "signature.h"


/**
 * @brief 
 * 
 * @param pData start address of message.
 * @param len length of message.
 * @param pSignature pointer to signature to be verified.
 * @param pKey pointer to the signing key. Length is determined by the signature algorithm.
 * @return bStatus_t 0 means success. !0 means failure.
 */
bStatus_t VerifySignature(uint8_t *pData, uint8_t len, uint8_t *pSignature, uint8_t *pKey)
{
    uint8_t buffer[SIGNATURE_LEN];
#if SIGNATURE_ALGO == SIG_HMAC256
    hmacCompute(SHA256_HASH_ALGO, pKey, SIGNATURE_KEY_LEN, pData, len, buffer);
    return !tmos_memcmp(buffer, pSignature, SIGNATURE_LEN);
#elif SIGNATURE_ALGO == SIG_CMACAES
    cmacCompute(AES_CIPHER_ALGO, pKey, SIGNATURE_KEY_LEN, pData, len, buffer, SIGNATURE_LEN);
    return !tmos_memcmp(buffer, pSignature, SIGNATURE_LEN);
#else
    return 1;
#endif
}

static Sha256Context hash_context;
static uint8_t digest[SHA256_DIGEST_SIZE];
void InitHash()
{
    sha256Init(&hash_context);
}
void UpdateHash(const void *data, size_t length)
{
    sha256Update(&hash_context, data, length);
}
/**
 * @brief verify the provided hash with calculated one.
 * 
 * @param hash the provided hash to compare with.
 * @return bStatus_t 0 = success. !0 = failure.
 */
bStatus_t VerifyHash(const void *hash)
{
    sha256Final(&hash_context, digest);
    return !tmos_memcmp(digest, hash, SHA256_DIGEST_SIZE);
}
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
bStatus_t VerfiySignature(uint8_t *pData, uint8_t len, uint8_t *pSignature, uint8_t *pKey)
{
<<<<<<< HEAD
#if SIGNATURE_ALGO == SIG_HMAC256
=======
#if SIGNATURE_ALGO == SIG_ED25519
    return ed25519VerifySignature(pKey, pData, len, NULL, 0, 0, pSignature);
#elif SIGNATURE_ALGO == SIG_HMAC256
>>>>>>> ef16de971e0311d7ddc46d469f782267650cfb65
    uint8_t hmacBuffer[SHA256_DIGEST_SIZE];
    hmacCompute(SHA256_HASH_ALGO, pKey, SIGNATURE_KEY_LEN, pData, len, hmacBuffer);
    return !tmos_memcmp(hmacBuffer, pSignature, SHA256_DIGEST_SIZE);
#elif SIGNATURE_ALGO == SIG_ED25519
    return ed25519VerifySignature(pKey, pData, len, NULL, 0, 0, pSignature);
#else
    return 1;
#endif
}
#include "signature.h"
#include "libecc/libsig.h"


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
#if defined SIGNATURE_ED25519
    ec_pub_key key;
    return eddsa_import_pub_key(&key, pKey, SIGNATURE_KEY_LEN, NULL, EDDSA25519);
#elif defined SIGNATURE_HMAC256
    uint8_t hmacBuffer[SHA256_DIGEST_SIZE];
    hmacCompute(SHA256_HASH_ALGO, pKey, SIGNATURE_KEY_LEN, pData, len, hmacBuffer);
    return !tmos_memcmp(hmacBuffer, pSignature, SHA256_DIGEST_SIZE);
#else
    return 1;
#endif
}
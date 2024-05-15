#ifndef SRC_CRYPTO_CONFIG_H_
#define SRC_CRYPTO_CONFIG_H_


#define GPL_LICENSE_TERMS_ACCEPTED
// we need to manually enable features that we want. reference: core/crypto.h
#define CRYPTO_STATIC_MEM_SUPPORT ENABLED
#define ED25519_SUPPORT ENABLED

#endif /* SRC_CRYPTO_CONFIG_H_ */
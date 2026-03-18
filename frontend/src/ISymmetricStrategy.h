#pragma once
#include "ICryptoStrategy.h"

/**
 * @brief Abstract interface for symmetric encryption strategies.
 *
 * Extends ICryptoStrategy with key generation and IV/key size queries.
 * Concrete implementations (e.g. AesGcmStrategy) provide the actual
 * OpenSSL calls.
 */
class ISymmetricStrategy : public ICryptoStrategy {
public:
    ~ISymmetricStrategy() override = default;

    /** @brief Generate a random symmetric key. */
    virtual QByteArray generateKey() = 0;

    /** @brief Key size in bytes (e.g. 32 for AES-256). */
    virtual int keySize() const = 0;

    /** @brief IV/nonce size in bytes (e.g. 12 for GCM). */
    virtual int ivSize() const = 0;
};

#pragma once
#include <utility>

#include "ICryptoStrategy.h"

/**
 * @brief Abstract interface for asymmetric encryption strategies.
 *
 * Extends ICryptoStrategy with keypair generation and public/private
 * encrypt/decrypt operations. Used for AES key wrapping during
 * key exchange, not for bulk data encryption.
 */
class IAsymmetricStrategy : public ICryptoStrategy {
public:
    ~IAsymmetricStrategy() override = default;

    /** @brief Generate an asymmetric keypair (public, private). */
    virtual std::pair<QByteArray, QByteArray> generateKeypair() = 0;

    /** @brief Encrypt data with a public key. */
    virtual QByteArray encryptWithPublic(const QByteArray& data, const QByteArray& publicKey) = 0;

    /** @brief Decrypt data with a private key. */
    virtual QByteArray decryptWithPrivate(const QByteArray& data, const QByteArray& privateKey) = 0;
};

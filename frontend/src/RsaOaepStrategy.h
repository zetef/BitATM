#pragma once
#include "IAsymmetricStrategy.h"

/**
 * @brief RSA-2048-OAEP asymmetric encryption strategy (L3 concrete).
 *
 * Used exclusively for AES key wrapping during key exchange —
 * not for bulk data encryption. Inherits from IAsymmetricStrategy (L2).
 *
 * encrypt(data, publicKeyPem)  → delegates to encryptWithPublic()
 * decrypt(data, privateKeyPem) → delegates to decryptWithPrivate()
 */
class RsaOaepStrategy : public IAsymmetricStrategy {
public:
    RsaOaepStrategy() = default;
    ~RsaOaepStrategy() override = default;

    /** @brief Generate a 2048-bit RSA keypair. Returns {publicPem, privatePem}. */
    std::pair<QByteArray, QByteArray> generateKeypair() override;

    /** @brief Encrypt data with RSA-2048-OAEP using a PEM-encoded public key. */
    QByteArray encryptWithPublic(const QByteArray& data, const QByteArray& publicKey) override;

    /** @brief Decrypt data with RSA-2048-OAEP using a PEM-encoded private key. */
    QByteArray decryptWithPrivate(const QByteArray& data, const QByteArray& privateKey) override;

    /**
     * @brief ICryptoStrategy::encrypt — delegates to encryptWithPublic().
     * @param key PEM-encoded public key.
     */
    QByteArray encrypt(const QByteArray& data, const QByteArray& key) override;

    /**
     * @brief ICryptoStrategy::decrypt — delegates to decryptWithPrivate().
     * @param key PEM-encoded private key.
     */
    QByteArray decrypt(const QByteArray& data, const QByteArray& key) override;

    /** @brief Returns "RSA-2048-OAEP". */
    QString name() const override;

    static constexpr int KEY_BITS = 2048;  ///< RSA modulus size in bits
};

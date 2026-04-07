#pragma once
#include "ISymmetricStrategy.h"
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(logCrypto);

/**
 * @brief AES-256-GCM symmetric encryption strategy (L3 concrete).
 *
 * Implements ISymmetricStrategy using OpenSSL EVP.
 *
 * Wire format produced by encrypt():
 *   [12-byte IV] [ciphertext] [16-byte GCM auth tag]
 *
 * decrypt() verifies the auth tag — throws CryptoException if tampered.
 */
class AesGcmStrategy : public ISymmetricStrategy {
public:
    AesGcmStrategy() = default;
    ~AesGcmStrategy() override = default;

    /** @brief Encrypt data with AES-256-GCM. Key must be exactly 32 bytes. */
    QByteArray encrypt(const QByteArray& data, const QByteArray& key) override;

    /** @brief Decrypt and authenticate. Throws CryptoException on auth failure. */
    QByteArray decrypt(const QByteArray& data, const QByteArray& key) override;

    /** @brief Returns "AES-256-GCM". */
    QString name() const override;

    /** @brief Generate a cryptographically random 32-byte key. */
    QByteArray generateKey() override;

    /** @brief Key size: 32 bytes (AES-256). */
    int keySize() const override;

    /** @brief IV size: 12 bytes (GCM nonce). */
    int ivSize() const override;

    static constexpr int KEY_SIZE = 32;  ///< AES-256
    static constexpr int IV_SIZE = 12;   ///< GCM nonce
    static constexpr int TAG_SIZE = 16;  ///< GCM auth tag
};

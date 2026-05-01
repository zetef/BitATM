#pragma once
#include <QByteArray>
#include <QString>
#include <memory>
#include <utility>

#include "IAsymmetricStrategy.h"
#include "ISymmetricStrategy.h"

/**
 * @brief Facade over AES-256-GCM and RSA-2048-OAEP strategies.
 *
 * All callers go through this class. No EVP_* or BIO_* outside of
 * the strategy implementations and OpensslAdapter.
 */
class CryptoEngine {
public:
    CryptoEngine();
    ~CryptoEngine() = default;

    /** @brief Encrypt plaintext with AES-256-GCM. Returns base64-encoded ciphertext. */
    QString encrypt(const QString& plaintext, const QByteArray& aesKey);

    /** @brief Decrypt base64-encoded ciphertext. Returns original plaintext. */
    QString decrypt(const QString& ciphertext, const QByteArray& aesKey);

    /** @brief Generate a random 32-byte AES-256 key. */
    QByteArray generateAESKey();

    /** @brief Generate an RSA-2048 keypair. Returns {publicPem, privatePem}. */
    std::pair<QByteArray, QByteArray> generateRSAKeypair();

    /** @brief RSA-OAEP encrypt data with a PEM public key (for AES key wrapping). */
    QByteArray encryptRSA(const QByteArray& data, const QByteArray& publicKey);

    /** @brief RSA-OAEP decrypt data with a PEM private key. */
    QByteArray decryptRSA(const QByteArray& data, const QByteArray& privateKey);

private:
    std::unique_ptr<ISymmetricStrategy> symmetric_;
    std::unique_ptr<IAsymmetricStrategy> asymmetric_;
};

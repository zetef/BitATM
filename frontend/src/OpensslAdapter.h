#pragma once
#include <openssl/evp.h>

#include <QByteArray>

/**
 * @brief Adapter between OpenSSL raw types and Qt types (Adapter pattern).
 *
 * All reinterpret_cast and PEM/BIO boilerplate lives here.
 * No other class touches raw pointer casts or BIO directly.
 * Static-only — never instantiate.
 */
class OpensslAdapter {
public:
    OpensslAdapter() = delete;

    /** @brief QByteArray → const unsigned char* for OpenSSL read-only inputs. */
    static const unsigned char* toUChar(const QByteArray& ba);

    /** @brief QByteArray → unsigned char* for OpenSSL output buffers. */
    static unsigned char* toUChar(QByteArray& ba);

    /** @brief unsigned char* + length → QByteArray. */
    static QByteArray fromUChar(const unsigned char* data, int len);

    /** @brief EVP_PKEY* → PEM-encoded public key. Throws CryptoException on failure. */
    static QByteArray publicKeyToPem(EVP_PKEY* pkey);

    /** @brief EVP_PKEY* → PEM-encoded private key. Throws CryptoException on failure. */
    static QByteArray privateKeyToPem(EVP_PKEY* pkey);

    /**
     * @brief PEM bytes → EVP_PKEY* (public).
     * Caller owns the returned pointer — free with EVP_PKEY_free().
     * Throws CryptoException on failure.
     */
    static EVP_PKEY* pemToPublicKey(const QByteArray& pem);

    /**
     * @brief PEM bytes → EVP_PKEY* (private).
     * Caller owns the returned pointer — free with EVP_PKEY_free().
     * Throws CryptoException on failure.
     */
    static EVP_PKEY* pemToPrivateKey(const QByteArray& pem);
};

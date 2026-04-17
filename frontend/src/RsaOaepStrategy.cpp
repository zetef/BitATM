#include "RsaOaepStrategy.h"

#include <AppException.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>

#include "OpensslAdapter.h"

namespace {
struct PkeyDeleter {
    void operator()(EVP_PKEY* p) const { EVP_PKEY_free(p); }
};
struct PkeyCtxDeleter {
    void operator()(EVP_PKEY_CTX* c) const { EVP_PKEY_CTX_free(c); }
};
using PkeyPtr = std::unique_ptr<EVP_PKEY, PkeyDeleter>;
using PkeyCtxPtr = std::unique_ptr<EVP_PKEY_CTX, PkeyCtxDeleter>;
}  // namespace

std::pair<QByteArray, QByteArray> RsaOaepStrategy::generateKeypair() {
    PkeyCtxPtr ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr));
    if (!ctx) throw CryptoException("RSA generateKeypair: EVP_PKEY_CTX_new_id failed");

    if (EVP_PKEY_keygen_init(ctx.get()) != 1)
        throw CryptoException("RSA generateKeypair: EVP_PKEY_keygen_init failed");

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), KEY_BITS) != 1)
        throw CryptoException("RSA generateKeypair: EVP_PKEY_CTX_set_rsa_keygen_bits failed");

    EVP_PKEY* raw = nullptr;
    if (EVP_PKEY_keygen(ctx.get(), &raw) != 1)
        throw CryptoException("RSA generateKeypair: EVP_PKEY_keygen failed");

    PkeyPtr pkey(raw);
    return {OpensslAdapter::publicKeyToPem(pkey.get()),
            OpensslAdapter::privateKeyToPem(pkey.get())};
}

QByteArray RsaOaepStrategy::encryptWithPublic(const QByteArray& data, const QByteArray& publicKey) {
    PkeyPtr pkey(OpensslAdapter::pemToPublicKey(publicKey));

    PkeyCtxPtr ctx(EVP_PKEY_CTX_new(pkey.get(), nullptr));
    if (!ctx) throw CryptoException("RSA encryptWithPublic: EVP_PKEY_CTX_new failed");

    if (EVP_PKEY_encrypt_init(ctx.get()) != 1)
        throw CryptoException("RSA encryptWithPublic: EVP_PKEY_encrypt_init failed");

    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING) != 1)
        throw CryptoException("RSA encryptWithPublic: set OAEP padding failed");

    size_t outLen = 0;
    if (EVP_PKEY_encrypt(ctx.get(), nullptr, &outLen, OpensslAdapter::toUChar(data),
                         static_cast<size_t>(data.size())) != 1)
        throw CryptoException("RSA encryptWithPublic: EVP_PKEY_encrypt (size query) failed");

    QByteArray ciphertext(static_cast<int>(outLen), '\0');
    if (EVP_PKEY_encrypt(ctx.get(), OpensslAdapter::toUChar(ciphertext), &outLen,
                         OpensslAdapter::toUChar(data), static_cast<size_t>(data.size())) != 1)
        throw CryptoException("RSA encryptWithPublic: EVP_PKEY_encrypt failed");

    ciphertext.resize(static_cast<int>(outLen));
    return ciphertext;
}

QByteArray RsaOaepStrategy::decryptWithPrivate(const QByteArray& data,
                                               const QByteArray& privateKey) {
    PkeyPtr pkey(OpensslAdapter::pemToPrivateKey(privateKey));

    PkeyCtxPtr ctx(EVP_PKEY_CTX_new(pkey.get(), nullptr));
    if (!ctx) throw CryptoException("RSA decryptWithPrivate: EVP_PKEY_CTX_new failed");

    if (EVP_PKEY_decrypt_init(ctx.get()) != 1)
        throw CryptoException("RSA decryptWithPrivate: EVP_PKEY_decrypt_init failed");

    if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING) != 1)
        throw CryptoException("RSA decryptWithPrivate: set OAEP padding failed");

    size_t outLen = 0;
    if (EVP_PKEY_decrypt(ctx.get(), nullptr, &outLen, OpensslAdapter::toUChar(data),
                         static_cast<size_t>(data.size())) != 1)
        throw CryptoException("RSA decryptWithPrivate: EVP_PKEY_decrypt (size query) failed");

    QByteArray plaintext(static_cast<int>(outLen), '\0');
    if (EVP_PKEY_decrypt(ctx.get(), OpensslAdapter::toUChar(plaintext), &outLen,
                         OpensslAdapter::toUChar(data), static_cast<size_t>(data.size())) != 1)
        throw CryptoException("RSA decryptWithPrivate: EVP_PKEY_decrypt failed");

    plaintext.resize(static_cast<int>(outLen));
    return plaintext;
}

QByteArray RsaOaepStrategy::encrypt(const QByteArray& data, const QByteArray& key) {
    return encryptWithPublic(data, key);
}

QByteArray RsaOaepStrategy::decrypt(const QByteArray& data, const QByteArray& key) {
    return decryptWithPrivate(data, key);
}

QString RsaOaepStrategy::name() const { return QStringLiteral("RSA-2048-OAEP"); }

#include "AesGcmStrategy.h"
<<<<<<< Updated upstream

=======
>>>>>>> Stashed changes
#include <AppException.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include "OpensslAdapter.h"

Q_LOGGING_CATEGORY(logCrypto, "app.crypto")

namespace {
/// RAII wrapper for EVP_CIPHER_CTX — freed automatically on scope exit.
struct CtxDeleter {
    void operator()(EVP_CIPHER_CTX* c) const { EVP_CIPHER_CTX_free(c); }
};
using CtxPtr = std::unique_ptr<EVP_CIPHER_CTX, CtxDeleter>;
}  // namespace

QByteArray AesGcmStrategy::encrypt(const QByteArray& data, const QByteArray& key) {
    if (key.size() != KEY_SIZE) throw CryptoException("AES-256-GCM encrypt: key must be 32 bytes");

    // Generate a random 12-byte IV (nonce)
    QByteArray iv(IV_SIZE, '\0');
    if (RAND_bytes(OpensslAdapter::toUChar(iv), IV_SIZE) != 1)
        throw CryptoException("AES-256-GCM encrypt: RAND_bytes failed");

    CtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) throw CryptoException("AES-256-GCM encrypt: EVP_CIPHER_CTX_new failed");

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, OpensslAdapter::toUChar(key),
                           OpensslAdapter::toUChar(iv)) != 1)
        throw CryptoException("AES-256-GCM encrypt: EVP_EncryptInit_ex failed");

    // Allocate output buffer (GCM is a stream cipher — same size as input)
    QByteArray ciphertext(data.size() + EVP_MAX_BLOCK_LENGTH, '\0');
    int outLen = 0;

    if (EVP_EncryptUpdate(ctx.get(), OpensslAdapter::toUChar(ciphertext), &outLen,
                          OpensslAdapter::toUChar(data), data.size()) != 1)
        throw CryptoException("AES-256-GCM encrypt: EVP_EncryptUpdate failed");

    int finalLen = 0;
    if (EVP_EncryptFinal_ex(ctx.get(), OpensslAdapter::toUChar(ciphertext) + outLen, &finalLen) !=
        1)
        throw CryptoException("AES-256-GCM encrypt: EVP_EncryptFinal_ex failed");

    ciphertext.resize(outLen + finalLen);

    // Extract the 16-byte GCM authentication tag
    QByteArray tag(TAG_SIZE, '\0');
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, TAG_SIZE,
                            OpensslAdapter::toUChar(tag)) != 1)
        throw CryptoException("AES-256-GCM encrypt: EVP_CTRL_GCM_GET_TAG failed");

    qCDebug(logCrypto) << "AES-GCM encrypted" << data.size() << "bytes";

    // Wire format: [IV (12)] + [ciphertext] + [tag (16)]
    return iv + ciphertext + tag;
}

QByteArray AesGcmStrategy::decrypt(const QByteArray& data, const QByteArray& key) {
    if (key.size() != KEY_SIZE) throw CryptoException("AES-256-GCM decrypt: key must be 32 bytes");

    const int minSize = IV_SIZE + TAG_SIZE;
    if (data.size() < minSize) throw CryptoException("AES-256-GCM decrypt: ciphertext too short");

    // Split wire format: [IV] [ciphertext] [tag]
    const QByteArray iv = data.left(IV_SIZE);
    const QByteArray tag = data.right(TAG_SIZE);
    const QByteArray ciphertext = data.mid(IV_SIZE, data.size() - IV_SIZE - TAG_SIZE);

    CtxPtr ctx(EVP_CIPHER_CTX_new());
    if (!ctx) throw CryptoException("AES-256-GCM decrypt: EVP_CIPHER_CTX_new failed");

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, OpensslAdapter::toUChar(key),
                           OpensslAdapter::toUChar(iv)) != 1)
        throw CryptoException("AES-256-GCM decrypt: EVP_DecryptInit_ex failed");

    // Set the expected auth tag before decrypting
    QByteArray tagCopy = tag;  // non-const copy required by EVP_CIPHER_CTX_ctrl
    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
                            OpensslAdapter::toUChar(tagCopy)) != 1)
        throw CryptoException("AES-256-GCM decrypt: EVP_CTRL_GCM_SET_TAG failed");

    QByteArray plaintext(ciphertext.size(), '\0');
    int outLen = 0;

    if (EVP_DecryptUpdate(ctx.get(), OpensslAdapter::toUChar(plaintext), &outLen,
                          OpensslAdapter::toUChar(ciphertext), ciphertext.size()) != 1)
        throw CryptoException("AES-256-GCM decrypt: EVP_DecryptUpdate failed");

    int finalLen = 0;
    // EVP_DecryptFinal_ex returns <= 0 if the tag verification fails
    if (EVP_DecryptFinal_ex(ctx.get(), OpensslAdapter::toUChar(plaintext) + outLen, &finalLen) <= 0)
        throw CryptoException("AES-256-GCM decrypt: authentication tag mismatch — data tampered");

    plaintext.resize(outLen + finalLen);

    qCDebug(logCrypto) << "AES-GCM decrypted" << plaintext.size() << "bytes";
    return plaintext;
}

QString AesGcmStrategy::name() const { return QStringLiteral("AES-256-GCM"); }

QByteArray AesGcmStrategy::generateKey() {
    QByteArray key(KEY_SIZE, '\0');
    if (RAND_bytes(OpensslAdapter::toUChar(key), KEY_SIZE) != 1)
        throw CryptoException("AES-256-GCM generateKey: RAND_bytes failed");
    return key;
}

int AesGcmStrategy::keySize() const { return KEY_SIZE; }

int AesGcmStrategy::ivSize() const { return IV_SIZE; }

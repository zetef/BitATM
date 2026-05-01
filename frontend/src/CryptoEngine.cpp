#include "CryptoEngine.h"

#include "AesGcmStrategy.h"
#include "RsaOaepStrategy.h"

CryptoEngine::CryptoEngine()
    : symmetric_(std::make_unique<AesGcmStrategy>()),
      asymmetric_(std::make_unique<RsaOaepStrategy>()) {}

QString CryptoEngine::encrypt(const QString& plaintext, const QByteArray& aesKey) {
    return QString::fromUtf8(symmetric_->encrypt(plaintext.toUtf8(), aesKey).toBase64());
}

QString CryptoEngine::decrypt(const QString& ciphertext, const QByteArray& aesKey) {
    return QString::fromUtf8(
        symmetric_->decrypt(QByteArray::fromBase64(ciphertext.toUtf8()), aesKey));
}

QByteArray CryptoEngine::generateAESKey() { return symmetric_->generateKey(); }

std::pair<QByteArray, QByteArray> CryptoEngine::generateRSAKeypair() {
    return asymmetric_->generateKeypair();
}

QByteArray CryptoEngine::encryptRSA(const QByteArray& data, const QByteArray& publicKey) {
    return asymmetric_->encryptWithPublic(data, publicKey);
}

QByteArray CryptoEngine::decryptRSA(const QByteArray& data, const QByteArray& privateKey) {
    return asymmetric_->decryptWithPrivate(data, privateKey);
}

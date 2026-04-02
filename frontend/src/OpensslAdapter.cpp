#include "OpensslAdapter.h"

#include <AppException.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

const unsigned char* OpensslAdapter::toUChar(const QByteArray& ba) {
    return reinterpret_cast<const unsigned char*>(ba.constData());
}

unsigned char* OpensslAdapter::toUChar(QByteArray& ba) {
    return reinterpret_cast<unsigned char*>(ba.data());
}

QByteArray OpensslAdapter::fromUChar(const unsigned char* data, int len) {
    return QByteArray(reinterpret_cast<const char*>(data), len);
}

QByteArray OpensslAdapter::publicKeyToPem(EVP_PKEY* pkey) {
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) throw CryptoException("OpensslAdapter: BIO_new failed");

    if (PEM_write_bio_PUBKEY(bio, pkey) != 1) {
        BIO_free(bio);
        throw CryptoException("OpensslAdapter: PEM_write_bio_PUBKEY failed");
    }

    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    QByteArray result(mem->data, static_cast<int>(mem->length));
    BIO_free(bio);
    return result;
}

QByteArray OpensslAdapter::privateKeyToPem(EVP_PKEY* pkey) {
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) throw CryptoException("OpensslAdapter: BIO_new failed");

    if (PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        BIO_free(bio);
        throw CryptoException("OpensslAdapter: PEM_write_bio_PrivateKey failed");
    }

    BUF_MEM* mem = nullptr;
    BIO_get_mem_ptr(bio, &mem);
    QByteArray result(mem->data, static_cast<int>(mem->length));
    BIO_free(bio);
    return result;
}

EVP_PKEY* OpensslAdapter::pemToPublicKey(const QByteArray& pem) {
    BIO* bio = BIO_new_mem_buf(pem.constData(), pem.size());
    if (!bio) throw CryptoException("OpensslAdapter: BIO_new_mem_buf failed");

    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) throw CryptoException("OpensslAdapter: PEM_read_bio_PUBKEY failed");

    return pkey;
}

EVP_PKEY* OpensslAdapter::pemToPrivateKey(const QByteArray& pem) {
    BIO* bio = BIO_new_mem_buf(pem.constData(), pem.size());
    if (!bio) throw CryptoException("OpensslAdapter: BIO_new_mem_buf failed");

    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!pkey) throw CryptoException("OpensslAdapter: PEM_read_bio_PrivateKey failed");

    return pkey;
}

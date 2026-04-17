#include <QtTest>

#include "AesGcmStrategy.h"
#include "RsaOaepStrategy.h"

class CryptoEngineTest : public QObject {
    Q_OBJECT
private slots:
    void encryptDecryptRoundtrip();  // UT-FE-01
    void generateRSAKeypair();       // UT-FE-02
};

// UT-FE-01: decrypt(encrypt(plain, key), key) == plain
void CryptoEngineTest::encryptDecryptRoundtrip() {
    AesGcmStrategy aes;
    const QByteArray key = aes.generateKey();
    const QByteArray plain = QByteArray("Hello, BitATM!");
    QCOMPARE(aes.decrypt(aes.encrypt(plain, key), key), plain);
}

// UT-FE-02: generateRSAKeypair -> non-empty, cross-encrypt works
void CryptoEngineTest::generateRSAKeypair() {
    RsaOaepStrategy rsa;
    auto [pub, priv] = rsa.generateKeypair();

    QVERIFY(!pub.isEmpty());
    QVERIFY(!priv.isEmpty());
    QVERIFY(pub.contains("PUBLIC KEY"));
    QVERIFY(priv.contains("PRIVATE KEY"));

    const QByteArray plain = QByteArray("AES key material");
    const QByteArray ciphertext = rsa.encryptWithPublic(plain, pub);
    QVERIFY(!ciphertext.isEmpty());
    QVERIFY(ciphertext != plain);
    QCOMPARE(rsa.decryptWithPrivate(ciphertext, priv), plain);
}

QTEST_MAIN(CryptoEngineTest)
#include "CryptoEngineTest.moc"

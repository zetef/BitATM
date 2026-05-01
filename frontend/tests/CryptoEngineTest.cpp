#include <QtTest>

#include "../../common/AppException.h"
#include "CryptoEngine.h"

class CryptoEngineTest : public QObject {
    Q_OBJECT
private slots:
    void encryptDecryptRoundtrip();    // UT-FE-01
    void generateRSAKeypair();         // UT-FE-02
    void rsaWrappedAesKeyRoundtrip();  // UT-FE-03
    void errorHandling();              // UT-FE-04
};

// UT-FE-01: decrypt(encrypt(plain, key), key) == plain
void CryptoEngineTest::encryptDecryptRoundtrip() {
    CryptoEngine engine;
    const QByteArray key = engine.generateAESKey();
    const QString plain = "Hello, BitATM!";
    QCOMPARE(engine.decrypt(engine.encrypt(plain, key), key), plain);
}

// UT-FE-02: RSA keypair is valid and cross-encrypt works
void CryptoEngineTest::generateRSAKeypair() {
    CryptoEngine engine;
    auto [pub, priv] = engine.generateRSAKeypair();
    QVERIFY(!pub.isEmpty());
    QVERIFY(!priv.isEmpty());
    QVERIFY(pub.contains("PUBLIC KEY"));
    QVERIFY(priv.contains("PRIVATE KEY"));
    const QByteArray plain = QByteArray("AES key material");
    const QByteArray ct = engine.encryptRSA(plain, pub);
    QVERIFY(!ct.isEmpty());
    QCOMPARE(engine.decryptRSA(ct, priv), plain);
}

// UT-FE-03: AES key wrapped with RSA survives round-trip
void CryptoEngineTest::rsaWrappedAesKeyRoundtrip() {
    CryptoEngine engine;
    const QByteArray aesKey = engine.generateAESKey();
    auto [pub, priv] = engine.generateRSAKeypair();
    const QByteArray wrapped = engine.encryptRSA(aesKey, pub);
    const QByteArray unwrapped = engine.decryptRSA(wrapped, priv);
    QCOMPARE(unwrapped, aesKey);
}

// UT-FE-04: empty key throws CryptoException; tampered ciphertext throws
void CryptoEngineTest::errorHandling() {
    CryptoEngine engine;
    QVERIFY_THROWS_EXCEPTION(CryptoException, engine.encrypt("test", QByteArray()));

    const QByteArray key = engine.generateAESKey();
    QString ct = engine.encrypt("secret", key);
    // Corrupt the last base64 character to invalidate the GCM auth tag
    ct[ct.size() - 1] = (ct[ct.size() - 1] == QChar('A')) ? QChar('B') : QChar('A');
    QVERIFY_THROWS_EXCEPTION(CryptoException, engine.decrypt(ct, key));
}

QTEST_MAIN(CryptoEngineTest)
#include "CryptoEngineTest.moc"

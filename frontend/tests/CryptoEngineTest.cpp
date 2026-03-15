#include <QtTest>

class CryptoEngineTest : public QObject {
    Q_OBJECT
private slots:
    void encryptDecryptRoundtrip() { QFAIL("not implemented"); }
    void generateRSAKeypair() { QFAIL("not implemented"); }
};

QTEST_MAIN(CryptoEngineTest)
#include "CryptoEngineTest.moc"
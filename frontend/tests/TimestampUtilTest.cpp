#include <QtTest>

#include "TimestampUtil.h"

/**
 * @brief UT-FE-16: TimestampUtil::canonical wire timestamp normalization.
 *
 * Regression tests for the 2026-07-12 defect where live client timestamps
 * ("...T...Z") and PostgreSQL replay text ("YYYY-MM-DD HH:MM:SS.f") never
 * compared equal, breaking message dedup and read-receipt matching.
 */
class TimestampUtilTest : public QObject {
    Q_OBJECT
private slots:
    void isoUtcIsAlreadyCanonical() {
        QCOMPARE(TimestampUtil::canonical("2026-07-12T18:57:48.068Z"),
                 QString("2026-07-12T18:57:48.068Z"));
    }

    void postgresTextNormalizes() {
        QCOMPARE(TimestampUtil::canonical("2026-07-12 18:57:48.068"),
                 QString("2026-07-12T18:57:48.068Z"));
    }

    void trimmedMillisecondsPadBackTo3Digits() {
        // PostgreSQL trims trailing zeros: .500 is stored back as .5
        QCOMPARE(TimestampUtil::canonical("2026-07-12 18:57:48.5"),
                 QString("2026-07-12T18:57:48.500Z"));
    }

    void missingMillisecondsBecomeZero() {
        QCOMPARE(TimestampUtil::canonical("2026-07-12 18:57:48"),
                 QString("2026-07-12T18:57:48.000Z"));
    }

    void zoneOffsetConvertsToUtc() {
        QCOMPARE(TimestampUtil::canonical("2026-07-12T20:57:48.068+02:00"),
                 QString("2026-07-12T18:57:48.068Z"));
    }

    void livePathAndReplayPathMatch() {
        // The exact pair that produced duplicate messages before the fix
        QCOMPARE(TimestampUtil::canonical("2026-07-12T18:57:48.068Z"),
                 TimestampUtil::canonical("2026-07-12 18:57:48.068"));
    }

    void unparseableInputPassesThrough() {
        QCOMPARE(TimestampUtil::canonical("not a timestamp"), QString("not a timestamp"));
        QCOMPARE(TimestampUtil::canonical(""), QString(""));
    }
};

QTEST_MAIN(TimestampUtilTest)
#include "TimestampUtilTest.moc"

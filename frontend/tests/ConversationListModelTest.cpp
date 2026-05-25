#include <QtTest>

#include "ConversationListModel.h"

class ConversationListModelTest : public QObject {
    Q_OBJECT
private slots:
    void removeExistingEntry();
    void removeNonExistentIsNoop();
    void removeGroupEntry();
};

void ConversationListModelTest::removeExistingEntry() {
    ConversationListModel model;
    model.addOrUpdate("alice", "hi", "2026-01-01T10:00:00");
    model.addOrUpdate("bob", "hey", "2026-01-01T10:00:01");
    QCOMPARE(model.rowCount(), 2);

    model.remove("alice");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), ConversationListModel::UsernameRole).toString(),
             QString("bob"));
}

void ConversationListModelTest::removeNonExistentIsNoop() {
    ConversationListModel model;
    model.addOrUpdate("alice", "hi", "2026-01-01T10:00:00");
    model.remove("nobody");
    QCOMPARE(model.rowCount(), 1);
}

void ConversationListModelTest::removeGroupEntry() {
    ConversationListModel model;
    model.addOrUpdateGroup("42", "TeamAlpha", "hello", "2026-01-01T10:00:00");
    model.addOrUpdate("alice", "hi", "2026-01-01T10:00:01");
    QCOMPARE(model.rowCount(), 2);

    model.remove("42");
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.data(model.index(0), ConversationListModel::UsernameRole).toString(),
             QString("alice"));
}

QTEST_MAIN(ConversationListModelTest)
#include "ConversationListModelTest.moc"

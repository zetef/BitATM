#include <QtTest>

#include "ChatModel.h"

class ChatModelTest : public QObject {
    Q_OBJECT
private slots:
    void appendMessageIncrementsRowCount();  // UT-FE-05
    void clearHistoryResetsModel();          // UT-FE-06
};

// UT-FE-05: appendMessage -> rowCount increments, role data is correct
void ChatModelTest::appendMessageIncrementsRowCount() {
    ChatModel model;
    QCOMPARE(model.rowCount(), 0);

    model.appendMessage("alice", "Hello!", "2025-01-01T10:00:00", false);
    QCOMPARE(model.rowCount(), 1);

    const QModelIndex idx = model.index(0);
    QCOMPARE(model.data(idx, ChatModel::SenderRole).toString(), QString("alice"));
    QCOMPARE(model.data(idx, ChatModel::ContentRole).toString(), QString("Hello!"));
    QCOMPARE(model.data(idx, ChatModel::IsOutgoingRole).toBool(), false);
}

// UT-FE-06: clearHistory -> rowCount == 0, no dangling state
void ChatModelTest::clearHistoryResetsModel() {
    ChatModel model;
    model.appendMessage("alice", "First", "2025-01-01T10:00:00", true);
    model.appendMessage("bob", "Second", "2025-01-01T10:00:01", false);
    QCOMPARE(model.rowCount(), 2);

    model.clearHistory();
    QCOMPARE(model.rowCount(), 0);
    QVERIFY(!model.index(0).isValid());
}

QTEST_MAIN(ChatModelTest)
#include "ChatModelTest.moc"

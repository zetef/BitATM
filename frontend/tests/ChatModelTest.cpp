#include <QtTest>

#include "ChatModel.h"

class ChatModelTest : public QObject {
    Q_OBJECT
private slots:
    void appendMessageIncrementsRowCount();   // UT-FE-05
    void clearHistoryResetsModel();           // UT-FE-06
    void perPeerCacheSwitchesConversation();  // UT-FE-09
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

// UT-FE-09: appendAndCache respects active peer - messages for other peers stay cached, not visible
void ChatModelTest::perPeerCacheSwitchesConversation() {
    ChatModel model;

    // Activate alice conversation first
    model.switchConversation("alice");
    QCOMPARE(model.rowCount(), 0);

    // Alice messages appear in view
    model.appendAndCache("alice", "alice", "Hi from Alice", "2025-01-01T10:00:00", false);
    QCOMPARE(model.rowCount(), 1);

    // Bob message is cached but NOT added to the view (alice is active)
    model.appendAndCache("bob", "bob", "Hi from Bob", "2025-01-01T10:01:00", false);
    QCOMPARE(model.rowCount(), 1);  // still 1 - bob hidden

    // Second alice message does appear
    model.appendAndCache("alice", "me", "Hey Alice", "2025-01-01T10:02:00", true);
    QCOMPARE(model.rowCount(), 2);

    // Switch to bob - only his 1 cached message visible
    model.switchConversation("bob");
    QCOMPARE(model.rowCount(), 1);
    const QModelIndex idx = model.index(0);
    QCOMPARE(model.data(idx, ChatModel::SenderRole).toString(), QString("bob"));

    // Switch back to alice - her 2 messages restored from cache
    model.switchConversation("alice");
    QCOMPARE(model.rowCount(), 2);

    // clearAll wipes everything
    model.clearAll();
    QCOMPARE(model.rowCount(), 0);
    model.switchConversation("alice");
    QCOMPARE(model.rowCount(), 0);  // cache was cleared, no messages
}

QTEST_MAIN(ChatModelTest)
#include "ChatModelTest.moc"

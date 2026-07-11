#include <QtTest>
#include <algorithm>
#include <stdexcept>
#include <vector>

#include "ConversationHistory.h"

namespace {
Message makeMessage(int id, const std::string& createdAt) {
    return Message{id, "alice", "bob", "body", "key", "", "sent", createdAt};
}
}  // namespace

class ConversationHistoryTest : public QObject {
    Q_OBJECT
private slots:
    // operator[] returns by position; past-the-end throws std::out_of_range
    // (kept outside the AppException hierarchy deliberately, mirroring std::vector::at)
    void indexOperatorReturnsByPosition() {
        ConversationHistory history{
            1,
            "alice",
            "bob",
            {makeMessage(1, "2026-07-01T10:00:00Z"), makeMessage(2, "2026-07-01T11:00:00Z"),
             makeMessage(3, "2026-07-01T12:00:00Z")}};
        QCOMPARE(history[1].getId(), 2);
        QVERIFY(history[2].getCreatedAt() == "2026-07-01T12:00:00Z");
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)history[9]);
        // exact boundary: index == size() must also throw
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)history[3]);
    }

    // operator+ merges two histories and drops duplicate message ids
    void mergeDeduplicatesById() {
        ConversationHistory h1{
            1,
            "alice",
            "bob",
            {makeMessage(1, "2026-07-01T10:00:00Z"), makeMessage(2, "2026-07-01T11:00:00Z")}};
        ConversationHistory h2{
            1,
            "alice",
            "bob",
            {makeMessage(2, "2026-07-01T11:00:00Z"), makeMessage(3, "2026-07-01T09:00:00Z")}};
        ConversationHistory merged = h1 + h2;
        QCOMPARE(merged.size(), static_cast<std::size_t>(3));
        int occurrencesOfId2 = 0;
        for (const auto& m : merged.getMessages())
            if (m.getId() == 2) ++occurrencesOfId2;
        QCOMPARE(occurrencesOfId2, 1);
    }

    // operator+ orders the merged result ascending by created_at (Message::operator<)
    void mergeSortsByTimestamp() {
        ConversationHistory h1{
            1,
            "alice",
            "bob",
            {makeMessage(1, "2026-07-01T10:00:00Z"), makeMessage(2, "2026-07-01T11:00:00Z")}};
        ConversationHistory h2{1, "alice", "bob", {makeMessage(3, "2026-07-01T09:00:00Z")}};
        ConversationHistory merged = h1 + h2;
        QCOMPARE(merged[0].getId(), 3);
        QCOMPARE(merged[1].getId(), 1);
        QCOMPARE(merged[2].getId(), 2);
    }

    // live call site for Message::operator> - newest-first sort
    void descendingSortUsesGreater() {
        std::vector<Message> messages{makeMessage(1, "2026-07-01T10:00:00Z"),
                                      makeMessage(2, "2026-07-01T12:00:00Z"),
                                      makeMessage(3, "2026-07-01T11:00:00Z")};
        std::sort(messages.begin(), messages.end(),
                  [](const Message& a, const Message& b) { return a > b; });
        QCOMPARE(messages[0].getId(), 2);
        QCOMPARE(messages[1].getId(), 3);
        QCOMPARE(messages[2].getId(), 1);
    }
};

QTEST_MAIN(ConversationHistoryTest)
#include "ConversationHistoryTest.moc"

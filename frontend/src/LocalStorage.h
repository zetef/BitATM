#pragma once
#include <QList>
#include <QObject>
#include <QSqlDatabase>
#include <QString>

/** @brief Value type for a stored message record. */
struct MessageRecord {
    QString peer;
    QString sender;
    QString content;
    QString timestamp;
    bool isOutgoing{false};
    QString status;
};

/** @brief Value type for a stored conversation summary record. */
struct ConversationRecord {
    QString peer;
    QString lastMessage;
    QString lastTimestamp;
};

/** @brief One group message from the group_messages SQLite table. */
struct GroupMessageRecord {
    QString groupId;
    QString sender;
    QString content;
    QString timestamp;
    bool isOutgoing{false};
};

/** @brief One group entry from the groups SQLite table. */
struct GroupRecord {
    QString groupId;
    QString name;
    QString role;
    QString lastMessage;
    QString lastTimestamp;
};

/**
 * @brief Singleton SQLite cache for messages and conversations.
 *
 * Opened once at app start. All reads/writes happen on the main thread
 * (same as NetworkManager). Not thread-safe by design.
 */
class LocalStorage {
public:
    /** @brief Returns the singleton instance. */
    static LocalStorage& instance();

    /** @brief Open (or create) the SQLite DB at AppDataLocation/bitatm.db. */
    bool open();

    /** @brief Close the database connection. */
    void close();

    /** @brief Save a message to the local cache. Silently ignores duplicates. */
    void saveMessage(const QString& peer, const QString& sender, const QString& content,
                     const QString& timestamp, bool isOutgoing, const QString& status = "sent");

    /** @brief Update the status field for a message identified by peer+timestamp. */
    void updateMessageStatus(const QString& peer, const QString& timestamp, const QString& status);

    /** @brief Load all messages for peer in ascending timestamp order. */
    QList<MessageRecord> loadMessages(const QString& peer);

    /** @brief Upsert a conversation summary row. */
    void saveConversation(const QString& peer, const QString& lastMessage,
                          const QString& lastTimestamp);

    /** @brief Load all conversation summaries. */
    QList<ConversationRecord> loadConversations();

    /** @brief Returns the ISO timestamp of the newest stored message, or empty string. */
    QString newestTimestamp();

    /** @brief Returns true if a message with this peer+sender+timestamp already exists. */
    bool isDuplicate(const QString& peer, const QString& sender, const QString& timestamp);

    /** @brief Save a group message. Silently ignores duplicates via UNIQUE index. */
    void saveGroupMessage(const QString& groupId, const QString& sender, const QString& content,
                          const QString& timestamp, bool isOutgoing);

    /** @brief Load all messages for a group in ascending timestamp order. */
    QList<GroupMessageRecord> loadGroupMessages(const QString& groupId);

    /** @brief Persist the AES key (base64) for a group. Overwrites existing entry. */
    void saveGroupKey(const QString& groupId, const QString& aesKeyBase64);

    /** @brief Load the stored AES key (base64) for a group, or empty string if absent. */
    QString loadGroupKey(const QString& groupId);

    /** @brief Upsert a group row (id, name, role). */
    void saveGroup(const QString& groupId, const QString& name, const QString& role);

    /** @brief Load all groups ordered by last_timestamp DESC. */
    QList<GroupRecord> loadGroups();

    /** @brief Returns true if a group message with this groupId+sender+timestamp already exists. */
    bool isGroupMessageDuplicate(const QString& groupId, const QString& sender,
                                 const QString& timestamp);

    /** @brief Remove a group and all related records (messages, key) from the local cache. */
    void removeGroup(const QString& groupId);

private:
    LocalStorage() = default;
    LocalStorage(const LocalStorage&) = delete;
    LocalStorage& operator=(const LocalStorage&) = delete;

    /** @brief Create tables and indexes if they do not exist. */
    bool createSchema();

    QSqlDatabase _db;
};

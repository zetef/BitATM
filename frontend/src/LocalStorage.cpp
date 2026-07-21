#include "LocalStorage.h"

#include <QDir>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(logStorage, "app.storage")

LocalStorage& LocalStorage::instance() {
    static LocalStorage s;
    return s;
}

bool LocalStorage::openForUser(const QString& username) {
    if (_db.isOpen()) close();

    QString sanitized = username;
    sanitized.replace(QRegularExpression("[^A-Za-z0-9_-]"), "_");

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                            "/accounts/" + sanitized;
    QDir().mkpath(dataDir);

    _db = QSqlDatabase::addDatabase("QSQLITE", "bitatm_local");
    _db.setDatabaseName(dataDir + "/bitatm.db");

    if (!_db.open()) {
        qCWarning(logStorage) << "Failed to open SQLite DB:" << _db.lastError().text();
        return false;
    }

    QSqlQuery(_db).exec("PRAGMA journal_mode=WAL");

    if (!createSchema()) {
        qCWarning(logStorage) << "Failed to create schema";
        return false;
    }

    qCInfo(logStorage) << "SQLite cache opened at" << _db.databaseName();
    return true;
}

void LocalStorage::close() {
    if (_db.isOpen()) {
        _db.close();
    }
    QSqlDatabase::removeDatabase("bitatm_local");
}

bool LocalStorage::createSchema() {
    QSqlQuery q(_db);

    const bool ok1 = q.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  peer        TEXT NOT NULL,"
        "  sender      TEXT NOT NULL,"
        "  content     TEXT NOT NULL,"
        "  timestamp   TEXT NOT NULL,"
        "  is_outgoing INTEGER NOT NULL DEFAULT 0,"
        "  status      TEXT NOT NULL DEFAULT 'sent'"
        ")");
    if (!ok1) {
        qCWarning(logStorage) << "Create messages table failed:" << q.lastError().text();
        return false;
    }

    const bool ok2 = q.exec(
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_messages_dedup "
        "ON messages(peer, sender, timestamp)");
    if (!ok2) {
        qCWarning(logStorage) << "Create dedup index failed:" << q.lastError().text();
        return false;
    }

    const bool ok3 = q.exec(
        "CREATE TABLE IF NOT EXISTS conversations ("
        "  peer           TEXT PRIMARY KEY,"
        "  last_message   TEXT NOT NULL,"
        "  last_timestamp TEXT NOT NULL"
        ")");
    if (!ok3) {
        qCWarning(logStorage) << "Create conversations table failed:" << q.lastError().text();
        return false;
    }

    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS group_messages ("
        "  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  group_id    TEXT    NOT NULL,"
        "  sender      TEXT    NOT NULL,"
        "  content     TEXT    NOT NULL,"
        "  timestamp   TEXT    NOT NULL,"
        "  is_outgoing INTEGER NOT NULL DEFAULT 0,"
        "  status      TEXT    NOT NULL DEFAULT 'sent'"
        ")");
    if (!ok) {
        qWarning() << "LocalStorage: schema error (group_messages):" << q.lastError().text();
        return false;
    }
    // Upgrade pre-status caches in place; failure just means the column exists
    q.exec("ALTER TABLE group_messages ADD COLUMN status TEXT NOT NULL DEFAULT 'sent'");

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS group_keys ("
        "  group_id       TEXT PRIMARY KEY,"
        "  aes_key_base64 TEXT NOT NULL"
        ")");
    if (!ok) {
        qWarning() << "LocalStorage: schema error (group_keys):" << q.lastError().text();
        return false;
    }

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS groups ("
        "  group_id       TEXT PRIMARY KEY,"
        "  name           TEXT NOT NULL,"
        "  role           TEXT NOT NULL DEFAULT 'member',"
        "  last_message   TEXT,"
        "  last_timestamp TEXT"
        ")");
    if (!ok) {
        qWarning() << "LocalStorage: schema error (groups):" << q.lastError().text();
        return false;
    }

    q.exec(
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_gm_dedup ON group_messages(group_id, sender, "
        "timestamp)");

    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS group_message_recipients ("
        "  group_id  TEXT NOT NULL,"
        "  timestamp TEXT NOT NULL,"
        "  member    TEXT NOT NULL,"
        "  delivered INTEGER NOT NULL DEFAULT 0,"
        "  seen      INTEGER NOT NULL DEFAULT 0,"
        "  PRIMARY KEY (group_id, timestamp, member)"
        ")");
    if (!ok) {
        qWarning() << "LocalStorage: schema error (group_message_recipients):"
                   << q.lastError().text();
        return false;
    }
    return true;
}

void LocalStorage::saveMessage(const QString& peer, const QString& sender, const QString& content,
                               const QString& timestamp, bool isOutgoing, const QString& status) {
    QSqlQuery q(_db);
    q.prepare(
        "INSERT OR IGNORE INTO messages(peer, sender, content, timestamp, is_outgoing, status) "
        "VALUES(?, ?, ?, ?, ?, ?)");
    q.addBindValue(peer);
    q.addBindValue(sender);
    q.addBindValue(content);
    q.addBindValue(timestamp);
    q.addBindValue(isOutgoing ? 1 : 0);
    q.addBindValue(status);
    if (!q.exec()) {
        qCWarning(logStorage) << "saveMessage failed:" << q.lastError().text();
    }
}

void LocalStorage::updateMessageStatus(const QString& peer, const QString& timestamp,
                                       const QString& status) {
    QSqlQuery q(_db);
    q.prepare("UPDATE messages SET status=? WHERE peer=? AND timestamp=?");
    q.addBindValue(status);
    q.addBindValue(peer);
    q.addBindValue(timestamp);
    if (!q.exec()) {
        qCWarning(logStorage) << "updateMessageStatus failed:" << q.lastError().text();
    }
}

QList<MessageRecord> LocalStorage::loadMessages(const QString& peer) {
    QSqlQuery q(_db);
    q.prepare(
        "SELECT peer, sender, content, timestamp, is_outgoing, status "
        "FROM messages WHERE peer=? ORDER BY timestamp ASC");
    q.addBindValue(peer);

    QList<MessageRecord> result;
    if (!q.exec()) {
        qCWarning(logStorage) << "loadMessages failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        MessageRecord r;
        r.peer = q.value(0).toString();
        r.sender = q.value(1).toString();
        r.content = q.value(2).toString();
        r.timestamp = q.value(3).toString();
        r.isOutgoing = q.value(4).toInt() != 0;
        r.status = q.value(5).toString();
        result.append(r);
    }
    return result;
}

void LocalStorage::saveConversation(const QString& peer, const QString& lastMessage,
                                    const QString& lastTimestamp) {
    QSqlQuery q(_db);
    q.prepare(
        "INSERT INTO conversations(peer, last_message, last_timestamp) VALUES(?, ?, ?) "
        "ON CONFLICT(peer) DO UPDATE SET last_message=excluded.last_message, "
        "last_timestamp=excluded.last_timestamp");
    q.addBindValue(peer);
    q.addBindValue(lastMessage);
    q.addBindValue(lastTimestamp);
    if (!q.exec()) {
        qCWarning(logStorage) << "saveConversation failed:" << q.lastError().text();
    }
}

QList<ConversationRecord> LocalStorage::loadConversations() {
    QSqlQuery q(_db);
    QList<ConversationRecord> result;
    if (!q.exec("SELECT peer, last_message, last_timestamp FROM conversations "
                "ORDER BY last_timestamp DESC")) {
        qCWarning(logStorage) << "loadConversations failed:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        ConversationRecord r;
        r.peer = q.value(0).toString();
        r.lastMessage = q.value(1).toString();
        r.lastTimestamp = q.value(2).toString();
        result.append(r);
    }
    return result;
}

QString LocalStorage::newestTimestamp() {
    QSqlQuery q(_db);
    // Sync cursor covers 1:1 AND group history; canonical ISO strings sort
    // lexicographically, so MAX over both tables is the true newest.
    if (!q.exec("SELECT MAX(ts) FROM (SELECT MAX(timestamp) AS ts FROM messages "
                "UNION SELECT MAX(timestamp) AS ts FROM group_messages)")) {
        qCWarning(logStorage) << "newestTimestamp failed:" << q.lastError().text();
        return {};
    }
    if (q.next()) {
        return q.value(0).toString();
    }
    return {};
}

bool LocalStorage::isDuplicate(const QString& peer, const QString& sender,
                               const QString& timestamp) {
    QSqlQuery q(_db);
    q.prepare("SELECT 1 FROM messages WHERE peer=? AND sender=? AND timestamp=? LIMIT 1");
    q.addBindValue(peer);
    q.addBindValue(sender);
    q.addBindValue(timestamp);
    if (!q.exec()) {
        qCWarning(logStorage) << "isDuplicate query failed:" << q.lastError().text();
        return false;
    }
    return q.next();
}

void LocalStorage::saveGroupMessage(const QString& groupId, const QString& sender,
                                    const QString& content, const QString& timestamp,
                                    bool isOutgoing) {
    QSqlQuery q(_db);
    q.prepare(
        "INSERT OR IGNORE INTO group_messages(group_id,sender,content,timestamp,is_outgoing) "
        "VALUES(?,?,?,?,?)");
    q.addBindValue(groupId);
    q.addBindValue(sender);
    q.addBindValue(content);
    q.addBindValue(timestamp);
    q.addBindValue(isOutgoing ? 1 : 0);
    if (!q.exec()) qWarning() << "saveGroupMessage:" << q.lastError().text();
}

QList<GroupMessageRecord> LocalStorage::loadGroupMessages(const QString& groupId) {
    QSqlQuery q(_db);
    q.prepare(
        "SELECT sender,content,timestamp,is_outgoing,status FROM group_messages "
        "WHERE group_id=? ORDER BY timestamp ASC");
    q.addBindValue(groupId);
    QList<GroupMessageRecord> result;
    if (q.exec()) {
        while (q.next()) {
            GroupMessageRecord r;
            r.groupId = groupId;
            r.sender = q.value(0).toString();
            r.content = q.value(1).toString();
            r.timestamp = q.value(2).toString();
            r.isOutgoing = q.value(3).toInt() != 0;
            r.status = q.value(4).toString();
            result.append(r);
        }
    }
    return result;
}

void LocalStorage::updateGroupMessageStatus(const QString& groupId, const QString& timestamp,
                                            const QString& status) {
    QSqlQuery q(_db);
    q.prepare("UPDATE group_messages SET status=? WHERE group_id=? AND timestamp=?");
    q.addBindValue(status);
    q.addBindValue(groupId);
    q.addBindValue(timestamp);
    if (!q.exec()) {
        qCWarning(logStorage) << "updateGroupMessageStatus failed:" << q.lastError().text();
    }
}

void LocalStorage::saveGroupKey(const QString& groupId, const QString& aesKeyBase64) {
    QSqlQuery q(_db);
    q.prepare(
        "INSERT INTO group_keys(group_id,aes_key_base64) VALUES(?,?) "
        "ON CONFLICT(group_id) DO UPDATE SET aes_key_base64=excluded.aes_key_base64");
    q.addBindValue(groupId);
    q.addBindValue(aesKeyBase64);
    if (!q.exec()) qWarning() << "saveGroupKey:" << q.lastError().text();
}

QString LocalStorage::loadGroupKey(const QString& groupId) {
    QSqlQuery q(_db);
    q.prepare("SELECT aes_key_base64 FROM group_keys WHERE group_id=?");
    q.addBindValue(groupId);
    if (q.exec() && q.next()) return q.value(0).toString();
    return {};
}

void LocalStorage::saveGroup(const QString& groupId, const QString& name, const QString& role) {
    QSqlQuery q(_db);
    q.prepare(
        "INSERT INTO groups(group_id,name,role) VALUES(?,?,?) "
        "ON CONFLICT(group_id) DO UPDATE SET name=excluded.name, role=excluded.role");
    q.addBindValue(groupId);
    q.addBindValue(name);
    q.addBindValue(role);
    if (!q.exec()) qWarning() << "saveGroup:" << q.lastError().text();
}

QList<GroupRecord> LocalStorage::loadGroups() {
    QSqlQuery q(
        "SELECT group_id,name,role,last_message,last_timestamp FROM groups "
        "ORDER BY last_timestamp DESC",
        _db);
    QList<GroupRecord> result;
    while (q.next()) {
        GroupRecord r;
        r.groupId = q.value(0).toString();
        r.name = q.value(1).toString();
        r.role = q.value(2).toString();
        r.lastMessage = q.value(3).toString();
        r.lastTimestamp = q.value(4).toString();
        result.append(r);
    }
    return result;
}

bool LocalStorage::isGroupMessageDuplicate(const QString& groupId, const QString& sender,
                                           const QString& timestamp) {
    QSqlQuery q(_db);
    q.prepare("SELECT 1 FROM group_messages WHERE group_id=? AND sender=? AND timestamp=? LIMIT 1");
    q.addBindValue(groupId);
    q.addBindValue(sender);
    q.addBindValue(timestamp);
    return q.exec() && q.next();
}

void LocalStorage::deleteConversation(const QString& peer) {
    QSqlQuery q(_db);
    q.prepare("DELETE FROM messages WHERE peer=?");
    q.addBindValue(peer);
    if (!q.exec())
        qCWarning(logStorage) << "deleteConversation (messages):" << q.lastError().text();
    q.prepare("DELETE FROM conversations WHERE peer=?");
    q.addBindValue(peer);
    if (!q.exec())
        qCWarning(logStorage) << "deleteConversation (conversations):" << q.lastError().text();
    qCInfo(logStorage) << "Deleted local conversation with" << peer;
}

void LocalStorage::clearAllData() {
    QSqlQuery q(_db);
    q.exec("DELETE FROM group_messages");
    q.exec("DELETE FROM group_keys");
    q.exec("DELETE FROM groups");
    q.exec("DELETE FROM messages");
    q.exec("DELETE FROM conversations");
    qCInfo(logStorage) << "LocalStorage cleared for user switch";
}

void LocalStorage::removeGroup(const QString& groupId) {
    QSqlQuery q(_db);
    q.prepare("DELETE FROM group_messages WHERE group_id=?");
    q.addBindValue(groupId);
    if (!q.exec()) qCWarning(logStorage) << "removeGroup (messages):" << q.lastError().text();

    q.prepare("DELETE FROM group_keys WHERE group_id=?");
    q.addBindValue(groupId);
    if (!q.exec()) qCWarning(logStorage) << "removeGroup (keys):" << q.lastError().text();

    q.prepare("DELETE FROM groups WHERE group_id=?");
    q.addBindValue(groupId);
    if (!q.exec()) qCWarning(logStorage) << "removeGroup (groups):" << q.lastError().text();
}

void LocalStorage::saveRecipientSnapshot(const QString& groupId, const QString& timestamp,
                                         const QStringList& members) {
    for (const QString& m : members) {
        QSqlQuery q(_db);
        q.prepare(
            "INSERT OR IGNORE INTO group_message_recipients(group_id, timestamp, member) "
            "VALUES(?, ?, ?)");
        q.addBindValue(groupId);
        q.addBindValue(timestamp);
        q.addBindValue(m);
        if (!q.exec()) qCWarning(logStorage) << "saveRecipientSnapshot:" << q.lastError().text();
    }
}

void LocalStorage::markRecipientDelivered(const QString& groupId, const QString& timestamp,
                                          const QString& member) {
    QSqlQuery q(_db);
    q.prepare(
        "UPDATE group_message_recipients SET delivered=1 "
        "WHERE group_id=? AND timestamp=? AND member=?");
    q.addBindValue(groupId);
    q.addBindValue(timestamp);
    q.addBindValue(member);
    if (!q.exec()) qCWarning(logStorage) << "markRecipientDelivered:" << q.lastError().text();
}

void LocalStorage::markRecipientSeen(const QString& groupId, const QString& timestamp,
                                     const QString& member) {
    QSqlQuery q(_db);
    q.prepare(
        "UPDATE group_message_recipients SET delivered=1, seen=1 "
        "WHERE group_id=? AND timestamp=? AND member=?");
    q.addBindValue(groupId);
    q.addBindValue(timestamp);
    q.addBindValue(member);
    if (!q.exec()) qCWarning(logStorage) << "markRecipientSeen:" << q.lastError().text();
}

int LocalStorage::recipientCount(const QString& groupId, const QString& timestamp) {
    QSqlQuery q(_db);
    q.prepare("SELECT COUNT(*) FROM group_message_recipients WHERE group_id=? AND timestamp=?");
    q.addBindValue(groupId);
    q.addBindValue(timestamp);
    if (!q.exec() || !q.next()) return 0;
    return q.value(0).toInt();
}

bool LocalStorage::allRecipientsDelivered(const QString& groupId, const QString& timestamp) {
    QSqlQuery q(_db);
    q.prepare(
        "SELECT COUNT(*) FROM group_message_recipients "
        "WHERE group_id=? AND timestamp=? AND delivered=0");
    q.addBindValue(groupId);
    q.addBindValue(timestamp);
    if (!q.exec() || !q.next()) return false;
    return q.value(0).toInt() == 0 && recipientCount(groupId, timestamp) > 0;
}

bool LocalStorage::allRecipientsSeen(const QString& groupId, const QString& timestamp) {
    QSqlQuery q(_db);
    q.prepare(
        "SELECT COUNT(*) FROM group_message_recipients "
        "WHERE group_id=? AND timestamp=? AND seen=0");
    q.addBindValue(groupId);
    q.addBindValue(timestamp);
    if (!q.exec() || !q.next()) return false;
    return q.value(0).toInt() == 0 && recipientCount(groupId, timestamp) > 0;
}

QVariantList LocalStorage::recipientStates(const QString& groupId, const QString& timestamp) {
    QVariantList result;
    QSqlQuery q(_db);
    q.prepare(
        "SELECT member, delivered, seen FROM group_message_recipients "
        "WHERE group_id=? AND timestamp=? ORDER BY member");
    q.addBindValue(groupId);
    q.addBindValue(timestamp);
    if (!q.exec()) return result;
    while (q.next()) {
        QVariantMap row;
        row["member"] = q.value(0).toString();
        row["delivered"] = q.value(1).toInt() != 0;
        row["seen"] = q.value(2).toInt() != 0;
        result.append(row);
    }
    return result;
}

QStringList LocalStorage::removeRecipientsNotIn(const QString& groupId,
                                                const QStringList& members) {
    // Collect the ex-members' rows first, then delete them
    QStringList affected;
    QSqlQuery q(_db);
    q.prepare("SELECT DISTINCT timestamp, member FROM group_message_recipients WHERE group_id=?");
    q.addBindValue(groupId);
    if (!q.exec()) return affected;
    QStringList doomedTs;
    QList<QPair<QString, QString>> doomed;
    while (q.next()) {
        const QString ts = q.value(0).toString();
        const QString member = q.value(1).toString();
        if (!members.contains(member)) {
            doomed.append({ts, member});
            if (!doomedTs.contains(ts)) doomedTs.append(ts);
        }
    }
    for (const auto& [ts, member] : doomed) {
        QSqlQuery del(_db);
        del.prepare(
            "DELETE FROM group_message_recipients WHERE group_id=? AND timestamp=? AND member=?");
        del.addBindValue(groupId);
        del.addBindValue(ts);
        del.addBindValue(member);
        if (!del.exec())
            qCWarning(logStorage) << "removeRecipientsNotIn:" << del.lastError().text();
    }
    // Only timestamps that still have rows can complete their aggregate
    for (const QString& ts : doomedTs)
        if (recipientCount(groupId, ts) > 0) affected.append(ts);
    return affected;
}

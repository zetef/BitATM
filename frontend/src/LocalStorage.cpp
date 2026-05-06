#include "LocalStorage.h"

#include <QDir>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

Q_LOGGING_CATEGORY(logStorage, "app.storage")

LocalStorage& LocalStorage::instance() {
    static LocalStorage s;
    return s;
}

bool LocalStorage::open() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    _db = QSqlDatabase::addDatabase("QSQLITE", "bitatm_local");
    _db.setDatabaseName(dataDir + "/bitatm.db");

    if (!_db.open()) {
        qCWarning(logStorage) << "Failed to open SQLite DB:" << _db.lastError().text();
        return false;
    }

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
    if (!q.exec("SELECT MAX(timestamp) FROM messages")) {
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

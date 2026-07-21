#include "ConversationListModel.h"

#include <algorithm>

#include "LocalStorage.h"

ConversationListModel::ConversationListModel(QObject* parent) : QAbstractListModel(parent) {}

void ConversationListModel::addOrUpdate(const QString& username, const QString& lastMessage,
                                        const QString& timestamp, bool incrementUnread) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&username](const ConvEntry& e) { return e.username == username; });

    if (it != entries_.end()) {
        it->lastMessage = lastMessage;
        it->timestamp = timestamp;
        if (incrementUnread) {
            ++it->unreadCount;
            LocalStorage::instance().incrementUnread(username);
        }
        int row = static_cast<int>(std::distance(entries_.begin(), it));
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx, {LastMessageRole, TimestampRole, UnreadCountRole});
    } else {
        int row = static_cast<int>(entries_.size());
        beginInsertRows(QModelIndex(), row, row);
        ConvEntry e;
        e.username = username;
        e.lastMessage = lastMessage;
        e.timestamp = timestamp;
        e.unreadCount = incrementUnread ? 1 : 0;
        if (incrementUnread) LocalStorage::instance().incrementUnread(username);
        entries_.push_back(e);
        endInsertRows();
    }
}

void ConversationListModel::remove(const QString& peer) {
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        const ConvEntry& e = entries_[static_cast<std::size_t>(i)];
        const bool matches = e.isGroup ? (e.groupId == peer) : (e.username == peer);
        if (matches) {
            beginRemoveRows(QModelIndex(), i, i);
            entries_.erase(entries_.begin() + i);
            endRemoveRows();
            return;
        }
    }
}

void ConversationListModel::markRead(const QString& peer) {
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        ConvEntry& e = entries_[static_cast<std::size_t>(i)];
        const bool matches = e.isGroup ? (e.groupId == peer) : (e.username == peer);
        if (matches) {
            if (e.unreadCount == 0) return;
            if (e.isGroup)
                LocalStorage::instance().resetGroupUnread(e.groupId);
            else
                LocalStorage::instance().resetUnread(e.username);
            e.unreadCount = 0;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {UnreadCountRole});
            return;
        }
    }
}

void ConversationListModel::clear() {
    if (entries_.empty()) {
        return;
    }
    beginResetModel();
    entries_.clear();
    endResetModel();
}

int ConversationListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(entries_.size());
}

QVariant ConversationListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(entries_.size())) {
        return {};
    }

    const ConvEntry& entry = entries_[static_cast<std::size_t>(index.row())];

    switch (role) {
        case UsernameRole:
            return entry.username;
        case LastMessageRole:
            return entry.lastMessage;
        case TimestampRole:
            return entry.timestamp;
        case IsGroupRole:
            return entry.isGroup;
        case GroupIdRole:
            return entry.groupId;
        case UnreadCountRole:
            return entry.unreadCount;
        default:
            return {};
    }
}

QHash<int, QByteArray> ConversationListModel::roleNames() const {
    return {
        {UsernameRole, "username"},   {LastMessageRole, "lastMessage"},
        {TimestampRole, "timestamp"}, {IsGroupRole, "is_group"},
        {GroupIdRole, "group_id"},    {UnreadCountRole, "unreadCount"},
    };
}

void ConversationListModel::addOrUpdateGroup(const QString& groupId, const QString& groupName,
                                             const QString& lastMessage,
                                             const QString& lastTimestamp, bool incrementUnread) {
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        if (entries_[static_cast<std::size_t>(i)].isGroup &&
            entries_[static_cast<std::size_t>(i)].groupId == groupId) {
            if (!lastMessage.isEmpty())
                entries_[static_cast<std::size_t>(i)].lastMessage = lastMessage;
            if (!lastTimestamp.isEmpty())
                entries_[static_cast<std::size_t>(i)].timestamp = lastTimestamp;
            if (incrementUnread) {
                ++entries_[static_cast<std::size_t>(i)].unreadCount;
                LocalStorage::instance().incrementGroupUnread(groupId);
            }
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {LastMessageRole, TimestampRole, UnreadCountRole});
            return;
        }
    }
    int row = static_cast<int>(entries_.size());
    beginInsertRows(QModelIndex(), row, row);
    ConvEntry e;
    e.username = groupName;
    e.groupId = groupId;
    e.isGroup = true;
    e.lastMessage = lastMessage;
    e.timestamp = lastTimestamp;
    e.unreadCount = incrementUnread ? 1 : 0;
    if (incrementUnread) LocalStorage::instance().incrementGroupUnread(groupId);
    entries_.push_back(e);
    endInsertRows();
}

void ConversationListModel::setUnreadCount(const QString& peer, int count) {
    for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
        ConvEntry& e = entries_[static_cast<std::size_t>(i)];
        const bool matches = e.isGroup ? (e.groupId == peer) : (e.username == peer);
        if (matches) {
            if (e.unreadCount == count) return;
            e.unreadCount = count;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {UnreadCountRole});
            return;
        }
    }
}

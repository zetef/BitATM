#include "ChatModel.h"

#include "LocalStorage.h"

ChatModel::ChatModel(QObject* parent) : QAbstractListModel(parent) {}

void ChatModel::appendMessage(const QString& sender, const QString& content,
                              const QString& timestamp, bool isOutgoing) {
    const int row = static_cast<int>(entries_.size());
    beginInsertRows(QModelIndex(), row, row);
    entries_.push_back({sender, content, timestamp, isOutgoing});
    endInsertRows();
}

void ChatModel::appendAndCache(const QString& peer, const QString& sender, const QString& content,
                               const QString& timestamp, bool isOutgoing, const QString& status) {
    ChatEntry entry{sender, content, timestamp, isOutgoing, status};
    cache_[peer].push_back(entry);

    if (activePeer_ == peer) {
        const int row = static_cast<int>(entries_.size());
        beginInsertRows(QModelIndex(), row, row);
        entries_.push_back(entry);
        endInsertRows();
    }
}

void ChatModel::switchConversation(const QString& peer) {
    activePeer_ = peer;
    beginResetModel();
    entries_.clear();
    if (cache_.contains(peer)) {
        entries_ = cache_[peer];
    }
    endResetModel();
}

void ChatModel::clearHistory() {
    beginResetModel();
    entries_.clear();
    endResetModel();
}

void ChatModel::clearAll() {
    beginResetModel();
    activePeer_.clear();
    cache_.clear();
    entries_.clear();
    endResetModel();
}

int ChatModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(entries_.size());
}

QVariant ChatModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(entries_.size()))
        return {};

    const ChatEntry& e = entries_[static_cast<std::size_t>(index.row())];
    switch (role) {
        case SenderRole:
            return e.sender;
        case ContentRole:
            return e.content;
        case TimestampRole:
            return e.timestamp;
        case IsOutgoingRole:
            return e.isOutgoing;
        case StatusRole:
            return e.status;
        default:
            return {};
    }
}

QHash<int, QByteArray> ChatModel::roleNames() const {
    return {
        {SenderRole, "sender"},         {ContentRole, "content"}, {TimestampRole, "timestamp"},
        {IsOutgoingRole, "isOutgoing"}, {StatusRole, "status"},
    };
}

void ChatModel::updateStatus(const QString& peer, const QString& timestamp, const QString& status) {
    // Persist to SQLite so status survives app restarts
    LocalStorage::instance().updateMessageStatus(peer, timestamp, status);

    // Update active view if the peer is currently displayed
    if (activePeer_ == peer) {
        for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
            if (entries_[static_cast<std::size_t>(i)].timestamp == timestamp) {
                entries_[static_cast<std::size_t>(i)].status = status;
                const QModelIndex idx = index(i);
                emit dataChanged(idx, idx, {StatusRole});
            }
        }
    }
    // Update cache for the specific peer
    if (cache_.contains(peer)) {
        for (auto& entry : cache_[peer]) {
            if (entry.timestamp == timestamp) {
                entry.status = status;
            }
        }
    }
}

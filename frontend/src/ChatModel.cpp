#include "ChatModel.h"

ChatModel::ChatModel(QObject* parent) : QAbstractListModel(parent) {}

void ChatModel::appendMessage(const QString& sender, const QString& content,
                              const QString& timestamp, bool isOutgoing) {
    const int row = static_cast<int>(entries_.size());
    beginInsertRows(QModelIndex(), row, row);
    entries_.push_back({sender, content, timestamp, isOutgoing});
    endInsertRows();
}

void ChatModel::appendAndCache(const QString& peer, const QString& sender, const QString& content,
                               const QString& timestamp, bool isOutgoing) {
    ChatEntry entry{sender, content, timestamp, isOutgoing};
    cache_[peer].push_back(entry);

    if (activePeer_ == peer || activePeer_.isEmpty()) {
        if (activePeer_.isEmpty()) activePeer_ = peer;
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
        default:
            return {};
    }
}

QHash<int, QByteArray> ChatModel::roleNames() const {
    return {
        {SenderRole, "sender"},
        {ContentRole, "content"},
        {TimestampRole, "timestamp"},
        {IsOutgoingRole, "isOutgoing"},
    };
}

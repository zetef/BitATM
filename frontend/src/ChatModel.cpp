#include "ChatModel.h"

ChatModel::ChatModel(QObject* parent) : QAbstractListModel(parent) {}

void ChatModel::appendMessage(const QString& sender, const QString& content,
                              const QString& timestamp, bool isOutgoing) {
    const int row = static_cast<int>(entries_.size());
    beginInsertRows(QModelIndex(), row, row);
    entries_.push_back({sender, content, timestamp, isOutgoing});
    endInsertRows();
}

void ChatModel::clearHistory() {
    beginResetModel();
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

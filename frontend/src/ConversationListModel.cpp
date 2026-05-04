#include "ConversationListModel.h"

#include <algorithm>

ConversationListModel::ConversationListModel(QObject* parent) : QAbstractListModel(parent) {}

void ConversationListModel::addOrUpdate(const QString& username, const QString& lastMessage,
                                        const QString& timestamp) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
                           [&username](const ConvEntry& e) { return e.username == username; });

    if (it != entries_.end()) {
        it->lastMessage = lastMessage;
        it->timestamp = timestamp;
        int row = static_cast<int>(std::distance(entries_.begin(), it));
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx, {LastMessageRole, TimestampRole});
    } else {
        int row = static_cast<int>(entries_.size());
        beginInsertRows(QModelIndex(), row, row);
        entries_.push_back({username, lastMessage, timestamp});
        endInsertRows();
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
        default:
            return {};
    }
}

QHash<int, QByteArray> ConversationListModel::roleNames() const {
    return {
        {UsernameRole, "username"},
        {LastMessageRole, "lastMessage"},
        {TimestampRole, "timestamp"},
    };
}

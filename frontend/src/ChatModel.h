#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <vector>

/**
 * @brief QAbstractListModel holding chat messages for one conversation.
 *
 * Always use appendMessage() and clearHistory() — never mutate entries_ directly.
 * Both methods call the required begin/end model change guards.
 */
class ChatModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { SenderRole = Qt::UserRole + 1, ContentRole, TimestampRole, IsOutgoingRole };

    explicit ChatModel(QObject* parent = nullptr);

    /** @brief Append one message. Emits rowsInserted. */
    Q_INVOKABLE void appendMessage(const QString& sender, const QString& content,
                                   const QString& timestamp, bool isOutgoing);

    /** @brief Remove all messages. Emits modelReset. */
    Q_INVOKABLE void clearHistory();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct ChatEntry {
        QString sender;
        QString content;
        QString timestamp;
        bool isOutgoing;
    };

    std::vector<ChatEntry> entries_;
};

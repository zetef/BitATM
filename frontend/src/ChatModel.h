#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QMap>
#include <QString>
#include <vector>

/**
 * @brief QAbstractListModel holding chat messages for one conversation.
 *
 * Always use appendMessage() / appendAndCache() / clearHistory() / switchConversation()
 * to mutate state - never touch entries_ or cache_ directly.
 * All mutating methods call the required begin/end model change guards.
 */
class ChatModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles { SenderRole = Qt::UserRole + 1, ContentRole, TimestampRole, IsOutgoingRole };

    explicit ChatModel(QObject* parent = nullptr);

    /** @brief Append one message to the flat display list only (existing API, tests use this). */
    Q_INVOKABLE void appendMessage(const QString& sender, const QString& content,
                                   const QString& timestamp, bool isOutgoing);

    /**
     * @brief Append a message to the per-peer cache and to the flat list if peer is active.
     *
     * @param peer      The conversation key (recipient username for outgoing, sender for incoming).
     * @param sender    Display sender name.
     * @param content   Plaintext content.
     * @param timestamp ISO timestamp string.
     * @param isOutgoing True for messages the local user sent.
     */
    Q_INVOKABLE void appendAndCache(const QString& peer, const QString& sender,
                                    const QString& content, const QString& timestamp,
                                    bool isOutgoing);

    /**
     * @brief Switch to a different conversation, replaying its cached messages.
     *
     * Clears the flat list and repopulates it from the cache for peer.
     * If no cache exists for peer, the list is cleared and stays empty.
     */
    Q_INVOKABLE void switchConversation(const QString& peer);

    /** @brief Remove all messages from the flat display list. Does not touch the cache. */
    Q_INVOKABLE void clearHistory();

    /** @brief Clear both the flat display list and the entire per-peer cache. */
    Q_INVOKABLE void clearAll();

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
    QMap<QString, std::vector<ChatEntry>> cache_;
    QString activePeer_;
};

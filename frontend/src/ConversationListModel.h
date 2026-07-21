#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <vector>

/**
 * @brief QAbstractListModel for the conversation sidebar.
 *
 * Holds a list of ConvEntry items, each representing a conversation
 * with a peer. Supports addOrUpdate() to insert new entries or
 * refresh existing ones in-place.
 */
class ConversationListModel : public QAbstractListModel {
    Q_OBJECT

public:
    /** @brief Role identifiers exposed to QML. */
    enum Roles {
        UsernameRole = Qt::UserRole + 1,
        LastMessageRole,
        TimestampRole,
        IsGroupRole = Qt::UserRole + 4,
        GroupIdRole = Qt::UserRole + 5,
        UnreadCountRole = Qt::UserRole + 6
    };

    /** @brief Constructs the model with an optional parent. */
    explicit ConversationListModel(QObject* parent = nullptr);

    /**
     * @brief Inserts a new entry or updates an existing one by username.
     *
     * If an entry with @p username already exists, its lastMessage and
     * timestamp fields are updated in-place and dataChanged is emitted.
     * Otherwise a new row is appended.
     *
     * @param username       Peer username (unique key).
     * @param lastMessage    Most recent message preview.
     * @param timestamp      Display timestamp string.
     * @param incrementUnread Increment the unread badge by one (a new incoming
     *        message for a conversation that isn't currently open).
     */
    Q_INVOKABLE void addOrUpdate(const QString& username, const QString& lastMessage,
                                 const QString& timestamp, bool incrementUnread = false);

    /** @brief Add or update a group conversation by groupId. See addOrUpdate() for incrementUnread.
     */
    Q_INVOKABLE void addOrUpdateGroup(const QString& groupId, const QString& groupName,
                                      const QString& lastMessage, const QString& lastTimestamp,
                                      bool incrementUnread = false);

    /** @brief Reset the unread badge for a conversation (peer or groupId) to zero. */
    Q_INVOKABLE void markRead(const QString& peer);

    /**
     * @brief Set the unread badge to an exact value, in memory only.
     *
     * Used to seed the badge from the persisted LocalStorage count during
     * local history load - does not itself write back to LocalStorage.
     */
    Q_INVOKABLE void setUnreadCount(const QString& peer, int count);

    /** @brief Removes all entries from the model. */
    Q_INVOKABLE void clear();

    /** @brief Remove the entry whose username (or groupId for groups) matches peer. No-op if not
     * found. */
    Q_INVOKABLE void remove(const QString& peer);

    /** @brief Returns the number of entries in the model. */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /** @brief Returns the data for a given index and role. */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /** @brief Returns the role name map used by QML delegates. */
    QHash<int, QByteArray> roleNames() const override;

private:
    /** @brief Internal storage for a single conversation entry. */
    struct ConvEntry {
        QString username;
        QString lastMessage;
        QString timestamp;
        bool isGroup{false};
        QString groupId;
        int unreadCount{0};
    };

    std::vector<ConvEntry> entries_;
};

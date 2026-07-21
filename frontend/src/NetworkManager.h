#pragma once
#include <QList>
#include <QMap>
#include <QNetworkInformation>
#include <QObject>
#include <QSet>
#include <QSettings>
#include <QSslError>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>

#include "../../common/protocol.h"
#include "CryptoEngine.h"
#include "LocalStorage.h"

/**
 * @brief Proxy to the BitATM WebSocket server with integrated E2EE and local history.
 *
 * Handles connection lifecycle, packet serialization, RSA keypair
 * management (QSettings-persisted), per-peer AES key wrapping, routing
 * of inbound packets to typed signals, and write-through message persistence.
 *
 * Threading: all methods run on the Qt main thread via the event loop.
 * QWebSocket is async - no manual thread management needed.
 */
class NetworkManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY lastMessageChanged)
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUsernameChanged)
public:
    explicit NetworkManager(QObject* parent = nullptr);

    /** @brief Open a WebSocket connection to the given URL. */
    void connectToServer(const QUrl& url);

    /** @brief Send a REGISTER packet with username and password. */
    Q_INVOKABLE void sendRegister(const QString& username, const QString& password);

    /** @brief Send a LOGIN packet with username and password. */
    Q_INVOKABLE void sendLogin(const QString& username, const QString& password);

    /**
     * @brief Encrypt plaintext and send a MESSAGE packet to 'to'.
     *
     * If 'to' has no cached public key, the send is deferred until
     * peerKeyReceived is emitted for that username.
     */
    Q_INVOKABLE void sendMessage(const QString& to, const QString& plaintext,
                                 const QString& timestamp = QString());

    /** @brief Request peer's RSA public key from the server. */
    Q_INVOKABLE void fetchPeerKey(const QString& username);

    /** @brief Ask the server to push all missed messages for current user. */
    Q_INVOKABLE void sendSyncHistory();

    /**
     * @brief Send READ_RECEIPT packets for all unread messages from peer.
     *
     * Drains the internal unread-timestamp list for peer, sending one
     * READ_RECEIPT per message. No-op if not authenticated or no unread messages.
     */
    Q_INVOKABLE void markConversationRead(const QString& peer);

    /** @brief Clear session state and return to login without closing the WebSocket. */
    Q_INVOKABLE void logout();

    /** @brief Create a new group. Encrypts AES key for each member via RSA. */
    Q_INVOKABLE void createGroup(const QString& name, const QStringList& members);

    /** @brief Encrypt and send a group message. */
    Q_INVOKABLE void sendGroupMessage(const QString& groupId, const QString& plaintext,
                                      const QString& timestamp);

    /**
     * @brief Recover this user's wrapped group key from the server.
     *
     * Sends GROUP_KEY_EXCHANGE with an empty key field; the server replies
     * with the stored RSA-wrapped AES key. Deduplicated per group until the
     * key arrives.
     */
    void requestGroupKey(const QString& groupId);

    /** @brief Admin: kick a member from a group. */
    Q_INVOKABLE void kickMember(const QString& groupId, const QString& username);

    /** @brief Admin: add a new member to a group. */
    Q_INVOKABLE void addGroupMember(const QString& groupId, const QString& username);

    /** @brief Creator: grant admin role to a member. */
    Q_INVOKABLE void grantAdmin(const QString& groupId, const QString& username);

    /** @brief Creator: revoke admin role from a member, demoting them back to member. */
    Q_INVOKABLE void revokeAdmin(const QString& groupId, const QString& username);

    /** @brief Leave a group voluntarily. */
    Q_INVOKABLE void leaveGroup(const QString& groupId);

    /** @brief Send DELETE_GROUP packet to permanently delete the group (creator only). */
    Q_INVOKABLE void deleteGroup(const QString& groupId);

    /** @brief Request updated group info (member list, roles). */
    Q_INVOKABLE void fetchGroupInfo(const QString& groupId);

    /** @brief Return the stored display name for a group, or groupId if unknown. */
    Q_INVOKABLE QString getGroupName(const QString& groupId) const;

    /** @brief Delete a conversation from local cache. Messages stay on the server. */
    Q_INVOKABLE void deleteConversation(const QString& peer);

    /**
     * @brief Delete a conversation for both participants.
     *
     * Wipes the local copy immediately and sends DELETE_CONVERSATION so the
     * server hard-deletes the messages and notifies the peer (now if online,
     * on their next login otherwise).
     */
    Q_INVOKABLE void deleteConversationForEveryone(const QString& peer);

    /** @brief Per-member receipt states for the info sheet: [{member, delivered, seen}]. */
    Q_INVOKABLE QVariantList groupMessageReceipts(const QString& groupId, const QString& timestamp);

    /** @brief Returns true if the WebSocket is in ConnectedState. */
    bool isConnected() const;

    /** @brief Returns the last status or error message. */
    QString lastMessage() const;

    /** @brief Returns true if the last operation resulted in an error. */
    bool hasError() const;

    /** @brief Returns the currently authenticated username. */
    QString currentUsername() const;

signals:
    /** @brief Raw inbound wire message forwarded to QML. */
    void messageReceived(const QString& message);
    void lastMessageChanged();
    void connectionChanged();
    void connected();
    void disconnected();
    void errorOcurred(const QString& errorString);
    void currentUsernameChanged();

    /** @brief Emitted when an incoming MESSAGE packet is successfully decrypted. */
    void messageDecrypted(const QString& from, const QString& plaintext, const QString& timestamp);

    /** @brief Emitted after a conversation is deleted from local cache. */
    void conversationDeleted(const QString& peer);

    /** @brief Emitted when a KEY_EXCHANGE response delivers a peer's public key. */
    void peerKeyReceived(const QString& username, const QString& publicKey);

    /** @brief Emitted after the server ACKs a SYNC_HISTORY request. */
    void syncComplete();

    /**
     * @brief Emitted when the server confirms receipt of an outgoing message.
     *
     * @param timestamp ISO timestamp of the original sent message, used to update its status.
     */
    void messageDelivered(const QString& timestamp);

    /**
     * @brief Emitted when the recipient confirms they have read a specific message.
     *
     * For groups (peer = numeric group id) this fires only when EVERY
     * snapshot recipient has seen the message (receipts v2 aggregate).
     *
     * @param peer      The conversation peer (recipient who sent the receipt).
     * @param timestamp ISO timestamp of the message that was read.
     */
    void messageSeen(const QString& peer, const QString& timestamp);

    /** @brief Group message reached delivered-to-all (per-member aggregate). */
    void groupMessageDelivered(const QString& groupId, const QString& timestamp);

    /**
     * @brief Fires on every individual per-member delivered/seen update for a
     *        group message, not just when the whole-group aggregate completes -
     *        lets an open message info sheet refresh its per-member breakdown live.
     */
    void groupReceiptUpdated(const QString& groupId, const QString& timestamp);

    /**
     * @brief Emitted once per stored message during local history load on login.
     *
     * QML handles this to populate chatModel and convListModel from local storage
     * before the server sync arrives.
     *
     * @param peer      The conversation key (sender username for incoming, recipient for outgoing).
     * @param sender    Display sender name.
     * @param content   Plaintext message content.
     * @param timestamp ISO 8601 timestamp string.
     * @param isOutgoing True if the local user sent this message.
     */
    void historySyncMessage(const QString& peer, const QString& sender, const QString& content,
                            const QString& timestamp, bool isOutgoing,
                            const QString& status = "sent");

    /**
     * @brief Emitted after loading local history and on new messages - updates conversation list.
     *
     * @param peer          The peer username.
     * @param lastMessage   Latest message content.
     * @param lastTimestamp ISO 8601 timestamp of the latest message.
     */
    void convListUpdated(const QString& peer, const QString& lastMessage,
                         const QString& lastTimestamp);

    /** @brief Emitted when the server sends a GROUP_INVITE packet for the current user. */
    void groupInviteReceived(const QString& groupId, const QString& groupName);

    /** @brief Emitted when an incoming group message is successfully decrypted. */
    void groupMessageDecrypted(const QString& groupId, const QString& sender,
                               const QString& plaintext, const QString& timestamp, bool isOutgoing);

    /** @brief Emitted when the group AES key is updated after a key rotation. */
    void groupKeyUpdated(const QString& groupId);

    /** @brief Emitted when group info (member list, roles) is received from the server. */
    void groupInfoReceived(const QString& groupId, const QString& groupName,
                           const QVariantList& members);

    /** @brief Emitted when the current user has successfully left a group. */
    void groupLeft(const QString& groupId);

    /** @brief Emitted once per group message during local history load. */
    void groupHistorySyncMessage(const QString& groupId, const QString& sender,
                                 const QString& content, const QString& timestamp, bool isOutgoing,
                                 const QString& status);

    /** @brief Emitted once per group during local history load for sidebar population. */
    void groupConvUpdated(const QString& groupId, const QString& groupName,
                          const QString& lastMessage, const QString& lastTimestamp);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onSslErrors(const QList<QSslError>& errors);
    void onReachabilityChanged(QNetworkInformation::Reachability reachability);

private:
    /** @brief Serialize and send a packet over the WebSocket. */
    void sendPacket(const Packet& packet);

    /** @brief Handle login/register ACK: set username, load/generate keypair, load history, sync.
     */
    void handleLoginAck(const Packet& p);

    /** @brief Decrypt an incoming MESSAGE packet, deduplicate, persist, and emit messageDecrypted.
     */
    void handleIncomingMessage(const Packet& p);

    /** @brief Cache peer public key and flush any pending message for that peer. */
    void handleKeyExchangeResponse(const Packet& p);

    /** @brief Handle an incoming READ_RECEIPT packet and emit messageSeen. */
    void handleReadReceipt(const Packet& p);

    /** @brief Group send-ACK: persist the recipient snapshot from the server. */
    void handleGroupSendAck(const Packet& p);

    /** @brief Per-member delivered event: update row and recompute aggregate. */
    void handleGroupDelivered(const Packet& p);

    /** @brief Flip group message status when per-member aggregates complete. */
    void recomputeGroupAggregate(const QString& groupId, const QString& ts);

    /** @brief Load RSA keypair from QSettings, or generate and persist a new one. */
    void loadOrGenerateKeypair();

    /** @brief Encrypt plaintext for 'to' using their cached public key, send, and persist. */
    void encryptAndSend(const QString& to, const QString& plaintext, const QString& timestamp);

    /**
     * @brief Load local message history for _currentUsername from QSettings.
     *
     * Emits historySyncMessage for each stored entry and populates _messageKeys.
     * Must be called after _currentUsername is set.
     */
    void loadLocalHistory();

    /**
     * @brief Persist one message to LocalStorage SQLite cache.
     *
     * @param peer      Conversation key (peer username).
     * @param sender    Display sender name.
     * @param content   Plaintext content.
     * @param timestamp ISO 8601 timestamp.
     * @param isOutgoing True if sent by local user.
     */
    void persistMessage(const QString& peer, const QString& sender, const QString& content,
                        const QString& timestamp, bool isOutgoing);

    QWebSocket _socket;
    QTimer _pingTimer;
    QUrl _serverUrl;
    QString _lastMessage;
    QString _currentUsername;
    bool _hasError = false;

    CryptoEngine _crypto;
    QByteArray _ownPrivKey;
    QByteArray _ownPubKey;
    QMap<QString, QByteArray> _peerKeys;
    QMap<QString, QList<QPair<QString, QString>>> _pendingMessages;
    bool _pendingRegister = false;
    bool _intentionallyConnecting = false;
    QMap<QString, QStringList> _unreadTimestamps;

    /** @brief Pending group member adds: username -> list of groupIds waiting for that key. */
    QMap<QString, QStringList> _pendingGroupMemberAdds;
    QSet<QString> _pendingGroupKeyRequests;
    QMap<QString, QList<Packet>> _pendingGroupPackets;  // awaiting key recovery
    QMap<QString, QList<QPair<QString, QString>>> _pendingGroupSends;

    struct PendingGroupCreate {
        QString name;
        QStringList members;
        QByteArray aesKey;
        QMap<QString, QByteArray> collectedKeys;  // username -> RSA-encrypted AES key bytes
        QStringList waitingFor;
    };
    QList<PendingGroupCreate> _pendingGroupCreations;

    /** @brief In-memory map from groupId to decrypted AES key bytes. */
    QMap<QString, QByteArray> _groupKeys;

    /** @brief In-memory map from groupId to group display name. */
    QMap<QString, QString> _groupNames;

    /** @brief Handle an incoming GROUP_INVITE packet. */
    void handleGroupInvite(const Packet& p);

    /** @brief Decrypt and emit an incoming GROUP_MESSAGE packet. */
    void handleGroupMessage(const Packet& p);

    /** @brief Update the stored AES key after a GROUP_KEY_EXCHANGE. */
    void handleGroupKeyExchange(const Packet& p);

    /** @brief Parse and emit group member list from a GROUP_INFO response. */
    void handleGroupInfo(const Packet& p);

    /** @brief Handle GROUP_LEAVE - either key rotation notice or self-leave confirmation. */
    void handleGroupLeave(const Packet& p);

    /** @brief Handle an incoming DELETE_CONVERSATION: peer deleted our shared conversation. */
    void handleDeleteConversation(const Packet& p);

    /** @brief Generate a new AES key for a group and request member list for redistribution. */
    void rotateGroupKey(const QString& groupId);

    /** @brief Build and send a CREATE_GROUP packet from a fully-resolved pending creation. */
    void sendCreateGroupPacket(const PendingGroupCreate& pending);
};

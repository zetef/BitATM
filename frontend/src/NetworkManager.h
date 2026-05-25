#pragma once
#include <QList>
#include <QMap>
#include <QNetworkInformation>
#include <QObject>
#include <QSettings>
#include <QSslError>
#include <QString>
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

    /** @brief Admin: kick a member from a group. */
    Q_INVOKABLE void kickMember(const QString& groupId, const QString& username);

    /** @brief Admin: add a new member to a group. */
    Q_INVOKABLE void addGroupMember(const QString& groupId, const QString& username);

    /** @brief Creator: grant admin role to a member. */
    Q_INVOKABLE void grantAdmin(const QString& groupId, const QString& username);

    /** @brief Leave a group voluntarily. */
    Q_INVOKABLE void leaveGroup(const QString& groupId);

    /** @brief Request updated group info (member list, roles). */
    Q_INVOKABLE void fetchGroupInfo(const QString& groupId);

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
     * @param peer      The conversation peer (recipient who sent the receipt).
     * @param timestamp ISO timestamp of the message that was read.
     */
    void messageSeen(const QString& peer, const QString& timestamp);

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
                            const QString& timestamp, bool isOutgoing);

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

    /** @brief Generate a new AES key for a group and request member list for redistribution. */
    void rotateGroupKey(const QString& groupId);

    /** @brief Build and send a CREATE_GROUP packet from a fully-resolved pending creation. */
    void sendCreateGroupPacket(const PendingGroupCreate& pending);
};

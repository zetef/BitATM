#pragma once
#include <QList>
#include <QMap>
#include <QNetworkInformation>
#include <QObject>
#include <QSet>
#include <QSettings>
#include <QSslError>
#include <QString>
#include <QUrl>
#include <QWebSocket>

#include "../../common/protocol.h"
#include "CryptoEngine.h"

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
    Q_INVOKABLE void sendMessage(const QString& to, const QString& plaintext);

    /** @brief Request peer's RSA public key from the server. */
    Q_INVOKABLE void fetchPeerKey(const QString& username);

    /** @brief Ask the server to push all missed messages for current user. */
    Q_INVOKABLE void sendSyncHistory();

    /** @brief Clear session state and return to login without closing the WebSocket. */
    Q_INVOKABLE void logout();

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

    /** @brief Load RSA keypair from QSettings, or generate and persist a new one. */
    void loadOrGenerateKeypair();

    /** @brief Encrypt plaintext for 'to' using their cached public key, send, and persist. */
    void encryptAndSend(const QString& to, const QString& plaintext);

    /**
     * @brief Load local message history for _currentUsername from QSettings.
     *
     * Emits historySyncMessage for each stored entry and populates _messageKeys.
     * Must be called after _currentUsername is set.
     */
    void loadLocalHistory();

    /**
     * @brief Persist one message to QSettings and track its dedup key.
     *
     * No-op if the dedup key is already in _messageKeys.
     *
     * @param peer      Conversation key (peer username).
     * @param sender    Display sender name.
     * @param content   Plaintext content.
     * @param timestamp ISO 8601 timestamp.
     * @param isOutgoing True if sent by local user.
     */
    void persistMessage(const QString& peer, const QString& sender, const QString& content,
                        const QString& timestamp, bool isOutgoing);

    /**
     * @brief Returns true if this message is already tracked in _messageKeys.
     *
     * Key format: peer + "|" + sender + "|" + timestamp
     */
    bool isDuplicate(const QString& peer, const QString& sender, const QString& timestamp) const;

    QWebSocket _socket;
    QUrl _serverUrl;
    QString _lastMessage;
    QString _currentUsername;
    bool _hasError = false;

    CryptoEngine _crypto;
    QByteArray _ownPrivKey;
    QByteArray _ownPubKey;
    QMap<QString, QByteArray> _peerKeys;
    QMap<QString, QList<QString>> _pendingMessages;
    QSet<QString> _messageKeys;
    bool _historyLoaded = false;
};

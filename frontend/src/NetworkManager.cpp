#include "NetworkManager.h"

#include <QDateTime>
#include <QDebug>
#include <QLoggingCategory>
#include <QSslConfiguration>
#include <QTimer>
#include <sstream>

Q_LOGGING_CATEGORY(logNetwork, "app.network")
Q_LOGGING_CATEGORY(logChat, "app.chat")

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    connect(&_socket, &QWebSocket::connected, this, &NetworkManager::onConnected);
    connect(&_socket, &QWebSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(&_socket, &QWebSocket::textMessageReceived, this,
            &NetworkManager::onTextMessageReceived);
    connect(&_socket, &QWebSocket::sslErrors, this, &NetworkManager::onSslErrors);
    connect(&_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError e) {
        qCWarning(logNetwork) << "Socket error:" << e << _socket.errorString();
    });

    if (QNetworkInformation::loadDefaultBackend()) {
        auto* netInfo = QNetworkInformation::instance();
        connect(netInfo, &QNetworkInformation::reachabilityChanged, this,
                &NetworkManager::onReachabilityChanged);
    } else {
        qCWarning(logNetwork) << "Could not load QNetworkInformation backend";
    }
}

void NetworkManager::connectToServer(const QUrl& url) {
    QSslConfiguration sslConfig = _socket.sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    _socket.setSslConfiguration(sslConfig);

    _serverUrl = url;
    _intentionallyConnecting = true;
    qCInfo(logNetwork) << "Connecting to" << url;
    _socket.open(url);
}

void NetworkManager::sendRegister(const QString& username, const QString& password) {
    _pendingRegister = true;
    Packet p;
    p.type = PacketType::REGISTER;
    p.from = username.toStdString();
    p.body = password.toStdString();
    sendPacket(p);
}

void NetworkManager::sendLogin(const QString& username, const QString& password) {
    _pendingRegister = false;
    Packet p;
    p.type = PacketType::LOGIN;
    p.from = username.toStdString();
    p.body = password.toStdString();
    sendPacket(p);
}

void NetworkManager::sendMessage(const QString& to, const QString& plaintext,
                                 const QString& timestamp) {
    if (!_peerKeys.contains(to)) {
        _pendingMessages[to].append(qMakePair(plaintext, timestamp));
        fetchPeerKey(to);
        return;
    }
    encryptAndSend(to, plaintext, timestamp);
}

void NetworkManager::fetchPeerKey(const QString& username) {
    Packet p;
    p.type = PacketType::KEY_EXCHANGE;
    p.from = _currentUsername.toStdString();
    p.to = username.toStdString();
    sendPacket(p);
}

void NetworkManager::sendSyncHistory() {
    Packet p;
    p.type = PacketType::SYNC_HISTORY;
    p.from = _currentUsername.toStdString();
    p.body = LocalStorage::instance().newestTimestamp().toStdString();
    sendPacket(p);
}

void NetworkManager::markConversationRead(const QString& peer) {
    if (_currentUsername.isEmpty() || !isConnected() || !_unreadTimestamps.contains(peer)) return;
    const QStringList timestamps = _unreadTimestamps.take(peer);
    for (const QString& ts : timestamps) {
        Packet p;
        p.type = PacketType::READ_RECEIPT;
        p.from = _currentUsername.toStdString();
        p.to = peer.toStdString();
        p.body = ts.toStdString();
        sendPacket(p);
    }
}

void NetworkManager::encryptAndSend(const QString& to, const QString& plaintext,
                                    const QString& timestamp) {
    try {
        QByteArray aesKey = _crypto.generateAESKey();
        QString encBody = _crypto.encrypt(plaintext, aesKey);
        QByteArray wrappedKey = _crypto.encryptRSA(aesKey, _peerKeys[to]);

        const QString ts = timestamp.isEmpty()
                               ? QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
                               : timestamp;

        Packet p;
        p.type = PacketType::MESSAGE;
        p.from = _currentUsername.toStdString();
        p.to = to.toStdString();
        p.body = encBody.toStdString();
        p.key = wrappedKey.toBase64().toStdString();
        p.timestamp = ts.toStdString();
        sendPacket(p);

        persistMessage(to, _currentUsername, plaintext, ts, true);
    } catch (const std::exception& e) {
        _hasError = true;
        _lastMessage = "Encrypt failed: " + QString::fromUtf8(e.what());
        emit lastMessageChanged();
    }
}

void NetworkManager::loadOrGenerateKeypair() {
    QSettings settings("BitATM", "BitATM");
    QByteArray pub = settings.value("crypto/pubkey").toByteArray();
    QByteArray priv = settings.value("crypto/privkey").toByteArray();

    if (!pub.isEmpty() && !priv.isEmpty()) {
        _ownPubKey = pub;
        _ownPrivKey = priv;
        qCInfo(logNetwork) << "Loaded existing RSA keypair";
    } else {
        auto [pubPem, privPem] = _crypto.generateRSAKeypair();
        _ownPubKey = pubPem;
        _ownPrivKey = privPem;
        settings.setValue("crypto/pubkey", _ownPubKey);
        settings.setValue("crypto/privkey", _ownPrivKey);
        qCInfo(logNetwork) << "Generated new RSA keypair";
    }
}

void NetworkManager::loadLocalHistory() {
    auto convs = LocalStorage::instance().loadConversations();
    for (auto& c : convs) {
        auto msgs = LocalStorage::instance().loadMessages(c.peer);
        for (auto& m : msgs) {
            emit historySyncMessage(m.peer, m.sender, m.content, m.timestamp, m.isOutgoing);
        }
        emit convListUpdated(c.peer, c.lastMessage, c.lastTimestamp);
    }
    qCInfo(logChat) << "Loaded local history:" << convs.size() << "conversations";
}

void NetworkManager::persistMessage(const QString& peer, const QString& sender,
                                    const QString& content, const QString& timestamp,
                                    bool isOutgoing) {
    LocalStorage::instance().saveMessage(peer, sender, content, timestamp, isOutgoing);
    LocalStorage::instance().saveConversation(peer, content, timestamp);
}

void NetworkManager::handleLoginAck(const Packet& p) {
    _hasError = false;
    _currentUsername = QString::fromStdString(p.to);
    _lastMessage = "Logged in as " + _currentUsername;
    emit currentUsernameChanged();

    if (_pendingRegister) {
        QSettings settings("BitATM", "BitATM");
        settings.remove("history/" + _currentUsername);
        _pendingRegister = false;
    }

    try {
        loadOrGenerateKeypair();
        Packet keyUp;
        keyUp.type = PacketType::KEY_EXCHANGE;
        keyUp.from = p.to;
        keyUp.key = _ownPubKey.toBase64().toStdString();
        sendPacket(keyUp);
    } catch (const std::exception& e) {
        _hasError = true;
        _lastMessage = QString("Key setup failed: ") + e.what();
        emit lastMessageChanged();
        return;
    }

    loadLocalHistory();
    sendSyncHistory();
}

void NetworkManager::handleIncomingMessage(const Packet& p) {
    const bool isOutgoingEcho = (p.errorMsg == "1");

    const QString ts = p.timestamp.empty()
                           ? QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
                           : QString::fromStdString(p.timestamp);

    if (isOutgoingEcho) {
        // Cannot decrypt - AES key is wrapped with recipient's public key, not ours.
        // Check local cache first (message may already be stored if sent from this device).
        const QString peer = QString::fromStdString(p.to);
        if (LocalStorage::instance().isDuplicate(peer, _currentUsername, ts)) return;
        // Cross-device outgoing: store as a placeholder since we cannot decrypt.
        persistMessage(peer, _currentUsername, "[Sent]", ts, true);
        emit historySyncMessage(peer, _currentUsername, "[Sent]", ts, true);
        return;
    }

    if (_ownPrivKey.isEmpty()) {
        qCWarning(logNetwork) << "Received MESSAGE but private key not loaded - dropping";
        return;
    }
    try {
        QByteArray wrappedKey = QByteArray::fromBase64(QByteArray::fromStdString(p.key));
        QByteArray aesKey = _crypto.decryptRSA(wrappedKey, _ownPrivKey);
        QString plaintext = _crypto.decrypt(QString::fromStdString(p.body), aesKey);

        const QString from = QString::fromStdString(p.from);
        if (LocalStorage::instance().isDuplicate(from, from, ts)) {
            qCInfo(logChat) << "Skipping duplicate message from" << from;
            return;
        }

        persistMessage(from, from, plaintext, ts, false);
        _unreadTimestamps[from].append(ts);
        emit messageDecrypted(from, plaintext, ts);
    } catch (const std::exception& e) {
        qCWarning(logChat) << "Failed to decrypt incoming message:" << e.what();
    }
}

void NetworkManager::handleReadReceipt(const Packet& p) {
    emit messageSeen(QString::fromStdString(p.from), QString::fromStdString(p.body));
}

void NetworkManager::handleKeyExchangeResponse(const Packet& p) {
    QString peerUsername = QString::fromStdString(p.from);
    QByteArray pubKey = QByteArray::fromBase64(QByteArray::fromStdString(p.key));
    _peerKeys[peerUsername] = pubKey;
    emit peerKeyReceived(peerUsername, QString::fromUtf8(pubKey));

    if (_pendingMessages.contains(peerUsername)) {
        const QList<QPair<QString, QString>> pending = _pendingMessages.take(peerUsername);
        for (const auto& [msg, ts] : pending) {
            encryptAndSend(peerUsername, msg, ts);
        }
    }
}

void NetworkManager::logout() {
    _currentUsername.clear();
    _ownPrivKey.clear();
    _ownPubKey.clear();
    _peerKeys.clear();
    _pendingMessages.clear();
    _unreadTimestamps.clear();
    emit currentUsernameChanged();
}

void NetworkManager::sendPacket(const Packet& packet) {
    if (!isConnected()) {
        _hasError = true;
        _lastMessage = "Not connected to server";
        emit lastMessageChanged();
        return;
    }
    std::ostringstream os;
    os << static_cast<int>(packet.type) << "|" << packet.version << "|" << packet.from << "|"
       << packet.to << "|" << packet.body << "|" << packet.key << "|" << packet.timestamp << "|"
       << packet.errorMsg;
    _socket.sendTextMessage(QString::fromStdString(os.str()));
}

bool NetworkManager::isConnected() const {
    return _socket.state() == QAbstractSocket::ConnectedState;
}

QString NetworkManager::lastMessage() const { return _lastMessage; }

bool NetworkManager::hasError() const { return _hasError; }

QString NetworkManager::currentUsername() const { return _currentUsername; }

void NetworkManager::onConnected() {
    _intentionallyConnecting = false;
    qCInfo(logNetwork) << "Connected to server";
    emit connectionChanged();
    emit connected();
}

void NetworkManager::onDisconnected() {
    _intentionallyConnecting = false;
    qCInfo(logNetwork) << "Disconnected from server";
    emit connectionChanged();
    emit disconnected();

    if (!_serverUrl.isEmpty()) {
        QTimer::singleShot(2000, this, [this]() {
            if (!isConnected()) {
                qCInfo(logNetwork) << "Attempting reconnect to" << _serverUrl;
                _socket.open(_serverUrl);
            }
        });
    }
}

void NetworkManager::onTextMessageReceived(const QString& message) {
    std::istringstream ss(message.toStdString());
    Packet p;
    ss >> p;

    if (ss.fail()) {
        _hasError = false;
        _lastMessage = message;
        emit lastMessageChanged();
        emit messageReceived(message);
        return;
    }

    switch (p.type) {
        case PacketType::ERR:
            _hasError = true;
            _lastMessage = QString::fromStdString(p.errorMsg);
            emit lastMessageChanged();
            break;

        case PacketType::ACK:
            _hasError = false;
            if (!p.body.empty()) {
                handleLoginAck(p);
            } else if (!p.timestamp.empty()) {
                emit messageDelivered(QString::fromStdString(p.timestamp));
            } else if (p.to == _currentUsername.toStdString()) {
                emit syncComplete();
            }
            emit lastMessageChanged();
            break;

        case PacketType::MESSAGE:
            handleIncomingMessage(p);
            break;

        case PacketType::KEY_EXCHANGE:
            handleKeyExchangeResponse(p);
            break;

        case PacketType::READ_RECEIPT:
            handleReadReceipt(p);
            break;

        default:
            break;
    }

    emit messageReceived(message);
}

void NetworkManager::onSslErrors(const QList<QSslError>& errors) {
    for (const QSslError& e : errors) {
        qCWarning(logNetwork) << "SSL error:" << e.errorString();
    }
    if (!errors.isEmpty()) {
        // Allow the connection to proceed despite SSL errors. On Android the vcpkg
        // OpenSSL instance does not load the Android system trust store, so valid
        // server certs (e.g. Let's Encrypt) are reported as untrusted.
        _socket.ignoreSslErrors();
        _hasError = true;
        _lastMessage = errors.first().errorString();
        emit lastMessageChanged();
        emit errorOcurred(_lastMessage);
    }
}

void NetworkManager::onReachabilityChanged(QNetworkInformation::Reachability reachability) {
    qCInfo(logNetwork) << "Network reachability changed:" << reachability;
    if (reachability != QNetworkInformation::Reachability::Online) {
        _socket.close();
    } else if (!isConnected() && !_serverUrl.isEmpty() && !_intentionallyConnecting) {
        qCInfo(logNetwork) << "Network back online, reconnecting...";
        _socket.open(_serverUrl);
    }
}

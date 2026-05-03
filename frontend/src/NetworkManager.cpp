#include "NetworkManager.h"

#include <QDebug>
#include <sstream>

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    connect(&_socket, &QWebSocket::connected, this, &NetworkManager::onConnected);
    connect(&_socket, &QWebSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(&_socket, &QWebSocket::textMessageReceived, this,
            &NetworkManager::onTextMessageReceived);
    connect(&_socket, &QWebSocket::sslErrors, this, &NetworkManager::onSslErrors);

    if (QNetworkInformation::loadDefaultBackend()) {
        auto* netInfo = QNetworkInformation::instance();
        connect(netInfo, &QNetworkInformation::reachabilityChanged, this,
                &NetworkManager::onReachabilityChanged);
    } else {
        qWarning() << "Could not load QNetworkInformation backend";
    }
}

void NetworkManager::connectToServer(const QUrl& url) {
    _serverUrl = url;
    qInfo() << "Connecting to" << url;
    _socket.open(url);
}

void NetworkManager::sendRegister(const QString& username, const QString& password) {
    Packet p;
    p.type = PacketType::REGISTER;
    p.from = username.toStdString();
    p.body = password.toStdString();
    sendPacket(p);
}

void NetworkManager::sendLogin(const QString& username, const QString& password) {
    Packet p;
    p.type = PacketType::LOGIN;
    p.from = username.toStdString();
    p.body = password.toStdString();
    sendPacket(p);
}

void NetworkManager::sendMessage(const QString& to, const QString& plaintext) {
    if (!_peerKeys.contains(to)) {
        _pendingMessages[to].append(plaintext);
        fetchPeerKey(to);
        return;
    }
    encryptAndSend(to, plaintext);
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
    sendPacket(p);
}

void NetworkManager::encryptAndSend(const QString& to, const QString& plaintext) {
    try {
        QByteArray aesKey = _crypto.generateAESKey();
        QString encBody = _crypto.encrypt(plaintext, aesKey);
        QByteArray wrappedKey = _crypto.encryptRSA(aesKey, _peerKeys[to]);

        Packet p;
        p.type = PacketType::MESSAGE;
        p.from = _currentUsername.toStdString();
        p.to = to.toStdString();
        p.body = encBody.toStdString();
        p.key = wrappedKey.toBase64().toStdString();
        sendPacket(p);
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
        qInfo() << "Loaded existing RSA keypair";
    } else {
        auto [pubPem, privPem] = _crypto.generateRSAKeypair();
        _ownPubKey = pubPem;
        _ownPrivKey = privPem;
        settings.setValue("crypto/pubkey", _ownPubKey);
        settings.setValue("crypto/privkey", _ownPrivKey);
        qInfo() << "Generated new RSA keypair";
    }
}

void NetworkManager::handleLoginAck(const Packet& p) {
    _hasError = false;
    _currentUsername = QString::fromStdString(p.to);
    _lastMessage = "Logged in as " + _currentUsername;
    emit currentUsernameChanged();

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
    sendSyncHistory();
}

void NetworkManager::handleIncomingMessage(const Packet& p) {
    if (_ownPrivKey.isEmpty()) {
        qWarning() << "Received MESSAGE but private key not loaded - dropping";
        return;
    }
    try {
        QByteArray wrappedKey = QByteArray::fromBase64(QByteArray::fromStdString(p.key));
        QByteArray aesKey = _crypto.decryptRSA(wrappedKey, _ownPrivKey);
        QString plaintext = _crypto.decrypt(QString::fromStdString(p.body), aesKey);

        emit messageDecrypted(QString::fromStdString(p.from), plaintext,
                              QString::fromStdString(p.timestamp));
    } catch (const std::exception& e) {
        qWarning() << "Failed to decrypt incoming message:" << e.what();
    }
}

void NetworkManager::handleKeyExchangeResponse(const Packet& p) {
    QString peerUsername = QString::fromStdString(p.from);
    QByteArray pubKey = QByteArray::fromBase64(QByteArray::fromStdString(p.key));
    _peerKeys[peerUsername] = pubKey;
    emit peerKeyReceived(peerUsername, QString::fromUtf8(pubKey));

    if (_pendingMessages.contains(peerUsername)) {
        const QList<QString> pending = _pendingMessages.take(peerUsername);
        for (const QString& msg : pending) {
            encryptAndSend(peerUsername, msg);
        }
    }
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
    qInfo() << "Connected to server";
    emit connectionChanged();
    emit connected();
}

void NetworkManager::onDisconnected() {
    qInfo() << "Disconnected from server";
    emit connectionChanged();
    emit disconnected();
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
                // Login ACK - body carries session token, to carries username
                handleLoginAck(p);
            } else if (p.to == _currentUsername.toStdString()) {
                // SYNC_HISTORY ACK
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

        default:
            break;
    }

    emit messageReceived(message);
}

void NetworkManager::onSslErrors(const QList<QSslError>& errors) {
    for (const QSslError& e : errors) {
        qWarning() << "SSL error:" << e.errorString();
    }
    if (!errors.isEmpty()) {
        _hasError = true;
        _lastMessage = errors.first().errorString();
        emit lastMessageChanged();
        emit errorOcurred(_lastMessage);
    }
}

void NetworkManager::onReachabilityChanged(QNetworkInformation::Reachability reachability) {
    qInfo() << "Network reachability changed:" << reachability;
    if (reachability != QNetworkInformation::Reachability::Online) {
        _socket.close();
    } else if (!isConnected() && !_serverUrl.isEmpty()) {
        qInfo() << "Network back online, reconnecting...";
        _socket.open(_serverUrl);
    }
}

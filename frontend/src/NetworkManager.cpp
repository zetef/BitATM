#include "NetworkManager.h"

#include <QDateTime>
#include <QDebug>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSslConfiguration>
#include <QTimer>
#include <sstream>

#include "TimestampUtil.h"

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

    // Keepalive: the server drops sockets silent for WS_RECEIVE_TIMEOUT_SEC,
    // so ping while connected to keep idle conversations alive.
    _pingTimer.setInterval(WS_CLIENT_PING_INTERVAL_SEC * 1000);
    connect(&_pingTimer, &QTimer::timeout, &_socket, [this]() { _socket.ping(); });

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
    const QString ts = LocalStorage::instance().newestTimestamp();
    if (QDateTime::fromString(ts, Qt::ISODateWithMs).isValid() ||
        QDateTime::fromString(ts, Qt::ISODate).isValid()) {
        p.body = ts.toStdString();
    }
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
    if (to == _currentUsername) return;
    try {
        QByteArray aesKey = _crypto.generateAESKey();
        QString encBody = _crypto.encrypt(plaintext, aesKey);
        QByteArray wrappedForRecipient = _crypto.encryptRSA(aesKey, _peerKeys[to]);
        QByteArray wrappedForSelf = _crypto.encryptRSA(aesKey, _ownPubKey);

        const QString ts = timestamp.isEmpty()
                               ? QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
                               : timestamp;

        Packet p;
        p.type = PacketType::MESSAGE;
        p.from = _currentUsername.toStdString();
        p.to = to.toStdString();
        p.body = encBody.toStdString();
        p.key = (QString::fromLatin1(wrappedForRecipient.toBase64()) + ";" +
                 QString::fromLatin1(wrappedForSelf.toBase64()))
                    .toStdString();
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
    // Keys are scoped per username so different accounts don't overwrite each other
    const QString pubKey = "crypto/" + _currentUsername + "/pubkey";
    const QString privKey = "crypto/" + _currentUsername + "/privkey";
    QByteArray pub = settings.value(pubKey).toByteArray();
    QByteArray priv = settings.value(privKey).toByteArray();

    if (!pub.isEmpty() && !priv.isEmpty()) {
        _ownPubKey = pub;
        _ownPrivKey = priv;
        qCInfo(logNetwork) << "Loaded existing RSA keypair for" << _currentUsername;
    } else {
        auto [pubPem, privPem] = _crypto.generateRSAKeypair();
        _ownPubKey = pubPem;
        _ownPrivKey = privPem;
        settings.setValue(pubKey, _ownPubKey);
        settings.setValue(privKey, _ownPrivKey);
        qCInfo(logNetwork) << "Generated new RSA keypair for" << _currentUsername;
    }
}

void NetworkManager::loadLocalHistory() {
    auto convs = LocalStorage::instance().loadConversations();
    for (auto& c : convs) {
        if (c.peer == _currentUsername) continue;  // skip any stale self-conversation rows
        auto msgs = LocalStorage::instance().loadMessages(c.peer);
        for (auto& m : msgs) {
            emit historySyncMessage(m.peer, m.sender, m.content, m.timestamp, m.isOutgoing,
                                    m.status);
        }
        emit convListUpdated(c.peer, c.lastMessage, c.lastTimestamp, c.unreadCount);
    }
    qCInfo(logChat) << "Loaded local history:" << convs.size() << "conversations";

    auto groups = LocalStorage::instance().loadGroups();
    for (auto& g : groups) {
        _groupNames[g.groupId] = g.name;
        const QString storedKey = LocalStorage::instance().loadGroupKey(g.groupId);
        if (!storedKey.isEmpty()) {
            _groupKeys[g.groupId] = QByteArray::fromBase64(storedKey.toLatin1());
        }
        auto msgs = LocalStorage::instance().loadGroupMessages(g.groupId);
        for (auto& m : msgs) {
            emit groupHistorySyncMessage(m.groupId, m.sender, m.content, m.timestamp, m.isOutgoing,
                                         m.status);
        }
        emit groupConvUpdated(g.groupId, g.name, g.lastMessage, g.lastTimestamp, g.unreadCount);
    }
    qCInfo(logChat) << "Loaded local group history:" << groups.size() << "groups";
}

void NetworkManager::persistMessage(const QString& peer, const QString& sender,
                                    const QString& content, const QString& timestamp,
                                    bool isOutgoing) {
    LocalStorage::instance().saveMessage(peer, sender, content, timestamp, isOutgoing);
    LocalStorage::instance().saveConversation(peer, content, timestamp);
}

void NetworkManager::handleLoginAck(const Packet& p) {
    _hasError = false;
    const QString previousUsername = _currentUsername;
    _currentUsername = QString::fromStdString(p.to);
    _lastMessage = "Logged in as " + _currentUsername;
    emit currentUsernameChanged();

    if (!previousUsername.isEmpty() && previousUsername != _currentUsername) {
        LocalStorage::instance().close();
    }
    if (!LocalStorage::instance().openForUser(_currentUsername)) {
        qCWarning(logNetwork) << "Failed to open local cache for" << _currentUsername
                              << "- history will not persist";
    }

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
                           : TimestampUtil::canonical(QString::fromStdString(p.timestamp));

    const QString keyField = QString::fromStdString(p.key);
    const int keySep = keyField.indexOf(';');

    if (isOutgoingEcho) {
        const QString peer = QString::fromStdString(p.to);
        if (peer == _currentUsername) return;  // ignore self-messages
        if (LocalStorage::instance().isDuplicate(peer, _currentUsername, ts)) return;

        // For real-time sibling echo, key field has "recipientKey;senderKey"
        // For sync replay, SyncHistoryHandler sends just the senderKey directly
        // In both cases: if there's a ';', the sender key is the second segment;
        // if no ';', treat the whole field as the sender key (sync path)
        const QString senderKeyB64 = (keySep >= 0) ? keyField.mid(keySep + 1) : keyField;

        if (senderKeyB64.isEmpty() || _ownPrivKey.isEmpty()) {
            // Legacy row (no sender key stored before migration)
            persistMessage(peer, _currentUsername, "[Sent]", ts, true);
            emit historySyncMessage(peer, _currentUsername, "[Sent]", ts, true);
            return;
        }
        try {
            QByteArray senderKey = QByteArray::fromBase64(senderKeyB64.toLatin1());
            QByteArray aesKey = _crypto.decryptRSA(senderKey, _ownPrivKey);
            QString plaintext = _crypto.decrypt(QString::fromStdString(p.body), aesKey);
            persistMessage(peer, _currentUsername, plaintext, ts, true);
            emit historySyncMessage(peer, _currentUsername, plaintext, ts, true);
        } catch (const std::exception& e) {
            qCWarning(logChat) << "Failed to decrypt outgoing sync message:" << e.what();
            persistMessage(peer, _currentUsername, "[Sent]", ts, true);
            emit historySyncMessage(peer, _currentUsername, "[Sent]", ts, true);
        }
        return;
    }

    if (_ownPrivKey.isEmpty()) {
        qCWarning(logNetwork) << "Received MESSAGE but private key not loaded - dropping";
        return;
    }
    try {
        // Recipient key is the first segment; strip the sender-key segment if present
        const QString recipientKeyB64 = (keySep >= 0) ? keyField.left(keySep) : keyField;
        QByteArray wrappedKey = QByteArray::fromBase64(recipientKeyB64.toLatin1());
        QByteArray aesKey = _crypto.decryptRSA(wrappedKey, _ownPrivKey);
        QString plaintext = _crypto.decrypt(QString::fromStdString(p.body), aesKey);

        const QString from = QString::fromStdString(p.from);
        if (from == _currentUsername) return;  // ignore self-messages
        if (LocalStorage::instance().isDuplicate(from, from, ts)) {
            qCInfo(logChat) << "Skipping duplicate message from" << from;
            return;
        }

        persistMessage(from, from, plaintext, ts, false);
        _unreadTimestamps[from].append(ts);
        emit messageDecrypted(from, plaintext, ts);
    } catch (const std::exception& e) {
        qCWarning(logChat) << "Failed to decrypt incoming message:" << e.what();
        const QString from = QString::fromStdString(p.from);
        // Undecryptable replays must dedup too, or every sync appends
        // another "[Encrypted]" row to the UI
        if (from != _currentUsername && !LocalStorage::instance().isDuplicate(from, from, ts)) {
            persistMessage(from, from, "[Encrypted]", ts, false);
            emit messageDecrypted(from, "[Encrypted]", ts);
        }
    }
}

void NetworkManager::handleReadReceipt(const Packet& p) {
    const QString ts = TimestampUtil::canonical(QString::fromStdString(p.body));
    // Group receipts carry the group id in 'to'; the conversation key is the
    // group id, not the reader.
    const QString to = QString::fromStdString(p.to);
    static const QRegularExpression numeric("^\\d+$");
    if (numeric.match(to).hasMatch()) {
        const QString reader = QString::fromStdString(p.from);
        // v2: per-member row update; the aggregate decides when the bubble
        // flips. First-receipt-wins v1 behavior is gone.
        LocalStorage::instance().markRecipientSeen(to, ts, reader);
        emit groupReceiptUpdated(to, ts);
        recomputeGroupAggregate(to, ts);
        return;
    }
    emit messageSeen(QString::fromStdString(p.from), ts);
}

void NetworkManager::handleGroupSendAck(const Packet& p) {
    const QString groupId = QString::fromStdString(p.errorMsg);
    const QString ts = TimestampUtil::canonical(QString::fromStdString(p.timestamp));
    const QStringList members = QString::fromStdString(p.key).split(";", Qt::SkipEmptyParts);
    if (!members.isEmpty()) LocalStorage::instance().saveRecipientSnapshot(groupId, ts, members);
}

void NetworkManager::handleGroupDelivered(const Packet& p) {
    const QString groupId = QString::fromStdString(p.to);
    const QString member = QString::fromStdString(p.from);
    const QString ts = TimestampUtil::canonical(QString::fromStdString(p.body));
    LocalStorage::instance().markRecipientDelivered(groupId, ts, member);
    emit groupReceiptUpdated(groupId, ts);
    recomputeGroupAggregate(groupId, ts);
}

void NetworkManager::recomputeGroupAggregate(const QString& groupId, const QString& ts) {
    auto& ls = LocalStorage::instance();
    if (ls.recipientCount(groupId, ts) == 0) return;  // legacy message or empty group
    if (ls.allRecipientsSeen(groupId, ts)) {
        ls.updateGroupMessageStatus(groupId, ts, "seen");
        emit messageSeen(groupId, ts);
    } else if (ls.allRecipientsDelivered(groupId, ts)) {
        ls.updateGroupMessageStatus(groupId, ts, "delivered");
        emit groupMessageDelivered(groupId, ts);
    }
}

QVariantList NetworkManager::groupMessageReceipts(const QString& groupId,
                                                  const QString& timestamp) {
    return LocalStorage::instance().recipientStates(groupId, timestamp);
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

    if (_pendingGroupMemberAdds.contains(peerUsername)) {
        const QStringList groupIds = _pendingGroupMemberAdds.take(peerUsername);
        for (const QString& gid : groupIds) {
            addGroupMember(gid, peerUsername);
        }
    }

    for (auto& pg : _pendingGroupCreations) {
        if (pg.waitingFor.contains(peerUsername)) {
            pg.waitingFor.removeOne(peerUsername);
            try {
                pg.collectedKeys[peerUsername] = _crypto.encryptRSA(pg.aesKey, pubKey);
            } catch (const std::exception& e) {
                qWarning() << "createGroup key encrypt failed for" << peerUsername << e.what();
            }
        }
    }
    _pendingGroupCreations.erase(
        std::remove_if(_pendingGroupCreations.begin(), _pendingGroupCreations.end(),
                       [this](const PendingGroupCreate& pg) {
                           if (pg.waitingFor.isEmpty()) {
                               sendCreateGroupPacket(pg);
                               return true;
                           }
                           return false;
                       }),
        _pendingGroupCreations.end());
}

void NetworkManager::logout() {
    LocalStorage::instance().close();
    _currentUsername.clear();
    _ownPrivKey.clear();
    _ownPubKey.clear();
    _peerKeys.clear();
    _pendingMessages.clear();
    _unreadTimestamps.clear();
    _groupKeys.clear();
    emit currentUsernameChanged();
}

void NetworkManager::sendCreateGroupPacket(const PendingGroupCreate& pending) {
    QStringList keyParts;
    for (auto it = pending.collectedKeys.constBegin(); it != pending.collectedKeys.constEnd();
         ++it) {
        keyParts.append(it.key() + ":" + QString::fromLatin1(it.value().toBase64()));
    }
    Packet p;
    p.type = PacketType::CREATE_GROUP;
    p.from = _currentUsername.toStdString();
    p.errorMsg = pending.name.toStdString();
    p.body = pending.members.join(",").toStdString();
    p.key = keyParts.join(";").toStdString();
    sendPacket(p);
}

void NetworkManager::createGroup(const QString& name, const QStringList& members) {
    try {
        QByteArray aesKey = _crypto.generateAESKey();
        _groupKeys["0"] = aesKey;

        PendingGroupCreate pending;
        pending.name = name;
        pending.members = members;
        pending.aesKey = aesKey;

        if (!_ownPubKey.isEmpty()) {
            pending.collectedKeys[_currentUsername] = _crypto.encryptRSA(aesKey, _ownPubKey);
        }
        for (const QString& m : members) {
            if (_peerKeys.contains(m)) {
                pending.collectedKeys[m] = _crypto.encryptRSA(aesKey, _peerKeys[m]);
            } else {
                pending.waitingFor.append(m);
                fetchPeerKey(m);
            }
        }

        if (pending.waitingFor.isEmpty()) {
            sendCreateGroupPacket(pending);
        } else {
            _pendingGroupCreations.append(std::move(pending));
        }
    } catch (const std::exception& e) {
        _hasError = true;
        _lastMessage = "createGroup failed: " + QString::fromUtf8(e.what());
        emit lastMessageChanged();
    }
}

void NetworkManager::requestGroupKey(const QString& groupId) {
    if (_pendingGroupKeyRequests.contains(groupId)) return;
    _pendingGroupKeyRequests.insert(groupId);
    Packet p;
    p.type = PacketType::GROUP_KEY_EXCHANGE;
    p.from = _currentUsername.toStdString();
    p.to = groupId.toStdString();
    // empty key field = recover my own stored key from the server
    sendPacket(p);
}

void NetworkManager::sendGroupMessage(const QString& groupId, const QString& plaintext,
                                      const QString& timestamp) {
    if (!_groupKeys.contains(groupId)) {
        qCInfo(logChat) << "sendGroupMessage: no AES key for group" << groupId
                        << "- queuing and requesting key";
        _pendingGroupSends[groupId].append(qMakePair(plaintext, timestamp));
        requestGroupKey(groupId);
        return;
    }
    try {
        QByteArray aesKey = _groupKeys[groupId];
        QString ciphertext = _crypto.encrypt(plaintext, aesKey);
        const QString ts = timestamp.isEmpty()
                               ? QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
                               : timestamp;
        Packet p;
        p.type = PacketType::GROUP_MESSAGE;
        p.from = _currentUsername.toStdString();
        p.to = groupId.toStdString();
        p.body = ciphertext.toStdString();
        p.timestamp = ts.toStdString();
        sendPacket(p);
        LocalStorage::instance().saveGroupMessage(groupId, _currentUsername, plaintext, ts, true);
    } catch (const std::exception& e) {
        _hasError = true;
        _lastMessage = "sendGroupMessage failed: " + QString::fromUtf8(e.what());
        emit lastMessageChanged();
    }
}

void NetworkManager::kickMember(const QString& groupId, const QString& username) {
    Packet p;
    p.type = PacketType::GROUP_LEAVE;
    p.from = _currentUsername.toStdString();
    p.to = groupId.toStdString();
    p.body = username.toStdString();
    sendPacket(p);
}

void NetworkManager::addGroupMember(const QString& groupId, const QString& username) {
    if (!_peerKeys.contains(username)) {
        _pendingGroupMemberAdds[username].append(groupId);
        fetchPeerKey(username);
        return;
    }
    if (!_groupKeys.contains(groupId)) {
        qWarning() << "addGroupMember: no AES key for group" << groupId;
        return;
    }
    try {
        QByteArray aesKey = _groupKeys[groupId];
        QByteArray enc = _crypto.encryptRSA(aesKey, _peerKeys[username]);
        Packet p;
        p.type = PacketType::GROUP_KEY_EXCHANGE;
        p.from = _currentUsername.toStdString();
        p.to = groupId.toStdString();
        p.key = (username + ":" + QString::fromLatin1(enc.toBase64())).toStdString();
        sendPacket(p);
    } catch (const std::exception& e) {
        qWarning() << "addGroupMember failed:" << e.what();
    }
}

void NetworkManager::grantAdmin(const QString& groupId, const QString& username) {
    Packet p;
    p.type = PacketType::GROUP_INFO;
    p.from = _currentUsername.toStdString();
    p.to = groupId.toStdString();
    p.body = "grant_admin";
    p.key = username.toStdString();
    sendPacket(p);
}

void NetworkManager::revokeAdmin(const QString& groupId, const QString& username) {
    Packet p;
    p.type = PacketType::GROUP_INFO;
    p.from = _currentUsername.toStdString();
    p.to = groupId.toStdString();
    p.body = "revoke_admin";
    p.key = username.toStdString();
    sendPacket(p);
}

void NetworkManager::leaveGroup(const QString& groupId) {
    Packet p;
    p.type = PacketType::GROUP_LEAVE;
    p.from = _currentUsername.toStdString();
    p.to = groupId.toStdString();
    sendPacket(p);
}

void NetworkManager::deleteGroup(const QString& groupId) {
    Packet p;
    p.type = PacketType::DELETE_GROUP;
    p.from = _currentUsername.toStdString();
    p.to = groupId.toStdString();
    sendPacket(p);
}

void NetworkManager::deleteConversation(const QString& peer) {
    LocalStorage::instance().deleteConversation(peer);
    emit conversationDeleted(peer);
}

void NetworkManager::deleteConversationForEveryone(const QString& peer) {
    LocalStorage::instance().deleteConversation(peer);
    emit conversationDeleted(peer);

    Packet p;
    p.type = PacketType::DELETE_CONVERSATION;
    p.from = _currentUsername.toStdString();
    p.to = peer.toStdString();
    sendPacket(p);
}

void NetworkManager::fetchGroupInfo(const QString& groupId) {
    Packet p;
    p.type = PacketType::GROUP_INFO;
    p.from = _currentUsername.toStdString();
    p.to = groupId.toStdString();
    sendPacket(p);
}

void NetworkManager::handleGroupInvite(const Packet& p) {
    const QString groupId = QString::fromStdString(p.body);
    const QString groupName = QString::fromStdString(p.errorMsg);
    const QString encKey = QString::fromStdString(p.key);
    try {
        QByteArray aesKey =
            _crypto.decryptRSA(QByteArray::fromBase64(encKey.toLatin1()), _ownPrivKey);
        _groupKeys[groupId] = aesKey;
        _groupNames[groupId] = groupName;
        LocalStorage::instance().saveGroup(groupId, groupName, "member");
        LocalStorage::instance().saveGroupKey(groupId, QString::fromLatin1(aesKey.toBase64()));
        emit groupInviteReceived(groupId, groupName);

        // The backend now also replies with GROUP_INVITE (not a bare
        // GROUP_KEY_EXCHANGE) for key-recovery requests, so this handler must
        // mirror handleGroupKeyExchange()'s cleanup: clear the in-flight guard
        // and drain anything that was buffered waiting on this group's key.
        _pendingGroupKeyRequests.remove(groupId);
        emit groupKeyUpdated(groupId);
        if (_pendingGroupPackets.contains(groupId)) {
            const QList<Packet> buffered = _pendingGroupPackets.take(groupId);
            for (const Packet& bp : buffered) handleGroupMessage(bp);
        }
        if (_pendingGroupSends.contains(groupId)) {
            const QList<QPair<QString, QString>> sends = _pendingGroupSends.take(groupId);
            for (const auto& [msg, ts] : sends) sendGroupMessage(groupId, msg, ts);
        }
    } catch (const std::exception& e) {
        qWarning() << "handleGroupInvite: decrypt failed:" << e.what();
    }
}

void NetworkManager::handleGroupMessage(const Packet& p) {
    const QString groupId = QString::fromStdString(p.to);
    const QString sender = QString::fromStdString(p.from);
    const QString ts = p.timestamp.empty()
                           ? QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)
                           : TimestampUtil::canonical(QString::fromStdString(p.timestamp));
    try {
        QByteArray aesKey;
        if (_groupKeys.contains(groupId)) {
            aesKey = _groupKeys[groupId];
        } else {
            QString stored = LocalStorage::instance().loadGroupKey(groupId);
            if (stored.isEmpty()) {
                // Lost local key (cache wipe, new device): buffer the packet
                // and recover our wrapped key from the server
                qCInfo(logChat) << "handleGroupMessage: no key for group" << groupId
                                << "- buffering and requesting key";
                _pendingGroupPackets[groupId].append(p);
                requestGroupKey(groupId);
                return;
            }
            aesKey = QByteArray::fromBase64(stored.toLatin1());
            _groupKeys[groupId] = aesKey;
        }
        const bool isOutgoing = (sender == _currentUsername);
        if (!isOutgoing) {
            // v2: explicit per-member delivered receipt, live and sync paths.
            // Sent before the dedup return so a crash-after-persist still ACKs;
            // the server-side mark is idempotent.
            Packet ack;
            ack.type = PacketType::ACK;
            ack.from = _currentUsername.toStdString();
            ack.to = groupId.toStdString();
            ack.body = ts.toStdString();
            ack.errorMsg = "delivered";
            sendPacket(ack);
        }
        if (LocalStorage::instance().isGroupMessageDuplicate(groupId, sender, ts)) return;
        QString plaintext = _crypto.decrypt(QString::fromStdString(p.body), aesKey);
        LocalStorage::instance().saveGroupMessage(groupId, sender, plaintext, ts, isOutgoing);
        if (!isOutgoing) _unreadTimestamps[groupId].append(ts);
        emit groupMessageDecrypted(groupId, sender, plaintext, ts, isOutgoing);
    } catch (const std::exception& e) {
        qWarning() << "handleGroupMessage: decrypt failed:" << e.what();
    }
}

void NetworkManager::handleGroupKeyExchange(const Packet& p) {
    const QString groupId = QString::fromStdString(p.to);
    const QString encKey = QString::fromStdString(p.key);
    try {
        QByteArray aesKey =
            _crypto.decryptRSA(QByteArray::fromBase64(encKey.toLatin1()), _ownPrivKey);
        _groupKeys[groupId] = aesKey;
        LocalStorage::instance().saveGroupKey(groupId, QString::fromLatin1(aesKey.toBase64()));
        _pendingGroupKeyRequests.remove(groupId);
        emit groupKeyUpdated(groupId);

        // Drain packets and sends that were waiting for this key
        if (_pendingGroupPackets.contains(groupId)) {
            const QList<Packet> buffered = _pendingGroupPackets.take(groupId);
            for (const Packet& bp : buffered) handleGroupMessage(bp);
        }
        if (_pendingGroupSends.contains(groupId)) {
            const QList<QPair<QString, QString>> sends = _pendingGroupSends.take(groupId);
            for (const auto& [msg, ts] : sends) sendGroupMessage(groupId, msg, ts);
        }
    } catch (const std::exception& e) {
        qWarning() << "handleGroupKeyExchange: decrypt failed:" << e.what();
    }
}

void NetworkManager::handleGroupInfo(const Packet& p) {
    const QString groupId = QString::fromStdString(p.body);
    const QString groupName = QString::fromStdString(p.errorMsg);
    const QString memberStr = QString::fromStdString(p.key);

    // If sentinel key exists, a CREATE_GROUP was just acked - remap to the real group id
    if (_groupKeys.contains("0")) {
        _groupKeys[groupId] = _groupKeys.take("0");
        // Persist the creator's key; the invite path does this for members
        LocalStorage::instance().saveGroupKey(groupId,
                                              QString::fromLatin1(_groupKeys[groupId].toBase64()));
    }

    LocalStorage::instance().saveGroup(groupId, groupName, "creator");
    _groupNames[groupId] = groupName;

    QVariantList members;
    const QStringList entries = memberStr.split(";", Qt::SkipEmptyParts);
    for (const QString& entry : entries) {
        const int sep = entry.indexOf(":");
        if (sep < 0) continue;
        QVariantMap m;
        m["username"] = entry.left(sep);
        m["role"] = entry.mid(sep + 1);
        members.append(m);
    }

    // Receipts churn rule: drop local snapshot rows of ex-members, then
    // recompute any aggregates their departure may have completed
    QStringList usernames;
    for (const QVariant& mv : members) usernames << mv.toMap().value("username").toString();
    const QStringList affected = LocalStorage::instance().removeRecipientsNotIn(groupId, usernames);
    for (const QString& t : affected) {
        emit groupReceiptUpdated(groupId, t);
        recomputeGroupAggregate(groupId, t);
    }

    emit groupInfoReceived(groupId, groupName, members);
}

void NetworkManager::handleGroupLeave(const Packet& p) {
    if (p.body == "rotate") {
        // Server asks us to rotate the key - groupId is in p.key field
        rotateGroupKey(QString::fromStdString(p.key));
    } else {
        const QString groupId = QString::fromStdString(p.body);
        _groupKeys.remove(groupId);
        _groupNames.remove(groupId);
        LocalStorage::instance().removeGroup(groupId);
        emit groupLeft(groupId);
    }
}

void NetworkManager::handleDeleteConversation(const Packet& p) {
    const QString peer = QString::fromStdString(p.from);
    LocalStorage::instance().deleteConversation(peer);
    emit conversationDeleted(peer);
}

void NetworkManager::rotateGroupKey(const QString& groupId) {
    try {
        QByteArray newKey = _crypto.generateAESKey();
        _groupKeys[groupId] = newKey;
        LocalStorage::instance().saveGroupKey(groupId, QString::fromLatin1(newKey.toBase64()));
        // Fetch member list so QML can redistribute the new key via addGroupMember
        fetchGroupInfo(groupId);
    } catch (const std::exception& e) {
        qWarning() << "rotateGroupKey failed:" << e.what();
    }
}

QString NetworkManager::getGroupName(const QString& groupId) const {
    return _groupNames.value(groupId, groupId);
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
    _pingTimer.start();
    qCInfo(logNetwork) << "Connected to server";
    emit connectionChanged();
    emit connected();
}

void NetworkManager::onDisconnected() {
    _intentionallyConnecting = false;
    _pingTimer.stop();
    qCInfo(logNetwork) << "Disconnected from server";
    emit connectionChanged();
    emit disconnected();

    // The auto-reconnect below only reopens the raw socket - it never re-sends
    // LOGIN, so the server treats the new connection as a fresh, unauthenticated
    // ClientSession (its own disconnect handling already deactivated the old
    // session token). Without this, _currentUsername stayed set across the drop
    // and the UI kept behaving as if still logged in while every authenticated
    // request (send message, group key recovery) silently failed server-side
    // with no visible error. Clearing it forces a real re-login once reconnected.
    if (!_currentUsername.isEmpty()) {
        _currentUsername.clear();
        emit currentUsernameChanged();
    }

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
            if (p.errorMsg == "delivered") {
                handleGroupDelivered(p);
            } else if (!p.errorMsg.empty() && !p.timestamp.empty()) {
                handleGroupSendAck(p);  // errorMsg = group id, key = recipient list
            } else if (!p.body.empty()) {
                handleLoginAck(p);
            } else if (!p.timestamp.empty()) {
                emit messageDelivered(
                    TimestampUtil::canonical(QString::fromStdString(p.timestamp)));
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

        case PacketType::GROUP_INVITE:
            handleGroupInvite(p);
            break;

        case PacketType::GROUP_MESSAGE:
            handleGroupMessage(p);
            break;

        case PacketType::GROUP_KEY_EXCHANGE:
            handleGroupKeyExchange(p);
            break;

        case PacketType::GROUP_INFO:
            handleGroupInfo(p);
            break;

        case PacketType::GROUP_LEAVE:
            handleGroupLeave(p);
            break;

        case PacketType::DELETE_CONVERSATION:
            handleDeleteConversation(p);
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

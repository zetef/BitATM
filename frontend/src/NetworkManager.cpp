#include "NetworkManager.h"

#include <QDebug>

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

void NetworkManager::sendText(const QString& text) {
    if (!isConnected()) {
        _hasError = true;
        _lastMessage = "Not connected to server";
        emit lastMessageChanged();
        return;
    }
    _socket.sendTextMessage(text);
}

bool NetworkManager::isConnected() const {
    return _socket.state() == QAbstractSocket::ConnectedState;
}

QString NetworkManager::lastMessage() const { return _lastMessage; }

bool NetworkManager::hasError() const { return _hasError; }

void NetworkManager::onConnected() {
    qInfo() << "Connected to server";
    emit connectionChanged();
    emit connected();
    if (_firstConnect) {
        _firstConnect = false;
        sendText("Hello from Qt!");
    }
}

void NetworkManager::onDisconnected() {
    qInfo() << "Disconnected from server";
    emit connectionChanged();
    emit disconnected();
}

void NetworkManager::onTextMessageReceived(const QString& message) {
    qInfo() << "Echo received:" << message;
    _hasError = false;
    _lastMessage = message;
    emit lastMessageChanged();
    emit messageReceived(message);
}

void NetworkManager::onSslErrors(const QList<QSslError>& errors) {
    for (const QSslError& e : errors) {
        qWarning() << "SSL error:" << e.errorString();
    }
    _hasError = true;
    _lastMessage = errors.first().errorString();
    emit lastMessageChanged();
    emit errorOcurred(_lastMessage);
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

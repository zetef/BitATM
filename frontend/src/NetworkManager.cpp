#include "NetworkManager.h"

#include <QDebug>

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    connect(&_socket, &QWebSocket::connected, this, &NetworkManager::onConnected);
    connect(&_socket, &QWebSocket::disconnected, this, &NetworkManager::onDisconnected);
    connect(&_socket, &QWebSocket::textMessageReceived, this,
            &NetworkManager::onTextMessageReceived);
    connect(&_socket, &QWebSocket::sslErrors, this, &NetworkManager::onSslErrors);
}

void NetworkManager::connectToServer(const QUrl& url) {
    qInfo() << "Connecting to" << url;
    _socket.open(url);
}

void NetworkManager::sendText(const QString& text) { _socket.sendTextMessage(text); }

bool NetworkManager::isConnected() const {
    return _socket.state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::onConnected() {
    qInfo() << "Connected to server";
    emit connected();
    sendText("Hello from Qt!");
}

void NetworkManager::onDisconnected() {
    qInfo() << "Disconnected from server";
    emit disconnected();
}

void NetworkManager::onTextMessageReceived(const QString& message) {
    qInfo() << "Echo received:" << message;
    emit messageReceived(message);
}

void NetworkManager::onSslErrors(const QList<QSslError>& errors) {
    for (const QSslError& e : errors) {
        qWarning() << "SSL error:" << e.errorString();
    }
}

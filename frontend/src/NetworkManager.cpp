#include "NetworkManager.h"

#include <QDebug>

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    connect(&_socket, &QWebSocket::connected, this, &NetworkManager::onConnected);
    connect(&_socket, &QWebSocket::textMessageReceived, this,
            &NetworkManager::onTextMessageReceived);
}

void NetworkManager::connectToServer(const QUrl& url) {
    qInfo() << "Connecting to" << url;
    _socket.open(url);
}

void NetworkManager::sendText(const QString& text) { _socket.sendTextMessage(text); }

void NetworkManager::onConnected() {
    qInfo() << "Connected to server";
    emit connected();
    sendText("Hello from Qt!");
}

void NetworkManager::onTextMessageReceived(const QString& message) {
    qInfo() << "Echo received:" << message;
    emit messageReceived(message);
}
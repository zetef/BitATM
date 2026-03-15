#pragma once
#include <QObject>
#include <QString>
#include <QUrl>
#include <QWebSocket>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);
    void connectToServer(const QUrl& url);
    void sendText(const QString& text);

signals:
    void messageReceived(const QString& message);
    void connected();
    void disconnected();

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);

private:
    QWebSocket _socket;
};
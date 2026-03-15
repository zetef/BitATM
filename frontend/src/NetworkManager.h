#pragma once
#include <QList>
#include <QObject>
#include <QSslError>
#include <QString>
#include <QUrl>
#include <QWebSocket>

class NetworkManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connected)
public:
    explicit NetworkManager(QObject* parent = nullptr);
    void connectToServer(const QUrl& url);
    void sendText(const QString& text);
    bool isConnected() const;

signals:
    void messageReceived(const QString& message);
    void connected();
    void disconnected();
    void errorOcurred(const QString& errorString);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onSslErrors(const QList<QSslError>& errors);

private:
    QWebSocket _socket;
};
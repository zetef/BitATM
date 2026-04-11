#pragma once
#include <QList>
#include <QNetworkInformation>
#include <QObject>
#include <QSslError>
#include <QString>
#include <QUrl>
#include <QWebSocket>

#include "../../common/protocol.h"

class NetworkManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY lastMessageChanged)
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY currentUsernameChanged)
public:
    explicit NetworkManager(QObject* parent = nullptr);
    void connectToServer(const QUrl& url);

    Q_INVOKABLE void sendRegister(const QString& username, const QString& password);
    Q_INVOKABLE void sendLogin(const QString& username, const QString& password);

    bool isConnected() const;
    QString lastMessage() const;
    bool hasError() const;
    QString currentUsername() const;

signals:
    void messageReceived(const QString& message);
    void lastMessageChanged();
    void connectionChanged();
    void connected();
    void disconnected();
    void errorOcurred(const QString& errorString);
    void currentUsernameChanged();

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onSslErrors(const QList<QSslError>& errors);
    void onReachabilityChanged(QNetworkInformation::Reachability reachability);

private:
    void sendPacket(const Packet& packet);

    QWebSocket _socket;
    QUrl _serverUrl;
    QString _lastMessage;
    QString _currentUsername;
    bool _hasError = false;
};

#pragma once
#include <QList>
#include <QNetworkInformation>
#include <QObject>
#include <QSslError>
#include <QString>
#include <QUrl>
#include <QWebSocket>

class NetworkManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY lastMessageChanged)
public:
    explicit NetworkManager(QObject* parent = nullptr);
    void connectToServer(const QUrl& url);
    Q_INVOKABLE void sendText(const QString& text);
    bool isConnected() const;
    QString lastMessage() const;
    bool hasError() const;

signals:
    void messageReceived(const QString& message);
    void lastMessageChanged();
    void connectionChanged();
    void connected();
    void disconnected();
    void errorOcurred(const QString& errorString);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onSslErrors(const QList<QSslError>& errors);
    void onReachabilityChanged(QNetworkInformation::Reachability reachability);

private:
    QWebSocket _socket;
    QUrl _serverUrl;
    QString _lastMessage;
    bool _hasError = false;
    bool _firstConnect = true;
};
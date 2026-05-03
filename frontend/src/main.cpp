#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ChatModel.h"
#include "ConversationListModel.h"
#include "NetworkManager.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    NetworkManager net;
    ChatModel chatModel;
    ConversationListModel convListModel;

    // Incoming decrypted messages go into both the chat view and the sidebar.
    QObject::connect(&net, &NetworkManager::messageDecrypted,
                     [&](const QString& from, const QString& plaintext, const QString& timestamp) {
                         chatModel.appendMessage(from, plaintext, timestamp, false);
                         convListModel.addOrUpdate(from, plaintext, timestamp);
                     });

    net.connectToServer(QUrl(QStringLiteral(BITATM_SERVER_URL)));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("networkManager", &net);
    engine.rootContext()->setContextProperty("chatModel", &chatModel);
    engine.rootContext()->setContextProperty("convListModel", &convListModel);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/BitATM/qml/Main.qml")));
    return app.exec();
}

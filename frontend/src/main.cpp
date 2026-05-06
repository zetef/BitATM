#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ChatModel.h"
#include "ConversationListModel.h"
#include "LocalStorage.h"
#include "NetworkManager.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    if (!LocalStorage::instance().open()) {
        qWarning() << "Failed to open local message cache - history will not persist";
    }

    NetworkManager net;
    ChatModel chatModel;
    ConversationListModel convListModel;

    net.connectToServer(QUrl(QStringLiteral(BITATM_SERVER_URL)));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("networkManager", &net);
    engine.rootContext()->setContextProperty("chatModel", &chatModel);
    engine.rootContext()->setContextProperty("convListModel", &convListModel);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/BitATM/qml/Main.qml")));
    return app.exec();
}

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "NetworkManager.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    NetworkManager net;
    net.connectToServer(QUrl("wss://api.zetef.xyz"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("networkManager", &net);
    engine.load(QUrl(QStringLiteral("qrc:/qt/qml/BitATM/qml/Main.qml")));
    return app.exec();
}

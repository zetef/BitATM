#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "NetworkManager.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);

    NetworkManager net;
    net.connectToServer(QUrl("wss://api.zetef.xyz"));

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/BitATM/qml/Main.qml")));
    return app.exec();
}
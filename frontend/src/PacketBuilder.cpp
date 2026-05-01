#include "PacketBuilder.h"

#include <QDateTime>
#include <stdexcept>

PacketBuilder& PacketBuilder::setType(PacketType t) {
    type_ = t;
    return *this;
}

PacketBuilder& PacketBuilder::setFrom(const QString& from) {
    from_ = from;
    return *this;
}

PacketBuilder& PacketBuilder::setTo(const QString& to) {
    to_ = to;
    return *this;
}

PacketBuilder& PacketBuilder::setBody(const QString& body) {
    body_ = body;
    return *this;
}

PacketBuilder& PacketBuilder::setKey(const QString& key) {
    key_ = key;
    return *this;
}

Packet PacketBuilder::build() {
    if (!type_.has_value()) throw std::logic_error("PacketBuilder: type not set");

    Packet p;
    p.type = *type_;
    p.version = PROTOCOL_VERSION;
    p.from = from_.toStdString();
    p.to = to_.toStdString();
    p.body = body_.toStdString();
    p.key = key_.toStdString();
    p.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    return p;
}

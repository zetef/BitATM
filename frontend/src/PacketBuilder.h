#pragma once
#include <QString>
#include <optional>

#include "../../common/protocol.h"

/**
 * @brief Fluent Builder for Packet objects.
 *
 * Usage:
 *   Packet p = PacketBuilder()
 *       .setType(PacketType::MESSAGE)
 *       .setFrom("alice").setTo("bob")
 *       .setBody(encryptedBody).setKey(wrappedKey)
 *       .build();
 *
 * build() throws std::logic_error if setType() was not called.
 */
class PacketBuilder {
public:
    PacketBuilder& setType(PacketType t);
    PacketBuilder& setFrom(const QString& from);
    PacketBuilder& setTo(const QString& to);
    PacketBuilder& setBody(const QString& body);
    PacketBuilder& setKey(const QString& key);

    /** @brief Construct the Packet. Throws std::logic_error if type is not set. */
    Packet build();

private:
    std::optional<PacketType> type_;
    QString from_;
    QString to_;
    QString body_;
    QString key_;
};

#include "KeyExchangeHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "UserRepository.h"

void KeyExchangeHandler::validate(const Packet& packet) {
    if (packet.from.empty()) throw ProtocolException("KEY_EXCHANGE: sender (from) is required");
    // Either uploading own key or requesting a peer's key — at least one must be present
    if (packet.key.empty() && packet.to.empty())
        throw ProtocolException(
            "KEY_EXCHANGE: provide key to upload or 'to' to request a peer key");
}

void KeyExchangeHandler::execute(Packet& packet, ClientSession& session) {
    UserRepository repo;

    // Upload own public key if provided
    if (!packet.key.empty()) {
        auto userOpt = repo.findByUsername(packet.from);
        if (!userOpt) throw ProtocolException("KEY_EXCHANGE: sender not found");
        userOpt->setPublicKey(packet.key);
        repo.save(*userOpt);
    }

    // Return peer's public key if requested
    if (!packet.to.empty()) {
        auto peerOpt = repo.findByUsername(packet.to);
        if (!peerOpt) throw ProtocolException("KEY_EXCHANGE: requested user not found");

        Packet reply;
        reply.type = PacketType::KEY_EXCHANGE;
        reply.from = packet.to;
        reply.to = packet.from;
        reply.key = peerOpt->getPublicKey();
        session.send(reply);
        return;
    }

    // Pure upload — acknowledge
    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = packet.from;
    session.send(ack);
}

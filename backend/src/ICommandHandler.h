#pragma once
#include "../../common/protocol.h"

class ClientSession;

/**
 * @brief Template Method base for all packet handlers.
 *
 * Fixed skeleton: validate() -> authorize() -> execute()
 * Subclasses override only execute().
 */
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;

    void handle(Packet& packet, ClientSession& session) {
        validate(packet);
        authorize(session);
        execute(packet, session);
    }

protected:
    virtual void validate(const Packet& packet) = 0;
    virtual void authorize(const ClientSession& session) = 0;
    virtual void execute(Packet& packet, ClientSession& session) = 0;
};
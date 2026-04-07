#pragma once
#include <functional>
#include <memory>
#include <unordered_map>

#include "../../common/protocol.h"
#include "ICommandHandler.h"

/**
 * @brief Factory Method - maps PacketType to its ICommandHandler.
 *
 * Handlers register themselves at startup via registerHandler().
 * create() returns a fresh unique_ptr for each invocation.
 * Throws ProtocolException for unregistered types.
 */
class PacketHandlerFactory {
public:
    using Creator = std::function<std::unique_ptr<ICommandHandler>()>;

    PacketHandlerFactory() = default;
    ~PacketHandlerFactory() = default;

    PacketHandlerFactory(const PacketHandlerFactory&) = delete;
    PacketHandlerFactory& operator=(const PacketHandlerFactory&) = delete;
    PacketHandlerFactory(PacketHandlerFactory&&) = delete;
    PacketHandlerFactory& operator=(PacketHandlerFactory&&) = delete;

    /** @brief Register a creator function for a given PacketType. */
    void registerHandler(PacketType type, Creator creator);

    /**
     * @brief Create a handler for the given PacketType.
     * @throws ProtocolException if no handler is registered for type.
     */
    std::unique_ptr<ICommandHandler> create(PacketType type) const;

private:
    std::unordered_map<int, Creator> _registry;
};

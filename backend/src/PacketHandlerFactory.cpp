#include "PacketHandlerFactory.h"

#include "../../common/AppException.h"

void PacketHandlerFactory::registerHandler(PacketType type, Creator creator) {
    _registry[static_cast<int>(type)] = std::move(creator);
}

std::unique_ptr<ICommandHandler> PacketHandlerFactory::create(PacketType type) const {
    auto it = _registry.find(static_cast<int>(type));
    if (it == _registry.end())
        throw ProtocolException("PacketHandlerFactory: no handler registered for type " +
                                std::to_string(static_cast<int>(type)));
    return it->second();
}

#pragma once
#include <Poco/Net/HTTPServer.h>
#include <Poco/Util/ServerApplication.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "ClientSession.h"
#include "PacketHandlerFactory.h"

/**
 * @brief Main server application. Owns the client map and handler factory.
 *
 * Inherits Poco::Util::ServerApplication — entry point via POCO_SERVER_MAIN.
 * One thread per client (Poco thread pool). All access to _clients is
 * guarded by _clientsMutex.
 */
class Server : public Poco::Util::ServerApplication {
public:
    /** @brief Register a connected, authenticated session. */
    void addClient(const std::string& username, std::shared_ptr<ClientSession> session);

    /** @brief Remove a session (on disconnect). */
    void removeClient(const std::string& username);

    /**
     * @brief Look up an online client by username.
     * @return nullptr if the user is not currently connected.
     */
    std::shared_ptr<ClientSession> findClient(const std::string& username);

    /** @brief Access the handler factory (used by ConnectionHandler). */
    PacketHandlerFactory& getFactory() { return _factory; }

protected:
    int main(const std::vector<std::string>& args) override;

private:
    void registerHandlers();

    std::unordered_map<std::string, std::shared_ptr<ClientSession>> _clients;
    std::mutex _clientsMutex;
    PacketHandlerFactory _factory;
};

#pragma once
#include <Poco/Net/HTTPServer.h>
#include <Poco/Timer.h>
#include <Poco/Util/ServerApplication.h>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ClientSession.h"
#include "PacketHandlerFactory.h"

/**
 * @brief Main server application. Owns the client map and handler factory.
 *
 * Inherits Poco::Util::ServerApplication - entry point via POCO_SERVER_MAIN.
 * One thread per client (Poco thread pool). All access to _clients is
 * guarded by _clientsMutex.
 * _clients maps username -> all active sessions (multi-device support).
 */
class Server : public Poco::Util::ServerApplication {
public:
    Server();

    /** @brief Register a connected, authenticated session. */
    void addClient(const std::string& username, std::shared_ptr<ClientSession> session);

    /** @brief Remove a specific session on disconnect. */
    void removeClient(const std::string& username, ClientSession* sessionPtr);

    /**
     * @brief Look up the first active session for a user (for routing to online users).
     * @return nullptr if the user has no active sessions.
     */
    std::shared_ptr<ClientSession> findClient(const std::string& username);

    /**
     * @brief Return all active sessions for a user (for multi-device fan-out).
     * @return Empty vector if user is not connected.
     */
    std::vector<std::shared_ptr<ClientSession>> getSessionsForUser(const std::string& username);

    /** @brief Access the handler factory (used by ConnectionHandler). */
    PacketHandlerFactory& getFactory() { return _factory; }

protected:
    int main(const std::vector<std::string>& args) override;

private:
    void registerHandlers();

    /**
     * @brief Poco::Timer callback - runs every hour.
     * Deactivates expired sessions and purges old delivered offline_queue rows.
     */
    void onCleanupTimer(Poco::Timer& timer);

    std::unordered_map<std::string, std::vector<std::shared_ptr<ClientSession>>> _clients;
    std::mutex _clientsMutex;
    PacketHandlerFactory _factory;
    Poco::Timer _cleanupTimer;
    Poco::TimerCallback<Server> _cleanupCallback;
};

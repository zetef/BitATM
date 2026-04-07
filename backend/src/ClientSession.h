#pragma once
#include <Poco/Net/WebSocket.h>

#include <mutex>
#include <string>

#include "../../common/protocol.h"

/**
 * @brief Represents one live WebSocket connection.
 *
 * Pattern: State - transitions through Connected -> Authenticating ->
 * Authenticated -> Disconnected. Shared ownership via shared_ptr
 * (Server holds the map, handler holds the per-thread reference).
 *
 * Thread safety: state and credentials are guarded by _mutex.
 * The WebSocket send/receive use Poco's internal frame locking.
 */
class ClientSession {
public:
    enum class State { Connected, Authenticating, Authenticated, Disconnected };

    explicit ClientSession(Poco::Net::WebSocket ws);
    ~ClientSession() = default;

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;
    ClientSession(ClientSession&&) = delete;
    ClientSession& operator=(ClientSession&&) = delete;

    /**
     * @brief Serialize and send a Packet over the WebSocket.
     * @throws NetworkException on send failure.
     */
    void send(const Packet& packet);

    /**
     * @brief Block until a frame arrives and deserialize it into packet.
     * @return false if the connection closed or a CLOSE frame was received.
     * @throws NetworkException on receive error.
     */
    bool receive(Packet& packet);

    State getState() const;
    void setState(State s);
    const std::string& getUsername() const;
    void setUsername(const std::string& u);
    const std::string& getSessionToken() const;
    void setSessionToken(const std::string& t);
    bool isAuthenticated() const;

private:
    Poco::Net::WebSocket _ws;
    State _state{State::Connected};
    std::string _username;
    std::string _sessionToken;
    mutable std::mutex _mutex;
};

#include "Server.h"

#include <Poco/Logger.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/WebSocket.h>

#include <cstdlib>

#include "../../common/AppException.h"
#include "AckHandler.h"
#include "DbManager.h"
#include "KeyExchangeHandler.h"
#include "LoginHandler.h"
#include "MessageHandler.h"
#include "RegisterHandler.h"
#include "SyncHistoryHandler.h"

// ---------------------------------------------------------------------------
// ConnectionHandler - one per WebSocket connection, runs in Poco thread pool
// ---------------------------------------------------------------------------
class ConnectionHandler : public Poco::Net::HTTPRequestHandler {
public:
    explicit ConnectionHandler(Server& server) : _server(server) {}

    void handleRequest(Poco::Net::HTTPServerRequest& req,
                       Poco::Net::HTTPServerResponse& res) override {
        Poco::Logger& log = Poco::Logger::get("ConnectionHandler");
        std::string username;
        try {
            Poco::Net::WebSocket ws(req, res);
            auto session = std::make_shared<ClientSession>(std::move(ws));

            Packet packet;
            while (true) {
                // Parse failure is per-packet: send ERR and keep connection alive.
                // Network failure (NetworkException) propagates to outer catch.
                try {
                    if (!session->receive(packet)) break;
                } catch (const ProtocolException& e) {
                    poco_warning(log, std::string("Bad packet: ") + e.what());
                    try {
                        Packet err;
                        err.type = PacketType::ERR;
                        err.errorMsg = e.what();
                        session->send(err);
                    } catch (const std::exception&) {
                    }
                    continue;
                }

                try {
                    auto handler = _server.getFactory().create(packet.type);
                    handler->handle(packet, *session);

                    // Register in client map once authenticated.
                    // On re-login (account switch), evict the old username first.
                    if (session->isAuthenticated() && !session->getUsername().empty()) {
                        const std::string newUsername = session->getUsername();
                        if (!username.empty() && username != newUsername)
                            _server.removeClient(username);
                        username = newUsername;
                        _server.addClient(username, session);
                    }
                } catch (const ProtocolException& e) {
                    poco_warning(log, std::string("Protocol error: ") + e.what());
                    try {
                        Packet err;
                        err.type = PacketType::ERR;
                        err.errorMsg = e.what();
                        session->send(err);
                    } catch (const std::exception&) {
                    }
                } catch (const AppException& e) {
                    poco_error(log, std::string("Handler error: ") + e.what());
                    try {
                        Packet err;
                        err.type = PacketType::ERR;
                        err.errorMsg = e.what();
                        session->send(err);
                    } catch (const std::exception&) {
                    }
                }
            }
        } catch (const Poco::Exception& e) {
            poco_error(log, std::string("Connection error: ") + e.message());
        } catch (const std::exception& e) {
            poco_error(log, std::string("Connection error: ") + e.what());
        }

        if (!username.empty()) _server.removeClient(username);
    }

private:
    Server& _server;
};

// ---------------------------------------------------------------------------
// ConnectionHandlerFactory
// ---------------------------------------------------------------------------
class ConnectionHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    explicit ConnectionHandlerFactory(Server& server) : _server(server) {}

    Poco::Net::HTTPRequestHandler* createRequestHandler(
        const Poco::Net::HTTPServerRequest&) override {
        return new ConnectionHandler(_server);
    }

private:
    Server& _server;
};

// ---------------------------------------------------------------------------
// Server
// ---------------------------------------------------------------------------
void Server::addClient(const std::string& username, std::shared_ptr<ClientSession> session) {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    _clients[username] = std::move(session);
}

void Server::removeClient(const std::string& username) {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    _clients.erase(username);
}

std::shared_ptr<ClientSession> Server::findClient(const std::string& username) {
    std::lock_guard<std::mutex> lock(_clientsMutex);
    auto it = _clients.find(username);
    return it != _clients.end() ? it->second : nullptr;
}

void Server::registerHandlers() {
    _factory.registerHandler(PacketType::REGISTER,
                             [] { return std::make_unique<RegisterHandler>(); });
    _factory.registerHandler(PacketType::LOGIN, [] { return std::make_unique<LoginHandler>(); });
    _factory.registerHandler(PacketType::MESSAGE,
                             [this] { return std::make_unique<MessageHandler>(*this); });
    _factory.registerHandler(PacketType::KEY_EXCHANGE,
                             [] { return std::make_unique<KeyExchangeHandler>(); });
    _factory.registerHandler(PacketType::ACK, [] { return std::make_unique<AckHandler>(); });
    _factory.registerHandler(PacketType::SYNC_HISTORY,
                             [] { return std::make_unique<SyncHistoryHandler>(); });
}

int Server::main(const std::vector<std::string>&) {
    Poco::Logger& log = Poco::Logger::get("Server");

    const char* dbUrl = std::getenv("DATABASE_URL");
    const std::string connStr =
        dbUrl ? std::string(dbUrl)
              : "host=localhost port=5432 dbname=bitatm_chat user=chatuser password=changeme";
    try {
        DbManager::instance().init(connStr);
        poco_information(log, "Database pool initialised");
    } catch (const DbException& e) {
        poco_error(log, std::string("DB init failed: ") + e.what());
    }

    registerHandlers();

    auto* params = new Poco::Net::HTTPServerParams;
    params->setKeepAlive(true);
    params->setMaxKeepAliveRequests(0);

    Poco::Net::HTTPServer server(new ConnectionHandlerFactory(*this), Poco::Net::ServerSocket(8080),
                                 params);
    server.start();
    poco_information(log, "BitATM server started on :8080");

    waitForTerminationRequest();

    server.stop();
    poco_information(log, "BitATM server stopped");
    return EXIT_OK;
}

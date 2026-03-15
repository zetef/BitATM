#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Util/ServerApplication.h>

#include <iostream>

class EchoHandler : public Poco::Net::HTTPRequestHandler {
public:
    void handleRequest(Poco::Net::HTTPServerRequest& req,
                       Poco::Net::HTTPServerResponse& res) override {
        try {
            Poco::Net::WebSocket ws(req, res);
            std::cout << "Client connected\n";
            char buf[4096];
            int flags, n;
            do {
                try {
                    n = ws.receiveFrame(buf, sizeof(buf), flags);
                    if (n > 0) ws.sendFrame(buf, n, flags);
                } catch (Poco::TimeoutException&) {
                    n = 1;
                    flags = 0;
                    continue;
                }
            } while (n > 0 && (flags & Poco::Net::WebSocket::FRAME_OP_BITMASK) !=
                                  Poco::Net::WebSocket::FRAME_OP_CLOSE);
            std::cout << "Client disconnected\n";
        } catch (Poco::Exception& e) {
            std::cerr << "Poco error: " << e.message() << "\n";
        }
    }
};

class EchoFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    Poco::Net::HTTPRequestHandler* createRequestHandler(
        const Poco::Net::HTTPServerRequest&) override {
        return new EchoHandler;
    }
};

class EchoServer : public Poco::Util::ServerApplication {
    int main(const std::vector<std::string>&) override {
        auto* params = new Poco::Net::HTTPServerParams;
        params->setKeepAlive(true);
        params->setMaxKeepAliveRequests(0);

        Poco::Net::HTTPServer server(new EchoFactory, Poco::Net::ServerSocket(8080), params);

        server.start();
        std::cout << "Echo server on :8080\n";
        waitForTerminationRequest();
        server.stop();
        return EXIT_OK;
    }
};

POCO_SERVER_MAIN(EchoServer)

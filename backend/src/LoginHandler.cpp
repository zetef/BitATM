#include "LoginHandler.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <iomanip>
#include <sstream>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "MessageRepository.h"
#include "OfflineQueueRepository.h"
#include "SessionRepository.h"
#include "UserRepository.h"

namespace {

std::string toHex(const unsigned char* data, std::size_t len) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < len; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    return ss.str();
}

std::string fromHex(const std::string& hex) {
    std::string bytes;
    bytes.reserve(hex.size() / 2);
    for (std::size_t i = 0; i + 1 < hex.size(); i += 2) {
        unsigned char byte = static_cast<unsigned char>(std::stoi(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(static_cast<char>(byte));
    }
    return bytes;
}

bool verifyPassword(const std::string& password, const std::string& stored) {
    constexpr int ITERATIONS = 100000;
    constexpr int KEY_LEN = 32;

    const auto sep = stored.find(':');
    if (sep == std::string::npos) return false;

    const std::string saltBytes = fromHex(stored.substr(0, sep));
    const std::string expected = stored.substr(sep + 1);

    unsigned char hash[KEY_LEN];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()),
                          reinterpret_cast<const unsigned char*>(saltBytes.data()),
                          static_cast<int>(saltBytes.size()), ITERATIONS, EVP_sha256(), KEY_LEN,
                          hash) != 1)
        throw CryptoException("LoginHandler: PBKDF2 verification failed");

    return toHex(hash, KEY_LEN) == expected;
}

std::string generateToken() {
    constexpr int TOKEN_BYTES = 32;
    unsigned char buf[TOKEN_BYTES];
    if (RAND_bytes(buf, TOKEN_BYTES) != 1) throw CryptoException("LoginHandler: RAND_bytes failed");
    return toHex(buf, TOKEN_BYTES);
}

}  // namespace

void LoginHandler::validate(const Packet& packet) {
    if (packet.from.empty()) throw ProtocolException("LOGIN: username (from) is required");
    if (packet.body.empty()) throw ProtocolException("LOGIN: password (body) is required");
}

void LoginHandler::authorize(const ClientSession& session) {
    if (session.isAuthenticated())
        throw ProtocolException("LOGIN: session is already authenticated");
}

void LoginHandler::execute(Packet& packet, ClientSession& session) {
    UserRepository userRepo;
    auto userOpt = userRepo.findByUsername(packet.from);
    if (!userOpt) throw ProtocolException("LOGIN: invalid credentials");

    if (!verifyPassword(packet.body, userOpt->getPasswordHash()))
        throw ProtocolException("LOGIN: invalid credentials");

    // Issue session token
    const std::string token = generateToken();
    SessionRepository sessionRepo;
    ::Session newSession{0, userOpt->getId(), token, {}, "NOW() + INTERVAL '24 hours'"};
    sessionRepo.save(newSession);

    // Transition session state
    session.setUsername(packet.from);
    session.setSessionToken(token);
    session.setState(ClientSession::State::Authenticated);

    // Flush pending offline messages
    OfflineQueueRepository offlineRepo;
    MessageRepository msgRepo;
    auto pending = offlineRepo.findUndeliveredByRecipient(packet.from);
    for (auto& entry : pending) {
        auto msgOpt = msgRepo.findById(entry.getMessageId());
        if (msgOpt) {
            Packet fwd;
            fwd.type = PacketType::MESSAGE;
            fwd.from = msgOpt->getSender();
            fwd.to = packet.from;
            fwd.body = msgOpt->getEncryptedKey();  // body carries encrypted key for offline
            fwd.timestamp = msgOpt->getCreatedAt();
            session.send(fwd);
            offlineRepo.markDelivered(entry.getId());
        }
    }

    // ACK with token in body
    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = packet.from;
    ack.body = token;
    session.send(ack);
}

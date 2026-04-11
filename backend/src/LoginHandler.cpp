#include "LoginHandler.h"

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/rand.h>

#include <cstdint>
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

// Verify password against a stored "saltHex:hashHex" produced by Argon2id.
bool verifyPassword(const std::string& password, const std::string& stored) {
    constexpr std::size_t HASH_LEN = 32;

    const auto sep = stored.find(':');
    if (sep == std::string::npos) return false;

    const std::string saltBytes = fromHex(stored.substr(0, sep));
    const std::string expectedHex = stored.substr(sep + 1);

    uint32_t memcost = 65536;  // OSSL_KDF_PARAM_ARGON2_MEMCOST, in KiB
    uint32_t iter = 3;         // OSSL_KDF_PARAM_ITER (t_cost)
    uint32_t lanes = 4;        // OSSL_KDF_PARAM_ARGON2_LANES

    EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
    if (!kdf) throw CryptoException("LoginHandler: Argon2id not available");
    EVP_KDF_CTX* ctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);
    if (!ctx) throw CryptoException("LoginHandler: EVP_KDF_CTX_new failed");

    OSSL_PARAM params[] = {OSSL_PARAM_construct_octet_string(
                               "pass", const_cast<char*>(password.data()), password.size()),
                           OSSL_PARAM_construct_octet_string(
                               "salt", const_cast<char*>(saltBytes.data()), saltBytes.size()),
                           OSSL_PARAM_construct_uint32("memcost", &memcost),
                           OSSL_PARAM_construct_uint32("iter", &iter),
                           OSSL_PARAM_construct_uint32("lanes", &lanes),
                           OSSL_PARAM_END};

    unsigned char out[HASH_LEN];
    if (EVP_KDF_derive(ctx, out, HASH_LEN, params) != 1) {
        EVP_KDF_CTX_free(ctx);
        throw CryptoException("LoginHandler: Argon2id verify failed");
    }
    EVP_KDF_CTX_free(ctx);

    return toHex(out, HASH_LEN) == expectedHex;
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

    const std::string token = generateToken();
    SessionRepository sessionRepo;
    ::Session newSession{0, userOpt->getId(), token, {}, "NOW() + INTERVAL '24 hours'"};
    sessionRepo.save(newSession);

    session.setUsername(packet.from);
    session.setSessionToken(token);
    session.setState(ClientSession::State::Authenticated);

    // flush pending offline messages
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
            fwd.body = msgOpt->getEncryptedBody();
            fwd.key = msgOpt->getEncryptedKey();
            fwd.timestamp = msgOpt->getCreatedAt();
            session.send(fwd);
            offlineRepo.markDelivered(entry.getId());
        }
    }

    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = packet.from;
    ack.body = token;
    session.send(ack);
}

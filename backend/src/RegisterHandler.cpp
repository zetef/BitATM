#include "RegisterHandler.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <iomanip>
#include <sstream>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "UserRepository.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

std::string toHex(const unsigned char* data, std::size_t len) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < len; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    return ss.str();
}

/** PBKDF2-SHA256, 100 000 iterations, 32-byte key. Returns "saltHex:hashHex". */
std::string hashPassword(const std::string& password) {
    constexpr int ITERATIONS = 100000;
    constexpr int KEY_LEN = 32;
    constexpr int SALT_LEN = 16;

    unsigned char salt[SALT_LEN];
    if (RAND_bytes(salt, SALT_LEN) != 1)
        throw CryptoException("RegisterHandler: RAND_bytes failed");

    unsigned char hash[KEY_LEN];
    if (PKCS5_PBKDF2_HMAC(password.c_str(), static_cast<int>(password.size()), salt, SALT_LEN,
                          ITERATIONS, EVP_sha256(), KEY_LEN, hash) != 1)
        throw CryptoException("RegisterHandler: PBKDF2 failed");

    return toHex(salt, SALT_LEN) + ":" + toHex(hash, KEY_LEN);
}

}  // namespace

// ---------------------------------------------------------------------------
// Handler implementation
// ---------------------------------------------------------------------------
void RegisterHandler::validate(const Packet& packet) {
    if (packet.from.empty()) throw ProtocolException("REGISTER: username (from) is required");
    if (packet.body.empty()) throw ProtocolException("REGISTER: password (body) is required");
    if (packet.from.size() > 64)
        throw ProtocolException("REGISTER: username exceeds 64 characters");
}

void RegisterHandler::authorize(const ClientSession& /*session*/) {
    // Registration does not require an existing authenticated session.
}

void RegisterHandler::execute(Packet& packet, ClientSession& session) {
    UserRepository repo;

    if (repo.findByUsername(packet.from).has_value())
        throw ProtocolException("REGISTER: username already taken");

    const std::string passwordHash = hashPassword(packet.body);
    User newUser{0, packet.from, passwordHash};
    repo.save(newUser);

    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = packet.from;
    session.send(ack);
}

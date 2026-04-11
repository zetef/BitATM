#include "RegisterHandler.h"

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/rand.h>

#include <cstdint>
#include <iomanip>
#include <sstream>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "UserRepository.h"

namespace {

std::string toHex(const unsigned char* data, std::size_t len) {
    std::ostringstream ss;
    for (std::size_t i = 0; i < len; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    return ss.str();
}

// Argon2id, m=65536 KB, t=3, p=4, 32-byte output. Returns "saltHex:hashHex".
std::string hashPassword(const std::string& password) {
    constexpr std::size_t SALT_LEN = 16;
    constexpr std::size_t HASH_LEN = 32;

    unsigned char salt[SALT_LEN];
    if (RAND_bytes(salt, static_cast<int>(SALT_LEN)) != 1)
        throw CryptoException("RegisterHandler: RAND_bytes failed");

    uint32_t m_cost = 65536;
    uint32_t t_cost = 3;
    uint32_t lanes = 4;
    uint32_t threads = 4;

    EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
    if (!kdf)
        throw CryptoException("RegisterHandler: Argon2id not available in this OpenSSL build");
    EVP_KDF_CTX* ctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);
    if (!ctx) throw CryptoException("RegisterHandler: EVP_KDF_CTX_new failed");

    OSSL_PARAM params[] = {OSSL_PARAM_construct_octet_string(
                               "pass", const_cast<char*>(password.data()), password.size()),
                           OSSL_PARAM_construct_octet_string("salt", salt, SALT_LEN),
                           OSSL_PARAM_construct_uint32("m_cost", &m_cost),
                           OSSL_PARAM_construct_uint32("t_cost", &t_cost),
                           OSSL_PARAM_construct_uint32("lanes", &lanes),
                           OSSL_PARAM_construct_uint32("threads", &threads),
                           OSSL_PARAM_END};

    unsigned char out[HASH_LEN];
    if (EVP_KDF_derive(ctx, out, HASH_LEN, params) != 1) {
        EVP_KDF_CTX_free(ctx);
        throw CryptoException("RegisterHandler: Argon2id derive failed");
    }
    EVP_KDF_CTX_free(ctx);

    return toHex(salt, SALT_LEN) + ":" + toHex(out, HASH_LEN);
}

}  // namespace

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

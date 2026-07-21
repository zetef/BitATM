#include "LoginHandler.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>
#include <Poco/Logger.h>
#include <Poco/Nullable.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/rand.h>

#include <cstdint>
#include <iomanip>
#include <sstream>

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "DbManager.h"
#include "MessageRepository.h"
#include "OfflineQueueRepository.h"
#include "PendingNotificationRepository.h"
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

void LoginHandler::authorize(const ClientSession& /*session*/) {
    // Re-login on the same connection is allowed (user switching accounts).
}

void LoginHandler::execute(Packet& packet, ClientSession& session) {
    UserRepository userRepo;
    auto userOpt = userRepo.findByUsername(packet.from);
    if (!userOpt) throw ProtocolException("LOGIN: invalid credentials");

    if (!verifyPassword(packet.body, userOpt->getPasswordHash()))
        throw ProtocolException("LOGIN: invalid credentials");

    const std::string token = generateToken();
    SessionRepository sessionRepo;
    sessionRepo.deactivateAllForUser(userOpt->getId());
    ::Session newSession{0, userOpt->getId(), token, {}, {}};
    sessionRepo.save(newSession);

    session.setUsername(packet.from);
    session.setSessionToken(token);
    session.setState(ClientSession::State::Authenticated);
    try {
        userRepo.updateLastSeen(packet.from);
    } catch (const std::exception& e) {
        // Non-fatal: stale last_seen is cosmetic, login must not fail over it.
        poco_warning(Poco::Logger::get("LoginHandler"),
                     std::string("updateLastSeen failed (non-fatal): ") + e.what());
    }

    Packet ack;
    ack.type = PacketType::ACK;
    ack.to = packet.from;
    ack.body = token;
    session.send(ack);

    // flush pending offline messages after ACK so the client has loaded its
    // private key before the first MESSAGE packet arrives
    OfflineQueueRepository offlineRepo;
    MessageRepository msgRepo;
    Poco::Logger& log = Poco::Logger::get("LoginHandler");
    auto pending = offlineRepo.findUndeliveredByRecipient(packet.from);
    for (auto& entry : pending) {
        try {
            // persist the attempt before sending so a crash mid-send still counts
            offlineRepo.incrementAttempts(entry.getId());
            auto msgOpt = msgRepo.findById(entry.getMessageId());
            if (!msgOpt) {
                poco_warning(log, "offline entry " + std::to_string(entry.getId()) +
                                      " references missing message " +
                                      std::to_string(entry.getMessageId()) + ", skipping");
                continue;
            }
            Packet fwd;
            fwd.type = PacketType::MESSAGE;
            fwd.from = msgOpt->getSender();
            fwd.to = packet.from;
            fwd.body = msgOpt->getEncryptedBody();
            fwd.key = msgOpt->getEncryptedKey();
            fwd.timestamp = msgOpt->getCreatedAt();
            session.send(fwd);
            offlineRepo.markDelivered(entry.getId());
        } catch (const NetworkException& e) {
            // dead socket: stop, do not burn attempts for the rest of the queue
            poco_warning(log, std::string("offline flush aborted, socket dead: ") + e.what());
            break;
        } catch (const std::exception& e) {
            poco_warning(log, "offline flush entry " + std::to_string(entry.getId()) +
                                  " failed: " + e.what());
        }
    }

    // flush queued read receipts - sent while this user was offline
    flushOfflineReadReceipts(packet.from, session);

    // flush queued control notifications (deleted conversations, deleted groups)
    flushPendingNotifications(packet.from, session);
}

void LoginHandler::flushOfflineReadReceipts(const std::string& username, ClientSession& session) {
    using namespace Poco::Data::Keywords;
    try {
        auto ses = DbManager::instance().session();
        std::vector<int> ids;
        std::vector<std::string> fromUsers, msgTimestamps, kinds;
        std::vector<int> groupIds;  // 0 = 1:1 receipt

        {
            int id;
            std::string fromUser, msgTs, kind;
            Poco::Nullable<int> groupId;
            Poco::Data::Statement sel(ses);
            std::string usernameParam = username;
            // clang-format off
            sel << "SELECT id, from_user, message_ts, group_id, kind FROM offline_read_receipts "
                   "WHERE to_user = $1 AND delivered = FALSE ORDER BY queued_at ASC",
                into(id), into(fromUser), into(msgTs), into(groupId), into(kind),
                use(usernameParam), range(0, 1);
            // clang-format on
            while (!sel.done()) {
                sel.execute();
                if (!fromUser.empty()) {
                    ids.push_back(id);
                    fromUsers.push_back(fromUser);
                    msgTimestamps.push_back(msgTs);
                    kinds.push_back(kind);
                    groupIds.push_back(groupId.isNull() ? 0 : groupId.value());
                    fromUser.clear();
                    kind.clear();
                    groupId.clear();
                }
            }
        }

        for (std::size_t i = 0; i < ids.size(); ++i) {
            Packet rr;
            if (kinds[i] == "delivered") {
                rr.type = PacketType::ACK;
                rr.errorMsg = "delivered";
            } else {
                rr.type = PacketType::READ_RECEIPT;
            }
            rr.from = fromUsers[i];
            // Group receipts keep the group id in 'to' so the client can
            // route the update to the right conversation
            rr.to = groupIds[i] > 0 ? std::to_string(groupIds[i]) : username;
            rr.body = msgTimestamps[i];
            session.send(rr);

            int id = ids[i];
            ses << "UPDATE offline_read_receipts SET delivered = TRUE WHERE id = $1", use(id), now;
        }
    } catch (const Poco::Exception& e) {
        throw DbException("LoginHandler::flushOfflineReadReceipts: " + e.message());
    }
}

void LoginHandler::flushPendingNotifications(const std::string& username, ClientSession& session) {
    PendingNotificationRepository pendingRepo;
    auto pending = pendingRepo.findAndClear(username);
    for (const auto& n : pending) {
        Packet fwd;
        fwd.type = n.type;
        fwd.from = n.fromUser;
        fwd.to = username;
        fwd.body = n.body;
        fwd.errorMsg = n.errorMsg;
        fwd.key = n.packetKey;
        try {
            session.send(fwd);
        } catch (const NetworkException& e) {
            poco_warning(
                Poco::Logger::get("LoginHandler"),
                "flushPendingNotifications: send failed for " + username + ": " + e.what());
        }
    }
}

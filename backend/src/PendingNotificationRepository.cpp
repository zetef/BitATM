#include "PendingNotificationRepository.h"

#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include "../../common/AppException.h"
#include "DbManager.h"

using namespace Poco::Data::Keywords;

void PendingNotificationRepository::queue(const std::string& recipient, PacketType type,
                                          const std::string& fromUser, const std::string& body) {
    try {
        auto ses = DbManager::instance().session();
        std::string recipientCopy = recipient;
        int typeInt = static_cast<int>(type);
        std::string fromCopy = fromUser;
        std::string bodyCopy = body;
        // clang-format off
        ses << "INSERT INTO pending_notifications(recipient, packet_type, from_user, body) "
               "VALUES($1, $2, $3, $4)",
            use(recipientCopy), use(typeInt), use(fromCopy), use(bodyCopy), now;
        // clang-format on
    } catch (const Poco::Exception& e) {
        throw DbException("PendingNotificationRepository::queue: " + e.message());
    }
}

std::vector<PendingNotification> PendingNotificationRepository::findAndClear(
    const std::string& recipient) {
    try {
        auto ses = DbManager::instance().session();
        std::vector<PendingNotification> rows;
        std::string recipientCopy = recipient;
        int id = 0, typeInt = 0;
        std::string fromUser, body;
        Poco::Data::Statement sel(ses);
        // clang-format off
        sel << "SELECT id, packet_type, from_user, body FROM pending_notifications "
               "WHERE recipient = $1 ORDER BY created_at ASC",
            into(id), into(typeInt), into(fromUser), into(body),
            use(recipientCopy), range(0, 1);
        // clang-format on
        while (!sel.done()) {
            sel.execute();
            if (id != 0) {
                rows.push_back(
                    PendingNotification{id, static_cast<PacketType>(typeInt), fromUser, body});
                id = 0;
                fromUser.clear();
                body.clear();
            }
        }
        ses << "DELETE FROM pending_notifications WHERE recipient = $1", use(recipientCopy), now;
        return rows;
    } catch (const Poco::Exception& e) {
        throw DbException("PendingNotificationRepository::findAndClear: " + e.message());
    }
}

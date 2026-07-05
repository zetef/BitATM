#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

#include <QtTest>
#include <cstdlib>

#include "DbManager.h"
#include "Session.h"
#include "SessionRepository.h"
#include "User.h"
#include "UserRepository.h"

using namespace Poco::Data::Keywords;

class SessionRepositoryTest : public QObject {
    Q_OBJECT
private:
    bool _dbAvailable = false;

    int makeUser(const std::string& name) {
        UserRepository userRepo;
        if (!userRepo.findByUsername(name)) userRepo.save(User{0, name, "hash:x", ""});
        return userRepo.findByUsername(name)->getId();
    }

    void removeUser(const std::string& name) {
        UserRepository userRepo;
        auto u = userRepo.findByUsername(name);
        if (u) userRepo.remove(u->getId());
    }

private slots:
    void initTestCase() {
        const char* url = std::getenv("DATABASE_URL");
        if (!url) return;
        try {
            DbManager::instance().init(std::string(url));
            _dbAvailable = true;
        } catch (...) {
        }
    }

    // UT-BE-13a: deactivated session token is no longer accepted
    void findByTokenRejectsInactive() {
        if (!_dbAvailable) QSKIP("DATABASE_URL not set");

        const std::string username = "ut_be_13a_user";
        const std::string token = "ut_be_13a_token";
        SessionRepository repo;

        int userId = makeUser(username);
        ::Session s{0, userId, token, {}, {}};
        repo.save(s);

        auto found = repo.findByToken(token);
        QVERIFY(found.has_value());
        int sessionId = found->getId();

        repo.deactivateByToken(token);
        QVERIFY(!repo.findByToken(token).has_value());

        repo.remove(sessionId);
        removeUser(username);
    }

    // UT-BE-13b: expired session token is no longer accepted
    void findByTokenRejectsExpired() {
        if (!_dbAvailable) QSKIP("DATABASE_URL not set");

        const std::string username = "ut_be_13b_user";
        const std::string token = "ut_be_13b_token";
        SessionRepository repo;

        int userId = makeUser(username);
        ::Session s{0, userId, token, {}, {}};
        repo.save(s);

        auto found = repo.findByToken(token);
        QVERIFY(found.has_value());
        int sessionId = found->getId();

        // force expiry in the past; still is_active = TRUE
        auto ses = DbManager::instance().session();
        std::string tokenParam = token;
        ses << "UPDATE sessions SET expires_at = NOW() - INTERVAL '1 hour' "
               "WHERE session_token = $1",
            use(tokenParam), now;

        QVERIFY(!repo.findByToken(token).has_value());

        repo.remove(sessionId);
        removeUser(username);
    }
};

QTEST_MAIN(SessionRepositoryTest)
#include "SessionRepositoryTest.moc"

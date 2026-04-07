#include "DbManager.h"

#include <Poco/Data/PostgreSQL/Connector.h>

#include "../../common/AppException.h"

DbManager& DbManager::instance() {
    static DbManager inst;
    return inst;
}

void DbManager::init(const std::string& connectionString, int poolSize) {
    Poco::Data::PostgreSQL::Connector::registerConnector();
    try {
        _pool =
            std::make_unique<Poco::Data::SessionPool>("PostgreSQL", connectionString, 1, poolSize);
    } catch (const Poco::Exception& e) {
        throw DbException("DbManager::init failed: " + e.message());
    }
}

Poco::Data::Session DbManager::session() {
    if (!_pool) throw DbException("DbManager: pool not initialised - call init() first");
    try {
        return _pool->get();
    } catch (const Poco::Exception& e) {
        throw DbException("DbManager: failed to acquire session: " + e.message());
    }
}

#include "BaseAuthHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"
#include "SessionRepository.h"

void BaseAuthHandler::authorize(const ClientSession& session) {
    if (!session.isAuthenticated())
        throw ProtocolException("BaseAuthHandler: request rejected - session not authenticated");

    const std::string& token = session.getSessionToken();
    if (token.empty())
        throw ProtocolException("BaseAuthHandler: request rejected - no session token");

    // Fail-closed: a DbException from findByToken propagates and rejects the
    // request rather than trusting stale in-memory state.
    SessionRepository repo;
    if (!repo.findByToken(token))
        throw ProtocolException("BaseAuthHandler: request rejected - session expired or revoked");
}

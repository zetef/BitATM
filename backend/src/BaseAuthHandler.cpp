#include "BaseAuthHandler.h"

#include "../../common/AppException.h"
#include "ClientSession.h"

void BaseAuthHandler::authorize(const ClientSession& session) {
    if (!session.isAuthenticated())
        throw ProtocolException("BaseAuthHandler: request rejected — session not authenticated");
}

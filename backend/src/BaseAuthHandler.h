#pragma once
#include "ICommandHandler.h"

/**
 * @brief L2 abstract handler for packet types that require an authenticated session.
 *
 * Implements authorize() - requires the in-memory Authenticated state AND an
 * active, unexpired session row in the DB. Subclasses (LoginHandler,
 * MessageHandler, KeyExchangeHandler, AckHandler) only need to override
 * validate() and execute().
 */
class BaseAuthHandler : public ICommandHandler {
protected:
    /**
     * @brief Reject the request unless the session is authenticated and its
     *        token maps to an active, unexpired row in the sessions table.
     * @throws ProtocolException if session state != Authenticated, the token
     *         is missing, or the DB session is expired/revoked.
     * @throws DbException on DB failure (fail-closed: requests are rejected
     *         rather than trusting stale in-memory state).
     */
    void authorize(const ClientSession& session) override;
};

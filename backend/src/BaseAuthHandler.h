#pragma once
#include "ICommandHandler.h"

/**
 * @brief L2 abstract handler for packet types that require an authenticated session.
 *
 * Implements authorize() — throws ProtocolException if the session is not
 * in the Authenticated state. Subclasses (LoginHandler, MessageHandler,
 * KeyExchangeHandler, AckHandler) only need to override validate() and execute().
 */
class BaseAuthHandler : public ICommandHandler {
protected:
    /**
     * @brief Reject the request if the session is not authenticated.
     * @throws ProtocolException if session state != Authenticated.
     */
    void authorize(const ClientSession& session) override;
};

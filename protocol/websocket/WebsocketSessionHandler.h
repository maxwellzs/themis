#ifndef WebsocketSessionHandler_h
#define WebsocketSessionHandler_h 1

#include "network/Session.h"

namespace themis
{

    /**
     * @brief this handler handle the session after the connection has upgraded to 
     * websocket session
     * 
     */
    class WebsocketSessionHandler : public SessionHandler {
    public:

        virtual void handleSession() override;
    };
    
} // namespace themis

#endif
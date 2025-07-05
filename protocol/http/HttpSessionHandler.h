#ifndef HttpSessionHandler_h
#define HttpSessionHandler_h 1

#include <functional>
#include "network/Session.h"
#include "HttpRequest.h"

namespace themis
{
    
    /**
     * @brief HttpSessionHandler handles the http requests
     * 
     */
    class HttpSessionHandler : public SessionHandler {
    public:

        // the function after handle succeeded
        using CallbackFunction = std::function<void (std::unique_ptr<HttpRequest>, 
        const std::unique_ptr<Session>&)>;

    private:

        std::unique_ptr<HttpRequest> pendingRequest;
        size_t awaitBodyCount = 0;

        enum SessionState {
            AWAIT_HEADER,
            AWAIT_BODY,
            COMPLETE
        } state = AWAIT_HEADER;

        void parseHeader();
        void parseParameter();
        void parseBody(BufferReader& reader);
        void parseBody() {
            BufferReader r(session->getInputBuffer());
            parseBody(r);
        }
        
        CallbackFunction cb;

    public:

        HttpSessionHandler(std::unique_ptr<Session> s, CallbackFunction func) : SessionHandler(std::move(s)), cb(func) {};
        virtual void handleSession() override;
    };

} // namespace themis


#endif
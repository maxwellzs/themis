#ifndef WebsocketSessionHandler_h
#define WebsocketSessionHandler_h 1

#include <variant>
#include "network/Session.h"
#include "WebsocketFrame.h"

namespace themis
{

    /**
     * @brief this handler handle the session after the connection has upgraded to 
     * websocket session
     * use an existing http handler to move-construct this one to ensure there are 
     * no memory leak anywhere
     */
    class WebsocketSessionHandler : public SessionHandler {
    public:

        /**
         * @brief the EventListener class is the user defined interface to access the data
         * parsed from websocket session handler
         * 
         */
        class EventListener {
        public:
            virtual ~EventListener() = default;
            virtual void onText(WebsocketSessionHandler& handler, const std::string& msg) {};
            virtual void onBinary(WebsocketSessionHandler& handler, const std::vector<uint8_t>& buffer) {};
            virtual void onDisconnect() {};
        };

    private:
        std::unique_ptr<WebsocketFrame> pendingFrame = std::make_unique<WebsocketFrame>();
        std::unique_ptr<EventListener> listener;
        std::variant<std::vector<uint8_t>, std::string> message;
    public:
        ~WebsocketSessionHandler() {
            // this means this handler is removed by reactor for some reason
            // thus calling the disconnect callback
            if(listener.get()) listener->onDisconnect();
        }

        WebsocketSessionHandler(const std::unique_ptr<Session>& old, std::unique_ptr<EventListener> listener) 
        : SessionHandler(std::move(*old.get())), listener(std::move(listener)) {
            // disable timeout
            session->setLastActive(0x7fffffffffffffffl);
        }

        virtual void handleSession() override;
    };
    
} // namespace themis

#endif
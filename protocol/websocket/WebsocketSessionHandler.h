#ifndef WebsocketSessionHandler_h
#define WebsocketSessionHandler_h 1

#include <variant>
#include "utils/EventQueue.h"
#include "network/Session.h"
#include "WebsocketFrame.h"
#include "WebsocketWriter.h"

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
        protected:
            /// @brief use this to set timed or interval events
            const std::unique_ptr<EventQueue>& eventQueue;
            /// @brief use this to push data to client
            WebsocketSessionHandler& ws;
        public:
            EventListener(WebsocketSessionHandler& handler, const std::unique_ptr<EventQueue>& queue) 
            : ws(handler), eventQueue(queue) {}
            virtual ~EventListener() = default;
            virtual void onText(WebsocketSessionHandler& handler, const std::string& msg) {};
            virtual void onBinary(WebsocketSessionHandler& handler, const std::vector<uint8_t>& msg) {};
            virtual void onDisconnect() {};
        };

    private:
        std::unique_ptr<WebsocketFrame> pendingFrame = std::make_unique<WebsocketFrame>();
        std::unique_ptr<EventListener> listener;
        std::variant<std::vector<uint8_t>, std::string> message;
        WebsocketWriter wsWriter;

        template<class ...Ty>
        struct MessageVisitor : Ty... {
            using Ty::operator()...;
        };
        template<class... Ts> MessageVisitor(Ts...) -> MessageVisitor<Ts...>;

        /**
         * @brief handle the frame when it reached the COMPLETE state
         * 
         */
        void dispatchFrame();

    public:
        std::ostream& getOutputStream() {
            return wsWriter.getOutputStream();
        }

        /**
         * @brief this proxy the finish from Websocket writer, but also active write event so that io can happen
         * 
         * @param text if the data in output stream is text
         */
        void finish(bool text);

        void setMaxPayloadSize(size_t newSize) {
            wsWriter.setMaxPayloadSize(newSize);
        }

        ~WebsocketSessionHandler() {
            // this means this handler is removed by reactor for some reason
            // thus calling the disconnect callback
            if(listener.get()) listener->onDisconnect();
        }

        void setListener(std::unique_ptr<EventListener> listener) {
            this->listener = std::move(listener);
        }

        WebsocketSessionHandler(const std::unique_ptr<Session>& old) 
        : SessionHandler(std::move(*old.get())), wsWriter(session->getOutputBuffer()) {
            // disable timeout
            // implement this feature by using ping mechanism
            session->setLastActive(0x7fffffffffffffffl);
        }

        virtual void handleSession() override;
    };
    
} // namespace themis

#endif
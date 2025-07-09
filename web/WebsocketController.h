#ifndef WebsocketController_h
#define WebsocketController_h 1

#include <map>
#include <string>
#include <memory>
#include "utils/EventQueue.h"
#include "network/Session.h"
#include "protocol/websocket/WebsocketSessionHandler.h"
#include "protocol/http/HttpRequest.h"

namespace themis
{
    
    class WebsocketController {
    public:
        using ListenerAllocator 
        = std::function<std::unique_ptr<WebsocketSessionHandler::EventListener> (const std::unique_ptr<EventQueue>& queue)>;
    private:
        /// @brief the path this controller listened to, when there are
        /// a connection upgrade http request, and the path equals this, then a connection upgrade will be made
        std::string path;
        ListenerAllocator allocator;
    public:
        
        ~WebsocketController() = default;

        const std::string& getPath() {
            return path;
        }

        WebsocketController(const std::string& path, ListenerAllocator listener) 
        : path(path), allocator(listener) {}

        std::unique_ptr<WebsocketSessionHandler::EventListener> 
        service(const std::unique_ptr<EventQueue>& queue);

    };

#define MAKE_WEBSOCKET(path, queue, func, ...) std::make_unique<themis::WebsocketController>(path, \
     [__VA_ARGS__](const std::unique_ptr<themis::EventQueue>& queue) -> std::unique_ptr<themis::WebsocketSessionHandler::EventListener> func)

    /**
     * @brief a web socket controller handles the connection upgrade and
     * dispatch the session to controller for futher operation
     * 
     */
    class WebsocketControllerManager {
    private:
        std::map<std::string, std::unique_ptr<WebsocketController>> controllerMap;
        std::unique_ptr<EventQueue> eventQueue;

        std::string calculateSecKey(std::string client);
        /**
         * @brief serve the handshake response to the old http session
         * 
         * @param secKey client secure key
         * @param old old session
         */
        void serveUpgradeResponse(std::string secKey, const std::unique_ptr<Session>& old);

    public:
        
        WebsocketControllerManager& addController(std::unique_ptr<WebsocketController> controller) {
            controllerMap.insert({controller->getPath(), std::move(controller)});
            return *this;
        }

        /**
         * @brief if the request hit any path in the controller map, then try
         * to upgrade the @param old session into a websocket session
         * 
         * @param request 
         * @param old 
         * @return std::unique_ptr<WebsocketSessionHandler> 
         */
        std::unique_ptr<WebsocketSessionHandler> 
        upgradeSession(const std::unique_ptr<HttpRequest>& request, const std::unique_ptr<Session>& old);
        
        /**
         * @brief poll the event queu for once
         * 
         * @return true if any event has been dispatched
         * @return false 
         */
        bool poll() {
            return eventQueue->poll();
        }
    };


} // namespace themis




#endif
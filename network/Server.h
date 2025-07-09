#include <thread>
#ifndef Server_h
#define Server_h 1

#include "Reactor.h"
#include "web/Controller.h"
#include "web/WebsocketController.h"

namespace themis
{

    /**
     * @brief a server conducts several reactors and
     * 
     */
    class Server {
    private:
        std::unique_ptr<Reactor> httpReactor;
        std::unique_ptr<Reactor> wsReactor;
        bool wsStop = false;
        std::unique_ptr<std::thread> wsThread;
        std::queue<std::unique_ptr<WebsocketSessionHandler>> upgradeQueue;
        std::atomic_flag upgradeFlag;
        ControllerManager controllerManager;
        WebsocketControllerManager wsControllerManager;

    public:
        ~Server();
        /**
         * @brief Construct a new Server listening http on ip:port
         * 
         * @param ip the server ip
         * @param port listening port
         */
        Server(const std::string& ip, uint16_t port);

        /**
         * @brief dispatch the server and loop through the reactor
         * 
         */
        void dispatch();

        ControllerManager& getControllerManager() {
            return controllerManager;
        }

        WebsocketControllerManager& getWebsocketControllerManager() {
            return wsControllerManager;
        }

    };

} // namespace themis




#endif
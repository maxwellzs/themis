#ifndef Server_h
#define Server_h 1

#include "Reactor.h"
#include "web/Controller.h"

namespace themis
{
    
    /**
     * @brief a server conducts several reactors and
     * 
     */
    class Server {
    private:
        std::unique_ptr<Reactor> httpReactor;
        ControllerManager controllerManager;

    public:
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

    };

} // namespace themis




#endif
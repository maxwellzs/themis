#ifndef Reactor_h
#define Reactor_h 1

#include <event2/event.h>
#include <event2/listener.h>
#include <ng-log/logging.h>

#include <string>
#include <list>
#include <functional>
#include "Session.h"


namespace themis
{

    /**
     * @brief a Reactor handles the network sockets and dispatch io events
     * a channel will be listening on a specific ip:port and will keep accept input
     * 
     */
    class Reactor {
    private:

        event_base* base;
        evutil_socket_t listenSocket = -1;
        evconnlistener* listener;

        void onAccept(evutil_socket_t fd, sockaddr_in addr);

        struct SessionDetail {
            Reactor& _this; // which reactor this session belongs to
            std::unique_ptr<SessionHandler> handler; // handler
            SessionDetail(const SessionDetail&) = delete;
            SessionDetail(SessionDetail&& d): _this(d._this), handler(std::move(d.handler)) {}
            SessionDetail(Reactor& r, std::unique_ptr<SessionHandler> handler): _this(r), handler(std::move(handler)) {}
        };

        std::list<SessionDetail> sessionList;
        using SessionIterator = std::list<SessionDetail>::iterator;
        /// this function yield an valid handler
        using HandlerAllocateFunction = std::function<std::unique_ptr<SessionHandler> (std::unique_ptr<Session>)>;

        HandlerAllocateFunction allocator;

        time_t timeout = -1;

        void handleSessionRead(evutil_socket_t fd, const std::unique_ptr<SessionHandler>& handler);
        void handleSessionWrite(evutil_socket_t fd, const std::unique_ptr<Session>& s);

        bool idle = true;
    public:
        /**
         * @brief Construct a new Channel object listening on the given ip and port
         * 
         * @param ip listen ip
         * @param port port
         */
        Reactor(const std::string& ip, uint16_t port, HandlerAllocateFunction allocator);
        ~Reactor();

        /**
         * @brief Set the Connection Timeout, after timeout seconds, the
         * connection will be removed out of the queue
         * 
         * @param timeout 
         */
        void setConnectionTimeout(time_t timeout);

        /**
         * @brief loop through events 
         * 
         */
        void loopOnce();

        bool isIdle() {
            return idle;
        }

    };

} // namespace themis




#endif
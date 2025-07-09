#ifndef Session_h
#define Session_h 1

#include <ng-log/logging.h>
#include "utils/Buffer.h"

namespace themis {

    class Reactor;

    /**
     * @brief a connection has two built in buffer, when 
     * 
     */
    class Session {
    private:
        evutil_socket_t fd;
        Buffer input, output;
        const sockaddr_in addr;
        event* readEvent,* writeEvent;
        time_t lastActiveTime;
        bool alive = true;

    public:
        ~Session();
        Session(sockaddr_in addr, evutil_socket_t fd);
        /// @brief use this move construct a new session during the connection upgrade
        /// @param s 
        Session(Session&& s) 
        : input(std::move(s.input)), output(std::move(s.output)), 
        addr(s.addr), lastActiveTime(s.lastActiveTime), fd(s.fd) {
            s.alive = false;
        }
        
        evutil_socket_t getSocket() { return fd; }
        void setReadEvent(event* e) { readEvent = e;}
        void setWriteEvent(event* e) { writeEvent = e;}
        Buffer& getInputBuffer() { return input;}
        Buffer& getOutputBuffer() { return output;}
        event* getReadEvent() { return readEvent;}
        event* getWriteEvent() { return writeEvent;}

        /**
         * @brief get if this connection is timed out
         * 
         * @param val 
         * @return true 
         * @return false 
         */
        bool isTimedout(time_t val);
        void setLastActive(time_t val) {
            lastActiveTime = val;
        }

        std::string toString();

    };

    /**
     * @brief a session handler is a container to a session, and handle all input
     * data, such as parsing the data into a request or message
     * 
     */
    class SessionHandler {
    protected:
        /// @brief the session this handler is associated to
        std::unique_ptr<Session> session; 

    public:
        void setWriteEvent(event* ev) {
            session->setWriteEvent(ev);
        }
        void setReadEvent(event* ev) {
            session->setReadEvent(ev);
        }

        SessionHandler(std::unique_ptr<Session> s) : session(std::move(s)) {};
        /// this function take over the session 
        /// after this, the old handler should not be in use any more
        SessionHandler(Session&& handler): session(std::make_unique<Session>(std::move(handler))) {}
        virtual ~SessionHandler() = default;
        virtual void handleSession() = 0;
        /// @brief get inner session
        /// @return session pointer
        const std::unique_ptr<Session>& getSession() {
            return session;
        }
        
    };

} // namespace themis

#endif
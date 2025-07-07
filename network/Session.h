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
        Buffer input, output;
        const sockaddr_in addr;
        event* readEvent,* writeEvent;

        time_t lastActiveTime;

        Reactor& parent;
    public:
        ~Session();
        Session(Reactor& parent, sockaddr_in addr);
        
        void setReadEvent(event* e) { readEvent = e;}
        void setWriteEvent(event* e) { writeEvent = e;}
        Buffer& getInputBuffer() { return input;}
        Buffer& getOutputBuffer() { return output;}
        event* getReadEvent() { return readEvent;}
        event* getWriteEvent() { return writeEvent;}
        Reactor& getParentReactor() { return parent;}

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
        SessionHandler(std::unique_ptr<Session> s) : session(std::move(s)) {};
        /// this function take over the handler from @param handler 
        /// after this, handler should be deconstructed
        SessionHandler(SessionHandler&& handler): session(std::move(handler.session)) {}
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
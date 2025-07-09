#include "Reactor.h"
#include <arpa/inet.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <stdexcept>
#include <unistd.h>

void themis::Reactor::onAccept(evutil_socket_t fd, sockaddr_in addr) {

    // add client to the connection list
    evutil_make_socket_nonblocking(fd);
    // make session object and the handler for it
    auto session = std::make_unique<Session>(addr, fd);
    auto handler = allocator(std::move(session));
    SessionDetail detail(*this, std::move(handler));
    sessionList.emplace_back(std::move(detail));
    SessionIterator* itPtr = new SessionIterator(--sessionList.end());
    prepareSession(itPtr);

    VLOG(5) << "accepted session : " << (**itPtr).handler->getSession()->toString();

}

void themis::Reactor::prepareSession(SessionIterator * itPtr) {
    
    evutil_socket_t fd = (**itPtr).handler->getSession()->getSocket();
    // set up read&write events
    event *readEvent = event_new(base, fd, EV_READ | EV_TIMEOUT | EV_PERSIST, [](evutil_socket_t fd, short ev, void *args) {

        SessionIterator *it = reinterpret_cast<SessionIterator *>(args);
        Reactor &parent = (**it)._this;
        parent.idle = false;
        const std::unique_ptr<SessionHandler>& handler = (**it).handler;

        // handle read
        try {
            if (ev & EV_TIMEOUT) {
                // connection timed out
                throw std::exception();
            }
            (**it).handler->getSession()->setLastActive(time(nullptr));
            parent.handleSessionRead(fd,handler);
        } catch(const SessionMovedException& move) {
            // the session no longer belongs to this reactor
            parent.sessionList.erase(*it);
            delete it;
        }catch (const std::exception &e) {
            // error in session
            // remove connection
            VLOG(5) << "session closed : " << handler->getSession()->toString();
            parent.sessionList.erase(*it);
            delete it;
        }
        
    },itPtr);

    event *writeEvent = event_new(base, fd, EV_WRITE | EV_TIMEOUT, [](evutil_socket_t fd, short ev, void *args) {

        SessionIterator *it = reinterpret_cast<SessionIterator *>(args);
        Reactor &parent = (**it)._this;
        parent.idle = false;
        const std::unique_ptr<Session>& session = (**it).handler->getSession();

        try {
            if (ev & EV_TIMEOUT) {
                // connection timed out
                throw std::exception();
            }
            (**it).handler->getSession()->setLastActive(time(nullptr));
            parent.handleSessionWrite(fd, session);
        } catch (const std::exception &e) {
            VLOG(5) << "session closed : " << session->toString();
            parent.sessionList.erase(*it);
            delete it;
        }
    },itPtr);

     // enable read event
    event_add(readEvent, nullptr);
    (**itPtr).handler->getSession()->setReadEvent(readEvent);
    (**itPtr).handler->getSession()->setWriteEvent(writeEvent);
}

void themis::Reactor::handleSessionRead(evutil_socket_t fd, const std::unique_ptr<SessionHandler> &handler) {
    BufferWriter writer(handler->getSession()->getInputBuffer());
    writer.receiveFrom(fd);
    // call handler to process the input buffer
    // if the session in the handler no longer exists, it means a connection upgrade
    handler->handleSession();
}

void themis::Reactor::handleSessionWrite(evutil_socket_t fd, const std::unique_ptr<Session> &s) {
    BufferReader reader(s->getOutputBuffer());
    bool again = reader.sendTo(fd);
    // if there are more data to send, toggle the write event again
    if (again) {
        event_add(s->getWriteEvent(), nullptr);
    }
}

themis::Reactor::Reactor(const std::string &ip, uint16_t port, HandlerAllocateFunction allocator) :
    base(event_base_new()), allocator(allocator) {
    if (!base) throw std::runtime_error("cannot create event base");

    sockaddr_in listenAddr;
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port = htons(port);
    listenAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    listenSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listenSocket < 0) throw std::runtime_error("cannot create listening socket");

    listener = evconnlistener_new_bind(base, [](struct evconnlistener *, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *ptr) {

        Reactor *thisPtr = reinterpret_cast<Reactor *>(ptr);
        thisPtr->idle = false;
        thisPtr->onAccept(fd, *reinterpret_cast<sockaddr_in *>(addr));

    },this, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 256, reinterpret_cast<sockaddr *>(&listenAddr), sizeof(sockaddr_in));

    evconnlistener_set_error_cb(listener, [](struct evconnlistener *, void *ptr) {
        LOG(FATAL) << "error when accept";
    });
    evconnlistener_enable(listener);

    LOG(INFO) << "Reactor listening at " << ip << ":" << port;
}

themis::Reactor::Reactor() : base(event_base_new()) {

}

themis::Reactor::~Reactor() {
    if(listener) evconnlistener_free(listener);
    event_base_free(base);
}

void themis::Reactor::setConnectionTimeout(time_t timeout) {
    timeout = timeout;
}

void themis::Reactor::addSessionHandler(std::unique_ptr<SessionHandler> handler) {

    SessionDetail detail(*this, std::move(handler));
    sessionList.emplace_back(std::move(detail));
    SessionIterator* it = new SessionIterator(--sessionList.end());
    prepareSession(it);

}

void themis::Reactor::loopOnce() {

    idle = true;
    // check for timeout if needed
    if (timeout > 0) {
        for (auto i = sessionList.begin(); i != sessionList.end();) {
            if ((*i).handler->getSession()->isTimedout(timeout)) {
                i = sessionList.erase(i);
            } else {
                ++i;
            }
        }
    }

    // loop once through events
    if (event_base_loop(base, EVLOOP_NONBLOCK) < 0) {
        throw std::runtime_error("event base loop error");
    }
}

#include "Server.h"
#include "network/Session.h"
#include "protocol/http/HttpSessionHandler.h"

themis::Server::Server(const std::string &ip, uint16_t port) {
    httpReactor = std::make_unique<Reactor>(ip, port,
        [this](std::unique_ptr<Session> session) -> std::unique_ptr<SessionHandler> {
            return std::make_unique<HttpSessionHandler>(std::move(session), [this](std::unique_ptr<HttpRequest> req, 
        const std::unique_ptr<Session>& session) {
            controllerManager.serveRequest(std::move(req), session);
        });
    });
}

void themis::Server::dispatch() {
    for(;;) {

        httpReactor->loopOnce();

        bool busy = 
        httpReactor->isIdle() | 
        controllerManager.poll();

        if(busy) {
            // no event has been dispatched, then yield current thread
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

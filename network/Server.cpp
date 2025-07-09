#include "Server.h"
#include "network/Session.h"
#include "protocol/http/HttpSessionHandler.h"
#include "utils/Spinlock.h"

themis::Server::~Server() {
    if(wsThread.get() && 
    wsThread->joinable()) wsThread->join();
}

themis::Server::Server(const std::string &ip, uint16_t port) {
    httpReactor = std::make_unique<Reactor>(ip, port,
        [this](std::unique_ptr<Session> session) -> std::unique_ptr<SessionHandler> {

            return std::make_unique<HttpSessionHandler>(std::move(session), [this](std::unique_ptr<HttpRequest> req, 
        const std::unique_ptr<Session>& session) {
            // firstly check if the session can be upgraded into a websocket session
            auto wsHandler = wsControllerManager.upgradeSession(req, session);
            if(wsHandler.get()) {
                // upgrade succeeded, then remove the original handler by raising an exception
                // also add to the second reactor
                Spinlock lock(upgradeFlag);
                upgradeQueue.push(std::move(wsHandler));
                lock.unlock();
                // inform the http reactor to clean this mess up
                throw SessionMovedException();
            } else {
                // no upgrade made, then 
                controllerManager.serveRequest(std::move(req), session);
            }
        });
        
    });

    wsReactor = std::make_unique<Reactor>();
    wsThread = std::make_unique<std::thread>([this]() {
        while(!wsStop) {
            if(! upgradeQueue.empty()) {
                Spinlock lock(upgradeFlag);
                // check if there are any pending upgrades
                while(!upgradeQueue.empty()) {
                    std::unique_ptr<SessionHandler> handler = std::move(upgradeQueue.front());
                    upgradeQueue.pop();
                    wsReactor->addSessionHandler(std::move(handler));
                }
                lock.unlock();
            }
            
            // loop through ws reactor for io events
            wsReactor->loopOnce();

            bool idle = 
            wsReactor->isIdle() |
            !wsControllerManager.poll();

            if(idle) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    });
}

void themis::Server::dispatch() {
    for(;;) {

        httpReactor->loopOnce();

        bool idle = 
        httpReactor->isIdle() | 
        !controllerManager.poll();

        if(idle) {
            // no event has been dispatched, then yield current thread
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

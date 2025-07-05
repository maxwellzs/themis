#include "Controller.h"

bool themis::MethodFilter::filter(const std::unique_ptr<HttpRequest> &req) {
    return method == req->getMethod();
}

void themis::ControllerManager::serveNotFound(const std::unique_ptr<Session> &session, const std::string &path) {
    // return a not found
    HttpResponse notfound;
    notfound.setStatus(404);
    notfound.getResponseStream() << "controller at path \"" << path << "\" not found";
    notfound.serializeToBuffer(session->getOutputBuffer());
    // enable write event and reset timeout
    session->setLastActive(time(nullptr));
    event_add(session->getWriteEvent(), nullptr);
}

void themis::ControllerManager::serveInternalError(const std::unique_ptr<Session> &session, const std::string& path, std::unique_ptr<std::exception> e) {
    
    HttpResponse internalError;
    internalError.setStatus(500);
    internalError.getResponseStream() << "controller at path \"" 
    << path << "\" failed the response promise with error : \r\n"
    <<  e->what();
    internalError.serializeToBuffer(session->getOutputBuffer());
    session->setLastActive(time(nullptr));
    event_add(session->getWriteEvent(), nullptr);
}

themis::ControllerManager &themis::ControllerManager::addController(std::unique_ptr<Controller> controller) {
    controllerMap.insert({controller->getPath(), std::move(controller)});
    return *this;
}

void themis::ControllerManager::serveRequest(std::unique_ptr<HttpRequest> req, 
    const std::unique_ptr<Session> &session) {
    // try to match a controller
    std::string& path = req->getPath();
    if(controllerMap.count(path)) {

        ResponseDetail detail(session, controllerMap.at(path)->service(std::move(req)), path);
        responseList.emplace_back(std::move(detail));
        // assign callback funciton when user resolve with response
        responseList.back().promise->then([detailIterator = --responseList.end(), 
        &detailList = responseList]
        (std::unique_ptr<HttpResponse> resp) {

            // user finish response
            const auto& session = (*detailIterator).sessionRef;
            resp->serializeToBuffer(session->getOutputBuffer());
            event_add(session->getWriteEvent(), nullptr);
            // disassociate response
            detailList.erase(detailIterator);

        })->except([detailIterator = --responseList.end(), 
        &detailList = responseList]
        (std::unique_ptr<std::exception> e) {

            const auto& session = (*detailIterator).sessionRef;
            // user fail the promise, then return a 500 internal error
            serveInternalError(session, (*detailIterator).path, std::move(e));
            // dissociate response
            detailList.erase(detailIterator);

        });

    } else {

        // not found
        serveNotFound(session, path);

    }
}

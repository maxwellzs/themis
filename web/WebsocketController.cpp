#include "WebsocketController.h"
#include "protocol/http/HttpResponse.h"
#include "protocol/websocket/WebsocketSessionHandler.h"
#include <boost/uuid/detail/sha1.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>

using boost::uuids::detail::sha1;
using namespace boost::archive::iterators;

std::string themis::WebsocketControllerManager::calculateSecKey(std::string client) {
    client += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    sha1 sha;
    sha.process_bytes(client.data(), client.length());
    unsigned char digest[20];
    sha.get_digest(digest);
    client.resize(20);
    std::memcpy(client.data(), digest, 20);
    // encode the hash into base64
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(client)), It(std::end(client)));
    return tmp.append((3 - client.size() % 3) % 3, '=');
}

void themis::WebsocketControllerManager::serveUpgradeResponse(std::string secKey, const std::unique_ptr<Session> &old) {
    std::string key = calculateSecKey(secKey);
    HttpResponse resp;
    // return a upgrade response to client
    resp.setStatus(101);
    resp.getHeaders().insert({"Upgrade", "websocket"});
    resp.getHeaders().insert({"Connection", "Upgrade"});
    resp.getHeaders().insert({"Sec-WebSocket-Accept", key});
    resp.serializeToBuffer(old->getOutputBuffer());
}

std::unique_ptr<themis::WebsocketSessionHandler>
themis::WebsocketControllerManager::upgradeSession(const std::unique_ptr<HttpRequest> &request, const std::unique_ptr<Session> &old) {
    auto& path = request->getPath();
    std::string connection;
    std::string secKey;

    if(controllerMap.count(path) && 
    request->getHeader("Connection", connection) && 
    connection == "Upgrade" &&
    request->getHeader("Sec-WebSocket-key", secKey)) {

        serveUpgradeResponse(secKey, old);
        // upgrade the old session into websocket session
        auto handler =  std::make_unique<WebsocketSessionHandler>(old);
        auto listener = controllerMap.at(path)->service(eventQueue, *handler.get());
        handler->setListener(std::move(listener));
        return handler;
    } 
    return nullptr;
}

std::unique_ptr<themis::WebsocketSessionHandler::EventListener> 
themis::WebsocketController::service(const std::unique_ptr<EventQueue> &queue, WebsocketSessionHandler& handler) {
    return allocator(queue, handler);
}

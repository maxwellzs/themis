#include "network/Server.h"
#include "protocol/websocket/WebsocketSessionHandler.h"
#include "web/WebsocketController.h"
#include <ng-log/logging.h>

class SampleEventListener : public themis::WebsocketSessionHandler::EventListener {
public:
    SampleEventListener(themis::WebsocketSessionHandler& handler, const std::unique_ptr<themis::EventQueue>& queue) 
    : themis::WebsocketSessionHandler::EventListener(handler, queue) {}
    virtual void onText(themis::WebsocketSessionHandler& handler, const std::string& msg) override {
        ws.getOutputStream() << std::string(1000, '@');
        ws.finish(true);
        LOG(INFO) << "message : " << msg;
    };
    virtual void onBinary(themis::WebsocketSessionHandler& handler, const std::vector<uint8_t>& msg) override {
        LOG(INFO) << "received binary of " << msg.size() << " byte(s)";
    };
    virtual void onDisconnect() override {
        LOG(INFO) << "client disconnected";
    };        
};

int main(int argc, char **argv) {
    FLAGS_alsologtostderr = 1;
    FLAGS_colorlogtostderr = 1;
    FLAGS_v = 10;
    FLAGS_log_dir = ".";
    
    nglog::InitializeLogging(argv[0]);
    LOG(INFO) << "themis 2.0 started";

    themis::Server server("0.0.0.0", 8080);
    server.getWebsocketControllerManager().addController<SampleEventListener>("/ws/sample");
    server.dispatch();

    nglog::ShutdownLogging();

    return 0;
}
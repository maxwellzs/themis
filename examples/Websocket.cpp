#include "network/Server.h"
#include "protocol/websocket/WebsocketSessionHandler.h"
#include "web/WebsocketController.h"
#include <ng-log/logging.h>

class SampleEventListener : public themis::WebsocketSessionHandler::EventListener {
public:
    virtual void onText(themis::WebsocketSessionHandler& handler, const std::string& msg) override {
        LOG(INFO) << "message : msg";
    };
    virtual void onBinary(themis::WebsocketSessionHandler& handler, const std::vector<uint8_t>& buffer) override {
        LOG(INFO) << "received binary of " << buffer.size() << " byte(s)";
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
    server.getWebsocketControllerManager()
    .addController(MAKE_WEBSOCKET("/ws/sample", queue, {
        return std::make_unique<SampleEventListener>();
    }));
    server.dispatch();

    nglog::ShutdownLogging();

    return 0;
}
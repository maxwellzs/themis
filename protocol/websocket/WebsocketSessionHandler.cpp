#include "WebsocketSessionHandler.h"
#include "protocol/websocket/WebsocketFrame.h"
#include "utils/Buffer.h"
#include <ng-log/logging.h>

void themis::WebsocketSessionHandler::handleSession() {
    BufferReader reader(session->getInputBuffer());
    
    try {
        pendingFrame->readFrom(reader);
    } catch(const std::exception& e) {
        // when frame fail to parse, close the session
        LOG(INFO) << "invalid websocket frame : " << e.what();
        throw e;
    }
    
    if(pendingFrame->getState() == WebsocketFrame::COMPLETE) {
        
    }
}
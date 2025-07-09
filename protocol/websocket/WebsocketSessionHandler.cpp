#include "WebsocketSessionHandler.h"
#include "protocol/websocket/WebsocketFrame.h"
#include "utils/Buffer.h"
#include <ng-log/logging.h>

void themis::WebsocketSessionHandler::dispatchFrame() {

    auto& payload = pendingFrame->getPayload();

    const auto writeVisitor = MessageVisitor {
        [&](std::vector<uint8_t>& v) {
            v.insert(v.end(), payload.begin(), payload.end());
        },
        [&](std::string& s) {
            s.append(reinterpret_cast<std::string::traits_type::char_type *>(payload.data()), payload.size());
        }
    };

    const auto readVisitor = MessageVisitor {
        [this](std::vector<uint8_t>& v) {
            listener->onBinary(*this, v);
        },
        [this](std::string& s) {
            listener->onText(*this, s);
        }
    };

    switch (pendingFrame->getOperationCode()) {
        case WebsocketFrameHeader::TEXT_FRAME:
            message = std::string();
            std::visit(writeVisitor,message);
            break;
        case WebsocketFrameHeader::BINARY_FRAME:
            message = std::vector<uint8_t>();
            std::visit(writeVisitor, message);
            break;
        case WebsocketFrameHeader::CONTINUATION_FRAME:
            std::visit(writeVisitor, message);
            break;
        case WebsocketFrameHeader::CONNECTION_CLOSE_FRAME:
            // inform reactor to end the session
            throw std::exception();
        case WebsocketFrameHeader::PING_FRAME:
            
        case WebsocketFrameHeader::PONG_FRAME:
          break;
    }

    if((pendingFrame->getOperationCode() == WebsocketFrameHeader::TEXT_FRAME ||
    pendingFrame->getOperationCode() == WebsocketFrameHeader::BINARY_FRAME || 
    pendingFrame->getOperationCode() == WebsocketFrameHeader::CONTINUATION_FRAME) && (pendingFrame->isFinalFrame())) {
        // the message has ended, invoke listener
        std::visit(readVisitor, message);
    }
}

void themis::WebsocketSessionHandler::handleSession() {
    BufferReader reader(session->getInputBuffer());
    
    try {
        pendingFrame->parseFrom(reader);
    } catch(const std::exception& e) {
        // when frame fail to parse, close the session
        LOG(INFO) << "invalid websocket frame : " << e.what();
        throw e;
    }
    
    if(pendingFrame->getState() == WebsocketFrame::COMPLETE) {
        dispatchFrame();
        // prepare next frame
        pendingFrame->reset();
    }
}
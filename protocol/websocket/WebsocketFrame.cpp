#include "WebsocketFrame.h"
#include <stdexcept>

const std::map<uint8_t, themis::WebsocketFrame::Operation> themis::WebsocketFrame::OPERATION_MAP = {
    {0x0, CONTINUATION_FRAME},
    {0x1, TEXT_FRAME},
    {0x2, BINARY_FRAME},
    {0x8, CONNECTION_CLOSE_FRAME},
    {0x9, PING_FRAME},
    {0xA, PONG_FRAME},
};

void themis::WebsocketFrame::parseBody(BufferReader &reader) {
    size_t acquired = reader.getBytes(payload.data() + receivedLength, payloadLength - receivedLength);
    // if the client use xor masking, then mask through the payload
    if(masked) {
        for (size_t i = receivedLength; i < acquired + receivedLength; i++)
        {
            payload[i] ^= maskingKey[i % 4];
        }
    }
    receivedLength += acquired;
    if(receivedLength == payloadLength) {
        state = COMPLETE;
    }
}

void themis::WebsocketFrame::parseHeader(BufferReader &reader) {
    uint8_t header[2];

    size_t acquired = reader.getBytes(header, 2);
    if(acquired != 2) {
        reader.revert();
        return;
    }
    // fin flag
    finalFrame = header[0] >> 7;
    rsv[0] = (header[0] >> 6) & 1;
    rsv[1] = (header[0] >> 5) & 1;
    rsv[2] = (header[0] >> 4) & 1;
    
    // opcode
    uint8_t op = header[0] & 0xf;
    if(!OPERATION_MAP.count(op)) throw std::runtime_error("unknown operation code : " + std::to_string(op));
    opcode = OPERATION_MAP.at(op);

    masked = header[1] >> 7;
    payloadLength = header[1] & 0x7f;
    if(payloadLength == 126) {
        // extended payload length
        uint16_t extended;
        acquired = reader.getBytes(&extended, 2);
        if(acquired != 2) {
            reader.revert();
            return;
        }
        payloadLength = ntohs(extended);
    } else if (payloadLength == 127) {
        uint32_t extended;
        acquired = reader.getBytes(&extended, 4);
        if(acquired != 4) {
            reader.revert();
            return;
        }
        payloadLength = ntohl(extended);
    } 

    payload.resize(payloadLength);

    if (masked) {
        // masked key
        acquired = reader.getBytes(maskingKey, 4);
        if(acquired != 4) {
            reader.revert();
            return;
        }
    }

    state = AWAIT_PAYLOAD;
    reader.finialize();
    parseBody(reader);
}

void themis::WebsocketFrame::readFrom(BufferReader &reader) {

    switch (state)
    {
    case AWAIT_HEADER:
        parseHeader(reader);
        break;
    case AWAIT_PAYLOAD:
        parseBody(reader);
        break;
    default: break;
    }
    
}

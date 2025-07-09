#include "WebsocketFrame.h"
#include <endian.h>
#include <stdexcept>

const std::map<uint8_t, themis::WebsocketFrameHeader::Operation> themis::WebsocketFrameHeader::OPERATION_MAP = {
    {0x0, CONTINUATION_FRAME},
    {0x1, TEXT_FRAME},
    {0x2, BINARY_FRAME},
    {0x8, CONNECTION_CLOSE_FRAME},
    {0x9, PING_FRAME},
    {0xA, PONG_FRAME},
};

const std::map<themis::WebsocketFrameHeader::Operation,uint8_t> themis::WebsocketFrameHeader::OPCODE_MAP = {
    {CONTINUATION_FRAME, 0x0},
    {TEXT_FRAME, 0x1},
    {BINARY_FRAME, 0x2},
    {CONNECTION_CLOSE_FRAME, 0x8},
    {PING_FRAME, 0x9},
    {PONG_FRAME, 0xA},
};

void themis::WebsocketFrame::parseBody(BufferReader &reader) {
    size_t acquired = reader.getBytes(payload.data() + receivedLength, header.payloadLength - receivedLength);
    // if the client use xor masking, then mask through the payload
    if(header.masked) {
        for (size_t i = receivedLength; i < acquired + receivedLength; i++)
        {
            payload[i] ^= header.maskingKey[i % 4];
        }
    }
    receivedLength += acquired;
    if(receivedLength == header.payloadLength) {
        state = COMPLETE;
        reader.finialize();
    }
}

void themis::WebsocketFrame::parseFrom(BufferReader &reader) {

    switch (state)
    {
    case AWAIT_HEADER:
        if(header.parseFrom(reader)) {
            // the header has ended, then parse body
            state = AWAIT_PAYLOAD;
            payload.resize(header.payloadLength);
            receivedLength = 0;
            parseBody(reader);
        }
        break;
    case AWAIT_PAYLOAD:
        parseBody(reader);
        break;
    default: break;
    }
    
}

bool themis::WebsocketFrameHeader::parseFrom(BufferReader &reader) {
    uint8_t header[2];

    size_t acquired = reader.getBytes(header, 2);
    if(acquired != 2) {
        reader.revert();
        return false;
    }
    // fin flag
    finalFrame = header[0] >> 7;
    rsv[0] = (header[0] >> 6) & 1;
    rsv[1] = (header[0] >> 5) & 1;
    rsv[2] = (header[0] >> 4) & 1;
    
    // opcode
    uint8_t op = header[0] & 0xf;
    if(!OPERATION_MAP.count(op)) { 
        throw std::runtime_error("unknown operation code : " + std::to_string(op));
    }
    opcode = OPERATION_MAP.at(op);

    masked = header[1] >> 7;
    payloadLength = header[1] & 0x7f;
    if(payloadLength == 126) {
        // extended payload length
        uint16_t extended;
        acquired = reader.getBytes(&extended, 2);
        if(acquired != 2) {
            reader.revert();
            return false;
        }
        payloadLength = be16toh(extended);
    } else if (payloadLength == 127) {
        uint64_t extended;
        acquired = reader.getBytes(&extended, 4);
        if(acquired != 4) {
            reader.revert();
            return false;
        }
        payloadLength = be64toh(extended);
    } 

    if (masked) {
        // masked key
        acquired = reader.getBytes(maskingKey, 4);
        if(acquired != 4) {
            reader.revert();
            return false;
        }
    }

    return true;
}

void themis::WebsocketFrameHeader::writeTo(BufferWriter &writer) {
    uint8_t op = OPCODE_MAP.at(opcode);
    uint8_t fin = finalFrame ? 0x80 : 0;
    uint8_t r = (rsv[0] ? 0x40 : 0) | (rsv[1] ? 0x20 : 0) | (rsv[2] ? 0x10 : 0);
    size_t headerSize = 2;
    if(payloadLength > 0xffff) {
        headerSize += 8;
    } else if(payloadLength > 125) {
        headerSize += 2;
    } 
    if(masked) headerSize += 4;
    std::vector<uint8_t> rawHeader(headerSize, 0);
    // write fin|rsv1|rsv2|rsv3|opcode|
    rawHeader[0] = op | fin | r;
    size_t offset = 0;
    // write masked|length|length extended
    if(payloadLength > 0xffff) {
        // the length take 64 bits
        rawHeader[1] = (masked ? 0x80 : 0) | 0x7f;
        uint64_t netPayloadLength = htobe64(payloadLength); 
        std::memcpy(rawHeader.data() + 2, &netPayloadLength, sizeof(netPayloadLength));
        offset = 8;
    } else if(payloadLength > 125) {
        // the length take 16 bits
        rawHeader[1] = (masked ? 0x80 : 0) | 0x7e;
        uint16_t netPayloadLength = htobe16((uint16_t) payloadLength);
        std::memcpy(rawHeader.data() + 2, &netPayloadLength, sizeof(netPayloadLength));
        offset = 2;
    } else {
        // no extended length
        rawHeader[1] = (masked ? 0x80 : 0) | ((uint8_t) payloadLength);
    }
    // write masking key if masked
    std::memcpy(rawHeader.data() + 2 + offset, maskingKey, 4);
    // put raw to buffer
    writer.write(rawHeader.data(), rawHeader.size());
}

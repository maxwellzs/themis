#include "WebsocketWriter.h"
#include "WebsocketFrame.h"

void themis::WebsocketWriter::finish(bool text) {
    
    WebsocketFrameHeader header;
    header.setOperation(text ? WebsocketFrameHeader::TEXT_FRAME : WebsocketFrameHeader::BINARY_FRAME);

    std::vector<uint8_t> temp(maxPayloadSize, 0);
    while(!outStream.eof()) {

        outStream.read(reinterpret_cast<char*>(temp.data()), maxPayloadSize);
        std::streamsize acquiredSize = outStream.gcount();
        // peek one incase the message is exactly the size of maxPayloadSize
        outStream.peek();

        header.setPayloadLength(acquiredSize);

        if (acquiredSize < maxPayloadSize ||
        outStream.eof()) {
            // last fragment
            header.setFinalFrame(true);
        }
        // mask the payload
        size_t index = 0;

        // serialize the header to buffer
        header.writeTo(baseWriter);
        // write payload
        baseWriter.write(temp.data(), acquiredSize);
        header.setOperation(WebsocketFrameHeader::CONTINUATION_FRAME);
        
    }   
    // when finished, reset the buffer and its data
    outStream.clear();
    outStream.str() = "";
}

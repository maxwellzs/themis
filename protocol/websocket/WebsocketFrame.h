#ifndef WebsocketFrame_h
#define WebsocketFrame_h 1

#include <cstddef>
#include <cstdint>
#include <vector>
#include <map>
#include "utils/Buffer.h"

namespace themis
{
    
    /**
     * @brief the websocket frame is the smallest transfer unit of the websocket protocol
     * 
     */
    class WebsocketFrame {
    public:
        
        enum Operation {
            CONTINUATION_FRAME, 
            TEXT_FRAME,
            BINARY_FRAME,
            CONNECTION_CLOSE_FRAME,
            PING_FRAME,
            PONG_FRAME
        };

        enum State {
            AWAIT_HEADER,
            AWAIT_PAYLOAD,
            COMPLETE
        };

        const static std::map<uint8_t, Operation> OPERATION_MAP;

    private:

        State state = AWAIT_HEADER;
        bool finalFrame = false;
        bool rsv[3];
        Operation opcode;
        bool masked;
        size_t payloadLength;
        size_t receivedLength;
        uint8_t maskingKey[4];
        std::vector<uint8_t> payload;

        void parseHeader(BufferReader& reader);
        void parseBody(BufferReader& reader);

    public:

        State getState() const { return state; }
        Operation getOperationCode() const { return opcode; }
        std::vector<uint8_t>& getPayload() { return payload; }
        /**
         * @brief try to convert data from the given buffer reader
         * 
         * @param reader reader
         * @return true if the frame is completed
         */
        void readFrom(BufferReader& reader);

    };

} // namespace themis




#endif
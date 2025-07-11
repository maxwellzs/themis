#ifndef WebsocketFrame_h
#define WebsocketFrame_h 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>
#include "utils/Buffer.h"

namespace themis
{

    class WebsocketFrame;

    class WebsocketFrameHeader {
    public:

        friend WebsocketFrame;
        enum Operation {
            CONTINUATION_FRAME, 
            TEXT_FRAME,
            BINARY_FRAME,
            CONNECTION_CLOSE_FRAME,
            PING_FRAME,
            PONG_FRAME
        };

    private:

        const static std::map<uint8_t, Operation> OPERATION_MAP;
        const static std::map<Operation, uint8_t> OPCODE_MAP;
        bool finalFrame = false;
        bool rsv[3];
        Operation opcode;
        bool masked;
        uint64_t payloadLength;
        uint8_t maskingKey[4];

    public:
        /**
         * @brief Construct a new Websocket Frame header using mask
         * 
         * @param mask the mask
         */
        WebsocketFrameHeader(uint8_t mask[4]) {
            masked = true;
            std::memset(rsv,0, sizeof(rsv));
            std::memcpy(maskingKey, mask, 4);
        }
        /**
         * @brief Construct a new Websocket Frame Header without masking operation
         * 
         */
        WebsocketFrameHeader() {
            masked = false;
            std::memset(rsv,0, sizeof(rsv));
        }
        /**
         * @brief try to parse the header from the given buffer
         * 
         * @param reader buffer reader referencing input buffer
         * @return true the header is completed
         * @return false 
         */
        bool parseFrom(BufferReader& reader);

        /**
         * @brief write the frame header to the given buffer writer
         * 
         * @param writer target buffer writer
         */
        void writeTo(BufferWriter& writer);

        void setFinalFrame(bool b) { finalFrame = b; }
        void setOperation(Operation op) { opcode = op; }
        void setPayloadLength(uint64_t length) { payloadLength = length; }
    };
    
    /**
     * @brief the websocket frame is the smallest transfer unit of the websocket protocol
     * 
     */
    class WebsocketFrame {
    public:

        enum State {
            AWAIT_HEADER,
            AWAIT_PAYLOAD,
            COMPLETE
        };

    private:

        State state = AWAIT_HEADER;
        WebsocketFrameHeader header;
        size_t receivedLength;
        std::vector<uint8_t> payload;

        void parseBody(BufferReader& reader);

    public:

        bool isFinalFrame() const { return header.finalFrame; }
        size_t getPayloadLength() const { return header.payloadLength; }
        State getState() const { return state; }
        WebsocketFrameHeader::Operation getOperationCode() const { return header.opcode; }
        std::vector<uint8_t>& getPayload() { return payload; }
        /**
         * @brief reset this frame into default state
         * 
         */
        void reset() {
            state = AWAIT_HEADER;
            payload.clear();
        }
        /**
         * @brief try to convert data from the given buffer reader
         * 
         * @param reader reader
         * @return true if the frame is completed
         */
        void parseFrom(BufferReader& reader);

    };

   

} // namespace themis




#endif
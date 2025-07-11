#ifndef WebsocketWriter_h
#define WebsocketWriter_h

#include "utils/Buffer.h"
#include <sstream>
#include <boost/random.hpp>

namespace themis
{
    
     /**
     * @brief this is the proxy class to the BufferWriter, 
     * which wrap the text & binary message to the websocket frames
     * 
     */
    class WebsocketWriter {
    private:
        BufferWriter baseWriter;
        std::stringstream outStream;
        // if the payload exceeded this, then fragment
        size_t maxPayloadSize = 256;
    public:
        WebsocketWriter(WebsocketWriter&& rhs) 
        : baseWriter(rhs.baseWriter), outStream(std::move(rhs.outStream)) {}
        WebsocketWriter(Buffer& writer) : baseWriter(writer) {}  
        std::ostream& getOutputStream() {
            return outStream;
        }

        /**
         * @brief parse the data in the outStream into web socket frames 
         * and move the generated data into the base writer
         * 
         * @param mode what mode this writer should interprete the data, true if the data is pure text
         */
        void finish(bool text);
        void setMaxPayloadSize(size_t newSize) {
            maxPayloadSize = newSize;
        }
    };

} // namespace themis




#endif
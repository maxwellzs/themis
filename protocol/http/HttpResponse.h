#ifndef HttpResponse_h
#define HttpResponse_h 1

#include <cstdint>
#include <string>
#include <map>
#include <sstream>
#include "utils/Buffer.h"

namespace themis {

    class HttpResponse {
    private:
        std::string status;
        std::map<std::string, std::string> headers;        
        std::ostringstream body;
    public:

        /**
         * @brief Set the Date field in header to now
         * 
         */
        void setDateToNow();
        void setStatus(uint16_t status);
        std::map<std::string, std::string>& getHeaders() {
            return headers;
        }
        std::ostringstream& getResponseStream() {
            return body;
        }
        
        HttpResponse() { 
            setStatus(200); 
            headers.insert({"Server", "themis"});
            headers.insert({"Content-Type", "text/plain"});
            setDateToNow();
        }
        
        /**
         * @brief serialize the entire response to the buffer, 
         * including status line, headers and body
         * 
         * @param buffer target buffer
         */
        void serializeToBuffer(Buffer& buffer);
    };

} // namespace themis

#endif
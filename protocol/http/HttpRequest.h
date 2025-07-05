#ifndef HttpRequest_h
#define HttpRequest_h 1

#include <string>
#include <cstdint>
#include <vector>
#include <map>

namespace themis
{

    class HttpRequest {
    public:
        enum Method {
            GET,
            HEAD,
            POST,
            PUT,
            DELETE,
            CONNECT,
            OPTIONS,
            TRACE,
            PATCH
        };

    private:

        Method m;
        std::string version;
        std::string path;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> parameters;
        std::vector<uint8_t> body;
        const static std::map<std::string, Method> METHOD_MAP;

    public:
        std::string& getPath() { return path; }
        std::string& getVersion() { return version; }
        std::map<std::string, std::string>& getHeaders() { return headers;}
        std::map<std::string, std::string>& getParameters() { return parameters;}
        std::vector<uint8_t>& getBody() { return body; }
        Method getMethod() { return m; }
        /**
         * @brief try to get a header value, if present, return true
         * otherwise do nothing and return false
         * 
         * @param key 
         * @param out 
         * @return true 
         * @return false 
         */
        bool getHeader(std::string key, std::string& out);
        /**
         * @brief Set the Method according the string, if the method is unknown, 
         * cast out an exception
         * 
         * @param methodStr the method
         */
        void setMethod(const std::string& methodStr);
    };

} // namespace themis




#endif
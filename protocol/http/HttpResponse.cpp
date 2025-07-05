#include "HttpResponse.h"
#include <cstring>
#include <stdexcept>

void themis::HttpResponse::setDateToNow() {
    const static std::string FORMAT = "%a, %d %b %Y %X GMT";
    time_t now = time(nullptr);
    tm* t = gmtime(&now);
    char tmp[49];
    size_t l = strftime(tmp, 48, FORMAT.c_str(), t);
    tmp[l] = 0;
    headers.insert({"Date", std::string(tmp)});
}

void themis::HttpResponse::setStatus(uint16_t status) {
    const static std::map<uint16_t, std::string> STATUS_MAP = {
#define _M(s,t) {s, std::string(#s) + " " + #t}
        _M(100,Continue),                     
        _M(101,Switching Protocols),
        _M(102,Processing),
        _M(103,Early Hints),
        _M(104,Upload Resumption Supported),
        _M(200,OK),
        _M(201,Created),
        _M(202,Accepted),
        _M(203,Non-Authoritative Information),
        _M(204,No Content),
        _M(205,Reset Content),
        _M(206,Partial Content),
        _M(207,Multi-Status),
        _M(208,Already Reported),
        _M(226,IM Used),
        _M(300,Multiple Choices),
        _M(301,Moved Permanently),
        _M(302,Found),
        _M(303,See Other),
        _M(304,Not Modified),
        _M(305,Use Proxy),
        _M(307,Temporary Redirect),
        _M(308,Permanent Redirect),
        _M(400,Bad Request),
        _M(401,Unauthorized),
        _M(402,Payment Required),
        _M(403,Forbidden),
        _M(404,Not Found),
        _M(405,Method Not Allowed),
        _M(406,Not Acceptable),
        _M(407,Proxy Authentication Required),
        _M(408,Request Timeout),
        _M(409,Conflict),
        _M(410,Gone),
        _M(411,Length Required),
        _M(412,Precondition Failed),
        _M(413,Content Too Large),
        _M(414,URI Too Long),
        _M(415,Unsupported Media Type),
        _M(416,Range Not Satisfiable),
        _M(417,Expectation Failed),
        _M(421,Misdirected Request),
        _M(422,Unprocessable Content),
        _M(423,Locked),
        _M(424,Failed Dependency),
        _M(425,Too Early),
        _M(426,Upgrade Required),
        _M(428,Precondition Required),
        _M(429,Too Many Requests),
        _M(431,Request Header Fields Too Large),
        _M(451,Unavailable For Legal Reasons),
        _M(500,Internal Server Error),
        _M(501,Not Implemented),
        _M(502,Bad Gateway),
        _M(503,Service Unavailable),
        _M(504,Gateway Timeout),
        _M(505,HTTP Version Not Supported),
        _M(506,Variant Also Negotiates),
        _M(507,Insufficient Storage),
        _M(508,Loop Detected),
        _M(511,Network Authentication Required)
#undef _M
    };
    if(!STATUS_MAP.count(status)) 
        throw std::runtime_error("unknown status code : " + std::to_string(status));
    this->status = STATUS_MAP.at(status);
}

void themis::HttpResponse::serializeToBuffer(Buffer &buffer) {

    auto str = body.str();
    headers.insert({"Content-Length", std::to_string(str.length())});

    BufferWriter writer(buffer);
    writer.write("HTTP/1.1 ");
    writer.write(status);
    writer.write("\r\n");
    for(auto i: headers) {
        writer.write(i.first);
        writer.write(": ");
        writer.write(i.second);
        writer.write("\r\n");
    }
    writer.write("\r\n");
    writer.write(str);
}

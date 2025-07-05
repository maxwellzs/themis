#include "HttpRequest.h"
#include <algorithm>
#include <cctype>

const std::map<std::string, themis::HttpRequest::Method> themis::HttpRequest::METHOD_MAP = {
#define __M(x) {#x, x}
    __M(GET),
    __M(HEAD),
    __M(POST),
    __M(PUT),
    __M(DELETE),
    __M(CONNECT),
    __M(OPTIONS),
    __M(TRACE),
    __M(PATCH)
#undef __M
};

bool themis::HttpRequest::getHeader(std::string key, std::string &out) {
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    if(headers.count(key)) {
        out = headers.at(key);
        return true;
    }
    return false;
}

void themis::HttpRequest::setMethod(const std::string &methodStr) {
    if(!METHOD_MAP.count(methodStr)) {
        throw std::exception();
    }
    m = METHOD_MAP.at(methodStr);
}

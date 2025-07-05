#include "HttpSessionHandler.h"
#include "protocol/http/HttpRequest.h"
#include <algorithm>

void themis::HttpSessionHandler::parseHeader() {

    pendingRequest = std::make_unique<HttpRequest>();

    // parse <Method> <path> <version>
    BufferReader reader(session->getInputBuffer());
    std::string requestLine;
    reader.getline(requestLine);
    size_t d1 = requestLine.find_first_of(' '), d2 = requestLine.find_last_of(' ');

    if(d1 == std::string::npos ||
    d2 == std::string::npos || 
    d1 == d2) throw std::runtime_error("malformed request : illegal request line");

    std::string method = requestLine.substr(0, d1);
    pendingRequest->setMethod(method);

    std::string path = requestLine.substr(d1 + 1, d2 - d1 - 1);
    std::string version = requestLine.substr(d2 + 1);
    if(version != "HTTP/1.1") throw std::runtime_error("unsupported protocol version : " + version);
    pendingRequest->getVersion() = version;
    pendingRequest->getPath() = path;
    parseParameter();
    
    // parse all headers untill body line
    std::string line, k, v;
    for(reader.getline(line); !line.empty(); reader.getline(line)) {

        d1 = line.find_first_of(":");
        d2 = line.find_first_not_of(' ', d1 + 1);

        if(d1 == std::string::npos || d2 == std::string::npos) throw std::runtime_error("malformed request : illegal request header");

        k = line.substr(0, d1);
        v = line.substr(d2);
        
        // transfrom all case to lower in case of sensitivity
        std::transform(k.begin(), k.end(), k.begin(),[](unsigned char c) {
            return std::tolower(c);
        }); 
        pendingRequest->getHeaders().insert({k, v});
    }

    if(pendingRequest->getMethod() == HttpRequest::PATCH ||
    pendingRequest->getMethod() == HttpRequest::POST ||
    pendingRequest->getMethod() == HttpRequest::PUT 
    ) {
        // try to find body : 
        reader.finialize();
        state = AWAIT_BODY;
        parseBody(reader);
    } else {
        // no body, just dispatch the request
        state = COMPLETE;
    }
}

void themis::HttpSessionHandler::parseParameter() {
    size_t start = pendingRequest->getPath().find_first_of('?');
    if(start == std::string::npos) return;
    std::string path = pendingRequest->getPath().substr(0, start);
    std::string param = pendingRequest->getPath().substr(start + 1);
    pendingRequest->getPath() = path;
    size_t prev = 0;
    start = param.find_first_of('&');
    for (;;) {
        std::string key,val;
        if(start == std::string::npos) {
            key = param.substr(prev);
        } else {
            key = param.substr(prev, start - prev);
        }
        size_t dim = key.find_first_of('=');
        if(dim != std::string::npos) {
            val = key.substr(dim + 1);
            key = key.substr(0, dim);
            pendingRequest->getParameters().insert({key,val});
        }
        if(start == std::string::npos) {
            return;
        }
        prev = start + 1;
        start = param.find_first_of('&', start + 1);
    }
}

void themis::HttpSessionHandler::parseBody(BufferReader& reader) {
    // determine encoding
    std::string contentLength;
    std::string transferEncoding;
    bool hasLength = pendingRequest->getHeader("Content-Length", contentLength);
    bool hasEncoding = pendingRequest->getHeader("Transfer-Encoding", transferEncoding);

    if(hasLength) {

        if(!awaitBodyCount) awaitBodyCount = std::stoul(contentLength);
        if(pendingRequest->getBody().size() < awaitBodyCount) pendingRequest->getBody().resize(awaitBodyCount);
        size_t acquired = reader.getBytes(pendingRequest->getBody(), awaitBodyCount);

        awaitBodyCount -= acquired;

        if(!awaitBodyCount) {
            // completed
            state = COMPLETE;
            awaitBodyCount = 0;
            return;
        }

    } else if(hasEncoding) {
        // not yet implemented
        throw std::runtime_error("chunked encoding is not yet encoded");
    } else {
        throw std::runtime_error("unrecognized encoding");
    }
}

void themis::HttpSessionHandler::handleSession() {
    switch (state)
    {
    case AWAIT_HEADER:
        // parse header
        parseHeader();
        break;
    case AWAIT_BODY:
        // acquire body
        parseBody();
        break;
    default: break;
    }
    if(state == COMPLETE) {
        // the request is already ready, prepare to dispatch
        // prevent the session from being removed
        session->setLastActive(0x7ffffffffffffffful);
        // pass the request to callback funciton
        cb(std::move(pendingRequest), session);
        // reset state to parse next request
        state = AWAIT_HEADER;
    }
}

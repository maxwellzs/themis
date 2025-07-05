#include "Session.h"
#include <arpa/inet.h>

themis::Session::~Session() {
    if(readEvent) event_free(readEvent);
    if(writeEvent) event_free(writeEvent);
}

themis::Session::Session(Reactor& parent, sockaddr_in addr) :
    parent(parent), addr(addr), lastActiveTime(time(nullptr)) {
}

bool themis::Session::isTimedout(time_t val) {
    return val < time(nullptr) - lastActiveTime;
}


std::string themis::Session::toString() {
    std::string res;
    res += inet_ntoa(addr.sin_addr);
    res += ":";
    res += std::to_string(ntohs(addr.sin_port));
    return res;
}

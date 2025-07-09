#include "Session.h"
#include <unistd.h>
#include <arpa/inet.h>

themis::Session::~Session() {
    if(readEvent) event_free(readEvent);
    if(writeEvent) event_free(writeEvent);
    if(alive) close(fd);
}

themis::Session::Session(sockaddr_in addr, evutil_socket_t fd) :
    addr(addr), lastActiveTime(time(nullptr)), fd(fd) {
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

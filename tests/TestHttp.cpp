#include <gtest/gtest.h>

#define private public
#include "protocol/http/HttpSessionHandler.h"

TEST(TestHttp, TestRequestParseWithLength) {
    using namespace themis;
    Reactor *r = nullptr;
    auto session = std::make_unique<Session>(*r, sockaddr_in());
    HttpSessionHandler handler(std::move(session), [](std::unique_ptr<HttpRequest>, 
        const std::unique_ptr<Session>&) {

        });

    std::string req = "POST /users?a=b&c=d&e=&malformed&=f HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "Content-Type: application/x-www-form-urlencoded\r\n"
                      "Content-Length: 45\r\n"
                      "\r\n"
                      "name=FirstName+LastName&email=bsmth%40";
    


    std::memcpy(handler.getSession()->getInputBuffer().chunks.front().data(), req.data(), req.length());
    handler.getSession()->getInputBuffer().writeIndex = req.length();
    
    handler.handleSession();
    std::string val;
    ASSERT_TRUE(handler.pendingRequest->getHeader("Content-Type", val));
    ASSERT_EQ(val, "application/x-www-form-urlencoded");
    ASSERT_TRUE(handler.pendingRequest->getHeader("Host", val));
    ASSERT_EQ(val, "example.com");
    ASSERT_EQ(handler.pendingRequest->getBody().size(), 45);
    ASSERT_EQ(handler.state, HttpSessionHandler::AWAIT_BODY);
    ASSERT_EQ(handler.pendingRequest->getParameters().size(), 4);
    std::string req1 = "example";
    std::memcpy(handler.getSession()->getInputBuffer().chunks.front().data() + req.length(), req1.data(), req1.length());
    handler.getSession()->getInputBuffer().writeIndex += req1.length();

    handler.handleSession();
    ASSERT_EQ(handler.state, HttpSessionHandler::AWAIT_HEADER);

}
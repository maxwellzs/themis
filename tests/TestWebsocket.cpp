#include <gtest/gtest.h>
#define private public
#include "web/WebsocketController.h"

TEST(TestWebsocket, TestCalculateSecKey) {

    using namespace themis;
    WebsocketControllerManager mgr;

    std::string client = "dGhlIHNhbXBsZSBub25jZQ==";
    ASSERT_EQ(mgr.calculateSecKey(client), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");

}
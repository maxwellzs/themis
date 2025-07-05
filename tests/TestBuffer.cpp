#include <gtest/gtest.h>
#define private public 
#include "utils/Buffer.h"
#include <cstring>

TEST(TestBuffer, TestBufferGetline) {
    using namespace themis;
    Buffer b(4);
    b.chunks.clear();
    std::vector<uint8_t> v1 = {'h', 'e', 'l', 'l'};
    std::vector<uint8_t> v2 = {'h', 'e', 'l', 'l'};
    std::vector<uint8_t> v3 = {'h', 'e', '\n', 'a'};
    std::vector<uint8_t> v4 = {'h', 'e', 'l', 'l'};
    std::vector<uint8_t> v5 = {'h', 'e', '\r', '\n'};
    std::vector<uint8_t> v6 = {'h', 'e', 'l', 'l'};
    std::vector<uint8_t> v7 = {'h', 'e', '\r', '\n'};
    std::vector<uint8_t> v8 = {'h', 'e', '\r', '\n'};
    std::vector<uint8_t> v9 = {'\n', '\r', '\n', 'a'};

    b.chunks.emplace_back(v1);
    b.chunks.emplace_back(v2);
    b.chunks.emplace_back(v3);
    b.chunks.emplace_back(v4);
    b.chunks.emplace_back(v5);
    b.chunks.emplace_back(v6);
    b.chunks.emplace_back(v7);
    b.chunks.emplace_back(v8);
    b.chunks.emplace_back(v9);
    b.chunks.emplace_back(std::vector<uint8_t>());


    std::string res;
    b.readIndex = 0;
    b.writeIndex = 2;
    BufferReader r(b);
    r.getline(res);
    ASSERT_EQ(res, "hellhellhe");
    ASSERT_EQ(b.readIndex, 3);
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "ahellhe");
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "hellhe");
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "he");
    ASSERT_EQ(b.readIndex, 4);
    r.revert();
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "hellhellhe");
    ASSERT_EQ(b.readIndex, 3);
    r.finialize();
    ASSERT_EQ(b.chunks.size(), 8);
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "ahellhe");
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "hellhe");
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "he");
    ASSERT_EQ(b.readIndex, 4);
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "");
    res.clear();
    r.getline(res);
    ASSERT_EQ(res, "");

    bool flag = false;
    try
    {
        /* code */
        res.clear();
        r.getline(res);
    }
    catch(const std::exception& e)
    {
        flag = true;
    }
    ASSERT_TRUE(flag);
    
}

TEST(TestBuffer, TestBufferWrite) {
    using namespace themis;
    Buffer b(10);
    b.chunks.clear();
    std::string data = "1234567890123456789012345678901234567";
    /**
     * prepare test data in four chunks
     * the last chunk is not full
     * 
     */
    b.allocateChunk();
    std::memcpy(b.chunks.back().data(), data.data(), 10);
    b.allocateChunk();
    std::memcpy(b.chunks.back().data(), data.data() + 10, 10);
    b.allocateChunk();
    std::memcpy(b.chunks.back().data(), data.data() + 20, 10);
    b.allocateChunk();
    std::memcpy(b.chunks.back().data(), data.data() + 30, 7);
    b.writeIndex = 7;
    b.readIndex = 3;

    BufferReader r(b);
    // use a test file for simulation
    FILE* fp = fopen("testbuffer.dat","wb+");
    ASSERT_NE(fp, nullptr);
    ASSERT_FALSE(r.sendTo(fp->_fileno));
    fseek(fp, 0, SEEK_SET);
    // read and check test data
    std::string str1(data.length() - 3,' ');
    ssize_t readSize = fread(str1.data(), 1, data.length(), fp);
    ASSERT_EQ(readSize, data.length() - 3);
    ASSERT_EQ(str1, data.substr(3));

    fclose(fp);
}

TEST(TestBuffer, TestBufferWriteString) {
    using namespace themis;
    Buffer b(10);
    BufferWriter br(b);
    br.write(std::string(10,'a'));
    ASSERT_EQ(b.chunks.size(), 2);
}
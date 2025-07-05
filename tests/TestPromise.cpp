#include <gtest/gtest.h>
#include "utils/Promise.h"

TEST(TestConcurrency, TestPromise) {
    using namespace themis;
    std::unique_ptr<EventQueue> q = std::make_unique<EventQueue>();
    Promise<int,int,float>::ResolveFunction resolve;
    Promise<int,int,int> p(q, [&](Promise<int,int,float>::ResolveFunction res, FailFunction fail) {
        //res(std::make_unique<int>(1));
        resolve = res;
    });
    int val = 0;
    const auto& ptr = p.then([&](std::unique_ptr<int> i) -> std::unique_ptr<int> {
        ++val;
        return std::make_unique<int>(*(i.get()) + 1);

    })->then([&](std::unique_ptr<int> i) -> std::unique_ptr<int> {
        ++val;
        return std::make_unique<int>(*(i.get()) + 1);

    });
    q->poll();
    ASSERT_EQ(val, 0);
    resolve(std::make_unique<int>(1));
    ASSERT_EQ(val, 0);
    q->poll();
    ASSERT_EQ(val, 2);
    ptr->then([&](std::unique_ptr<int> i) {
        ++val;
        ASSERT_EQ(*i.get(), 3);
    });
    ASSERT_EQ(val, 3);
}

TEST(TestConcurrency, TestPromiseException) {
    using namespace themis;
    std::unique_ptr<EventQueue> q = std::make_unique<EventQueue>();
    int val = 0;
    Promise<int,int,int> p(q, [](Promise<int,int,float>::ResolveFunction res, FailFunction fail) {
        res(std::make_unique<int>(1));
    });
    const auto& ptr = p.then([](std::unique_ptr<int> i, FailFunction fail) -> std::unique_ptr<int> {

        fail(std::make_unique<std::exception>());
        return std::make_unique<int>(*(i.get()) + 1);

    })->then([&](std::unique_ptr<int> i) -> std::unique_ptr<int> {

        ++val;
        return std::make_unique<int>(*(i.get()) + 1);

    });
    bool flag = false;
    q->poll();
    ptr->then([](std::unique_ptr<int> i) {

        ASSERT_EQ(1,2);

    })->except([&](std::unique_ptr<std::exception> e) {
        flag = true;
    });
    ASSERT_TRUE(flag);
    q->poll();
    ASSERT_EQ(val, 0);
}
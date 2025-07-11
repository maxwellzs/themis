cmake_minimum_required(VERSION 4.0)
project("themis")

set(CMAKE_PREFIX_PATH /usr/local/lib/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)

find_package(Libevent REQUIRED)
find_package(ng-log REQUIRED)
find_package(GTest REQUIRED)

# find postgresql if exist
find_package(PostgreSQL REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})

add_library(themisRuntime SHARED 
    "network/Reactor.cpp"
    "network/Server.cpp"
    "network/Session.cpp"
    "protocol/http/HttpRequest.cpp"
    "protocol/http/HttpResponse.cpp"
    "protocol/http/HttpSessionHandler.cpp"
    "protocol/websocket/WebsocketSessionHandler.cpp"
    "protocol/websocket/WebsocketFrame.cpp"
    "protocol/websocket/WebsocketWriter.cpp"
    "utils/Buffer.cpp"
    "utils/Promise.cpp"
    "utils/EventQueue.cpp"
    "web/WebsocketController.cpp"
    "web/Controller.cpp"
    "sql/driver/detail/PostgresqlConnectionPool.cpp"
    "sql/driver/PostgresqlDriver.cpp"
    "sql/Driver.cpp"
)

add_executable(themis 
    "main.cpp"
)
add_executable(example_websocket
    "examples/Websocket.cpp"
)
target_link_libraries(themisRuntime
    libevent::core
    ng-log::ng-log
    PostgreSQL::PostgreSQL
)
enable_testing()
add_executable(themis_tests 
    "tests/TestBuffer.cpp"
    "tests/TestHttp.cpp"
    "tests/TestPromise.cpp"
    "tests/TestSQL.cpp"
    "tests/TestWebsocket.cpp"
)
target_link_libraries(themis_tests
    themisRuntime
    GTest::gtest_main
)
target_link_libraries(themis 
    themisRuntime
)
target_link_libraries(example_websocket 
    themisRuntime
)
include(GoogleTest)
gtest_discover_tests(themis_tests)
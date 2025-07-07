#include <gtest/gtest.h>
#include <ng-log/logging.h>
#include "sql/Driver.h"
#include "sql/driver/PostgresqlDriver.h"
#include "sql/driver/detail/PostgresqlConnectionPool.h"

TEST(TestSQL, TestPostgresDriverBasic) {
    FLAGS_alsologtostderr = 1;
    FLAGS_colorlogtostderr = 1;
    FLAGS_v = 10;
    FLAGS_log_dir = ".";
    
    nglog::InitializeLogging("");

    using namespace themis;
    DatasourceConfig config;
    config.getAddress() = "127.0.0.1:5432";
    config.getUsername() = "postgres";
    config.getPassword() = "admin";
    config.getDatabase() = "themis_test";
    // PostgresqlDriver::get()->addConfig(config);
    PostgresqlDriver::get()->addConfigToPool(config);
    PostgresqlDriver::get()->initialize();
    
    auto ptr = PostgresqlDriver::get()->query([](PGconn * conn) {
        PQsendQuery(conn, "select * from test_table;");
    });
    
    ptr->then([] (std::unique_ptr<PGResultSets> result) {
        LOG(INFO) << PQfname(result->at(0), 0);
        LOG(INFO) << PQfname(result->at(0), 1);
    })->except([] (std::unique_ptr<std::exception> e) {
        LOG(ERROR) << e->what();
        ASSERT_TRUE(false);
    });
    std::this_thread::sleep_for(std::chrono::seconds(10));

    PostgresqlDriver::get()->shutdown();
}
#include <gtest/gtest.h>
#include <ng-log/logging.h>
#include "sql/Driver.h"
#include "sql/driver/PostgresqlDriver.h"

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
    // config.getDatabase() = "themis_test";
    // PostgresqlDriver::get()->addConfig(config);
    PostgresqlDriver::get()->initialize();
    
    PostgresqlDriver::get()->shutdown();
}
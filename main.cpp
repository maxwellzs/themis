#include "network/Server.h"
#include <ng-log/logging.h>

int main(int argc, char **argv) {
    FLAGS_alsologtostderr = 1;
    FLAGS_colorlogtostderr = 1;
    FLAGS_v = 10;
    FLAGS_log_dir = ".";

    nglog::InitializeLogging(argv[0]);
    LOG(INFO) << "themis 2.0 started";

    themis::Server server("0.0.0.0", 8080);
    server.dispatch();

    nglog::ShutdownLogging();

    return 0;
}
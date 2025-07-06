#include "PostgresqlDriver.h"
#include "utils/Spinlock.h"
#include <ng-log/logging.h>

themis::PostgresqlDriver* themis::PostgresqlDriver::instance = nullptr;

themis::PostgresqlDriver::PostgresqlDriver() {
    // initialize event base for driver sockets
    base = event_base_new();
}

themis::PostgresqlDriver *themis::PostgresqlDriver::get() {
    return instance ? instance : (instance = new PostgresqlDriver);
}

void themis::PostgresqlDriver::loopOnce() {

    Spinlock lock(driverFlag);
    busy = false;
    event_base_loop(base, EVLOOP_NONBLOCK);
    busy |= eventQueue->poll();

    // check for unexecuted tasks
    
}

void themis::PostgresqlDriver::initialize() {
    if(! initialized) {

        Spinlock lock(driverFlag);
        // assign event base for each pool
        for(auto& i: pools) {
            i.second->setEventbase(base);
        }

        initializeAllPools();
        driverThread = std::make_unique<std::thread>([this]() {
            while(!stop) {
                loopOnce();
                if(!busy) std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
        initialized = true;
    }
}

void themis::PostgresqlDriver::shutdown() {
    // stop driver thread
    stop = true;
    if(driverThread->joinable()) driverThread->join();
    // clean all active connections
    // free event base
    event_base_free(base);
    // free all pool
    pools.clear();
}

const std::unique_ptr<themis::PostgresqlDriver::QueryPromise> &
themis::PostgresqlDriver::query(std::string poolID, PostgresqlConnectionPool::QueryFunction func) {

    Spinlock lock(driverFlag);
    if(!pools.count(poolID)) throw std::runtime_error("the pool with id \"" + poolID + "\" has no connection config");

    auto& pool = pools.at(poolID);
    activeQuery.emplace_back(std::make_unique<QueryPromise>(eventQueue,
        [this, func, &pool](QueryPromise::ResolveFunction resolve, FailFunction fail) {
            
            pool->submit(func, [resolve](std::unique_ptr<PGResultSets> result) {
                resolve(std::move(result));
            }, 
            [fail](std::unique_ptr<std::exception> e) {
                fail(std::move(e));
            });

        }));

    return activeQuery.back();
}

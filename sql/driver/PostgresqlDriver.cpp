#include "PostgresqlDriver.h"
#include "utils/Spinlock.h"
#include <ng-log/logging.h>

themis::PostgresqlDriver* themis::PostgresqlDriver::instance = nullptr;

themis::PostgresqlDriver::PostgresqlDriver() 
: eventQueue(std::make_unique<EventQueue>()) {
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
    for (auto& i: pools) {
        for (auto& j: i.second->basePool) {
            if(!PQisBusy(j->conn) && !j->queries.empty()) {
                // the connection is idle for new task, submit one
                // if the function blocked here, the whole driver will jam
                // make the pending result objects to hand over to the user later
                j->pendingResult = std::make_unique<PGResultSets>();
                j->queries.front().query(j->conn);
                // enable write so that the query can be flush out, if there are any
                if(PQisBusy(j->conn)) {
                    event_add(j->writeEvent, nullptr);
                } else {
                    // nothing happened in query, inform user
                    j->queries.front()
                    .onErr(std::make_unique<std::runtime_error>("there are no query submitted to connection"));
                    j->queries.pop();
                }
            }
        }
    }
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

std::unique_ptr<themis::PostgresqlDriver::QueryPromise>
themis::PostgresqlDriver::query(std::string poolID, PostgresqlConnectionPool::QueryFunction func) {

    Spinlock lock(driverFlag);
    if(!pools.count(poolID)) throw std::runtime_error("the pool with id \"" + poolID + "\" has no connection config");

    auto& pool = pools.at(poolID);

    return std::make_unique<QueryPromise>(eventQueue, 
        [this, func, &pool](QueryPromise::ResolveFunction resolve, FailFunction fail) {
            pool->submit(func, [resolve](std::unique_ptr<PGResultSets> result) {
                resolve(std::move(result));
            }, [fail](std::unique_ptr<std::exception> e) {
                fail(std::move(e));
            });
    });
}

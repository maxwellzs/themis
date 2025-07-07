#include "PostgresqlConnectionPool.h"
#include <libpq-fe.h>
#include <ng-log/logging.h>
#include "../PostgresqlDriver.h"

themis::PostgresqlConnectionPool::ConnectionDetail::ConnectionDetail(PGconn *conn, event_base *base, size_t pos, PostgresqlConnectionPool& parentPool)
: conn(conn), pos(pos), parentPool(parentPool) {
    // allocate read&write
    if(PQsetnonblocking(conn, 1)) {
        throw std::runtime_error("cannot set connection to non-blocking : \r\n" + 
        std::string(PQerrorMessage(conn)));
    }
    int socket = PQsocket(conn);
    // assign socket to event base
    readEvent = event_new(base, socket, EV_READ | EV_PERSIST | EV_TIMEOUT, 
        [](evutil_socket_t fd, short ev, void *args) {
        
        PostgresqlDriver::get()->busy = true;

        // on read event
        ConnectionDetail* _this = reinterpret_cast<ConnectionDetail *>(args);
        DatasourceConfig& config = _this->parentPool.configs[_this->pos];

        if(ev & EV_TIMEOUT) {
            // timed out, then inform the driver to reconnect to database
            // delete this connection 
            LOG(WARNING) << "connection timed out for " << config.toString();
            _this->handleConnectionError();
            return;
        } 

        // consume postgresql input
        int result = PQconsumeInput(_this->conn);
        if(result != 1) {
            // error
            LOG(WARNING) << "fatal error in consume " << config.toString() 
            << "\r\n" <<  PQerrorMessage(_this->conn);
            _this->handleConnectionError();
            return;
        }
        try {
            _this->handleConnectionResponse();
        } catch(const std::exception& e) {
            // the connection is probably broken
            _this->handleConnectionError();
        }

    }, this);

    writeEvent = event_new(base, socket, EV_WRITE | EV_TIMEOUT, 
        [](evutil_socket_t fd, short ev, void *args) {

        PostgresqlDriver::get()->busy = true;
        // on write event
        ConnectionDetail* _this = reinterpret_cast<ConnectionDetail *>(args);
        DatasourceConfig& config = _this->parentPool.configs[_this->pos];
        
        if(ev & EV_TIMEOUT) {
            LOG(WARNING) << "connection timed out for " << config.toString();
            _this->handleConnectionError();
            return;
        }

        int result = PQflush(_this->conn);
        if(result == -1) {
            // error
            LOG(WARNING) << "fatal error in flush " << config.toString() 
            << "\r\n" <<  PQerrorMessage(_this->conn);
            _this->handleConnectionError();
            return;
        } else if(result == 1) {
            // write again 
            event_add(_this->writeEvent, nullptr);
        }
        
    }, this);

    reconnectEvent = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, 
        [] (evutil_socket_t fd, short what, void *arg) {

            ConnectionDetail* _this = reinterpret_cast<ConnectionDetail *>(arg);
            // try to reconnect once
            try
            {
                _this->parentPool.buildConnection(_this->pos);
            }
            catch(const std::exception& e)
            {
                // not succeeded, add up retry count
                ++ _this->retryCount;
                auto& config = _this->parentPool.configs[_this->pos];
                if (_this->retryCount > config.getMaxRetry()) {
                    // retry procedure failed, thus should remove this connection 
                    LOG(WARNING) << "connection with config " << config.toString() 
                    << " cannot resume after " << config.getMaxRetry() << " times of retry";
                    _this->parentPool.basePool[_this->pos] = nullptr;
                }
            }
            // if this succeeded, _this pointer is no longer active and should immediately return

        }, this);

    // register events
    event_add(readEvent, nullptr);
    // event_add(writeEvent, nullptr);
}


void themis::PostgresqlConnectionPool::ConnectionDetail::submitQuery(QueryFunction func, QueryCallbackFunction cb, QueryErrorCallbackFunction fail) {
    queries.push(QueryTask(func, cb, fail));
}

void themis::PostgresqlConnectionPool::ConnectionDetail::handleConnectionResponse() {
    // not yet completed current task
    if(PQisBusy(conn)) return;
    // the current query has ended
    // retrieve the result and invoke user callback
    bool hasResult = false;
    for (PGresult* result = PQgetResult(conn); 
    result != nullptr; 
    result = PQgetResult(conn))
    {
        hasResult = true;
        ExecStatusType status = PQresultStatus(result);
        if(status != PGRES_COMMAND_OK &&
        status != PGRES_TUPLES_OK) {
            queries.front().onErr(std::make_unique<std::runtime_error>("postgresql query returned a fatal error " 
                + std::string(PQresultErrorMessage(result))));
            // clear current pending request
            pendingResult = nullptr;
            break;
        } 
        if(status == PGRES_NONFATAL_ERROR) {
            // warning but not fatal
            LOG(WARNING) << "a warning message was generated by database : " << PQresultErrorMessage(result);
        }
        // query ok
        pendingResult->addResult(result);
    }
    if(!hasResult) throw std::exception();
    // if no error occurred, call user callback and remove active task
    queries.front().cb(std::move(pendingResult));
    queries.pop();
}

void themis::PostgresqlConnectionPool::ConnectionDetail::handleConnectionError() {
    // retry untill max retry
    // if the timeout value is to small the connect operation will immediately fail
    timeval tm {3,0};
    event_add(reconnectEvent, &tm);
}

void themis::PGResultSets::clearResults() {
    // clean results
    for(auto i: sets) {
        PQclear(i);
    }
    sets.clear();
}

themis::PGResultSets::~PGResultSets() {
    clearResults();
}

void themis::PGResultSets::addResult(PGresult *result) {
    sets.emplace_back(result);
}

void themis::PostgresqlConnectionPool::buildConnection(size_t configIndex) {

    // try to build connection up
    auto& config = configs[configIndex];
    
    // check if config has required fields
    if(config.getUsername().empty() ||
    config.getAddress().empty() ||
    config.getPassword().empty()) {
        throw std::runtime_error("invalid config : " + config.toString());
    }

    std::ostringstream uriBuilder;
    uriBuilder << "postgresql://" << config.getUsername() << "@" << config.getAddress() 
    << "/" << config.getDatabase() << "?password=" << config.getPassword();

    std::string uri = uriBuilder.str();

    LOG(INFO) << "connecting to postgresql database using profile : " << config.toString();
    clock_t begin = clock();
    PGconn* connection = PQconnectdb(uri.c_str());
    clock_t elapsed = ((clock() - begin) * 1000) / CLOCKS_PER_SEC;
    
    ConnStatusType status = PQstatus(connection);
    if(status != CONNECTION_OK) {
        // error in connect
        LOG(WARNING) << "cannot connect to databaes using config : " << config.toString();
        throw std::runtime_error("connection failed for config " + config.toString());
    }

    LOG(INFO) << "succeded to connect in " << elapsed << " ms";
    // note this immediately free the last connection (if exists)
    basePool[configIndex] = 
    std::make_unique<ConnectionDetail>(connection, driverBase, configIndex, *this);
}

void themis::PostgresqlConnectionPool::initialize() {
    for (size_t i = 0; i < configs.size(); i++)
    {
        basePool.emplace_back(nullptr);
        buildConnection(i);   
    }
}

void themis::PostgresqlConnectionPool::submit
(QueryFunction func, QueryCallbackFunction cb, QueryErrorCallbackFunction fail) {

    std::vector<size_t> suitable;
    for (size_t i = 0; i < basePool.size(); i++)
    {
        if(basePool[i].get()) suitable.push_back(i);
    }
    
    // if there are no suitable connection, fail immediately
    if(suitable.empty()) {
        fail(std::make_unique<std::runtime_error>("all connection in the required pool is down"));
        return;
    }

    size_t idx = (++indexGen) % suitable.size();
    basePool[idx]->submitQuery(func, cb, fail);
}

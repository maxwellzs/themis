#ifndef PostgresqlConnectionPool_h
#define PostgresqlConnectionPool_h 1

#include "sql/Driver.h"
#include <event2/event.h>
#include <libpq-fe.h>
#include <vector>
#include <queue>
#include <ng-log/logging.h>
#include <stdexcept>
#include <functional>

namespace themis
{
    
    struct PGResultSets {
    private:
        std::vector<PGresult *> sets;

        void clearResults();
    public:
        ~PGResultSets();
        PGResultSets() = default;
        PGResultSets(const PGResultSets&) = delete;
        void addResult(PGresult * result);
        PGresult* at(size_t index) {
            return sets[index];
        }
    };

    class PostgresqlDriver;

    class PostgresqlConnectionPool : public ConnectionPool {
        friend PostgresqlDriver;

    public:
        /// @brief use this function to submit query
        /// do NOT use any block method inside this otherwise the driver thread will jam
        using QueryFunction = std::function<void (PGconn *)>;
        /// @brief connection callback when a statement fulfilled
        using QueryCallbackFunction = std::function<void (std::unique_ptr<PGResultSets>)>;
        /// @brief callback when a statement failed
        using QueryErrorCallbackFunction = std::function<void (std::unique_ptr<std::exception>)>;
        
    private:
    
        struct ConnectionDetail {
            PGconn *conn;
            event *readEvent; 
            event *writeEvent;
            event *reconnectEvent;
            PostgresqlConnectionPool& parentPool;
            /// @brief position in parent connection pool
            size_t pos; 

            size_t retryCount = 0;
            /// @brief the sets to temporarily holds the result from query
            std::unique_ptr<PGResultSets> pendingResult;

            struct QueryTask {
                QueryFunction query;
                QueryCallbackFunction cb;
                QueryErrorCallbackFunction onErr;

                QueryTask(QueryFunction query, QueryCallbackFunction cb, QueryErrorCallbackFunction onErr)
                : query(query), cb(cb), onErr(onErr) {}
                QueryTask(const QueryTask& t) 
                : query(t.query), cb(t.cb), onErr(t.onErr) {}
            };

            /// @brief queries yet to be submit to database
            std::queue<QueryTask> queries;

            ~ConnectionDetail() {

                LOG(INFO) << "connection with detail " << parentPool.configs[pos].toString() << " is closing"; 
                // fail all queries 
                while(!queries.empty()) {
                    QueryTask task = queries.front();
                    queries.pop();
                    task.onErr(std::make_unique<std::runtime_error>
                        ("the query is submitted to the pool, but the connection associated with it is down"));
                }

                if(readEvent) event_free(readEvent);
                if(writeEvent) event_free(writeEvent);
                if(reconnectEvent) event_free(reconnectEvent);
                if(conn) PQfinish(conn);
            }
            ConnectionDetail(PGconn* conn, event_base* base, size_t pos, PostgresqlConnectionPool& parentPool);
            ConnectionDetail(const ConnectionDetail& d) = delete;
            void operator=(const ConnectionDetail& d) = delete;

            void submitQuery(QueryFunction func, QueryCallbackFunction cb, QueryErrorCallbackFunction fail);
            void handleConnectionResponse();
            void handleConnectionError();
        };

        std::vector<std::unique_ptr<ConnectionDetail>> basePool;
        event_base* driverBase;

        /**
         * @brief try to build the connection using the config at index
         * 
         * @param configIndex the index of the config
         */
        void buildConnection(size_t configIndex);

        size_t indexGen;

    public:

        void setEventbase(event_base* base) {
            driverBase = base;
        }

        /**
         * @brief call this after configuring
         * 
         */
        virtual void initialize() override;

        /**
         * @brief submit the query task to this pool
         * 
         * @param func user query function
         * @param cb callback after query finished
         * @param fail callback after query failed
         */
        void submit(QueryFunction func, QueryCallbackFunction cb, QueryErrorCallbackFunction fail);
    };

} // namespace themis




#endif
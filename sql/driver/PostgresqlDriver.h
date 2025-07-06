#ifndef PostgresqlDriver_h
#define PostgresqlDriver_h 1

#include "detail/PostgresqlConnectionPool.h"
#include "utils/EventQueue.h"
#include "utils/Promise.h"
#include <memory>
#include <thread>
#include <libpq-fe.h>

namespace themis
{
    
    class PostgresqlDriver : public Driver<PostgresqlConnectionPool> {
    public:
        using QueryPromise = Promise<PGResultSets>;
        friend PostgresqlConnectionPool;

    private:
        
        static PostgresqlDriver* instance;

        std::unique_ptr<EventQueue> eventQueue;
        std::unique_ptr<std::thread> driverThread;

        event_base* base;
        bool busy = false;
        bool stop = false;
        std::atomic_flag driverFlag;

        /**
         * @brief execute base event loop and check for query tasks
         * 
         */
        void loopOnce();

        PostgresqlDriver();

    public:

        static PostgresqlDriver* get();
        virtual void initialize() override;
        virtual void shutdown() override;

        /**
         * @brief execute a query, using the default connection pool
         * 
         * @param poolID the id of the pool
         * @param func the query callback function
         * @return const std::unique_ptr<QueryPromise>& the query promise, 
         * the event queue this promise associated to is managed by driver thread, thus
         * the then and except function must be made thread-safe
         * do NOT free the promise until it fail/fulfill, otherwise the entire driver will break
         */
        std::unique_ptr<QueryPromise> query(std::string poolID, PostgresqlConnectionPool::QueryFunction func);
        
        /**
         * @brief execute a query to the default pool
         * 
         * @param func query callback function
         * @return const std::unique_ptr<QueryPromise>& 
         */
        std::unique_ptr<QueryPromise> query(PostgresqlConnectionPool::QueryFunction func) {
            return query("default_pool", func);
        }
    };

} // namespace themis


#endif
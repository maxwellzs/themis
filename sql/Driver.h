#ifndef Driver_h
#define Driver_h 1

#include <vector>
#include <string>
#include <memory>
#include <map>

namespace themis
{
    

    class DatasourceConfig {
    private:
        std::string address;
        std::string username;
        std::string password;
        std::string database;
        size_t maxRetry = 3;
    public:
        DatasourceConfig() = default;
        DatasourceConfig(const DatasourceConfig& config) 
        : address(config.address), username(config.username), 
        password(config.password), database(config.database) {}
        void operator=(const DatasourceConfig& config) {
            address = config.address;
            username = config.username;
            password = config.password;
            database = config.database;
        }
        std::string& getAddress() { return address; }
        std::string& getUsername() { return username; }
        std::string& getPassword() { return password; }
        std::string& getDatabase() { return database; }
        size_t& getMaxRetry() { return maxRetry; }
        std::string toString() {
            std::string str = "{addr=\"" + address 
            + "\", username=\"" + username + "\""
            + "\", database=\"" + database + "\"}";
            return str;
        }
    };

    class ConnectionPool {
    protected:
        std::vector<DatasourceConfig> configs;

    public:
        /**
         * @brief add a config to the driver, call this before 
         * initialize to take effect
         * 
         * @param config 
         */
        void addConfig(DatasourceConfig config) {
            configs.push_back(config);
        }

        /**
         * @brief call this to initialize all config in this pool
         * 
         */
        virtual void initialize() = 0;
    };

    /**
     * @brief base class for all sql driver
     * a driver is a single instance class that manages the connections and handle queries, 
     * explain result row and inject beans
     * 
     */
    template<typename TPoolType>
    class Driver {
    protected:
        bool initialized = false;

        std::map<std::string, std::unique_ptr<TPoolType>> pools;

        void initializeAllPools() {
            static_assert(std::is_base_of<ConnectionPool, TPoolType>(), 
            "TPoolType must be derived of ConnectionPool");
            for(auto& p: pools) {
                p.second->initialize();
            }
        }

    public:

        /**
         * @brief add the pool associated with given identifier
         * 
         * @param id identifier
         * @param pool pool
         */
        void addConfigToPool(std::string id, DatasourceConfig config) {
            static_assert(std::is_base_of<ConnectionPool, TPoolType>(), 
            "TPoolType must be derived of ConnectionPool");
            if(!pools.count(id)) {
                pools.insert({id, std::make_unique<TPoolType>()});
            }
            pools.at(id)->addConfig(config);
        }

        /**
         * @brief add a config to connection pool
         * 
         * @param pool connection pool
         */
        void addConfigToPool(DatasourceConfig config) {
            addConfigToPool("default_pool", config);
        }

        virtual ~Driver() = default;
        /**
         * @brief initialize driver, call once for each driver 
         * set initialized to true in this method
         * 
         */
        virtual void initialize() = 0;

        /**
         * @brief clean up driver, call once for each driver
         * 
         */
        virtual void shutdown() = 0;
    };

} // namespace themis




#endif
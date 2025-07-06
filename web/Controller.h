#ifndef Controller_h
#define Controller_h 1

#include <memory>
#include <list>
#include <map>
#include "utils/Promise.h"
#include "protocol/http/HttpResponse.h"
#include "protocol/http/HttpRequest.h"
#include "network/Session.h"

namespace themis
{

    using HttpResponsePromise = Promise<HttpResponse>;

    class ControllerFilter {
    public:
        virtual ~ControllerFilter() = default;
        /**
         * @brief determine whether this controller should accept this request
         * 
         * @param req request
         * @return true if should accept
         * @return false if should not accept
         */
        virtual bool filter(const std::unique_ptr<HttpRequest>& req) = 0;
    };

    class MethodFilter : public ControllerFilter {
    private:
        HttpRequest::Method method;
    public:
        MethodFilter(HttpRequest::Method method) : method(method) {}
        virtual bool filter(const std::unique_ptr<HttpRequest>& req) override;
    };

    class Controller;

    /**
     * @brief controllerManager register and manages controller instance, and host a event queue
     * all controller use this event queue to submit events/promises
     * also this manager object dispatches the request object to controller whose filter 
     * fit the given request
     * 
     */
    class ControllerManager {
    private:
        std::unique_ptr<EventQueue> queue = std::make_unique<EventQueue>();
        std::map<std::string, std::unique_ptr<Controller>> controllerMap;

        struct ResponseDetail {
            const std::unique_ptr<Session>& sessionRef;
            std::unique_ptr<HttpResponsePromise> promise;
            std::string path;

            ResponseDetail(const ResponseDetail&) = delete;
            ResponseDetail(ResponseDetail&& d) : promise(std::move(d.promise)), sessionRef(d.sessionRef), path(d.path) {}
            ResponseDetail(const std::unique_ptr<Session>& session, 
                std::unique_ptr<HttpResponsePromise> promise, const std::string& path) : promise(std::move(promise)), sessionRef(session), path(path) {}
        };

        std::list<ResponseDetail> responseList;
        using DetailIterator = std::list<ResponseDetail>::iterator;

        /// some preset function that might come in handy
        static void serveNotFound(const std::unique_ptr<Session>& session, const std::string& path);
        static void serveInternalError(const std::unique_ptr<Session>& session, const std::string& path, std::unique_ptr<std::exception> e);

    public:
        /**
         * @brief assign this controller to this manager
         * 
         * @param controller controller object
         * @return this reference
         */
        ControllerManager& addController(std::unique_ptr<Controller> controller);

        void serveRequest(std::unique_ptr<HttpRequest> req, const std::unique_ptr<Session>& session);

        /// @brief poll base queue once
        bool poll() {
            return queue->poll();
        }
    };

    /**
     * @brief a controller defines a web interface that can be accessed by 
     * a http client
     * 
     */
    class Controller {
    protected:
        std::vector<std::unique_ptr<ControllerFilter>> filters;
        /// @brief the identifier of this controller that will be matched in the manager
        const std::string path;
    public:
        const std::string& getPath() {
            return path;
        }

        virtual ~Controller() = default;
        Controller(const std::string& path) : path(path) {}
        /**
         * @brief handle the given request
         * this function should return immediately, and resolve the response promise afterwards
         * 
         * @param req request 
         * @param queue event queue to construct new promise
         * @return std::unique_ptr<Response> response promise
         */
        virtual std::unique_ptr<HttpResponsePromise> service(std::unique_ptr<HttpRequest> req, const std::unique_ptr<EventQueue>& queue) = 0;
    };
    
} // namespace themis


#endif
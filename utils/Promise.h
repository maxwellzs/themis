#ifndef Promise_h
#define Promise_h 1

#include <utility>
#include <memory>
#include "EventQueue.h"


namespace themis
{
    
    enum PromiseState {
        /// @brief a promise in PENDING state indicates that the promise is meant to fulfill later
        PENDING, 
        /// @brief FULFILLED state means the promise finished
        FULFILLED,
        /// @brief FAILED state means fail function has been called
        FAILED
    };

    template<typename ...T> class Promise;
    /// @brief this function throw exception through promise chain
    using FailFunction = std::function<void (std::unique_ptr<std::exception>)>;


    /**
     * @brief a Promise delegate a task that might complete/fail in the future
     * 
     * @tparam TParam the parameter type provided by the resolve function
     * @tparam TResult the parameter type to pass to next resolve function
     */
    template<typename TParam, typename TResult, typename ...T>
    class Promise<TParam, TResult, T...> {
    public:
        /// @brief this function resolve parameter to this promise
        using ResolveFunction = std::function<void (std::unique_ptr<TParam>)>;
        
        /// @brief this function is user defined, accept the parameter from resolve and yield a result
        using FulfillFunction = std::function<std::unique_ptr<TResult> (std::unique_ptr<TParam>, FailFunction)>;
        /// @brief fulfill with no error handle
        using DefaultFulfillFunction = std::function<std::unique_ptr<TResult> (std::unique_ptr<TParam>)>;

        /// @brief this function is the next resolve function in promise chain
        using ResolveNext = std::function<void (std::unique_ptr<TResult>)>;
        /// @brief if next promise is not yet registered, put the exception in this temporarily
        std::unique_ptr<std::exception> e;

    private:
        PromiseState state = PENDING;
        const std::unique_ptr<EventQueue>& queue;
        ResolveNext resolveNext;
        FailFunction failNext;
        FulfillFunction onFulfill;
        std::unique_ptr<Promise<TResult, T...>> next;
        std::unique_ptr<TParam> result;

        void resolve() {
            state = FULFILLED;
            if(next.get()) {
                std::unique_ptr<TResult> val = onFulfill(std::move(result), [this](std::unique_ptr<std::exception> e) {
                    raiseException(std::move(e));
                });
                if(state != FAILED) {
                    resolveNext(std::move(val));
                }
            }
        }

        void raiseException(std::unique_ptr<std::exception> e) {
            // already registered next, then pass the exception to next
            state = FAILED;
            if(next.get()) 
                // if next promise present present, then fail next
                failNext(std::move(e));
            else {
                // otherwise save the exception for now
                this->e = std::move(e);
            }
        }

    public:
        Promise(const std::unique_ptr<EventQueue>& q, std::function<void (ResolveFunction, FailFunction)> fn) : queue(q) {
            fn([this](std::unique_ptr<TParam> r) {
                result = std::move(r);
                queue->addImmediate([this]() {
                    resolve();
                });
            }, [this](std::unique_ptr<std::exception> e) {
                raiseException(std::move(e));
            });
        }

        /**
         * @brief standard version to register a callbak
         * 
         * @param f the on-fulfill callback, the first parameter is the result of last promise,
         * the second parameter is a on-error callback
         * do not throw, use the second callback instead, otherwise event queue will instantly break
         * @return const std::unique_ptr<Promise<TResult, T...>>& 
         */
        const std::unique_ptr<Promise<TResult, T...>>& then(FulfillFunction f) {
            next = std::make_unique<Promise<TResult, T...>>(queue, [this, f](ResolveNext rn, FailFunction fail) {
                if(state == FULFILLED) {
                    // this promise is already fulfilled, and should be called immediately
                    rn(onFulfill(std::move(result), [this](std::unique_ptr<std::exception> e) {
                        raiseException(std::move(e));
                    }));
                } else if(state == FAILED) {
                    // an exception already occurred, immediately fail the next promise
                    fail(std::move(e));
                } else {
                    resolveNext = rn;
                    failNext = fail;
                    onFulfill = f;
                }
            });
            return next;
        };

        /**
         * @brief register a on-fulfill function with no error handle
         * do not throw in the callback, if there might be error use the version with
         * error handle callback
         * 
         * @param f on-fulfill callback function
         * @return const std::unique_ptr<Promise<TResult, T...>>& 
         */
        const std::unique_ptr<Promise<TResult, T...>>& then(DefaultFulfillFunction f) {
            return then([f](std::unique_ptr<TParam> result, FailFunction fail) -> std::unique_ptr<TResult> {
                return f(std::move(result));
            });
        }
    
    };

    /**
     * @brief final promise in promise chain
     * 
     * @tparam TParam parameter type this promise take
     */
    template<typename TParam>
    class Promise<TParam> {
    public:

        using ResolveFunction = std::function<void (std::unique_ptr<TParam>)>;
        using FulfillFunction = std::function<void (std::unique_ptr<TParam>, FailFunction)>;
        using DefaultFulfillFunction = std::function<void (std::unique_ptr<TParam>)>;

    private:
        PromiseState state = PENDING;
        std::unique_ptr<TParam> result;
        const std::unique_ptr<EventQueue>& queue;
        FulfillFunction onFulfill;
        bool hasCallback = false;

        void raiseException(std::unique_ptr<std::exception> err) {
            // actual point to handle the exception
            state = FAILED;
            if(hasErrorCallback) {
                fail(std::move(err));
            } else {
                // save exception for now
                e = std::move(err);
            }
        }

        void resolve() {
            state = FULFILLED;
            if(hasCallback) {
                onFulfill(std::move(result), [this](std::unique_ptr<std::exception> e) {
                    raiseException(std::move(e));
                });
            }
        }

        std::unique_ptr<std::exception> e;
        FailFunction fail;
        bool hasErrorCallback = false;

    public:

        
        Promise(const std::unique_ptr<EventQueue>& q, std::function<void (ResolveFunction,FailFunction)> fn) : queue(q) {
            fn([this](std::unique_ptr<TParam> r) {
                result = std::move(r);
                queue->addImmediate([this]() {
                    resolve();              
                });
            }, [this](std::unique_ptr<std::exception> e) {
                raiseException(std::move(e));
            });
        }

        Promise<TParam>* then(FulfillFunction fn) {
            if(state == FULFILLED) {
                // call immediately if there are already result
                fn(std::move(result), [this](std::unique_ptr<std::exception> e) {
                    raiseException(std::move(e));
                });
            } else {
                onFulfill = fn;
                hasCallback = true;
            }
            return this;
        }

        Promise<TParam>* then(DefaultFulfillFunction fn) {
            return then([fn](std::unique_ptr<TParam> r, FailFunction fail) {
                fn(std::move(r));
            });
        }

        void except(FailFunction err) {
            if(state == FAILED) {
                // already thrown an exception but not yet handled
                // thus call handler immediately
                err(std::move(e));
            } else {
                fail = err;
                hasErrorCallback = true;    
            }
        }
    };


} // namespace themis

#endif
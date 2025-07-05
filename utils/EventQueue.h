#ifndef EventQueue_h
#define EventQueue_h 1

#include <functional>
#include <queue>
#include <atomic>

namespace themis
{

    /**
     * @brief a event queue is a queue that accept events and provide events 
     * there are multiple inner queues, when an event is added to queue, 
     * main thread should poll and run its callback
     */
    class EventQueue {
    private:
        using CallbackFunction = std::function<void ()>;

        struct ImmediateQueue {
            std::atomic_flag f;
            std::queue<CallbackFunction> callbacks;
        } immediate;

    public:
        /**
         * @brief add a callback to the immediate queue, 
         * this callback will be invoked once next poll happen
         * 
         * @param fn function
         * @param e function to run when exception occured in function fn
         */
        void addImmediate(CallbackFunction fn);

        /**
         * @brief poll and run all active events
         * 
         * @return true if any events happened
         * @return false nothing happened
         */
        bool poll();
    };
    
} // namespace themis




#endif
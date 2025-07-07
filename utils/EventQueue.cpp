#include "EventQueue.h"
#include "Spinlock.h"
#include <ng-log/logging.h>


void themis::EventQueue::addImmediate(CallbackFunction fn) {
    Spinlock l(immediate.f);
    immediate.callbacks.push(fn);
}

bool themis::EventQueue::poll() {
    bool busy = true;
    // try immediate quue
    Spinlock lock(immediate.f, true);
    busy |= immediate.callbacks.empty();
    for(;;) {
        lock.lock();
        if(immediate.callbacks.empty()) return busy;
        auto p = immediate.callbacks.front();
        immediate.callbacks.pop();
        lock.unlock();
        // if not unlocked before calling user callback, 
        // there will be deadlock if user call addImmediate
        try
        {
            p();
        }
        catch(const std::exception& e)
        {
            LOG(WARNING) << "an uncaught exception happened in the event queue, do not do this otherwise there might be unhandled exception";
        }
    }
    return busy;
}

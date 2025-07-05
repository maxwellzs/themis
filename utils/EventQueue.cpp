#include "EventQueue.h"
#include "Spinlock.h"


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
        p();
    }
    return busy;
}

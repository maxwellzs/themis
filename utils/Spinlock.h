#ifndef Spinlock_h
#define Spinlock_h 1

#include <atomic>

namespace themis
{
    
    /**
     * @brief a simple rotating lock implementation
     * 
     */
    class Spinlock {
    private:
        std::atomic_flag& flag;
        bool locked = false;
    public:
        void lock() {
            if(locked) return;
            while(flag.test_and_set(std::memory_order_acquire));
            locked = true;
        }    
        void unlock() {
            if(locked) flag.clear(std::memory_order_release);
            locked = false;
        }

        Spinlock(std::atomic_flag& flag, bool deferLock = false) : flag(flag) {
            if(!deferLock) {
                // lock immediately
                lock();
            }
        }
        ~Spinlock() {
            unlock();
        }
    };

} // namespace themis




#endif
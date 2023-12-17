
#if !defined(__PANGLOS_MUTEX__)
#define __PANGLOS_MUTEX__

namespace panglos {

class Mutex 
{
public:
    virtual ~Mutex(){}

    virtual void lock() = 0;
    virtual void unlock() = 0;

    typedef enum {
        TASK_LOCK,          // suspends the scheduler
        SYSTEM,             // scheduler still runs
        CRITICAL_SECTION,   // disable/enable interrupts
        RECURSIVE,          // can be called again from the same task
    }   Type;

    Type get_type();

    static Mutex *create(Type type=TASK_LOCK);
};

    /*
     *
     */

class Lock
{
    Mutex *mutex;
public:
    Lock(Mutex *m) : mutex(m)
    {
        if (mutex)
        {
            mutex->lock();
        }
    }

    ~Lock()
    {
        if (mutex)
        {
            mutex->unlock();
        }
    }
};

}   //  namespace

#endif // __PANGLOS_MUTEX__

//  FIN

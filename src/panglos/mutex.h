
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

    /*
     *
     */

class Semaphore;

class PostHook
{
public:
    virtual void post(Semaphore *s) = 0;
};

    /*
     *
     */

class Semaphore
{
public:
    Semaphore *next;
    Semaphore() : next(0) { }

    virtual ~Semaphore(){}

    virtual void post() = 0;
    virtual void wait() = 0;

    virtual void set_hook(PostHook *hook) = 0;

    static Semaphore *create();

    static Semaphore **next_fn(Semaphore *s) { return & s->next; }
};

}   //  namespace

#endif // __PANGLOS_MUTEX__

//  FIN

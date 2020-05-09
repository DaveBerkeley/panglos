
#if !defined(__MUTEX_H__)
#define __MUTEX_H__

namespace panglos {

class Mutex 
{
public:
    virtual ~Mutex(){}

    virtual void lock() = 0;
    virtual void unlock() = 0;

    static Mutex *create();
    static Mutex *create_critical_section();
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

#endif // __MUTEX_H__

//  FIN

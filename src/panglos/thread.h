
#if !defined(__PANGLOS_THREAD__)
#define __PANGLOS_THREAD__

namespace panglos {

class Thread
{
public:
    virtual ~Thread() { }

    typedef enum {
        High,
        Medium,
        Low,
    }   Priority;

    static Thread *create(const char *name, size_t stack=0, Priority priority=Medium);

    virtual void start(void (*fn)(void *arg), void *arg) = 0;
    virtual void join() = 0;

    virtual const char *get_name() = 0;

    static Thread *get_current();
};

class ThreadPool
{
    Thread **threads;
    int count;
    char **names;

public:
    ThreadPool(const char *name, int n, size_t stack=0, Thread::Priority p=Thread::Medium);
    ~ThreadPool();
    void start(void (*fn)(void *arg), void *arg);
    void join();
};

}   //  namespace panglos

#endif  //  __PANGLOS_THREAD__

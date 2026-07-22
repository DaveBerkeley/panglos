
#if !defined(__PANGLOS_THREAD__)
#define __PANGLOS_THREAD__

namespace panglos {

class Out;

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

    virtual void start(void (*fn)(void *arg), void *arg, int core=-1) = 0;
    virtual void join() = 0;

    virtual const char *get_name() = 0;
    virtual int get_core() { return -1; }

    virtual Out *get_out() { return 0; }
    virtual void set_out(Out *) { }

    static void yield();

    static Thread *get_current();
    static void visit(int (*fn)(Thread*, void*), void*);
    static Thread *get_by_name(const char *name);
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

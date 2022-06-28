
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

    static Thread *create(const char *name, size_t stack, Priority priority=Medium);

    virtual void start(void (*fn)(void *arg), void *arg) = 0;
    virtual void join() = 0;
};

}   //  namespace panglos

#endif  //  __PANGLOS_THREAD__

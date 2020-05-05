
#if !defined(__IRQ_TASK_H__)
#define __IRQ_TASK_H__

#include "deque.h"

namespace panglos {

class Dispatch
{
public:

    class Callback {
    public:
        const char *debug;
        Callback *next;

        Callback(const char *_debug=0) : debug(_debug), next(0) { }
        virtual ~Callback() {}

        virtual void execute() = 0;
    };

    class FnArg : public Callback
    {
        void (*fn)(void *arg);
        void *arg;

    public:
        FnArg(void (*_fn)(void *arg), void *_arg, const char *_debug=0);

        virtual void execute();
    };

    // do-nothing Callback
    class Nowt : public Callback
    {
        virtual void execute() { }
    };

private:

    Deque<Dispatch::Callback*> deque;
    Mutex *mutex;
    Semaphore *semaphore;
    bool dead;

public:
    Dispatch();
    ~Dispatch();

    /// called from within irq context
    void put(Callback *cb);

    /// main dispatch loop
    void run();

    void kill();
};

}   //  namespace panglos

#endif // __IRQ_TASK_H__

// FIN

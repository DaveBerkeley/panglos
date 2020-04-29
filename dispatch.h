
#if !defined(__IRQ_TASK_H__)
#define __IRQ_TASK_H__

#include "msg_queue.h"

namespace panglos {

class Dispatch
{
public:

    class Callback {
    public:
        const char *debug;

        Callback(const char *_debug=0) : debug(_debug) { }
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

    static Nowt nowt;

private:
    typedef MsgQueue<Callback*> Queue;

    Queue::Deque *deque;
    Mutex *mutex;
    Semaphore *semaphore;
    Queue *queue;

public:
    Dispatch();
    ~Dispatch();

    /// called from within irq context
    void put(Callback *cb);

    /// main dispatch loop
    void run();
};

}   //  namespace panglos

#endif // __IRQ_TASK_H__

// FIN

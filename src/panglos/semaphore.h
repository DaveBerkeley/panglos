
#if !defined(__PANGLOS_SEMAPHORE__)
#define __PANGLOS_SEMAPHORE__

namespace panglos {

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

#endif // __PANGLOS_SEMAPHORE__

//  FIN

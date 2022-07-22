
#if !defined(__PANGLOS_SEMAPHORE__)
#define __PANGLOS_SEMAPHORE__

namespace panglos {

    /*
     *
     */

class Semaphore
{
public:
    typedef enum {
        NORMAL,
        COUNTING,
    }   Type;

    virtual ~Semaphore(){}

    virtual void post() = 0;
    virtual void wait() = 0;
    virtual void wait_timeout(int ticks) = 0;

    static Semaphore *create(Type type=NORMAL, int n=0, int initial=0);
};

}   //  namespace

#endif // __PANGLOS_SEMAPHORE__

//  FIN

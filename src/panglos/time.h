
#if !defined(__PANGLOS_TIME__)
#define __PANGLOS_TIME__

namespace panglos {

class Time
{
public:
    typedef uint32_t tick_t;

    static void sleep(int secs);
    static void msleep(int msecs);

    static tick_t get();
    static bool elapsed(tick_t start, tick_t period);
};

}   //  panglos

#endif // __PANGLOS_TIME__

//  FIN

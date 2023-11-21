
#if !defined(__VERBOSE__)
#define __VERBOSE__

namespace panglos {

struct Verbose
{
    const char *name;
    bool verbose;
    struct Verbose *next;

    static struct Verbose* verboses;
};

#define VERBOSE(xname, label, state) \
    Verbose xname = { .name = label, .verbose = state, .next = 0 }; \
    __attribute__((constructor(101))) \
    static void init_##xname() \
    { \
        xname.next = Verbose::verboses; \
        Verbose::verboses = & xname; \
    }

void verbose_init();

}   //  namespace panglos

#endif  //  __VERBOSE__

//  FIN

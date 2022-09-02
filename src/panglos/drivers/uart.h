
#if !defined(__PANGLOS_UART__)
#define __PANGLOS_UART__

#include "panglos/io.h"

namespace panglos {

    /*
     *
     */

class UART : public IO
{
public:
    typedef enum {
        EXIT,
        READ,
        WRITE,
        ERROR,
        TIMEOUT,
    }   EventCode;

    class Event
    {
    public:
        EventCode code;
    };
};

}   //  namespace panglos

#endif // __PANGLOS_UART__

//  FIN

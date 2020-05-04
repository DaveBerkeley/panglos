
#if !defined(__BUFFER_H__)
#define __BUFFER_H__

#include <stdint.h>

#include "mutex.h"
#include "event.h"

namespace panglos {

class RingBuffer
{
    Mutex *mutex;
    Mutex *delete_mutex;
    Semaphore *semaphore;

    uint8_t *data;
    int in, out, size;

    int _add(uint8_t c, Mutex *mutex);

public:

    RingBuffer(int _size, Semaphore *s, Mutex *mutex_override=0);
    ~RingBuffer();

    int add(uint8_t c);
    int add(const uint8_t *s, int n);

    bool empty();
    bool full();
    int _getc();
    int _gets(uint8_t *s, int n);

    void reset();

    int wait(EventQueue *q, timer_t timeout);
};

}   //  namespace panglos

#endif // __BUFFER_H__

//  FIN

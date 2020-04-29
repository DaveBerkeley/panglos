
#if !defined(__BUFFER_H__)
#define __BUFFER_H__

#include <stdint.h>

#include "mutex.h"
#include "event.h"

namespace panglos {

class RingBuffer
{
    Mutex *mutex;
    Semaphore *semaphore;

    uint8_t *data;
    int in, out, size;

public:

    RingBuffer(int _size);
    ~RingBuffer();

    int add(uint8_t c, bool use_mutex=true);
    int add(const uint8_t *s, int n, bool use_mutex=true);

    bool empty(bool use_mutex=true);
    bool full(bool use_mutex=true);
    int getc(bool use_mutex=true);
    int gets(uint8_t *s, int n, bool use_mutex=true);

    int wait(EventQueue *q, Semaphore *s, timer_t timeout);

    void set_semaphore_in(Semaphore *s) { semaphore = s; }
};

}   //  namespace panglos

#endif // __BUFFER_H__

//  FIN

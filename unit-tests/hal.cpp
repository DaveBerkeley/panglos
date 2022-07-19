
#include <pthread.h>
#include <semaphore.h>

#include <panglos/debug.h>
#include <panglos/event.h>

#include "mock.h"

namespace panglos {

    /*
     *
     */

#if 0
static timer_t cycles;

void timer_irq()
{
    cycles += 1;

    // TODO : Call the EventQueue handler
}
#endif

    /*
     *
     */

void *get_task_id()
{
    return (void*) pthread_self();
}

}   //  namespace panglos

//  FIN

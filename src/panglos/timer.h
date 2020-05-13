
#include <stdint.h>

namespace panglos {

typedef uint32_t timer_t;
typedef int32_t d_timer_t;

timer_t timer_now();
void timer_set(d_timer_t dt);
void timer_wait(d_timer_t dt);
void timer_init();

#define TIMER_MS    (10)
#define TIMER_S     (1000 * TIMER_MS)

}   //  namespace panglos

//  FIN

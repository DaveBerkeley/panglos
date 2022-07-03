
    /*
     *  Proved Queue class wrapper for FreeRTOS Queues
     */

#if !defined(__PANGLOS__FREERTOS_QUEUE__)
#define __PANGLOS__FREERTOS_QUEUE__

namespace panglos {

class Queue;
    
Queue *queue_wrap(QueueHandle_t queue);

}   //  namespace panglos

#endif  //  __PANGLOS__FREERTOS_QUEUE__

//  FIN

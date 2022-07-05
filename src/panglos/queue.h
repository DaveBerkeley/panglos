
#if !defined(__PANGLOS_QUEUE__)
#define __PANGLOS_QUEUE__

namespace panglos {

class Queue
{
public:
    virtual ~Queue() { }

    static Queue *create(int size, int num);

    class Message;

    virtual bool get(Message *msg, int timeout) = 0;
    virtual bool put(Message *) = 0;

    virtual int queued() = 0;
};

}   //  namespace panglos

#endif  //  __PANGLOS_QUEUE__

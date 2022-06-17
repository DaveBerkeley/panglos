
#if !defined(__GPIO_H__)
#define __GPIO_H__

namespace panglos {

class GPIO
{
public:
    virtual ~GPIO() { }
 
    virtual void set(bool state) = 0;
    virtual bool get() = 0;
    virtual void toggle() { set(get()); }

    virtual void set_interrupt_handler(void (*fn)(void *arg), void *arg) = 0;
    virtual void on_interrupt() = 0;

    virtual bool flush() { return false; }
};

}   //  namespace panglos

#endif // __GPIO_H__

//  FIN

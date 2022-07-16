
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

    enum Interrupt {
        OFF,
        RISE,
        FALL,
        CHANGE,
    };

    virtual void set_interrupt_handler(enum Interrupt irq, void (*fn)(void *arg), void *arg)
    { 
        IGNORE(irq);
        IGNORE(fn);
        IGNORE(arg);
    }

    virtual void on_interrupt() {}

    virtual bool flush() { return false; }
};

}   //  namespace panglos

#endif // __GPIO_H__

//  FIN

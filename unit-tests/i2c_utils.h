
    /*
     *
     */

#include "panglos/drivers/gpio.h"

class SimGpio : public panglos::GPIO
{
    typedef void (*set_fn)(bool s, void *arg);
    typedef bool (*get_fn)(void *arg);

    set_fn setf;
    get_fn getf;
    void *arg;

public:
    SimGpio() : setf(0), getf(0), arg(0) { }

    virtual void set(bool s) override
    {
        ASSERT(setf);
        return setf(s, arg);
    }
    virtual bool get() override
    {
        ASSERT(getf);
        return getf(arg);
    }
    virtual void toggle()
    {
        set(!get());
    }
    void set_handlers(set_fn s, get_fn g, void *_arg)
    {
        setf = s;
        getf = g;
        arg = _arg;
    }
};

//  FIN

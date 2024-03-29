
#if !defined(__PANGLOS_PWM__)
#define __PANGLOS_PWM__

namespace panglos {

class PWM
{
public:
    virtual ~PWM() { }

    virtual bool set(int idx, uint32_t value, bool flush=true) = 0;
    virtual void flush() {}
};

class PWM_Timer : public PWM
{
    int timer;
    int npins;
    bool verbose;

public:
    PWM_Timer(int timer, int freq, int *pins, bool verbose=false);

    virtual bool set(int idx, uint32_t value, bool flush) override;
};

}   //  namespace panglos

#endif  //  __PANGLOS_PWM__

//  FIN

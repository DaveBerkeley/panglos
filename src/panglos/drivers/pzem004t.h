
#if !defined(__PZEM004T__)
#define __PZEM004T__

namespace panglos {

class Out;

class PZEM004T
{
public:
    uint8_t addr;

    enum Cmd {
        READ_REGS = 0x04,
        ERROR = 0xff,
    };

    PZEM004T(uint8_t addr=0x01);

    struct Status
    {
        float volts;
        float current;
        float power;
        float energy;
        float freq;
        float power_factor;
    };

    bool crc_check(const uint8_t *data, int n);
    enum Cmd get_command(const uint8_t *data, int n);

    bool parse(struct Status *s, const uint8_t *data, int n);
    bool request(Out *out);

    static uint16_t crc(const uint8_t *data, int n);
};

}   //  namespace panglos

#endif  //  __PZEM004T__

//  FIN

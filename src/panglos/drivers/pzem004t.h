
#if !defined(__PZEM004T__)
#define __PZEM004T__

namespace panglos {

class Out;

class PZEM004T
{
    uint8_t addr;
public:
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

    bool parse(struct Status *s, const uint8_t *data, int n);
    bool request(Out *out);

    static uint16_t crc(const uint8_t *data, int n);
};

}   //  namespace panglos

#endif  //  __PZEM004T__

//  FIN

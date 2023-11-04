    /*
     *
     */

#include "panglos/drivers/7-segment.h"
#include "panglos/drivers/2_wire_bitbang.h"

namespace panglos {

class TM1637 : public Display
{
    TwoWire *tw;

public:
    TM1637(TwoWire *_tw);

    virtual void write(const char *text) override;

    bool command(uint8_t cmd, uint8_t data);
    bool init();
};

}   //  namespace panglos


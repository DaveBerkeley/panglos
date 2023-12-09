
    /*
     *  Microchip MCP6S91/2/3 SPI Programmable Gain Amplifier
     */

namespace panglos {

class SpiDevice;

class PGA
{
public:
    virtual uint8_t set_gain(uint8_t gain) = 0;
    virtual uint8_t get_gain() = 0;
};

class MCP6S9x : public PGA
{
    SpiDevice *spi;
    uint8_t gain;
public:
    MCP6S9x(SpiDevice *spi);

    virtual uint8_t set_gain(uint8_t gain) override;
    virtual uint8_t get_gain() override;
    void set_chan(uint8_t chan);
};

}   //  namespace panglos

//  FIN

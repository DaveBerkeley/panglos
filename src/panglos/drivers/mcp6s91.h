
    /*
     *  Microchip MCP6S91/2/3 SPI Programmable Gain Amplifier
     */

namespace panglos {

class SpiDevice;

class MCP6S9x
{
    SpiDevice *spi;
public:
    MCP6S9x(SpiDevice *spi);

    uint8_t set_gain(uint8_t gain);
    void set_chan(uint8_t chan);
};

}   //  namespace panglos

//  FIN
